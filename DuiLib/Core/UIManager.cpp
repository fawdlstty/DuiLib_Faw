﻿#include "StdAfx.h"
#include <zmouse.h>

namespace DuiLib {

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//

	static void GetChildWndRect (HWND hWnd, HWND hChildWnd, RECT& rcChildWnd) {
		::GetWindowRect (hChildWnd, &rcChildWnd);

		POINT pt = { 0 };
		pt.x = rcChildWnd.left;
		pt.y = rcChildWnd.top;
		::ScreenToClient (hWnd, &pt);
		rcChildWnd.left = pt.x;
		rcChildWnd.top = pt.y;

		pt.x = rcChildWnd.right;
		pt.y = rcChildWnd.bottom;
		::ScreenToClient (hWnd, &pt);
		rcChildWnd.right = pt.x;
		rcChildWnd.bottom = pt.y;
	}

	static UINT MapKeyState () {
		UINT uState = 0;
		if (::GetKeyState (VK_CONTROL) < 0) uState |= MK_CONTROL;
		if (::GetKeyState (VK_LBUTTON) < 0) uState |= MK_LBUTTON;
		if (::GetKeyState (VK_RBUTTON) < 0) uState |= MK_RBUTTON;
		if (::GetKeyState (VK_SHIFT) < 0) uState |= MK_SHIFT;
		if (::GetKeyState (VK_MENU) < 0) uState |= MK_ALT;
		return uState;
	}

	typedef struct tagFINDTABINFO {
		CControlUI* pFocus;
		CControlUI* pLast;
		bool bForward;
		bool bNextIsIt;
	} FINDTABINFO;

	typedef struct tagFINDSHORTCUT {
		TCHAR ch;
		bool bPickNext;
	} FINDSHORTCUT;

	typedef struct tagTIMERINFO {
		CControlUI* pSender;
		UINT nLocalID;
		HWND hWnd;
		UINT uWinTimer;
		bool bKilled;
	} TIMERINFO;


	tagTDrawInfo::tagTDrawInfo () {
		Clear ();
	}

	void tagTDrawInfo::Parse (faw::string_t pStrImage, faw::string_t pStrModify, CPaintManagerUI *paintManager) {
		// 1、aaa.jpg
		// 2、file='aaa.jpg' res='' restype='0' dest='0,0,0,0' source='0,0,0,0' corner='0,0,0,0' 
		// mask='#FF0000' fade='255' hole='false' xtiled='false' ytiled='false'
		sDrawString = pStrImage;
		sDrawModify = pStrModify;
		sImageName = pStrImage;

		for (size_t i = 0; i < 2; ++i) {
			std::map<faw::string_t, faw::string_t> m = FawTools::parse_keyvalue_pairs (i == 0 ? pStrImage : pStrModify);
			for (auto[str_key, str_value] : m) {
				if (str_key == _T ("file") || str_key == _T ("res")) {
					sImageName = str_value;
				} else if (str_key == _T ("restype")) {
					sResType = str_value;
				} else if (str_key == _T ("dest")) {
					rcDest = FawTools::parse_rect (str_value);
					if (paintManager)
						paintManager->GetDPIObj ()->Scale (&rcDest);
				} else if (str_key == _T ("source")) {
					rcSource = FawTools::parse_rect (str_value);
					if (paintManager)
						paintManager->GetDPIObj ()->Scale (&rcSource);
				} else if (str_key == _T ("corner")) {
					rcCorner = FawTools::parse_rect (str_value);
					if (paintManager)
						paintManager->GetDPIObj ()->Scale (&rcCorner);
				} else if (str_key == _T ("mask")) {
					dwMask = (DWORD) FawTools::parse_hex (str_value);
				} else if (str_key == _T ("fade")) {
					uFade = (BYTE) _ttoi (str_value.c_str ());
				} else if (str_key == _T ("hole")) {
					bHole = FawTools::parse_bool (str_value);
				} else if (str_key == _T ("xtiled")) {
					bTiledX = FawTools::parse_bool (str_value);
				} else if (str_key == _T ("ytiled")) {
					bTiledY = FawTools::parse_bool (str_value);
				} else if (str_key == _T ("hsl")) {
					bHSL = FawTools::parse_bool (str_value);
				} else if (str_key == _T ("iconsize")) {
					szIcon = FawTools::parse_size (str_value);
				} else if (str_key == _T ("iconalign")) {
					sIconAlign = str_value;
				}
			}
		}

		// 调整DPI资源
		if (paintManager && paintManager->GetDPIObj ()->GetScale () != 100) {
			faw::string_t sScale = std::format (_T ("@{}."), (int) paintManager->GetDPIObj ()->GetScale ());
			FawTools::replace_self (sImageName, _T ("."), sScale);
		}
	}
	void tagTDrawInfo::Clear () {
		sDrawString.clear ();
		sDrawModify.clear ();
		sImageName.clear ();

		memset (&rcDest, 0, sizeof (RECT));
		memset (&rcSource, 0, sizeof (RECT));
		memset (&rcCorner, 0, sizeof (RECT));
		dwMask = 0;
		uFade = 255;
		bHole = false;
		bTiledX = false;
		bTiledY = false;
		bHSL = false;
		szIcon = { 0, 0 };
		sIconAlign.clear ();
	}

	/////////////////////////////////////////////////////////////////////////////////////
	typedef BOOL (__stdcall *PFUNCUPDATELAYEREDWINDOW)(HWND, HDC, POINT*, SIZE*, HDC, POINT*, COLORREF, BLENDFUNCTION*, DWORD);
	PFUNCUPDATELAYEREDWINDOW g_fUpdateLayeredWindow = nullptr;

	HPEN m_hUpdateRectPen = nullptr;

	HINSTANCE CPaintManagerUI::m_hResourceInstance = nullptr;
	faw::string_t CPaintManagerUI::m_pStrResourcePath;
	faw::string_t CPaintManagerUI::m_pStrResourceZip;
	faw::string_t CPaintManagerUI::m_pStrResourceZipPwd;  //Garfield 20160325 带密码zip包解密
	HANDLE CPaintManagerUI::m_hResourceZip = nullptr;
	bool CPaintManagerUI::m_bCachedResourceZip = true;
	int CPaintManagerUI::m_nResType = UILIB_FILE;
	TResInfo CPaintManagerUI::m_SharedResInfo;
	HINSTANCE CPaintManagerUI::m_hInstance = nullptr;
	bool CPaintManagerUI::m_bUseHSL = false;
	short CPaintManagerUI::m_H = 180;
	short CPaintManagerUI::m_S = 100;
	short CPaintManagerUI::m_L = 100;
	CStdPtrArray CPaintManagerUI::m_aPreMessages;
	CStdPtrArray CPaintManagerUI::m_aPlugins;

	CPaintManagerUI::CPaintManagerUI ():
		m_hWndPaint (nullptr),
		m_hDcPaint (nullptr),
		m_hDcOffscreen (nullptr),
		m_hDcBackground (nullptr),
		m_bOffscreenPaint (true),
		m_hbmpOffscreen (nullptr),
		m_pOffscreenBits (nullptr),
		m_hbmpBackground (nullptr),
		m_pBackgroundBits (nullptr),
		m_hwndTooltip (nullptr),
		m_uTimerID (0x1000),
		m_pRoot (nullptr),
		m_pFocus (nullptr),
		m_pEventHover (nullptr),
		m_pEventClick (nullptr),
		m_pEventKey (nullptr),
		m_bFirstLayout (true),
		m_bFocusNeeded (false),
		m_bUpdateNeeded (false),
		m_bMouseTracking (false),
		m_bMouseCapture (false),
		m_bUsedVirtualWnd (false),
		m_bAsyncNotifyPosted (false),
		m_bForceUseSharedRes (false),
		m_nOpacity (0xFF),
		m_bLayered (false),
		m_bLayeredChanged (false),
		m_bShowUpdateRect (false),
		m_bUseGdiplusText (true),
		m_trh (0),
		m_bDragMode (false),
		m_hDragBitmap (nullptr),
		m_pDPI (nullptr),
		m_iHoverTime (400UL) {
		if (m_SharedResInfo.m_DefaultFontInfo.sFontName.empty ()) {
			m_SharedResInfo.m_dwDefaultDisabledColor = 0xFFA7A6AA;
			m_SharedResInfo.m_dwDefaultFontColor = 0xFF000000;
			m_SharedResInfo.m_dwDefaultLinkFontColor = 0xFF0000FF;
			m_SharedResInfo.m_dwDefaultLinkHoverFontColor = 0xFFD3215F;
			m_SharedResInfo.m_dwDefaultSelectedBkColor = 0xFFBAE4FF;

			LOGFONT lf = { 0 };
			::GetObject (::GetStockObject (DEFAULT_GUI_FONT), sizeof (LOGFONT), &lf);
			lf.lfCharSet = DEFAULT_CHARSET;
			HFONT hDefaultFont = ::CreateFontIndirect (&lf);
			m_SharedResInfo.m_DefaultFontInfo.hFont = hDefaultFont;
			m_SharedResInfo.m_DefaultFontInfo.sFontName = lf.lfFaceName;
			m_SharedResInfo.m_DefaultFontInfo.iSize = -lf.lfHeight;
			m_SharedResInfo.m_DefaultFontInfo.bBold = (lf.lfWeight >= FW_BOLD);
			m_SharedResInfo.m_DefaultFontInfo.bUnderline = (lf.lfUnderline == TRUE);
			m_SharedResInfo.m_DefaultFontInfo.bItalic = (lf.lfItalic == TRUE);
			::ZeroMemory (&m_SharedResInfo.m_DefaultFontInfo.tm, sizeof (m_SharedResInfo.m_DefaultFontInfo.tm));
		}

		m_ResInfo.m_dwDefaultDisabledColor = m_SharedResInfo.m_dwDefaultDisabledColor;
		m_ResInfo.m_dwDefaultFontColor = m_SharedResInfo.m_dwDefaultFontColor;
		m_ResInfo.m_dwDefaultLinkFontColor = m_SharedResInfo.m_dwDefaultLinkFontColor;
		m_ResInfo.m_dwDefaultLinkHoverFontColor = m_SharedResInfo.m_dwDefaultLinkHoverFontColor;
		m_ResInfo.m_dwDefaultSelectedBkColor = m_SharedResInfo.m_dwDefaultSelectedBkColor;

		if (!m_hUpdateRectPen) {
			m_hUpdateRectPen = ::CreatePen (PS_SOLID, 1, RGB (220, 0, 0));
			// Boot Windows Common Controls (for the ToolTip control)
			::InitCommonControls ();
			::LoadLibrary (_T ("msimg32.dll"));
		}

		m_szMinWindow.cx = 0;
		m_szMinWindow.cy = 0;
		m_szMaxWindow.cx = 0;
		m_szMaxWindow.cy = 0;
		m_szInitWindowSize.cx = 0;
		m_szInitWindowSize.cy = 0;
		m_szRoundCorner.cx = m_szRoundCorner.cy = 0;
		::ZeroMemory (&m_rcSizeBox, sizeof (m_rcSizeBox));
		::ZeroMemory (&m_rcCaption, sizeof (m_rcCaption));
		::ZeroMemory (&m_rcLayeredInset, sizeof (m_rcLayeredInset));
		::ZeroMemory (&m_rcLayeredUpdate, sizeof (m_rcLayeredUpdate));
		m_ptLastMousePos.x = m_ptLastMousePos.y = -1;

		m_pGdiplusStartupInput = new Gdiplus::GdiplusStartupInput;
		Gdiplus::GdiplusStartup (&m_gdiplusToken, m_pGdiplusStartupInput, nullptr); // 加载GDI接口

		CShadowUI::Initialize (m_hInstance);
	}

	CPaintManagerUI::~CPaintManagerUI () {
		// Delete the control-tree structures
		for (int i = 0; i < m_aDelayedCleanup.GetSize (); i++) delete static_cast<CControlUI*>(m_aDelayedCleanup[i]);
		m_aDelayedCleanup.Resize (0);
		for (int i = 0; i < m_aAsyncNotify.GetSize (); i++) delete static_cast<TNotifyUI*>(m_aAsyncNotify[i]);
		m_aAsyncNotify.Resize (0);

		m_mNameHash.Resize (0);
		if (m_pRoot) delete m_pRoot;

		::DeleteObject (m_ResInfo.m_DefaultFontInfo.hFont);
		RemoveAllFonts ();
		RemoveAllImages ();
		RemoveAllStyle ();
		RemoveAllDefaultAttributeList ();
		RemoveAllWindowCustomAttribute ();
		RemoveAllOptionGroups ();
		RemoveAllTimers ();
		RemoveAllDrawInfos ();
		RemoveResourceFontCollection ();

		if (m_hwndTooltip) {
			::DestroyWindow (m_hwndTooltip);
			m_hwndTooltip = nullptr;
		}
		if (!m_aFonts.empty ()) {
			for (int i = 0; i < m_aFonts.GetSize (); ++i) {
				HANDLE handle = static_cast<HANDLE>(m_aFonts.GetAt (i));
				::RemoveFontMemResourceEx (handle);
			}
		}
		if (m_hDcOffscreen) ::DeleteDC (m_hDcOffscreen);
		if (m_hDcBackground) ::DeleteDC (m_hDcBackground);
		if (m_hbmpOffscreen) ::DeleteObject (m_hbmpOffscreen);
		if (m_hbmpBackground) ::DeleteObject (m_hbmpBackground);
		if (m_hDcPaint) ::ReleaseDC (m_hWndPaint, m_hDcPaint);
		m_aPreMessages.Remove (m_aPreMessages.Find (this));
		// 销毁拖拽图片
		if (m_hDragBitmap) ::DeleteObject (m_hDragBitmap);
		//卸载GDIPlus
		Gdiplus::GdiplusShutdown (m_gdiplusToken);
		delete m_pGdiplusStartupInput;
		// DPI管理对象
		if (m_pDPI) {
			delete m_pDPI;
			m_pDPI = nullptr;
		}
	}

	void CPaintManagerUI::Init (HWND hWnd, faw::string_t pstrName) {
		ASSERT (::IsWindow (hWnd));

		m_mNameHash.Resize ();
		RemoveAllFonts ();
		RemoveAllImages ();
		RemoveAllStyle ();
		RemoveAllDefaultAttributeList ();
		RemoveAllWindowCustomAttribute ();
		RemoveAllOptionGroups ();
		RemoveAllTimers ();
		RemoveResourceFontCollection ();

		m_sName.clear ();
		if (!pstrName.empty ()) m_sName = pstrName;

		if (m_hWndPaint != hWnd) {
			m_hWndPaint = hWnd;
			m_hDcPaint = ::GetDC (hWnd);
			m_aPreMessages.Add (this);
		}

		SetTargetWnd (hWnd);
		InitDragDrop ();
	}

	void CPaintManagerUI::DeletePtr (void* ptr) {
		if (ptr) {
			delete ptr; ptr = nullptr;
		}
	}

	HINSTANCE CPaintManagerUI::GetInstance () {
		return m_hInstance;
	}

	faw::string_t CPaintManagerUI::GetInstancePath () {
		if (!m_hInstance) return _T ("");

		TCHAR tszModule[MAX_PATH + 1] = { 0 };
		::GetModuleFileName (m_hInstance, tszModule, MAX_PATH);
		faw::string_t sInstancePath = tszModule;
		size_t pos = sInstancePath.rfind (_T ('\\'));
		if (pos != faw::string_t::npos) sInstancePath = sInstancePath.substr (0, pos + 1);
		return sInstancePath;
	}

	faw::string_t CPaintManagerUI::GetCurrentPath () {
		TCHAR tszModule[MAX_PATH + 1] = { 0 };
		::GetCurrentDirectory (MAX_PATH, tszModule);
		return tszModule;
	}

	HINSTANCE CPaintManagerUI::GetResourceDll () {
		return m_hResourceInstance ? m_hResourceInstance : m_hInstance;
	}

	const faw::string_t CPaintManagerUI::GetResourcePath () {
		return m_pStrResourcePath;
	}

	const faw::string_t CPaintManagerUI::GetResourceZip () {
		return m_pStrResourceZip;
	}

	const faw::string_t CPaintManagerUI::GetResourceZipPwd () {
		return m_pStrResourceZipPwd;
	}

	bool CPaintManagerUI::IsCachedResourceZip () {
		return m_bCachedResourceZip;
	}

	HANDLE CPaintManagerUI::GetResourceZipHandle () {
		return m_hResourceZip;
	}

	void CPaintManagerUI::SetInstance (HINSTANCE hInst) {
		m_hInstance = hInst;
	}

	void CPaintManagerUI::SetCurrentPath (faw::string_t pStrPath) {
		::SetCurrentDirectory (pStrPath.data ());
	}

	void CPaintManagerUI::SetResourceDll (HINSTANCE hInst) {
		m_hResourceInstance = hInst;
	}

	void CPaintManagerUI::SetResourcePath (faw::string_t pStrPath) {
		m_pStrResourcePath = pStrPath;
		if (m_pStrResourcePath.empty ()) return;
		TCHAR cEnd = m_pStrResourcePath[m_pStrResourcePath.size () - 1];
		if (cEnd != _T ('\\') && cEnd != _T ('/')) m_pStrResourcePath += _T ('\\');
	}

	void CPaintManagerUI::SetResourceZip (LPVOID pVoid, unsigned int len, faw::string_t password) {
		if (m_pStrResourceZip == _T ("membuffer")) return;
		if (m_bCachedResourceZip && m_hResourceZip) {
			CloseZip ((HZIP) m_hResourceZip);
			m_hResourceZip = nullptr;
		}
		m_pStrResourceZip = _T ("membuffer");
		m_bCachedResourceZip = true;
		m_pStrResourceZipPwd = password;  //Garfield 20160325 带密码zip包解密
		if (m_bCachedResourceZip) {
			std::string pwd = FawTools::T_to_gb18030 (password);
			m_hResourceZip = (HANDLE) OpenZip (pVoid, len, pwd.c_str ());
		}
	}

	void CPaintManagerUI::SetResourceZip (faw::string_t pStrPath, bool bCachedResourceZip, faw::string_t password) {
		if (m_pStrResourceZip == pStrPath && m_bCachedResourceZip == bCachedResourceZip) return;
		if (m_bCachedResourceZip && m_hResourceZip) {
			CloseZip ((HZIP) m_hResourceZip);
			m_hResourceZip = nullptr;
		}
		m_pStrResourceZip = pStrPath;
		m_bCachedResourceZip = bCachedResourceZip;
		m_pStrResourceZipPwd = password;
		if (m_bCachedResourceZip) {
			faw::string_t sFile = CPaintManagerUI::GetResourcePath ();
			sFile += CPaintManagerUI::GetResourceZip ();
			std::string pwd = FawTools::T_to_gb18030 (password);
			m_hResourceZip = (HANDLE) OpenZip (sFile.c_str (), pwd.c_str ());
		}
	}

	void CPaintManagerUI::SetResourceType (int nType) {
		m_nResType = nType;
	}

	int CPaintManagerUI::GetResourceType () {
		return m_nResType;
	}

	bool CPaintManagerUI::GetHSL (short* H, short* S, short* L) {
		*H = m_H;
		*S = m_S;
		*L = m_L;
		return m_bUseHSL;
	}

	void CPaintManagerUI::SetHSL (bool bUseHSL, short H, short S, short L) {
		if (m_bUseHSL || m_bUseHSL != bUseHSL) {
			m_bUseHSL = bUseHSL;
			if (H == m_H && S == m_S && L == m_L) return;
			m_H = CLAMP (H, 0, 360);
			m_S = CLAMP (S, 0, 200);
			m_L = CLAMP (L, 0, 200);
			AdjustSharedImagesHSL ();
			for (int i = 0; i < m_aPreMessages.GetSize (); i++) {
				CPaintManagerUI* pManager = static_cast<CPaintManagerUI*>(m_aPreMessages[i]);
				if (pManager) pManager->AdjustImagesHSL ();
			}
		}
	}

	void CPaintManagerUI::ReloadSkin () {
		ReloadSharedImages ();
		for (int i = 0; i < m_aPreMessages.GetSize (); i++) {
			CPaintManagerUI* pManager = static_cast<CPaintManagerUI*>(m_aPreMessages[i]);
			if (pManager) pManager->ReloadImages ();
		}
	}

	CPaintManagerUI* CPaintManagerUI::GetPaintManager (faw::string_t pstrName) {
		//if (pstrName.empty ()) return nullptr;
		for (int i = 0; i < m_aPreMessages.GetSize (); i++) {
			CPaintManagerUI* pManager = static_cast<CPaintManagerUI*>(m_aPreMessages[i]);
			if (pManager && pstrName == pManager->GetName ()) return pManager;
		}
		return nullptr;
	}

	CStdPtrArray* CPaintManagerUI::GetPaintManagers () {
		return &m_aPreMessages;
	}

	bool CPaintManagerUI::LoadPlugin (faw::string_t pstrModuleName) {
		ASSERT (!pstrModuleName.empty ());
		if (pstrModuleName.empty ()) return false;
		HMODULE hModule = ::LoadLibrary (pstrModuleName.data ());
		if (hModule) {
			LPCREATECONTROL lpCreateControl = (LPCREATECONTROL)::GetProcAddress (hModule, "CreateControl");
			if (lpCreateControl) {
				if (m_aPlugins.Find (lpCreateControl) >= 0) return true;
				m_aPlugins.Add (lpCreateControl);
				return true;
			}
		}
		return false;
	}

	CStdPtrArray* CPaintManagerUI::GetPlugins () {
		return &m_aPlugins;
	}

	HWND CPaintManagerUI::GetPaintWindow () const {
		return m_hWndPaint;
	}

	HWND CPaintManagerUI::GetTooltipWindow () const {
		return m_hwndTooltip;
	}
	int CPaintManagerUI::GetHoverTime () const {
		return m_iHoverTime;
	}

	void CPaintManagerUI::SetHoverTime (int iTime) {
		m_iHoverTime = iTime;
	}

	faw::string_t CPaintManagerUI::GetName () const {
		return m_sName;
	}

	HDC CPaintManagerUI::GetPaintDC () const {
		return m_hDcPaint;
	}

	POINT CPaintManagerUI::GetMousePos () const {
		return m_ptLastMousePos;
	}

	SIZE CPaintManagerUI::GetClientSize () const {
		RECT rcClient = { 0 };
		::GetClientRect (m_hWndPaint, &rcClient);
		return { rcClient.right - rcClient.left, rcClient.bottom - rcClient.top };
	}

	SIZE CPaintManagerUI::GetInitSize () {
		return m_szInitWindowSize;
	}

	void CPaintManagerUI::SetInitSize (int cx, int cy) {
		m_szInitWindowSize.cx = cx;
		m_szInitWindowSize.cy = cy;
		if (!m_pRoot && m_hWndPaint) {
			::SetWindowPos (m_hWndPaint, nullptr, 0, 0, cx, cy, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
		}
	}

	RECT& CPaintManagerUI::GetSizeBox () {
		return m_rcSizeBox;
	}

	void CPaintManagerUI::SetSizeBox (RECT& rcSizeBox) {
		m_rcSizeBox = rcSizeBox;
	}

	RECT& CPaintManagerUI::GetCaptionRect () {
		return m_rcCaption;
	}

	void CPaintManagerUI::SetCaptionRect (RECT& rcCaption) {
		m_rcCaption = rcCaption;
	}

	SIZE CPaintManagerUI::GetRoundCorner () const {
		return m_szRoundCorner;
	}

	void CPaintManagerUI::SetRoundCorner (int cx, int cy) {
		m_szRoundCorner.cx = cx;
		m_szRoundCorner.cy = cy;
	}

	SIZE CPaintManagerUI::GetMinInfo () const {
		return m_szMinWindow;
	}

	void CPaintManagerUI::SetMinInfo (int cx, int cy) {
		ASSERT (cx >= 0 && cy >= 0);
		m_szMinWindow.cx = cx;
		m_szMinWindow.cy = cy;
	}

	SIZE CPaintManagerUI::GetMaxInfo () const {
		return m_szMaxWindow;
	}

	void CPaintManagerUI::SetMaxInfo (int cx, int cy) {
		ASSERT (cx >= 0 && cy >= 0);
		m_szMaxWindow.cx = cx;
		m_szMaxWindow.cy = cy;
	}

	bool CPaintManagerUI::IsShowUpdateRect () const {
		return m_bShowUpdateRect;
	}

	void CPaintManagerUI::SetShowUpdateRect (bool show) {
		m_bShowUpdateRect = show;
	}

	bool CPaintManagerUI::IsNoActivate () {
		return m_bNoActivate;
	}

	void CPaintManagerUI::SetNoActivate (bool bNoActivate) {
		m_bNoActivate = bNoActivate;
	}

	BYTE CPaintManagerUI::GetOpacity () const {
		return m_nOpacity;
	}

	void CPaintManagerUI::SetOpacity (BYTE nOpacity) {
		m_nOpacity = nOpacity;
		if (m_hWndPaint) {
			typedef BOOL (__stdcall *PFUNCSETLAYEREDWINDOWATTR)(HWND, COLORREF, BYTE, DWORD);
			PFUNCSETLAYEREDWINDOWATTR fSetLayeredWindowAttributes = nullptr;

			HMODULE hUser32 = ::GetModuleHandle (_T ("User32.dll"));
			if (hUser32) {
				fSetLayeredWindowAttributes =
					(PFUNCSETLAYEREDWINDOWATTR)::GetProcAddress (hUser32, "SetLayeredWindowAttributes");
				if (!fSetLayeredWindowAttributes) return;
			}

			DWORD dwStyle = ::GetWindowLong (m_hWndPaint, GWL_EXSTYLE);
			DWORD dwNewStyle = dwStyle;
			if (nOpacity >= 0 && nOpacity < 256) dwNewStyle |= WS_EX_LAYERED;
			else dwNewStyle &= ~WS_EX_LAYERED;
			if (dwStyle != dwNewStyle) ::SetWindowLong (m_hWndPaint, GWL_EXSTYLE, dwNewStyle);
			fSetLayeredWindowAttributes (m_hWndPaint, 0, nOpacity, LWA_ALPHA);
		}
	}

	bool CPaintManagerUI::IsLayered () {
		return m_bLayered;
	}

	void CPaintManagerUI::SetLayered (bool bLayered) {
		if (m_hWndPaint && bLayered != m_bLayered) {
			UINT uStyle = GetWindowStyle (m_hWndPaint);
			if ((uStyle & WS_CHILD) != 0) return;
			if (!g_fUpdateLayeredWindow) {
				HMODULE hUser32 = ::GetModuleHandle (_T ("User32.dll"));
				if (hUser32) {
					g_fUpdateLayeredWindow =
						(PFUNCUPDATELAYEREDWINDOW)::GetProcAddress (hUser32, "UpdateLayeredWindow");
					if (!g_fUpdateLayeredWindow) return;
				}
			}
			m_bLayered = bLayered;
			if (m_pRoot) m_pRoot->NeedUpdate ();
			Invalidate ();
		}
	}

	RECT& CPaintManagerUI::GetLayeredInset () {
		return m_rcLayeredInset;
	}

	void CPaintManagerUI::SetLayeredInset (RECT& rcLayeredInset) {
		m_rcLayeredInset = rcLayeredInset;
		m_bLayeredChanged = true;
		Invalidate ();
	}

	BYTE CPaintManagerUI::GetLayeredOpacity () {
		return m_nOpacity;
	}

	void CPaintManagerUI::SetLayeredOpacity (BYTE nOpacity) {
		m_nOpacity = nOpacity;
		m_bLayeredChanged = true;
		Invalidate ();
	}

	faw::string_t CPaintManagerUI::GetLayeredImage () {
		return m_diLayered.sDrawString;
	}

	void CPaintManagerUI::SetLayeredImage (faw::string_t pstrImage) {
		m_diLayered.sDrawString = pstrImage;
		RECT rcnullptr = { 0 };
		CRenderEngine::DrawImageInfo (nullptr, this, rcnullptr, rcnullptr, &m_diLayered);
		m_bLayeredChanged = true;
		Invalidate ();
	}

	CShadowUI* CPaintManagerUI::GetShadow () {
		return &m_shadow;
	}

	void CPaintManagerUI::SetUseGdiplusText (bool bUse) {
		m_bUseGdiplusText = bUse;
	}

	bool CPaintManagerUI::IsUseGdiplusText () const {
		return m_bUseGdiplusText;
	}

	void CPaintManagerUI::SetGdiplusTextRenderingHint (int trh) {
		m_trh = trh;
	}

	int CPaintManagerUI::GetGdiplusTextRenderingHint () const {
		return m_trh;
	}

	std::optional<LRESULT> CPaintManagerUI::PreMessageHandler (UINT uMsg, WPARAM wParam, LPARAM lParam) {
		for (int i = 0; i < m_aPreMessageFilters.GetSize (); i++) {
			const std::optional<LRESULT> _ret = static_cast<IMessageFilterUI*>(m_aPreMessageFilters[i])->MessageHandler (uMsg, wParam, lParam);
			if (_ret.has_value ())
				return 0;
		}
		switch (uMsg) {
		case WM_KEYDOWN:
		{
			// Tabbing between controls
			if (wParam == VK_TAB) {
				if (m_pFocus && m_pFocus->IsVisible () && m_pFocus->IsEnabled () && m_pFocus->GetClass ().find (_T ("RichEdit")) != faw::string_t::npos) {
					if (dynamic_cast<CRichEditUI*>(m_pFocus)->IsWantTab ()) return std::nullopt;
				}
				if (m_pFocus && m_pFocus->IsVisible () && m_pFocus->IsEnabled () && m_pFocus->GetClass ().find (_T ("WkeWebkitUI")) != faw::string_t::npos) {
					return std::nullopt;
				}
				SetNextTabControl (::GetKeyState (VK_SHIFT) >= 0);
				return 0;
			}
		}
		break;
		case WM_SYSCHAR:
		{
			// Handle ALT-shortcut key-combinations
			FINDSHORTCUT fs = { 0 };
			fs.ch = (TCHAR) toupper ((int) wParam);
			CControlUI* pControl = m_pRoot->FindControl (__FindControlFromShortcut, &fs, UIFIND_ENABLED | UIFIND_ME_FIRST | UIFIND_TOP_FIRST);
			if (pControl) {
				pControl->SetFocus ();
				pControl->Activate ();
				return 0;
			}
		}
		break;
		case WM_SYSKEYDOWN:
		{
			if (m_pFocus) {
				TEventUI event = { 0 };
				event.Type = UIEVENT_SYSKEY;
				event.chKey = (TCHAR) wParam;
				event.ptMouse = m_ptLastMousePos;
				event.wKeyState = (WORD) MapKeyState ();
				event.dwTimestamp = ::GetTickCount ();
				m_pFocus->Event (event);
			}
		}
		break;
		}
		return std::nullopt;
	}

	std::optional<LRESULT> CPaintManagerUI::MessageHandler (UINT uMsg, WPARAM wParam, LPARAM lParam) {
		if (!m_hWndPaint) return std::nullopt;
		// Cycle through listeners
		std::optional<LRESULT> lRes = std::nullopt;
		for (int i = 0; i < m_aMessageFilters.GetSize (); i++) {
			lRes = static_cast<IMessageFilterUI*>(m_aMessageFilters[i])->MessageHandler (uMsg, wParam, lParam);
			if (lRes.has_value ()) {
				switch (uMsg) {
				case WM_MOUSEMOVE:
				case WM_LBUTTONDOWN:
				case WM_LBUTTONDBLCLK:
				case WM_LBUTTONUP:
				{
					POINT pt = { GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam) };
					m_ptLastMousePos = pt;
				}
				break;
				case WM_CONTEXTMENU:
				case WM_MOUSEWHEEL:
				{
					POINT pt = { GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam) };
					::ScreenToClient (m_hWndPaint, &pt);
					m_ptLastMousePos = pt;
				}
				break;
				}
				return lRes.value ();
			}
		}

		if (m_bLayered) {
			switch (uMsg) {
			case WM_NCACTIVATE:
				if (!::IsIconic (m_hWndPaint)) {
					return (wParam == 0) ? 1 : 0;
				}
				break;
			case WM_NCCALCSIZE:
			case WM_NCPAINT:
				return 0;
			}
		}
		// Custom handling of events
		switch (uMsg) {
		case WM_APP + 1:
		{
			for (int i = 0; i < m_aDelayedCleanup.GetSize (); i++)
				delete static_cast<CControlUI*>(m_aDelayedCleanup[i]);
			m_aDelayedCleanup.Empty ();

			m_bAsyncNotifyPosted = false;

			TNotifyUI* pMsg = nullptr;
			while (!!(pMsg = static_cast<TNotifyUI*>(m_aAsyncNotify.GetAt (0)))) {
				m_aAsyncNotify.Remove (0);
				if (pMsg->pSender) {
					if (pMsg->pSender->OnNotify) pMsg->pSender->OnNotify (pMsg);
				}
				for (int j = 0; j < m_aNotifiers.GetSize (); j++) {
					static_cast<INotifyUI*>(m_aNotifiers[j])->Notify (*pMsg);
				}
				delete pMsg;
			}
		}
		break;
		case WM_CLOSE:
		{
			// Make sure all matching "closing" events are sent
			TEventUI event = { 0 };
			event.ptMouse = m_ptLastMousePos;
			event.wKeyState = (WORD) MapKeyState ();
			event.dwTimestamp = ::GetTickCount ();
			if (m_pEventHover) {
				event.Type = UIEVENT_MOUSELEAVE;
				event.pSender = m_pEventHover;
				m_pEventHover->Event (event);
			}
			if (m_pEventClick) {
				event.Type = UIEVENT_BUTTONUP;
				event.pSender = m_pEventClick;
				m_pEventClick->Event (event);
			}

			SetFocus (nullptr);

			if (::GetActiveWindow () == m_hWndPaint) {
				HWND hwndParent = GetWindowOwner (m_hWndPaint);
				if (hwndParent) ::SetFocus (hwndParent);
			}

			if (m_hwndTooltip) {
				::DestroyWindow (m_hwndTooltip);
				m_hwndTooltip = nullptr;
			}
		}
		break;
		case WM_ERASEBKGND:
		{
			// We'll do the painting here...
		}
		return 1;
		case WM_PAINT:
		{
			if (!m_pRoot) {
				PAINTSTRUCT ps = { 0 };
				::BeginPaint (m_hWndPaint, &ps);
				CRenderEngine::DrawColor (m_hDcPaint, ps.rcPaint, 0xFF000000);
				::EndPaint (m_hWndPaint, &ps);
				return lRes;
			}

			RECT rcClient = { 0 };
			::GetClientRect (m_hWndPaint, &rcClient);

			RECT rcPaint = { 0 };
			if (!::GetUpdateRect (m_hWndPaint, &rcPaint, FALSE)) return lRes;

			//if( m_bLayered ) {
			//	m_bOffscreenPaint = true;
			//	rcPaint = m_rcLayeredUpdate;
			//	if( ::IsRectEmpty(&m_rcLayeredUpdate) ) {
			//		PAINTSTRUCT ps = { 0 };
			//		::BeginPaint(m_hWndPaint, &ps);
			//		::EndPaint(m_hWndPaint, &ps);
			//		return lRes;
			//	}
			//	if( rcPaint.right > rcClient.right ) rcPaint.right = rcClient.right;
			//	if( rcPaint.bottom > rcClient.bottom ) rcPaint.bottom = rcClient.bottom;
			//	::ZeroMemory(&m_rcLayeredUpdate, sizeof(m_rcLayeredUpdate));
			//}
			//else {
			//	if( !::GetUpdateRect(m_hWndPaint, &rcPaint, FALSE) ) return lRes;
			//}

			// Set focus to first control?
			if (m_bFocusNeeded) {
				SetNextTabControl ();
			}

			SetPainting (true);

			bool bNeedSizeMsg = false;
			DWORD dwWidth = rcClient.right - rcClient.left;
			DWORD dwHeight = rcClient.bottom - rcClient.top;

			SetPainting (true);
			if (m_bUpdateNeeded) {
				m_bUpdateNeeded = false;
				if (!::IsRectEmpty (&rcClient) && !::IsIconic (m_hWndPaint)) {
					if (m_pRoot->IsUpdateNeeded ()) {
						RECT rcRoot = rcClient;
						if (m_hDcOffscreen) ::DeleteDC (m_hDcOffscreen);
						if (m_hDcBackground) ::DeleteDC (m_hDcBackground);
						if (m_hbmpOffscreen) ::DeleteObject (m_hbmpOffscreen);
						if (m_hbmpBackground) ::DeleteObject (m_hbmpBackground);
						m_hDcOffscreen = nullptr;
						m_hDcBackground = nullptr;
						m_hbmpOffscreen = nullptr;
						m_hbmpBackground = nullptr;
						if (m_bLayered) {
							rcRoot.left += m_rcLayeredInset.left;
							rcRoot.top += m_rcLayeredInset.top;
							rcRoot.right -= m_rcLayeredInset.right;
							rcRoot.bottom -= m_rcLayeredInset.bottom;
						}
						m_pRoot->SetPos (rcRoot, true);
						bNeedSizeMsg = true;
					} else {
						CControlUI* pControl = nullptr;
						m_aFoundControls.Empty ();
						m_pRoot->FindControl (__FindControlsFromUpdate, nullptr, UIFIND_VISIBLE | UIFIND_ME_FIRST | UIFIND_UPDATETEST);
						for (int it = 0; it < m_aFoundControls.GetSize (); it++) {
							pControl = static_cast<CControlUI*>(m_aFoundControls[it]);
							if (!pControl->IsFloat ()) pControl->SetPos (pControl->GetPos (), true);
							else pControl->SetPos (pControl->GetRelativePos (), true);
						}
						bNeedSizeMsg = true;
					}
					// We'll want to notify the window when it is first initialized
					// with the correct layout. The window form would take the time
					// to submit swipes/animations.
					if (m_bFirstLayout) {
						m_bFirstLayout = false;
						SendNotify (m_pRoot, DUI_MSGTYPE_WINDOWINIT, 0, 0, false);
						if (m_bLayered && m_bLayeredChanged) {
							Invalidate ();
							SetPainting (false);
							//return lRes;
						}
						// 更新阴影窗口显示
						m_shadow.Update (m_hWndPaint);
					}
				}
			} else if (m_bLayered && m_bLayeredChanged) {
				RECT rcRoot = rcClient;
				if (m_pOffscreenBits) ::ZeroMemory (m_pOffscreenBits, (rcRoot.right - rcRoot.left)
					* (rcRoot.bottom - rcRoot.top) * 4);
				rcRoot.left += m_rcLayeredInset.left;
				rcRoot.top += m_rcLayeredInset.top;
				rcRoot.right -= m_rcLayeredInset.right;
				rcRoot.bottom -= m_rcLayeredInset.bottom;
				m_pRoot->SetPos (rcRoot, true);
			}

			if (m_bLayered) {
				DWORD dwExStyle = ::GetWindowLong (m_hWndPaint, GWL_EXSTYLE);
				DWORD dwNewExStyle = dwExStyle | WS_EX_LAYERED;
				if (dwExStyle != dwNewExStyle) ::SetWindowLong (m_hWndPaint, GWL_EXSTYLE, dwNewExStyle);
				m_bOffscreenPaint = true;
				UnionRect (&rcPaint, &rcPaint, &m_rcLayeredUpdate);
				if (rcPaint.right > rcClient.right) rcPaint.right = rcClient.right;
				if (rcPaint.bottom > rcClient.bottom) rcPaint.bottom = rcClient.bottom;
				::ZeroMemory (&m_rcLayeredUpdate, sizeof (m_rcLayeredUpdate));
			}

			//
			// Render screen
			//
			// Prepare offscreen bitmap
			if (m_bOffscreenPaint && !m_hbmpOffscreen) {
				m_hDcOffscreen = ::CreateCompatibleDC (m_hDcPaint);
				m_hbmpOffscreen = CRenderEngine::CreateARGB32Bitmap (m_hDcPaint, dwWidth, dwHeight, (LPBYTE*) &m_pOffscreenBits);
				ASSERT (m_hDcOffscreen);
				ASSERT (m_hbmpOffscreen);
			}
			// Begin Windows paint
			PAINTSTRUCT ps = { 0 };
			::BeginPaint (m_hWndPaint, &ps);
			if (m_bOffscreenPaint) {
				HBITMAP hOldBitmap = (HBITMAP) ::SelectObject (m_hDcOffscreen, m_hbmpOffscreen);
				int iSaveDC = ::SaveDC (m_hDcOffscreen);
				if (m_bLayered) {
					for (LONG y = rcClient.bottom - rcPaint.bottom; y < rcClient.bottom - rcPaint.top; ++y) {
						for (LONG x = rcPaint.left; x < rcPaint.right; ++x) {
							int i = (y * dwWidth + x) * 4;
							*(DWORD*) (&m_pOffscreenBits[i]) = 0;
						}
					}
				}
				m_pRoot->Paint (m_hDcOffscreen, rcPaint, nullptr);

				if (m_bLayered) {
					for (int i = 0; i < m_aNativeWindow.GetSize (); ) {
						HWND hChildWnd = static_cast<HWND>(m_aNativeWindow[i]);
						if (!::IsWindow (hChildWnd)) {
							m_aNativeWindow.Remove (i);
							m_aNativeWindowControl.Remove (i);
							continue;
						}
						++i;
						if (!::IsWindowVisible (hChildWnd)) continue;
						RECT rcChildWnd = GetNativeWindowRect (hChildWnd);
						RECT rcTemp = { 0 };
						if (!::IntersectRect (&rcTemp, &rcPaint, &rcChildWnd)) continue;

						COLORREF* pChildBitmapBits = nullptr;
						HDC hChildMemDC = ::CreateCompatibleDC (m_hDcOffscreen);
						HBITMAP hChildBitmap = CRenderEngine::CreateARGB32Bitmap (hChildMemDC, rcChildWnd.right - rcChildWnd.left, rcChildWnd.bottom - rcChildWnd.top, (BYTE**) &pChildBitmapBits);
						::ZeroMemory (pChildBitmapBits, (rcChildWnd.right - rcChildWnd.left)*(rcChildWnd.bottom - rcChildWnd.top) * 4);
						HBITMAP hOldChildBitmap = (HBITMAP) ::SelectObject (hChildMemDC, hChildBitmap);
						::SendMessage (hChildWnd, WM_PRINT, (WPARAM) hChildMemDC, (LPARAM) (PRF_CHECKVISIBLE | PRF_CHILDREN | PRF_CLIENT | PRF_OWNED));
						COLORREF* pChildBitmapBit;
						for (LONG y = 0; y < rcChildWnd.bottom - rcChildWnd.top; y++) {
							for (LONG x = 0; x < rcChildWnd.right - rcChildWnd.left; x++) {
								pChildBitmapBit = pChildBitmapBits + y * (rcChildWnd.right - rcChildWnd.left) + x;
								if (*pChildBitmapBit != 0x00000000) *pChildBitmapBit |= 0xff000000;
							}
						}
						::BitBlt (m_hDcOffscreen, rcChildWnd.left, rcChildWnd.top, rcChildWnd.right - rcChildWnd.left,
							rcChildWnd.bottom - rcChildWnd.top, hChildMemDC, 0, 0, SRCCOPY);
						::SelectObject (hChildMemDC, hOldChildBitmap);
						::DeleteObject (hChildBitmap);
						::DeleteDC (hChildMemDC);
					}
				}

				for (int i = 0; i < m_aPostPaintControls.GetSize (); i++) {
					CControlUI* pPostPaintControl = static_cast<CControlUI*>(m_aPostPaintControls[i]);
					pPostPaintControl->DoPostPaint (m_hDcOffscreen, rcPaint);
				}

				::RestoreDC (m_hDcOffscreen, iSaveDC);

				if (m_bLayered) {
					RECT rcWnd = { 0 };
					::GetWindowRect (m_hWndPaint, &rcWnd);
					if (!m_diLayered.sDrawString.empty ()) {
						DWORD _dwWidth = rcClient.right - rcClient.left;
						DWORD _dwHeight = rcClient.bottom - rcClient.top;
						RECT rcLayeredClient = rcClient;
						rcLayeredClient.left += m_rcLayeredInset.left;
						rcLayeredClient.top += m_rcLayeredInset.top;
						rcLayeredClient.right -= m_rcLayeredInset.right;
						rcLayeredClient.bottom -= m_rcLayeredInset.bottom;

						COLORREF* pOffscreenBits = (COLORREF*) m_pOffscreenBits;
						COLORREF* pBackgroundBits = m_pBackgroundBits;
						BYTE A = 0;
						BYTE R = 0;
						BYTE G = 0;
						BYTE B = 0;
						if (!m_diLayered.sDrawString.empty ()) {
							if (!m_hbmpBackground) {
								m_hDcBackground = ::CreateCompatibleDC (m_hDcPaint);
								m_hbmpBackground = CRenderEngine::CreateARGB32Bitmap (m_hDcPaint, _dwWidth, _dwHeight, (BYTE**) &m_pBackgroundBits);
								::ZeroMemory (m_pBackgroundBits, _dwWidth * _dwHeight * 4);
								::SelectObject (m_hDcBackground, m_hbmpBackground);
								CRenderClip clip;
								CRenderClip::GenerateClip (m_hDcBackground, rcLayeredClient, clip);
								CRenderEngine::DrawImageInfo (m_hDcBackground, this, rcLayeredClient, rcLayeredClient, &m_diLayered);
							} else if (m_bLayeredChanged) {
								::ZeroMemory (m_pBackgroundBits, _dwWidth * _dwHeight * 4);
								CRenderClip clip;
								CRenderClip::GenerateClip (m_hDcBackground, rcLayeredClient, clip);
								CRenderEngine::DrawImageInfo (m_hDcBackground, this, rcLayeredClient, rcLayeredClient, &m_diLayered);
							}
							for (LONG y = rcClient.bottom - rcPaint.bottom; y < rcClient.bottom - rcPaint.top; ++y) {
								for (LONG x = rcPaint.left; x < rcPaint.right; ++x) {
									pOffscreenBits = (COLORREF*) (m_pOffscreenBits + y * _dwWidth + x);
									pBackgroundBits = m_pBackgroundBits + y * _dwWidth + x;
									A = (BYTE) ((*pBackgroundBits) >> 24);
									R = (BYTE) ((*pOffscreenBits) >> 16) * A / 255;
									G = (BYTE) ((*pOffscreenBits) >> 8) * A / 255;
									B = (BYTE) (*pOffscreenBits) * A / 255;
									*pOffscreenBits = RGB (B, G, R) + ((DWORD) A << 24);
								}
							}
						}
					} else {
						for (LONG y = rcClient.bottom - rcPaint.bottom; y < rcClient.bottom - rcPaint.top; ++y) {
							for (LONG x = rcPaint.left; x < rcPaint.right; ++x) {
								int i = (y * dwWidth + x) * 4;
								if ((m_pOffscreenBits[i + 3] == 0) && (m_pOffscreenBits[i + 0] != 0 || m_pOffscreenBits[i + 1] != 0 || m_pOffscreenBits[i + 2] != 0))
									m_pOffscreenBits[i + 3] = 255;
							}
						}
					}

					BLENDFUNCTION bf = { AC_SRC_OVER, 0, m_nOpacity, AC_SRC_ALPHA };
					POINT ptPos = { rcWnd.left, rcWnd.top };
					SIZE sizeWnd = { (LONG) dwWidth, (LONG) dwHeight };
					POINT ptSrc = { 0, 0 };
					g_fUpdateLayeredWindow (m_hWndPaint, m_hDcPaint, &ptPos, &sizeWnd, m_hDcOffscreen, &ptSrc, 0, &bf, ULW_ALPHA);
				} else {
					::BitBlt (m_hDcPaint, rcPaint.left, rcPaint.top, rcPaint.right - rcPaint.left, rcPaint.bottom - rcPaint.top, m_hDcOffscreen, rcPaint.left, rcPaint.top, SRCCOPY);
				}
				::SelectObject (m_hDcOffscreen, hOldBitmap);

				if (m_bShowUpdateRect && !m_bLayered) {
					HPEN hOldPen = (HPEN)::SelectObject (m_hDcPaint, m_hUpdateRectPen);
					::SelectObject (m_hDcPaint, ::GetStockObject (HOLLOW_BRUSH));
					::Rectangle (m_hDcPaint, rcPaint.left, rcPaint.top, rcPaint.right, rcPaint.bottom);
					::SelectObject (m_hDcPaint, hOldPen);
				}
			} else {
				// A standard paint job
				int iSaveDC = ::SaveDC (m_hDcPaint);
				m_pRoot->Paint (m_hDcPaint, rcPaint, nullptr);
				for (int i = 0; i < m_aPostPaintControls.GetSize (); i++) {
					CControlUI* pPostPaintControl = static_cast<CControlUI*>(m_aPostPaintControls[i]);
					pPostPaintControl->DoPostPaint (m_hDcPaint, rcPaint);
				}
				::RestoreDC (m_hDcPaint, iSaveDC);
			}
			// All Done!
			::EndPaint (m_hWndPaint, &ps);

			// 绘制结束
			SetPainting (false);
			m_bLayeredChanged = false;
			if (m_bUpdateNeeded) Invalidate ();

			// 发送窗口大小改变消息
			if (bNeedSizeMsg) {
				this->SendNotify (m_pRoot, DUI_MSGTYPE_WINDOWSIZE, 0, 0, true);
			}
			return lRes;
		}
		case WM_PRINTCLIENT:
		{
			if (!m_pRoot) break;
			RECT rcClient = { 0 };
			::GetClientRect (m_hWndPaint, &rcClient);
			HDC hDC = (HDC) wParam;
			int save = ::SaveDC (hDC);
			m_pRoot->Paint (hDC, rcClient, nullptr);
			if ((lParam & PRF_CHILDREN) != 0) {
				HWND hWndChild = ::GetWindow (m_hWndPaint, GW_CHILD);
				while (hWndChild) {
					RECT rcPos = { 0 };
					::GetWindowRect (hWndChild, &rcPos);
					::MapWindowPoints (HWND_DESKTOP, m_hWndPaint, reinterpret_cast<LPPOINT>(&rcPos), 2);
					::SetWindowOrgEx (hDC, -rcPos.left, -rcPos.top, nullptr);
					::SendMessage (hWndChild, WM_PRINT, wParam, lParam | PRF_NONCLIENT);
					hWndChild = ::GetWindow (hWndChild, GW_HWNDNEXT);
				}
			}
			::RestoreDC (hDC, save);
		}
		break;
		case WM_GETMINMAXINFO:
		{
			MONITORINFO Monitor = {};
			Monitor.cbSize = sizeof (Monitor);
			::GetMonitorInfo (::MonitorFromWindow (m_hWndPaint, MONITOR_DEFAULTTOPRIMARY), &Monitor);
			RECT rcWork = Monitor.rcWork;
			if (Monitor.dwFlags != MONITORINFOF_PRIMARY) {
				::OffsetRect (&rcWork, -rcWork.left, -rcWork.top);
			}

			LPMINMAXINFO lpMMI = (LPMINMAXINFO) lParam;
			if (m_szMinWindow.cx > 0) lpMMI->ptMinTrackSize.x = m_szMinWindow.cx;
			if (m_szMinWindow.cy > 0) lpMMI->ptMinTrackSize.y = m_szMinWindow.cy;
			if (m_szMaxWindow.cx > 0) lpMMI->ptMaxTrackSize.x = m_szMaxWindow.cx;
			if (m_szMaxWindow.cy > 0) lpMMI->ptMaxTrackSize.y = m_szMaxWindow.cy;
			if (m_szMaxWindow.cx > 0) lpMMI->ptMaxSize.x = m_szMaxWindow.cx;
			if (m_szMaxWindow.cy > 0) lpMMI->ptMaxSize.y = m_szMaxWindow.cy;
		}
		break;
		case WM_SIZE:
		{
			if (m_pFocus) {
				TEventUI event = { 0 };
				event.Type = UIEVENT_WINDOWSIZE;
				event.pSender = m_pFocus;
				event.dwTimestamp = ::GetTickCount ();
				m_pFocus->Event (event);
			}
			if (m_pRoot) m_pRoot->NeedUpdate ();
		}
		return 0;
		case WM_TIMER:
		{
			for (int i = 0; i < m_aTimers.GetSize (); i++) {
				const TIMERINFO* pTimer = static_cast<TIMERINFO*>(m_aTimers[i]);
				if (pTimer->hWnd == m_hWndPaint &&
					pTimer->uWinTimer == LOWORD (wParam) &&
					pTimer->bKilled == false) {
					TEventUI event = { 0 };
					event.Type = UIEVENT_TIMER;
					event.pSender = pTimer->pSender;
					event.dwTimestamp = ::GetTickCount ();
					event.ptMouse = m_ptLastMousePos;
					event.wKeyState = (WORD) MapKeyState ();
					event.wParam = pTimer->nLocalID;
					event.lParam = lParam;
					pTimer->pSender->Event (event);
					break;
				}
			}
		}
		break;
		case WM_MOUSEHOVER:
		{
			m_bMouseTracking = false;
			POINT pt = { GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam) };
			CControlUI* pHover = FindControl (pt);
			if (!pHover) break;
			// Generate mouse hover event
			if (m_pEventHover) {
				TEventUI event = { 0 };
				event.Type = UIEVENT_MOUSEHOVER;
				event.pSender = m_pEventHover;
				event.wParam = wParam;
				event.lParam = lParam;
				event.dwTimestamp = ::GetTickCount ();
				event.ptMouse = pt;
				event.wKeyState = (WORD) MapKeyState ();
				m_pEventHover->Event (event);
			}
			// Create tooltip information
			faw::string_t sToolTip = pHover->GetToolTip ();
			if (sToolTip.empty ()) return 0;
			::ZeroMemory (&m_ToolTip, sizeof (TOOLINFO));
			m_ToolTip.cbSize = sizeof (TOOLINFO);
			m_ToolTip.uFlags = TTF_IDISHWND;
			m_ToolTip.hwnd = m_hWndPaint;
			m_ToolTip.uId = (UINT_PTR) m_hWndPaint;
			m_ToolTip.hinst = m_hInstance;
			m_ToolTip.lpszText = &sToolTip[0];
			m_ToolTip.rect = pHover->GetPos ();
			if (!m_hwndTooltip) {
				m_hwndTooltip = ::CreateWindowEx (0, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWndPaint, NULL, m_hInstance, nullptr);
				::SendMessage (m_hwndTooltip, TTM_ADDTOOL, 0, (LPARAM) &m_ToolTip);
				::SendMessage (m_hwndTooltip, TTM_SETMAXTIPWIDTH, 0, pHover->GetToolTipWidth ());
			}
			if (!::IsWindowVisible (m_hwndTooltip)) {
				// 设置工具提示显示在最前面,避免被其他窗体遮挡
				::SetWindowPos(m_hwndTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				::SendMessage (m_hwndTooltip, TTM_SETTOOLINFO, 0, (LPARAM) &m_ToolTip);
				::SendMessage (m_hwndTooltip, TTM_TRACKACTIVATE, TRUE, (LPARAM) &m_ToolTip);
			}
		}
		return 0;
		case WM_MOUSELEAVE:
		{
			if (m_hwndTooltip) ::SendMessage (m_hwndTooltip, TTM_TRACKACTIVATE, FALSE, (LPARAM) &m_ToolTip);
			if (m_bMouseTracking) {
				POINT pt = { 0 };
				RECT rcWnd = { 0 };
				::GetCursorPos (&pt);
				::GetWindowRect (m_hWndPaint, &rcWnd);
				if (!::IsIconic (m_hWndPaint) && ::GetActiveWindow () == m_hWndPaint && ::PtInRect (&rcWnd, pt)) {
					if (::SendMessage (m_hWndPaint, WM_NCHITTEST, 0, MAKELPARAM (pt.x, pt.y)) == HTCLIENT) {
						::ScreenToClient (m_hWndPaint, &pt);
						::SendMessage (m_hWndPaint, WM_MOUSEMOVE, 0, MAKELPARAM (pt.x, pt.y));
					} else
						::SendMessage (m_hWndPaint, WM_MOUSEMOVE, 0, (LPARAM) -1);
				} else
					::SendMessage (m_hWndPaint, WM_MOUSEMOVE, 0, (LPARAM) -1);
			}
			m_bMouseTracking = false;
		}
		break;
		case WM_MOUSEMOVE:
		{
			// Start tracking this entire window again...
			if (!m_bMouseTracking) {
				TRACKMOUSEEVENT tme = { 0 };
				tme.cbSize = sizeof (TRACKMOUSEEVENT);
				tme.dwFlags = TME_HOVER | TME_LEAVE;
				tme.hwndTrack = m_hWndPaint;
				tme.dwHoverTime = !m_hwndTooltip ? m_iHoverTime : (DWORD) ::SendMessage (m_hwndTooltip, TTM_GETDELAYTIME, TTDT_INITIAL, 0L);
				_TrackMouseEvent (&tme);
				m_bMouseTracking = true;
			}

			// Generate the appropriate mouse messages
			POINT pt = { GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam) };
			// 是否移动
			bool bNeedDrag = true;
			if (m_ptLastMousePos.x == pt.x && m_ptLastMousePos.y == pt.y) {
				bNeedDrag = false;
			}
			// 记录鼠标位置
			m_ptLastMousePos = pt;
			CControlUI* pNewHover = FindControl (pt);
			if (pNewHover && pNewHover->GetManager () != this) break;

			// 拖拽事件
			if (bNeedDrag && m_bDragMode && wParam == MK_LBUTTON) {
				::ReleaseCapture ();
				CIDropSource* pdsrc = new CIDropSource;
				if (!pdsrc) return std::nullopt;
				pdsrc->AddRef ();

				CIDataObject* pdobj = new CIDataObject (pdsrc);
				if (!pdobj) return std::nullopt;
				pdobj->AddRef ();

				FORMATETC fmtetc = { 0 };
				STGMEDIUM medium = { 0 };
				fmtetc.dwAspect = DVASPECT_CONTENT;
				fmtetc.lindex = -1;
				//////////////////////////////////////
				fmtetc.cfFormat = CF_BITMAP;
				fmtetc.tymed = TYMED_GDI;
				medium.tymed = TYMED_GDI;
				HBITMAP hBitmap = (HBITMAP) OleDuplicateData (m_hDragBitmap, fmtetc.cfFormat, NULL);
				medium.hBitmap = hBitmap;
				pdobj->SetData (&fmtetc, &medium, FALSE);
				//////////////////////////////////////
				BITMAP bmap;
				GetObject (hBitmap, sizeof (BITMAP), &bmap);
				RECT rc = { 0, 0, bmap.bmWidth, bmap.bmHeight };
				fmtetc.cfFormat = CF_ENHMETAFILE;
				fmtetc.tymed = TYMED_ENHMF;
				HDC hMetaDC = CreateEnhMetaFile (m_hDcPaint, nullptr, nullptr, nullptr);
				HDC hdcMem = CreateCompatibleDC (m_hDcPaint);
				HGDIOBJ hOldBmp = ::SelectObject (hdcMem, hBitmap);
				::BitBlt (hMetaDC, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);
				::SelectObject (hdcMem, hOldBmp);
				medium.hEnhMetaFile = CloseEnhMetaFile (hMetaDC);
				DeleteDC (hdcMem);
				medium.tymed = TYMED_ENHMF;
				pdobj->SetData (&fmtetc, &medium, TRUE);
				//////////////////////////////////////
				CDragSourceHelper dragSrcHelper;
				POINT ptDrag = { 0 };
				ptDrag.x = bmap.bmWidth / 2;
				ptDrag.y = bmap.bmHeight / 2;
				dragSrcHelper.InitializeFromBitmap (hBitmap, ptDrag, rc, pdobj); //will own the bmp
				DWORD dwEffect;
				HRESULT hr = ::DoDragDrop (pdobj, pdsrc, DROPEFFECT_COPY | DROPEFFECT_MOVE, &dwEffect);
				if (dwEffect)
					pdsrc->Release ();
				delete pdsrc;
				pdobj->Release ();
				m_bDragMode = false;
				break;
			}
			TEventUI event = { 0 };
			event.ptMouse = pt;
			event.wParam = wParam;
			event.lParam = lParam;
			event.dwTimestamp = ::GetTickCount ();
			event.wKeyState = (WORD) MapKeyState ();
			if (!IsCaptured ()) {
				pNewHover = FindControl (pt);
				if (pNewHover && pNewHover->GetManager () != this) break;
				if (pNewHover != m_pEventHover && m_pEventHover) {
					event.Type = UIEVENT_MOUSELEAVE;
					event.pSender = m_pEventHover;

					CStdPtrArray aNeedMouseLeaveNeeded (m_aNeedMouseLeaveNeeded.GetSize ());
					aNeedMouseLeaveNeeded.Resize (m_aNeedMouseLeaveNeeded.GetSize ());
					::CopyMemory (aNeedMouseLeaveNeeded.GetData (), m_aNeedMouseLeaveNeeded.GetData (), m_aNeedMouseLeaveNeeded.GetSize () * sizeof (LPVOID));
					for (int i = 0; i < aNeedMouseLeaveNeeded.GetSize (); i++) {
						static_cast<CControlUI*>(aNeedMouseLeaveNeeded[i])->Event (event);
					}

					m_pEventHover->Event (event);
					m_pEventHover = nullptr;
					if (m_hwndTooltip) ::SendMessage (m_hwndTooltip, TTM_TRACKACTIVATE, FALSE, (LPARAM) &m_ToolTip);
				}
				if (pNewHover != m_pEventHover && pNewHover) {
					event.Type = UIEVENT_MOUSEENTER;
					event.pSender = pNewHover;
					pNewHover->Event (event);
					m_pEventHover = pNewHover;
				}
			}
			if (m_pEventClick) {
				event.Type = UIEVENT_MOUSEMOVE;
				event.pSender = m_pEventClick;
				m_pEventClick->Event (event);
			} else if (pNewHover) {
				event.Type = UIEVENT_MOUSEMOVE;
				event.pSender = pNewHover;
				pNewHover->Event (event);
			}
		}
		break;
		case WM_LBUTTONDOWN:
		{
			// We alway set focus back to our app (this helps
			// when Win32 child windows are placed on the dialog
			// and we need to remove them on focus change).
			if (!m_bNoActivate) ::SetFocus (m_hWndPaint);
			if (!m_pRoot) break;
			// 查找控件
			POINT pt = { GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam) };
			m_ptLastMousePos = pt;
			CControlUI* pControl = FindControl (pt);
			if (!pControl) break;
			if (pControl->GetManager () != this) break;

			// 准备拖拽
			if (pControl->IsDragEnabled ()) {
				m_bDragMode = true;
				if (m_hDragBitmap) {
					::DeleteObject (m_hDragBitmap);
					m_hDragBitmap = nullptr;
				}
				m_hDragBitmap = CRenderEngine::GenerateBitmap (this, pControl, pControl->GetPos ());
			}

			// 开启捕获
			SetCapture ();
			// 事件处理
			m_pEventClick = pControl;
			pControl->SetFocus ();

			TEventUI event = { 0 };
			event.Type = UIEVENT_BUTTONDOWN;
			event.pSender = pControl;
			event.wParam = wParam;
			event.lParam = lParam;
			event.ptMouse = pt;
			event.wKeyState = (WORD) wParam;
			event.dwTimestamp = ::GetTickCount ();
			pControl->Event (event);
		}
		break;
		case WM_LBUTTONDBLCLK:
		{
			if (!m_bNoActivate) ::SetFocus (m_hWndPaint);

			POINT pt = { GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam) };
			m_ptLastMousePos = pt;
			CControlUI* pControl = FindControl (pt);
			if (!pControl) break;
			if (pControl->GetManager () != this) break;
			SetCapture ();
			TEventUI event = { 0 };
			event.Type = UIEVENT_DBLCLICK;
			event.pSender = pControl;
			event.ptMouse = pt;
			event.wParam = wParam;
			event.lParam = lParam;
			event.wKeyState = (WORD) wParam;
			event.dwTimestamp = ::GetTickCount ();
			pControl->Event (event);
			m_pEventClick = pControl;
		}
		break;
		case WM_LBUTTONUP:
		{
			POINT pt = { GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam) };
			m_ptLastMousePos = pt;
			if (!m_pEventClick) break;
			ReleaseCapture ();
			TEventUI event = { 0 };
			event.Type = UIEVENT_BUTTONUP;
			event.pSender = m_pEventClick;
			event.wParam = wParam;
			event.lParam = lParam;
			event.ptMouse = pt;
			event.wKeyState = (WORD) wParam;
			event.dwTimestamp = ::GetTickCount ();

			CControlUI* pClick = m_pEventClick;
			m_pEventClick = nullptr;
			pClick->Event (event);
		}
		break;
		case WM_RBUTTONDOWN:
		{
			if (!m_bNoActivate) ::SetFocus (m_hWndPaint);
			POINT pt = { GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam) };
			m_ptLastMousePos = pt;
			CControlUI* pControl = FindControl (pt);
			if (!pControl) break;
			if (pControl->GetManager () != this) break;
			pControl->SetFocus ();
			SetCapture ();
			TEventUI event = { 0 };
			event.Type = UIEVENT_RBUTTONDOWN;
			event.pSender = pControl;
			event.wParam = wParam;
			event.lParam = lParam;
			event.ptMouse = pt;
			event.wKeyState = (WORD) wParam;
			event.dwTimestamp = ::GetTickCount ();
			pControl->Event (event);
			m_pEventClick = pControl;
		}
		break;
		case WM_RBUTTONUP:
		{
			POINT pt = { GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam) };
			m_ptLastMousePos = pt;
			m_pEventClick = FindControl (pt);
			if (!m_pEventClick) break;
			ReleaseCapture ();
			TEventUI event = { 0 };
			event.Type = UIEVENT_RBUTTONUP;
			event.pSender = m_pEventClick;
			event.wParam = wParam;
			event.lParam = lParam;
			event.ptMouse = pt;
			event.wKeyState = (WORD) wParam;
			event.dwTimestamp = ::GetTickCount ();
			m_pEventClick->Event (event);
		}
		break;
		case WM_MBUTTONDOWN:
		{
			if (!m_bNoActivate) ::SetFocus (m_hWndPaint);
			POINT pt = { GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam) };
			m_ptLastMousePos = pt;
			CControlUI* pControl = FindControl (pt);
			if (!pControl) break;
			if (pControl->GetManager () != this) break;
			pControl->SetFocus ();
			SetCapture ();
			TEventUI event = { 0 };
			event.Type = UIEVENT_MBUTTONDOWN;
			event.pSender = pControl;
			event.wParam = wParam;
			event.lParam = lParam;
			event.ptMouse = pt;
			event.wKeyState = (WORD) wParam;
			event.dwTimestamp = ::GetTickCount ();
			pControl->Event (event);
			m_pEventClick = pControl;
		}
		break;
		case WM_MBUTTONUP:
		{
			POINT pt = { GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam) };
			m_ptLastMousePos = pt;
			m_pEventClick = FindControl (pt);
			if (!m_pEventClick) break;
			ReleaseCapture ();

			TEventUI event = { 0 };
			event.Type = UIEVENT_MBUTTONUP;
			event.pSender = m_pEventClick;
			event.wParam = wParam;
			event.lParam = lParam;
			event.ptMouse = pt;
			event.wKeyState = (WORD) wParam;
			event.dwTimestamp = ::GetTickCount ();
			m_pEventClick->Event (event);
		}
		break;
		case WM_CONTEXTMENU:
		{
			if (!m_pRoot) break;
			POINT pt = { GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam) };
			::ScreenToClient (m_hWndPaint, &pt);
			m_ptLastMousePos = pt;
			if (!m_pEventClick) break;
			ReleaseCapture ();
			TEventUI event = { 0 };
			event.Type = UIEVENT_CONTEXTMENU;
			event.pSender = m_pEventClick;
			event.wParam = wParam;
			event.lParam = lParam;
			event.ptMouse = pt;
			event.wKeyState = (WORD) wParam;
			event.lParam = (LPARAM) m_pEventClick;
			event.dwTimestamp = ::GetTickCount ();
			m_pEventClick->Event (event);
			m_pEventClick = nullptr;
		}
		break;
		case WM_MOUSEWHEEL:
		{
			if (!m_pRoot) break;
			POINT pt = { GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam) };
			::ScreenToClient (m_hWndPaint, &pt);
			m_ptLastMousePos = pt;
			CControlUI* pControl = FindControl (pt);
			if (!pControl) break;
			if (pControl->GetManager () != this) break;
			int zDelta = (int) (short) HIWORD (wParam);
			TEventUI event = { 0 };
			event.Type = UIEVENT_SCROLLWHEEL;
			event.pSender = pControl;
			event.wParam = MAKELPARAM (zDelta < 0 ? SB_LINEDOWN : SB_LINEUP, 0);
			event.lParam = lParam;
			event.ptMouse = pt;
			event.wKeyState = (WORD) MapKeyState ();
			event.dwTimestamp = ::GetTickCount ();
			pControl->Event (event);

			// Let's make sure that the scroll item below the cursor is the same as before...
			::SendMessage (m_hWndPaint, WM_MOUSEMOVE, 0, (LPARAM) MAKELPARAM (m_ptLastMousePos.x, m_ptLastMousePos.y));
		}
		break;
		case WM_CHAR:
		{
			if (!m_pRoot) break;
			if (!m_pFocus) break;
			TEventUI event = { 0 };
			event.Type = UIEVENT_CHAR;
			event.pSender = m_pFocus;
			event.wParam = wParam;
			event.lParam = lParam;
			event.chKey = (TCHAR) wParam;
			event.ptMouse = m_ptLastMousePos;
			event.wKeyState = (WORD) MapKeyState ();
			event.dwTimestamp = ::GetTickCount ();
			m_pFocus->Event (event);
		}
		break;
		case WM_KEYDOWN:
		{
			if (!m_pRoot) break;
			if (!m_pFocus) break;
			TEventUI event = { 0 };
			event.Type = UIEVENT_KEYDOWN;
			event.pSender = m_pFocus;
			event.wParam = wParam;
			event.lParam = lParam;
			event.chKey = (TCHAR) wParam;
			event.ptMouse = m_ptLastMousePos;
			event.wKeyState = (WORD) MapKeyState ();
			event.dwTimestamp = ::GetTickCount ();
			m_pFocus->Event (event);
			m_pEventKey = m_pFocus;
		}
		break;
		case WM_KEYUP:
		{
			if (!m_pRoot) break;
			if (!m_pEventKey) break;
			TEventUI event = { 0 };
			event.Type = UIEVENT_KEYUP;
			event.pSender = m_pEventKey;
			event.wParam = wParam;
			event.lParam = lParam;
			event.chKey = (TCHAR) wParam;
			event.ptMouse = m_ptLastMousePos;
			event.wKeyState = (WORD) MapKeyState ();
			event.dwTimestamp = ::GetTickCount ();
			m_pEventKey->Event (event);
			m_pEventKey = nullptr;
		}
		break;
		case WM_SETCURSOR:
		{
			if (!m_pRoot) break;
			if (LOWORD (lParam) != HTCLIENT) break;
			if (m_bMouseCapture) return 0;

			POINT pt = { 0 };
			::GetCursorPos (&pt);
			::ScreenToClient (m_hWndPaint, &pt);
			CControlUI* pControl = FindControl (pt);
			if (!pControl) break;
			if ((pControl->GetControlFlags () & UIFLAG_SETCURSOR) == 0) break;
			TEventUI event = { 0 };
			event.Type = UIEVENT_SETCURSOR;
			event.pSender = pControl;
			event.wParam = wParam;
			event.lParam = lParam;
			event.ptMouse = pt;
			event.wKeyState = (WORD) MapKeyState ();
			event.dwTimestamp = ::GetTickCount ();
			pControl->Event (event);
		}
		return 0;
		case WM_SETFOCUS:
		{
			if (m_pFocus) {
				TEventUI event = { 0 };
				event.Type = UIEVENT_SETFOCUS;
				event.wParam = wParam;
				event.lParam = lParam;
				event.pSender = m_pFocus;
				event.dwTimestamp = ::GetTickCount ();
				m_pFocus->Event (event);
			}
			break;
		}
		//case WM_KILLFOCUS:
		//{
		//	if (IsCaptured ()) ReleaseCapture ();
		//	break;
		//}
		case WM_NOTIFY:
		{
			if (lParam == 0) break;
			LPNMHDR lpNMHDR = (LPNMHDR) lParam;
			if (lpNMHDR) return ::SendMessage (lpNMHDR->hwndFrom, OCM__BASE + uMsg, wParam, lParam);
			return 0;
		}
		break;
		case WM_COMMAND:
		{
			if (lParam == 0) break;
			HWND hWndChild = (HWND) lParam;
			lRes = ::SendMessage (hWndChild, OCM__BASE + uMsg, wParam, lParam);
			if (lRes != std::nullopt) return lRes;
		}
		break;
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
		{
			// Refer To: http://msdn.microsoft.com/en-us/library/bb761691(v=vs.85).aspx
			// Read-only or disabled edit controls do not send the WM_CTLCOLOREDIT message; instead, they send the WM_CTLCOLORSTATIC message.
			if (lParam == 0) break;
			HWND hWndChild = (HWND) lParam;
			lRes = ::SendMessage (hWndChild, OCM__BASE + uMsg, wParam, lParam);
			if (lRes != std::nullopt) return lRes;
		}
		break;
		default:
			break;
		}
		return std::nullopt;
	}

	bool CPaintManagerUI::IsUpdateNeeded () const {
		return m_bUpdateNeeded;
	}

	void CPaintManagerUI::NeedUpdate () {
		m_bUpdateNeeded = true;
	}

	void CPaintManagerUI::Invalidate () {
		RECT rcClient = { 0 };
		::GetClientRect (m_hWndPaint, &rcClient);
		::UnionRect (&m_rcLayeredUpdate, &m_rcLayeredUpdate, &rcClient);
		::InvalidateRect (m_hWndPaint, nullptr, FALSE);
	}

	void CPaintManagerUI::Invalidate (RECT& rcItem) {
		if (rcItem.left < 0) rcItem.left = 0;
		if (rcItem.top < 0) rcItem.top = 0;
		if (rcItem.right < rcItem.left) rcItem.right = rcItem.left;
		if (rcItem.bottom < rcItem.top) rcItem.bottom = rcItem.top;
		::UnionRect (&m_rcLayeredUpdate, &m_rcLayeredUpdate, &rcItem);
		// TODO 不加这一段将使得InvalidateRect被多次调用
		if (rcItem.right - rcItem.left <= 0 || rcItem.bottom - rcItem.top <= 0)
			return;
		::InvalidateRect (m_hWndPaint, &rcItem, FALSE);
	}

	bool CPaintManagerUI::AttachDialog (CControlUI* pControl) {
		ASSERT (::IsWindow (m_hWndPaint));
		// 创建阴影窗口
		m_shadow.Create (this);

		// Reset any previous attachment
		SetFocus (nullptr);
		m_pEventKey = nullptr;
		m_pEventHover = nullptr;
		m_pEventClick = nullptr;
		// Remove the existing control-tree. We might have gotten inside this function as
		// a result of an event fired or similar, so we cannot just delete the objects and
		// pull the internal memory of the calling code. We'll delay the cleanup.
		if (m_pRoot) {
			m_aPostPaintControls.Empty ();
			AddDelayedCleanup (m_pRoot);
		}
		// Set the dialog root element
		m_pRoot = pControl;
		// Go ahead...
		m_bUpdateNeeded = true;
		m_bFirstLayout = true;
		m_bFocusNeeded = false;
		// Initiate all control
		return InitControls (pControl);
	}

	bool CPaintManagerUI::InitControls (CControlUI* pControl, CControlUI* pParent /*= nullptr*/) {
		ASSERT (pControl);
		if (!pControl) return false;
		pControl->SetManager (this, pParent ? pParent : pControl->GetParent (), true);
		pControl->FindControl (__FindControlFromNameHash, this, UIFIND_ALL);
		return true;
	}

	void CPaintManagerUI::ReapObjects (CControlUI* pControl) {
		if (pControl == m_pEventKey) m_pEventKey = nullptr;
		if (pControl == m_pEventHover) m_pEventHover = nullptr;
		if (pControl == m_pEventClick) m_pEventClick = nullptr;
		if (pControl == m_pFocus) m_pFocus = nullptr;
		KillTimer (pControl);
		const faw::string_t sName = pControl->GetName ();
		if (!sName.empty ()) {
			if (pControl == FindControl (sName)) m_mNameHash.Remove (sName);
		}
		for (int i = 0; i < m_aAsyncNotify.GetSize (); i++) {
			TNotifyUI* pMsg = static_cast<TNotifyUI*>(m_aAsyncNotify[i]);
			if (pMsg->pSender == pControl) pMsg->pSender = nullptr;
		}
	}

	bool CPaintManagerUI::AddOptionGroup (faw::string_t pStrGroupName, CControlUI* pControl) {
		LPVOID lp = m_mOptionGroup.Find (pStrGroupName);
		if (lp) {
			CStdPtrArray* aOptionGroup = static_cast<CStdPtrArray*>(lp);
			for (int i = 0; i < aOptionGroup->GetSize (); i++) {
				if (static_cast<CControlUI*>(aOptionGroup->GetAt (i)) == pControl) {
					return false;
				}
			}
			aOptionGroup->Add (pControl);
		} else {
			CStdPtrArray* aOptionGroup = new CStdPtrArray (6);
			aOptionGroup->Add (pControl);
			m_mOptionGroup.Insert (pStrGroupName, aOptionGroup);
		}
		return true;
	}

	CStdPtrArray* CPaintManagerUI::GetOptionGroup (faw::string_t pStrGroupName) {
		LPVOID lp = m_mOptionGroup.Find (pStrGroupName);
		if (lp) return static_cast<CStdPtrArray*>(lp);
		return nullptr;
	}

	void CPaintManagerUI::RemoveOptionGroup (faw::string_t pStrGroupName, CControlUI* pControl) {
		LPVOID lp = m_mOptionGroup.Find (pStrGroupName);
		if (lp) {
			CStdPtrArray* aOptionGroup = static_cast<CStdPtrArray*>(lp);
			if (!aOptionGroup) return;
			for (int i = 0; i < aOptionGroup->GetSize (); i++) {
				if (static_cast<CControlUI*>(aOptionGroup->GetAt (i)) == pControl) {
					aOptionGroup->Remove (i);
					break;
				}
			}
			if (aOptionGroup->empty ()) {
				delete aOptionGroup;
				m_mOptionGroup.Remove (pStrGroupName);
			}
		}
	}

	void CPaintManagerUI::RemoveAllOptionGroups () {
		CStdPtrArray* aOptionGroup;
		for (int i = 0; i < m_mOptionGroup.GetSize (); i++) {
			faw::string_t key = m_mOptionGroup.GetAt (i)->Key;
			if (!key.empty ()) {
				aOptionGroup = static_cast<CStdPtrArray*>(m_mOptionGroup.Find (key));
				delete aOptionGroup;
			}
		}
		m_mOptionGroup.RemoveAll ();
	}

	void CPaintManagerUI::MessageLoop () {
		MSG msg = { 0 };
		while (::GetMessage (&msg, nullptr, 0, 0)) {
			if (!CPaintManagerUI::TranslateMessage (&msg)) {
				::TranslateMessage (&msg);
				try {
					::DispatchMessage (&msg);
				} catch (...) {
					DUITRACE (_T ("EXCEPTION: %s(%d)\n"), __FILET__, __LINE__);
#ifdef _DEBUG
					throw "CPaintManagerUI::MessageLoop";
#endif
				}
			}
		}
	}

	void CPaintManagerUI::Term () {
		// 销毁资源管理器
		CResourceManager::GetInstance ()->Release ();
		CControlFactory::GetInstance ()->Release ();
		//CMenuWnd::DestroyMenu();

		// 清理共享资源
		// 图片
		TImageInfo* data;
		for (int i = 0; i < m_SharedResInfo.m_ImageHash.GetSize (); i++) {
			faw::string_t key = m_SharedResInfo.m_ImageHash.GetAt (i)->Key;
			if (!key.empty ()) {
				data = static_cast<TImageInfo*>(m_SharedResInfo.m_ImageHash.Find (key, false));
				if (data) {
					CRenderEngine::FreeImage (data);
					data = nullptr;
				}
			}
		}
		m_SharedResInfo.m_ImageHash.RemoveAll ();
		// 字体
		TFontInfo* pFontInfo;
		for (int i = 0; i < m_SharedResInfo.m_CustomFonts.GetSize (); i++) {
			faw::string_t key = m_SharedResInfo.m_CustomFonts.GetAt (i)->Key;
			if (!key.empty ()) {
				pFontInfo = static_cast<TFontInfo*>(m_SharedResInfo.m_CustomFonts.Find (key, false));
				if (pFontInfo) {
					::DeleteObject (pFontInfo->hFont);
					delete pFontInfo;
					pFontInfo = nullptr;
				}
			}
		}
		m_SharedResInfo.m_CustomFonts.RemoveAll ();
		// 默认字体
		if (m_SharedResInfo.m_DefaultFontInfo.hFont) {
			::DeleteObject (m_SharedResInfo.m_DefaultFontInfo.hFont);
		}
		// 样式
		faw::string_t* pStyle;
		for (int i = 0; i < m_SharedResInfo.m_StyleHash.GetSize (); i++) {
			faw::string_t key = m_SharedResInfo.m_StyleHash.GetAt (i)->Key;
			if (!key.empty ()) {
				pStyle = static_cast<faw::string_t*>(m_SharedResInfo.m_StyleHash.Find (key, false));
				if (pStyle) {
					delete pStyle;
					pStyle = nullptr;
				}
			}
		}
		m_SharedResInfo.m_StyleHash.RemoveAll ();

		// 样式
		faw::string_t* pAttr;
		for (int i = 0; i < m_SharedResInfo.m_AttrHash.GetSize (); i++) {
			faw::string_t key = m_SharedResInfo.m_AttrHash.GetAt (i)->Key;
			if (!key.empty ()) {
				pAttr = static_cast<faw::string_t*>(m_SharedResInfo.m_AttrHash.Find (key, false));
				if (pAttr) {
					delete pAttr;
					pAttr = nullptr;
				}
			}
		}
		m_SharedResInfo.m_AttrHash.RemoveAll ();

		// 关闭ZIP
		if (m_bCachedResourceZip && m_hResourceZip) {
			CloseZip ((HZIP) m_hResourceZip);
			m_hResourceZip = nullptr;
		}
	}

	CDPI * DuiLib::CPaintManagerUI::GetDPIObj () {
		if (!m_pDPI) {
			m_pDPI = new CDPI;
		}
		return m_pDPI;
	}

	void DuiLib::CPaintManagerUI::SetDPI (int iDPI) {
		int scale1 = GetDPIObj ()->GetScale ();
		GetDPIObj ()->SetScale (iDPI);
		int scale2 = GetDPIObj ()->GetScale ();
		ResetDPIAssets ();
		RECT rcWnd = { 0 };
		::GetWindowRect (GetPaintWindow (), &rcWnd);
		RECT*  prcNewWindow = &rcWnd;
		if (!::IsZoomed (GetPaintWindow ())) {
			RECT rc = rcWnd;
			rc.right = rcWnd.left + (rcWnd.right - rcWnd.left) * scale2 / scale1;
			rc.bottom = rcWnd.top + (rcWnd.bottom - rcWnd.top) * scale2 / scale1;
			prcNewWindow = &rc;
		}
		SetWindowPos (GetPaintWindow (), NULL, prcNewWindow->left, prcNewWindow->top, prcNewWindow->right - prcNewWindow->left, prcNewWindow->bottom - prcNewWindow->top, SWP_NOZORDER | SWP_NOACTIVATE);
		if (GetRoot ()) GetRoot ()->NeedUpdate ();
		::PostMessage (GetPaintWindow (), UIMSG_SET_DPI, 0, 0);
	}

	void DuiLib::CPaintManagerUI::SetAllDPI (int iDPI) {
		for (int i = 0; i < m_aPreMessages.GetSize (); i++) {
			CPaintManagerUI* pManager = static_cast<CPaintManagerUI*>(m_aPreMessages[i]);
			pManager->SetDPI (iDPI);
		}
	}

	void DuiLib::CPaintManagerUI::ResetDPIAssets () {
		RemoveAllDrawInfos ();
		RemoveAllImages ();;

		for (int it = 0; it < m_ResInfo.m_CustomFonts.GetSize (); it++) {
			TFontInfo* pFontInfo = static_cast<TFontInfo*>(m_ResInfo.m_CustomFonts[it]->Data);
			RebuildFont (pFontInfo);
		}
		RebuildFont (&m_ResInfo.m_DefaultFontInfo);

		for (int it = 0; it < m_SharedResInfo.m_CustomFonts.GetSize (); it++) {
			TFontInfo* pFontInfo = static_cast<TFontInfo*>(m_SharedResInfo.m_CustomFonts[it]->Data);
			RebuildFont (pFontInfo);
		}
		RebuildFont (&m_SharedResInfo.m_DefaultFontInfo);

		CStdPtrArray *richEditList = FindSubControlsByClass (GetRoot (), _T ("RichEdit"));
		for (int i = 0; i < richEditList->GetSize (); i++) {
			CRichEditUI* pT = static_cast<CRichEditUI*>((*richEditList)[i]);
			pT->SetFont (pT->GetFont ());

		}
	}

	void DuiLib::CPaintManagerUI::RebuildFont (TFontInfo * pFontInfo) {
		::DeleteObject (pFontInfo->hFont);
		LOGFONT lf = { 0 };
		::GetObject (::GetStockObject (DEFAULT_GUI_FONT), sizeof (LOGFONT), &lf);
		_tcsncpy (lf.lfFaceName, pFontInfo->sFontName.c_str (), LF_FACESIZE);
		lf.lfCharSet = DEFAULT_CHARSET;
		lf.lfHeight = -GetDPIObj ()->Scale (pFontInfo->iSize);
		lf.lfQuality = CLEARTYPE_QUALITY;
		if (pFontInfo->bBold) lf.lfWeight += FW_BOLD;
		if (pFontInfo->bUnderline) lf.lfUnderline = TRUE;
		if (pFontInfo->bItalic) lf.lfItalic = TRUE;
		HFONT hFont = ::CreateFontIndirect (&lf);
		pFontInfo->hFont = hFont;
		::ZeroMemory (&(pFontInfo->tm), sizeof (pFontInfo->tm));
		if (m_hDcPaint) {
			HFONT hOldFont = (HFONT) ::SelectObject (m_hDcPaint, hFont);
			::GetTextMetrics (m_hDcPaint, &pFontInfo->tm);
			::SelectObject (m_hDcPaint, hOldFont);
		}
	}

	CControlUI* CPaintManagerUI::GetFocus () const {
		return m_pFocus;
	}

	void CPaintManagerUI::SetFocus (CControlUI* pControl) {
		// Paint manager window has focus?
		HWND hFocusWnd = ::GetFocus ();
		if (hFocusWnd != m_hWndPaint && pControl != m_pFocus) ::SetFocus (m_hWndPaint);
		// Already has focus?
		if (pControl == m_pFocus) return;
		// Remove focus from old control
		if (m_pFocus) {
			TEventUI event = { 0 };
			event.Type = UIEVENT_KILLFOCUS;
			event.pSender = pControl;
			event.dwTimestamp = ::GetTickCount ();
			m_pFocus->Event (event);
			SendNotify (m_pFocus, DUI_MSGTYPE_KILLFOCUS);
			m_pFocus = nullptr;
		}
		if (!pControl) return;
		// Set focus to new control
		if (pControl
			&& pControl->GetManager () == this
			&& pControl->IsVisible ()
			&& pControl->IsEnabled ()) {
			m_pFocus = pControl;
			TEventUI event = { 0 };
			event.Type = UIEVENT_SETFOCUS;
			event.pSender = pControl;
			event.dwTimestamp = ::GetTickCount ();
			m_pFocus->Event (event);
			SendNotify (m_pFocus, DUI_MSGTYPE_SETFOCUS);
		}
	}

	void CPaintManagerUI::SetFocusNeeded (CControlUI* pControl) {
		::SetFocus (m_hWndPaint);
		if (!pControl) return;
		if (m_pFocus) {
			TEventUI event = { 0 };
			event.Type = UIEVENT_KILLFOCUS;
			event.pSender = pControl;
			event.dwTimestamp = ::GetTickCount ();
			m_pFocus->Event (event);
			SendNotify (m_pFocus, DUI_MSGTYPE_KILLFOCUS);
			m_pFocus = nullptr;
		}
		FINDTABINFO info = { 0 };
		info.pFocus = pControl;
		info.bForward = false;
		m_pFocus = m_pRoot->FindControl (__FindControlFromTab, &info, UIFIND_VISIBLE | UIFIND_ENABLED | UIFIND_ME_FIRST);
		m_bFocusNeeded = true;
		if (m_pRoot) m_pRoot->NeedUpdate ();
	}

	bool CPaintManagerUI::SetTimer (CControlUI* pControl, UINT nTimerID, UINT uElapse) {
		ASSERT (pControl);
		ASSERT (uElapse > 0);
		for (int i = 0; i < m_aTimers.GetSize (); i++) {
			TIMERINFO* pTimer = static_cast<TIMERINFO*>(m_aTimers[i]);
			if (pTimer->pSender == pControl
				&& pTimer->hWnd == m_hWndPaint
				&& pTimer->nLocalID == nTimerID) {
				if (pTimer->bKilled == true) {
					if (::SetTimer (m_hWndPaint, pTimer->uWinTimer, uElapse, nullptr)) {
						pTimer->bKilled = false;
						return true;
					}
					return false;
				}
				return false;
			}
		}

		m_uTimerID = (++m_uTimerID) % 0xF0; //0xf1-0xfe特殊用途
		if (!::SetTimer (m_hWndPaint, m_uTimerID, uElapse, nullptr)) return FALSE;
		TIMERINFO* pTimer = new TIMERINFO;
		if (!pTimer) return FALSE;
		pTimer->hWnd = m_hWndPaint;
		pTimer->pSender = pControl;
		pTimer->nLocalID = nTimerID;
		pTimer->uWinTimer = m_uTimerID;
		pTimer->bKilled = false;
		return m_aTimers.Add (pTimer);
	}

	bool CPaintManagerUI::KillTimer (CControlUI* pControl, UINT nTimerID) {
		ASSERT (pControl);
		for (int i = 0; i < m_aTimers.GetSize (); i++) {
			TIMERINFO* pTimer = static_cast<TIMERINFO*>(m_aTimers[i]);
			if (pTimer->pSender == pControl
				&& pTimer->hWnd == m_hWndPaint
				&& pTimer->nLocalID == nTimerID) {
				if (pTimer->bKilled == false) {
					if (::IsWindow (m_hWndPaint)) ::KillTimer (pTimer->hWnd, pTimer->uWinTimer);
					pTimer->bKilled = true;
					return true;
				}
			}
		}
		return false;
	}

	void CPaintManagerUI::KillTimer (CControlUI* pControl) {
		ASSERT (pControl);
		int count = m_aTimers.GetSize ();
		for (int i = 0, j = 0; i < count; i++) {
			TIMERINFO* pTimer = static_cast<TIMERINFO*>(m_aTimers[i - j]);
			if (pTimer->pSender == pControl && pTimer->hWnd == m_hWndPaint) {
				if (pTimer->bKilled == false) ::KillTimer (pTimer->hWnd, pTimer->uWinTimer);
				delete pTimer;
				m_aTimers.Remove (i - j);
				j++;
			}
		}
	}

	void CPaintManagerUI::RemoveAllTimers () {
		for (int i = 0; i < m_aTimers.GetSize (); i++) {
			TIMERINFO* pTimer = static_cast<TIMERINFO*>(m_aTimers[i]);
			if (pTimer->hWnd == m_hWndPaint) {
				if (pTimer->bKilled == false) {
					if (::IsWindow (m_hWndPaint)) ::KillTimer (m_hWndPaint, pTimer->uWinTimer);
				}
				delete pTimer;
			}
		}

		m_aTimers.Empty ();
	}

	void CPaintManagerUI::SetCapture () {
		::SetCapture (m_hWndPaint);
		m_bMouseCapture = true;
	}

	void CPaintManagerUI::ReleaseCapture () {
		::ReleaseCapture ();
		m_bMouseCapture = false;
		m_bDragMode = false;
	}

	bool CPaintManagerUI::IsCaptured () {
		return m_bMouseCapture;
	}

	bool CPaintManagerUI::IsPainting () {
		return m_bIsPainting;
	}

	void CPaintManagerUI::SetPainting (bool bIsPainting) {
		m_bIsPainting = bIsPainting;
	}

	bool CPaintManagerUI::SetNextTabControl (bool bForward) {
		// If we're in the process of restructuring the layout we can delay the
		// focus calulation until the next repaint.
		if (m_bUpdateNeeded && bForward) {
			m_bFocusNeeded = true;
			::InvalidateRect (m_hWndPaint, nullptr, FALSE);
			return true;
		}
		// Find next/previous tabbable control
		FINDTABINFO info1 = { 0 };
		info1.pFocus = m_pFocus;
		info1.bForward = bForward;
		CControlUI* pControl = m_pRoot->FindControl (__FindControlFromTab, &info1, UIFIND_VISIBLE | UIFIND_ENABLED | UIFIND_ME_FIRST);
		if (!pControl) {
			if (bForward) {
				// Wrap around
				FINDTABINFO info2 = { 0 };
				info2.pFocus = bForward ? nullptr : info1.pLast;
				info2.bForward = bForward;
				pControl = m_pRoot->FindControl (__FindControlFromTab, &info2, UIFIND_VISIBLE | UIFIND_ENABLED | UIFIND_ME_FIRST);
			} else {
				pControl = info1.pLast;
			}
		}
		if (pControl) SetFocus (pControl);
		m_bFocusNeeded = false;
		return true;
	}

	bool CPaintManagerUI::AddNotifier (INotifyUI* pNotifier) {
		ASSERT (m_aNotifiers.Find (pNotifier) < 0);
		return m_aNotifiers.Add (pNotifier);
	}

	bool CPaintManagerUI::RemoveNotifier (INotifyUI* pNotifier) {
		for (int i = 0; i < m_aNotifiers.GetSize (); i++) {
			if (static_cast<INotifyUI*>(m_aNotifiers[i]) == pNotifier) {
				return m_aNotifiers.Remove (i);
			}
		}
		return false;
	}

	bool CPaintManagerUI::AddPreMessageFilter (IMessageFilterUI* pFilter) {
		ASSERT (m_aPreMessageFilters.Find (pFilter) < 0);
		return m_aPreMessageFilters.Add (pFilter);
	}

	bool CPaintManagerUI::RemovePreMessageFilter (IMessageFilterUI* pFilter) {
		for (int i = 0; i < m_aPreMessageFilters.GetSize (); i++) {
			if (static_cast<IMessageFilterUI*>(m_aPreMessageFilters[i]) == pFilter) {
				return m_aPreMessageFilters.Remove (i);
			}
		}
		return false;
	}

	bool CPaintManagerUI::AddMessageFilter (IMessageFilterUI* pFilter) {
		ASSERT (m_aMessageFilters.Find (pFilter) < 0);
		return m_aMessageFilters.Add (pFilter);
	}

	bool CPaintManagerUI::RemoveMessageFilter (IMessageFilterUI* pFilter) {
		for (int i = 0; i < m_aMessageFilters.GetSize (); i++) {
			if (static_cast<IMessageFilterUI*>(m_aMessageFilters[i]) == pFilter) {
				return m_aMessageFilters.Remove (i);
			}
		}
		return false;
	}

	int CPaintManagerUI::GetPostPaintCount () const {
		return m_aPostPaintControls.GetSize ();
	}

	bool CPaintManagerUI::IsPostPaint (CControlUI* pControl) {
		return m_aPostPaintControls.Find (pControl) >= 0;
	}

	bool CPaintManagerUI::AddPostPaint (CControlUI* pControl) {
		ASSERT (m_aPostPaintControls.Find (pControl) < 0);
		return m_aPostPaintControls.Add (pControl);
	}

	bool CPaintManagerUI::RemovePostPaint (CControlUI* pControl) {
		for (int i = 0; i < m_aPostPaintControls.GetSize (); i++) {
			if (static_cast<CControlUI*>(m_aPostPaintControls[i]) == pControl) {
				return m_aPostPaintControls.Remove (i);
			}
		}
		return false;
	}

	bool CPaintManagerUI::SetPostPaintIndex (CControlUI* pControl, int iIndex) {
		RemovePostPaint (pControl);
		return m_aPostPaintControls.InsertAt (iIndex, pControl);
	}

	int CPaintManagerUI::GetNativeWindowCount () const {
		return m_aNativeWindow.GetSize ();
	}

	bool CPaintManagerUI::AddNativeWindow (CControlUI* pControl, HWND hChildWnd) {
		if (!pControl || !hChildWnd) return false;

		RECT rcChildWnd = GetNativeWindowRect (hChildWnd);
		Invalidate (rcChildWnd);

		if (m_aNativeWindow.Find (hChildWnd) >= 0) return false;
		if (m_aNativeWindow.Add (hChildWnd)) {
			m_aNativeWindowControl.Add (pControl);
			return true;
		}
		return false;
	}

	bool CPaintManagerUI::RemoveNativeWindow (HWND hChildWnd) {
		for (int i = 0; i < m_aNativeWindow.GetSize (); i++) {
			if (static_cast<HWND>(m_aNativeWindow[i]) == hChildWnd) {
				if (m_aNativeWindow.Remove (i)) {
					m_aNativeWindowControl.Remove (i);
					return true;
				}
				return false;
			}
		}
		return false;
	}

	RECT CPaintManagerUI::GetNativeWindowRect (HWND hChildWnd) {
		RECT rcChildWnd = { 0 };
		::GetWindowRect (hChildWnd, &rcChildWnd);
		::ScreenToClient (m_hWndPaint, (LPPOINT) (&rcChildWnd));
		::ScreenToClient (m_hWndPaint, (LPPOINT) (&rcChildWnd) + 1);
		return rcChildWnd;
	}

	void CPaintManagerUI::AddDelayedCleanup (CControlUI* pControl) {
		if (!pControl) return;
		pControl->SetManager (this, nullptr, false);
		m_aDelayedCleanup.Add (pControl);
		PostAsyncNotify ();
	}

	void CPaintManagerUI::AddMouseLeaveNeeded (CControlUI* pControl) {
		if (!pControl) return;
		for (int i = 0; i < m_aNeedMouseLeaveNeeded.GetSize (); i++) {
			if (static_cast<CControlUI*>(m_aNeedMouseLeaveNeeded[i]) == pControl) {
				return;
			}
		}
		m_aNeedMouseLeaveNeeded.Add (pControl);
	}

	bool CPaintManagerUI::RemoveMouseLeaveNeeded (CControlUI* pControl) {
		if (!pControl) return false;
		for (int i = 0; i < m_aNeedMouseLeaveNeeded.GetSize (); i++) {
			if (static_cast<CControlUI*>(m_aNeedMouseLeaveNeeded[i]) == pControl) {
				return m_aNeedMouseLeaveNeeded.Remove (i);
			}
		}
		return false;
	}

	void CPaintManagerUI::SendNotify (CControlUI* pControl, faw::string_t pstrMessage, WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/, bool bAsync /*= false*/) {
		TNotifyUI Msg;
		Msg.pSender = pControl;
		Msg.sType = pstrMessage;
		Msg.wParam = wParam;
		Msg.lParam = lParam;
		SendNotify (Msg, bAsync);
	}

	void CPaintManagerUI::SendNotify (TNotifyUI& Msg, bool bAsync /*= false*/) {
		Msg.ptMouse = m_ptLastMousePos;
		Msg.dwTimestamp = ::GetTickCount ();
		if (m_bUsedVirtualWnd) {
			Msg.sVirtualWnd = Msg.pSender->GetVirtualWnd ();
		}

		if (!bAsync) {
			// Send to all listeners
			if (Msg.pSender) {
				if (Msg.pSender->OnNotify) Msg.pSender->OnNotify (&Msg);
			}
			for (int i = 0; i < m_aNotifiers.GetSize (); i++) {
				static_cast<INotifyUI*>(m_aNotifiers[i])->Notify (Msg);
			}
		} else {
			TNotifyUI *pMsg = new TNotifyUI;
			pMsg->pSender = Msg.pSender;
			pMsg->sType = Msg.sType;
			pMsg->wParam = Msg.wParam;
			pMsg->lParam = Msg.lParam;
			pMsg->ptMouse = Msg.ptMouse;
			pMsg->dwTimestamp = Msg.dwTimestamp;
			m_aAsyncNotify.Add (pMsg);

			PostAsyncNotify ();
		}
	}

	bool CPaintManagerUI::IsForceUseSharedRes () const {
		return m_bForceUseSharedRes;
	}

	void CPaintManagerUI::SetForceUseSharedRes (bool bForce) {
		m_bForceUseSharedRes = bForce;
	}

	DWORD CPaintManagerUI::GetDefaultDisabledColor () const {
		return m_ResInfo.m_dwDefaultDisabledColor;
	}

	void CPaintManagerUI::SetDefaultDisabledColor (DWORD dwColor, bool bShared) {
		if (bShared) {
			if (m_ResInfo.m_dwDefaultDisabledColor == m_SharedResInfo.m_dwDefaultDisabledColor)
				m_ResInfo.m_dwDefaultDisabledColor = dwColor;
			m_SharedResInfo.m_dwDefaultDisabledColor = dwColor;
		} else {
			m_ResInfo.m_dwDefaultDisabledColor = dwColor;
		}
	}

	DWORD CPaintManagerUI::GetDefaultFontColor () const {
		return m_ResInfo.m_dwDefaultFontColor;
	}

	void CPaintManagerUI::SetDefaultFontColor (DWORD dwColor, bool bShared) {
		if (bShared) {
			if (m_ResInfo.m_dwDefaultFontColor == m_SharedResInfo.m_dwDefaultFontColor)
				m_ResInfo.m_dwDefaultFontColor = dwColor;
			m_SharedResInfo.m_dwDefaultFontColor = dwColor;
		} else {
			m_ResInfo.m_dwDefaultFontColor = dwColor;
		}
	}

	DWORD CPaintManagerUI::GetDefaultLinkFontColor () const {
		return m_ResInfo.m_dwDefaultLinkFontColor;
	}

	void CPaintManagerUI::SetDefaultLinkFontColor (DWORD dwColor, bool bShared) {
		if (bShared) {
			if (m_ResInfo.m_dwDefaultLinkFontColor == m_SharedResInfo.m_dwDefaultLinkFontColor)
				m_ResInfo.m_dwDefaultLinkFontColor = dwColor;
			m_SharedResInfo.m_dwDefaultLinkFontColor = dwColor;
		} else {
			m_ResInfo.m_dwDefaultLinkFontColor = dwColor;
		}
	}

	DWORD CPaintManagerUI::GetDefaultLinkHoverFontColor () const {
		return m_ResInfo.m_dwDefaultLinkHoverFontColor;
	}

	void CPaintManagerUI::SetDefaultLinkHoverFontColor (DWORD dwColor, bool bShared) {
		if (bShared) {
			if (m_ResInfo.m_dwDefaultLinkHoverFontColor == m_SharedResInfo.m_dwDefaultLinkHoverFontColor)
				m_ResInfo.m_dwDefaultLinkHoverFontColor = dwColor;
			m_SharedResInfo.m_dwDefaultLinkHoverFontColor = dwColor;
		} else {
			m_ResInfo.m_dwDefaultLinkHoverFontColor = dwColor;
		}
	}

	DWORD CPaintManagerUI::GetDefaultSelectedBkColor () const {
		return m_ResInfo.m_dwDefaultSelectedBkColor;
	}

	void CPaintManagerUI::SetDefaultSelectedBkColor (DWORD dwColor, bool bShared) {
		if (bShared) {
			if (m_ResInfo.m_dwDefaultSelectedBkColor == m_SharedResInfo.m_dwDefaultSelectedBkColor)
				m_ResInfo.m_dwDefaultSelectedBkColor = dwColor;
			m_SharedResInfo.m_dwDefaultSelectedBkColor = dwColor;
		} else {
			m_ResInfo.m_dwDefaultSelectedBkColor = dwColor;
		}
	}

	TFontInfo* CPaintManagerUI::GetDefaultFontInfo () {
		if (m_ResInfo.m_DefaultFontInfo.sFontName.empty ()) {
			if (m_SharedResInfo.m_DefaultFontInfo.tm.tmHeight == 0) {
				HFONT hOldFont = (HFONT) ::SelectObject (m_hDcPaint, m_SharedResInfo.m_DefaultFontInfo.hFont);
				::GetTextMetrics (m_hDcPaint, &m_SharedResInfo.m_DefaultFontInfo.tm);
				::SelectObject (m_hDcPaint, hOldFont);
			}
			return &m_SharedResInfo.m_DefaultFontInfo;
		} else {
			if (m_ResInfo.m_DefaultFontInfo.tm.tmHeight == 0) {
				HFONT hOldFont = (HFONT) ::SelectObject (m_hDcPaint, m_ResInfo.m_DefaultFontInfo.hFont);
				::GetTextMetrics (m_hDcPaint, &m_ResInfo.m_DefaultFontInfo.tm);
				::SelectObject (m_hDcPaint, hOldFont);
			}
			return &m_ResInfo.m_DefaultFontInfo;
		}
	}

	void CPaintManagerUI::SetDefaultFont (faw::string_t pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic, bool bShared) {
		LOGFONT lf = { 0 };
		::GetObject (::GetStockObject (DEFAULT_GUI_FONT), sizeof (LOGFONT), &lf);
		if (pStrFontName.length () > 0) {
			_tcsncpy (lf.lfFaceName, pStrFontName.data (), lengthof (lf.lfFaceName));
		}
		lf.lfCharSet = DEFAULT_CHARSET;
		lf.lfHeight = -GetDPIObj ()->Scale (nSize);;
		if (bBold) lf.lfWeight += FW_BOLD;
		if (bUnderline) lf.lfUnderline = TRUE;
		if (bItalic) lf.lfItalic = TRUE;

		HFONT hFont = ::CreateFontIndirect (&lf);
		if (!hFont) return;

		if (bShared) {
			::DeleteObject (m_SharedResInfo.m_DefaultFontInfo.hFont);
			m_SharedResInfo.m_DefaultFontInfo.hFont = hFont;
			m_SharedResInfo.m_DefaultFontInfo.sFontName = lf.lfFaceName;
			m_SharedResInfo.m_DefaultFontInfo.iSize = nSize;
			m_SharedResInfo.m_DefaultFontInfo.bBold = bBold;
			m_SharedResInfo.m_DefaultFontInfo.bUnderline = bUnderline;
			m_SharedResInfo.m_DefaultFontInfo.bItalic = bItalic;
			::ZeroMemory (&m_SharedResInfo.m_DefaultFontInfo.tm, sizeof (m_SharedResInfo.m_DefaultFontInfo.tm));
			if (m_hDcPaint) {
				HFONT hOldFont = (HFONT) ::SelectObject (m_hDcPaint, hFont);
				::GetTextMetrics (m_hDcPaint, &m_SharedResInfo.m_DefaultFontInfo.tm);
				::SelectObject (m_hDcPaint, hOldFont);
			}
		} else {
			::DeleteObject (m_ResInfo.m_DefaultFontInfo.hFont);
			m_ResInfo.m_DefaultFontInfo.hFont = hFont;
			m_ResInfo.m_DefaultFontInfo.sFontName = lf.lfFaceName;
			m_ResInfo.m_DefaultFontInfo.iSize = nSize;
			m_ResInfo.m_DefaultFontInfo.bBold = bBold;
			m_ResInfo.m_DefaultFontInfo.bUnderline = bUnderline;
			m_ResInfo.m_DefaultFontInfo.bItalic = bItalic;
			::ZeroMemory (&m_ResInfo.m_DefaultFontInfo.tm, sizeof (m_ResInfo.m_DefaultFontInfo.tm));
			if (m_hDcPaint) {
				HFONT hOldFont = (HFONT) ::SelectObject (m_hDcPaint, hFont);
				::GetTextMetrics (m_hDcPaint, &m_ResInfo.m_DefaultFontInfo.tm);
				::SelectObject (m_hDcPaint, hOldFont);
			}
		}
	}

	DWORD CPaintManagerUI::GetCustomFontCount (bool bShared) const {
		if (bShared)
			return m_SharedResInfo.m_CustomFonts.GetSize ();
		else
			return m_ResInfo.m_CustomFonts.GetSize ();
	}

	static int CALLBACK _enum_font_cb (CONST LOGFONTA *, CONST TEXTMETRICA *, DWORD, LPARAM lParam) {
		return 10;
	}

	static faw::string_t _CheckFontExist (HDC _dc, faw::string_t _fonts) {
		auto _v = FawTools::split (_fonts, _T (','));
		LOGFONT _lf = { 0 };
		for (auto _font : _v) {
			lstrcpy (_lf.lfFaceName, _font.data ());
			if (EnumFontFamiliesEx (_dc, &_lf, (FONTENUMPROC) _enum_font_cb, 0, 0) == 10)
				return _font;
		}
		//AddFontMemResourceEx添加的字体找不到,先创建看看,可以创建就可以用
		if (_fonts.length () > 0) {
			_tcsncpy (_lf.lfFaceName, _fonts.data (), lengthof (_lf.lfFaceName));
			HFONT hFont = ::CreateFontIndirect (&_lf);
			if (hFont) return _fonts;
		}
		return _T ("Microsoft YaHei");
	}

	HFONT CPaintManagerUI::AddFont (int id, faw::string_t pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic, bool bShared) {
		pStrFontName = _CheckFontExist (m_hDcPaint, pStrFontName);
		LOGFONT lf = { 0 };
		::GetObject (::GetStockObject (DEFAULT_GUI_FONT), sizeof (LOGFONT), &lf);
		if (pStrFontName.length () > 0) {
			_tcsncpy (lf.lfFaceName, pStrFontName.data (), lengthof (lf.lfFaceName));
		}
		lf.lfCharSet = DEFAULT_CHARSET;
		lf.lfHeight = -GetDPIObj ()->Scale (nSize);
		if (bBold) lf.lfWeight = FW_BOLD;
		if (bUnderline) lf.lfUnderline = TRUE;
		if (bItalic) lf.lfItalic = TRUE;
		HFONT hFont = ::CreateFontIndirect (&lf);
		if (!hFont) return nullptr;

		TFontInfo* pFontInfo = new TFontInfo;
		if (!pFontInfo) return nullptr;
		pFontInfo->hFont = hFont;
		pFontInfo->sFontName = lf.lfFaceName;
		pFontInfo->iSize = nSize;
		pFontInfo->bBold = bBold;
		pFontInfo->bUnderline = bUnderline;
		pFontInfo->bItalic = bItalic;
		if (m_hDcPaint) {
			HFONT hOldFont = (HFONT) ::SelectObject (m_hDcPaint, hFont);
			::GetTextMetrics (m_hDcPaint, &pFontInfo->tm);
			::SelectObject (m_hDcPaint, hOldFont);
		}
		TCHAR idBuffer[16];
		::ZeroMemory (idBuffer, sizeof (idBuffer));
		_itot (id, idBuffer, 10);
		if (bShared || m_bForceUseSharedRes) {
			TFontInfo* pOldFontInfo = static_cast<TFontInfo*>(m_SharedResInfo.m_CustomFonts.Find (idBuffer));
			if (pOldFontInfo) {
				::DeleteObject (pOldFontInfo->hFont);
				delete pOldFontInfo;
				m_SharedResInfo.m_CustomFonts.Remove (idBuffer);
			}

			if (!m_SharedResInfo.m_CustomFonts.Insert (idBuffer, pFontInfo)) {
				::DeleteObject (hFont);
				delete pFontInfo;
				return nullptr;
			}
		} else {
			TFontInfo* pOldFontInfo = static_cast<TFontInfo*>(m_ResInfo.m_CustomFonts.Find (idBuffer));
			if (pOldFontInfo) {
				::DeleteObject (pOldFontInfo->hFont);
				delete pOldFontInfo;
				m_ResInfo.m_CustomFonts.Remove (idBuffer);
			}

			if (!m_ResInfo.m_CustomFonts.Insert (idBuffer, pFontInfo)) {
				::DeleteObject (hFont);
				delete pFontInfo;
				return nullptr;
			}
		}

		return hFont;
	}
	void CPaintManagerUI::AddFontArray (faw::string_t pstrPath) {
		LPBYTE pData = nullptr;
		DWORD dwSize = 0;
		do {
			faw::string_t sFile = CPaintManagerUI::GetResourcePath ();
			if (CPaintManagerUI::GetResourceZip ().empty ()) {
				sFile += pstrPath;
				HANDLE hFile = ::CreateFile (sFile.c_str (), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFile == INVALID_HANDLE_VALUE) break;
				dwSize = ::GetFileSize (hFile, nullptr);
				if (dwSize == 0) break;

				DWORD dwRead = 0;
				pData = new BYTE[dwSize];
				::ReadFile (hFile, pData, dwSize, &dwRead, nullptr);
				::CloseHandle (hFile);

				if (dwRead != dwSize) {
					delete[] pData;
					pData = nullptr;
					break;
				}
			} else {
				sFile += CPaintManagerUI::GetResourceZip ();
				HZIP hz = nullptr;
				if (CPaintManagerUI::IsCachedResourceZip ()) hz = (HZIP) CPaintManagerUI::GetResourceZipHandle ();
				else {
					faw::string_t sFilePwd = CPaintManagerUI::GetResourceZipPwd ();
					std::string pwd = FawTools::T_to_gb18030 (sFilePwd);
					hz = OpenZip (sFile.c_str (), pwd.c_str ());
				}
				if (!hz) break;
				ZIPENTRY ze;
				int i = 0;
				faw::string_t key = pstrPath;
				FawTools::replace_self (key, _T ("\\"), _T ("/"));
				if (FindZipItem (hz, key.c_str (), true, &i, &ze) != 0) break;
				dwSize = ze.unc_size;
				if (dwSize == 0) break;
				pData = new BYTE[dwSize];
				int res = UnzipItem (hz, i, pData, dwSize);
				if (res != 0x00000000 && res != 0x00000600) {
					delete[] pData;
					pData = nullptr;
					if (!CPaintManagerUI::IsCachedResourceZip ()) CloseZip (hz);
					break;
				}
				if (!CPaintManagerUI::IsCachedResourceZip ()) CloseZip (hz);
			}

		} while (0);

		while (!pData) {
			//读不到图片, 则直接去读取bitmap.m_lpstr指向的路径
			HANDLE hFile = ::CreateFile (pstrPath.data (), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile == INVALID_HANDLE_VALUE) break;
			dwSize = ::GetFileSize (hFile, nullptr);
			if (dwSize == 0) break;

			DWORD dwRead = 0;
			pData = new BYTE[dwSize];
			::ReadFile (hFile, pData, dwSize, &dwRead, nullptr);
			::CloseHandle (hFile);

			if (dwRead != dwSize) {
				delete[] pData;
				pData = nullptr;
			}
			break;
		}
		if (!pData || dwSize == 0) return;

		DWORD nFonts;
		//AddFontMemResourceEx 添加的字体始终是发出调用且不可枚举的进程专用的。
		HANDLE hFont = ::AddFontMemResourceEx (pData, dwSize, nullptr, &nFonts);
		m_aFonts.Add (hFont);

		// 添加字体
		Gdiplus::PrivateFontCollection* pCollection = new Gdiplus::PrivateFontCollection ();
		pCollection->AddMemoryFont (pData, dwSize);
		delete[] pData;
		pData = nullptr;

		int count = pCollection->GetFamilyCount ();
		if (count == 0) {
			delete pCollection;
			return;
		}

		int found = 0;
		Gdiplus::FontFamily* pFontFamily = (Gdiplus::FontFamily*) malloc (count * sizeof (Gdiplus::FontFamily));
		pCollection->GetFamilies (count, pFontFamily, &found);
		if (found == 0) {
			free (pFontFamily);
			delete pCollection;
			return;
		}
		// 获取字体名称
		WCHAR  fontFamilyName[MAX_PATH] = { 0 };
		pFontFamily->GetFamilyName (fontFamilyName);
		free (pFontFamily);

#ifdef _UNICODE
		if (!m_mGDIPlusFontCollection.Find (fontFamilyName))
			m_mGDIPlusFontCollection.Set (fontFamilyName, pCollection);
#else
		auto FontFamilyName = FawTools::utf16_to_gb18030 (fontFamilyName);
		if (!m_mGDIPlusFontCollection.Find (FontFamilyName))
			m_mGDIPlusFontCollection.Set (FontFamilyName, pCollection);
#endif // _UNICODE
		else
			delete pCollection;
	}
	HFONT CPaintManagerUI::GetFont (int id) {
		if (id < 0) return GetDefaultFontInfo ()->hFont;

		TCHAR idBuffer[16];
		::ZeroMemory (idBuffer, sizeof (idBuffer));
		_itot (id, idBuffer, 10);
		TFontInfo* pFontInfo = static_cast<TFontInfo*>(m_ResInfo.m_CustomFonts.Find (idBuffer));
		if (!pFontInfo) pFontInfo = static_cast<TFontInfo*>(m_SharedResInfo.m_CustomFonts.Find (idBuffer));
		if (!pFontInfo) return GetDefaultFontInfo ()->hFont;
		return pFontInfo->hFont;
	}

	HFONT CPaintManagerUI::GetFont (faw::string_t pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic) {
		TFontInfo* pFontInfo = nullptr;
		for (int i = 0; i < m_ResInfo.m_CustomFonts.GetSize (); i++) {
			faw::string_t key = m_ResInfo.m_CustomFonts.GetAt (i)->Key;
			if (!key.empty ()) {
				pFontInfo = static_cast<TFontInfo*>(m_ResInfo.m_CustomFonts.Find (key));
				if (pFontInfo && pFontInfo->sFontName == pStrFontName && pFontInfo->iSize == nSize &&
					pFontInfo->bBold == bBold && pFontInfo->bUnderline == bUnderline && pFontInfo->bItalic == bItalic)
					return pFontInfo->hFont;
			}
		}
		for (int i = 0; i < m_SharedResInfo.m_CustomFonts.GetSize (); i++) {
			faw::string_t key = m_SharedResInfo.m_CustomFonts.GetAt (i)->Key;
			if (!key.empty ()) {
				pFontInfo = static_cast<TFontInfo*>(m_SharedResInfo.m_CustomFonts.Find (key));
				if (pFontInfo && pFontInfo->sFontName == pStrFontName && pFontInfo->iSize == nSize &&
					pFontInfo->bBold == bBold && pFontInfo->bUnderline == bUnderline && pFontInfo->bItalic == bItalic)
					return pFontInfo->hFont;
			}
		}

		return nullptr;
	}

	Gdiplus::Font* CPaintManagerUI::GetResourceFont (faw::string_t pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic)
	{
		auto pCollection = (Gdiplus::PrivateFontCollection*) m_mGDIPlusFontCollection.Find (pStrFontName);
		if (pCollection) {
			Gdiplus::FontFamily fontFamily;
			int nNumFound = 0;
			pCollection->GetFamilies (1, &fontFamily, &nNumFound);

			if (nNumFound > 0) {
				int fontStyle = Gdiplus::FontStyle::FontStyleRegular;
				if (bBold && bItalic)
					fontStyle = Gdiplus::FontStyle::FontStyleBoldItalic;
				else if (bBold)
					fontStyle = Gdiplus::FontStyle::FontStyleBold;
				else if (bUnderline)
					fontStyle = Gdiplus::FontStyle::FontStyleUnderline;
				else if (bItalic)
					fontStyle = Gdiplus::FontStyle::FontStyleItalic;
				Gdiplus::Font* font = Gdiplus::Font (&fontFamily, nSize, fontStyle, Gdiplus::Unit::UnitPixel).Clone ();
				return font;
			}
		}
		return Gdiplus::Font (GetDC (NULL), GetFont (-1)).Clone ();
	}

	int CPaintManagerUI::GetFontIndex (HFONT hFont, bool bShared) {
		TFontInfo* pFontInfo = nullptr;
		if (bShared) {
			for (int i = 0; i < m_SharedResInfo.m_CustomFonts.GetSize (); i++) {
				faw::string_t key = m_SharedResInfo.m_CustomFonts.GetAt (i)->Key;
				if (!key.empty ()) {
					pFontInfo = static_cast<TFontInfo*>(m_SharedResInfo.m_CustomFonts.Find (key));
					if (pFontInfo && pFontInfo->hFont == hFont) return _ttoi (key.c_str ());
				}
			}
		} else {
			for (int i = 0; i < m_ResInfo.m_CustomFonts.GetSize (); i++) {
				faw::string_t key = m_ResInfo.m_CustomFonts.GetAt (i)->Key;
				if (!key.empty ()) {
					pFontInfo = static_cast<TFontInfo*>(m_ResInfo.m_CustomFonts.Find (key));
					if (pFontInfo && pFontInfo->hFont == hFont) return _ttoi (key.c_str ());
				}
			}
		}

		return -1;
	}

	int CPaintManagerUI::GetFontIndex (faw::string_t pStrFontName, int nSize, bool bBold, bool bUnderline, bool bItalic, bool bShared) {
		TFontInfo* pFontInfo = nullptr;
		if (bShared) {
			for (int i = 0; i < m_SharedResInfo.m_CustomFonts.GetSize (); i++) {
				faw::string_t key = m_SharedResInfo.m_CustomFonts.GetAt (i)->Key;
				if (!key.empty ()) {
					pFontInfo = static_cast<TFontInfo*>(m_SharedResInfo.m_CustomFonts.Find (key));
					if (pFontInfo && pFontInfo->sFontName == pStrFontName && pFontInfo->iSize == nSize &&
						pFontInfo->bBold == bBold && pFontInfo->bUnderline == bUnderline && pFontInfo->bItalic == bItalic)
						return _ttoi (key.c_str ());
				}
			}
		} else {
			for (int i = 0; i < m_ResInfo.m_CustomFonts.GetSize (); i++) {
				faw::string_t key = m_ResInfo.m_CustomFonts.GetAt (i)->Key;
				if (!key.empty ()) {
					pFontInfo = static_cast<TFontInfo*>(m_ResInfo.m_CustomFonts.Find (key));
					if (pFontInfo && pFontInfo->sFontName == pStrFontName && pFontInfo->iSize == nSize &&
						pFontInfo->bBold == bBold && pFontInfo->bUnderline == bUnderline && pFontInfo->bItalic == bItalic)
						return _ttoi (key.c_str ());
				}
			}
		}

		return -1;
	}

	void CPaintManagerUI::RemoveFont (HFONT hFont, bool bShared) {
		TFontInfo* pFontInfo = nullptr;
		if (bShared) {
			for (int i = 0; i < m_SharedResInfo.m_CustomFonts.GetSize (); i++) {
				faw::string_t key = m_SharedResInfo.m_CustomFonts.GetAt (i)->Key;
				if (!key.empty ()) {
					pFontInfo = static_cast<TFontInfo*>(m_SharedResInfo.m_CustomFonts.Find (key));
					if (pFontInfo && pFontInfo->hFont == hFont) {
						::DeleteObject (pFontInfo->hFont);
						delete pFontInfo;
						m_SharedResInfo.m_CustomFonts.Remove (key.c_str ());
						return;
					}
				}
			}
		} else {
			for (int i = 0; i < m_ResInfo.m_CustomFonts.GetSize (); i++) {
				faw::string_t key = m_ResInfo.m_CustomFonts.GetAt (i)->Key;
				if (!key.empty ()) {
					pFontInfo = static_cast<TFontInfo*>(m_ResInfo.m_CustomFonts.Find (key));
					if (pFontInfo && pFontInfo->hFont == hFont) {
						::DeleteObject (pFontInfo->hFont);
						delete pFontInfo;
						m_ResInfo.m_CustomFonts.Remove (key);
						return;
					}
				}
			}
		}
	}

	void CPaintManagerUI::RemoveFont (int id, bool bShared) {
		TCHAR idBuffer[16];
		::ZeroMemory (idBuffer, sizeof (idBuffer));
		_itot (id, idBuffer, 10);

		TFontInfo* pFontInfo = nullptr;
		if (bShared) {
			pFontInfo = static_cast<TFontInfo*>(m_SharedResInfo.m_CustomFonts.Find (idBuffer));
			if (pFontInfo) {
				::DeleteObject (pFontInfo->hFont);
				delete pFontInfo;
				m_SharedResInfo.m_CustomFonts.Remove (idBuffer);
			}
		} else {
			pFontInfo = static_cast<TFontInfo*>(m_ResInfo.m_CustomFonts.Find (idBuffer));
			if (pFontInfo) {
				::DeleteObject (pFontInfo->hFont);
				delete pFontInfo;
				m_ResInfo.m_CustomFonts.Remove (idBuffer);
			}
		}
	}

	void CPaintManagerUI::RemoveAllFonts (bool bShared) {
		TFontInfo* pFontInfo;
		if (bShared) {
			for (int i = 0; i < m_SharedResInfo.m_CustomFonts.GetSize (); i++) {
				faw::string_t key = m_SharedResInfo.m_CustomFonts.GetAt (i)->Key;
				if (!key.empty ()) {
					pFontInfo = static_cast<TFontInfo*>(m_SharedResInfo.m_CustomFonts.Find (key, false));
					if (pFontInfo) {
						::DeleteObject (pFontInfo->hFont);
						delete pFontInfo;
					}
				}
			}
			m_SharedResInfo.m_CustomFonts.RemoveAll ();
		} else {
			for (int i = 0; i < m_ResInfo.m_CustomFonts.GetSize (); i++) {
				faw::string_t key = m_ResInfo.m_CustomFonts.GetAt (i)->Key;
				if (!key.empty ()) {
					pFontInfo = static_cast<TFontInfo*>(m_ResInfo.m_CustomFonts.Find (key, false));
					if (pFontInfo) {
						::DeleteObject (pFontInfo->hFont);
						delete pFontInfo;
					}
				}
			}
			m_ResInfo.m_CustomFonts.RemoveAll ();
		}
	}

	void CPaintManagerUI::RemoveResourceFontCollection ()
	{
		for (int i = 0; i < m_mGDIPlusFontCollection.GetSize (); i++) {
			faw::string_t key = m_mGDIPlusFontCollection.GetAt (i)->Key;
			if (!key.empty ()) {
				auto FontCollection = static_cast<Gdiplus::PrivateFontCollection*> (m_mGDIPlusFontCollection.Find (key, false));
				if (FontCollection)
					delete FontCollection;
			}
		}
		m_mGDIPlusFontCollection.RemoveAll ();
	}

	TFontInfo* CPaintManagerUI::GetFontInfo (int id) {
		if (id < 0) return GetDefaultFontInfo ();

		TCHAR idBuffer[16];
		::ZeroMemory (idBuffer, sizeof (idBuffer));
		_itot (id, idBuffer, 10);
		TFontInfo* pFontInfo = static_cast<TFontInfo*>(m_ResInfo.m_CustomFonts.Find (idBuffer));
		if (!pFontInfo) pFontInfo = static_cast<TFontInfo*>(m_SharedResInfo.m_CustomFonts.Find (idBuffer));
		if (!pFontInfo) pFontInfo = GetDefaultFontInfo ();
		if (pFontInfo->tm.tmHeight == 0) {
			HFONT hOldFont = (HFONT) ::SelectObject (m_hDcPaint, pFontInfo->hFont);
			::GetTextMetrics (m_hDcPaint, &pFontInfo->tm);
			::SelectObject (m_hDcPaint, hOldFont);
		}
		return pFontInfo;
	}

	TFontInfo* CPaintManagerUI::GetFontInfo (HFONT hFont) {
		TFontInfo* pFontInfo = nullptr;
		for (int i = 0; i < m_ResInfo.m_CustomFonts.GetSize (); i++) {
			faw::string_t key = m_ResInfo.m_CustomFonts.GetAt (i)->Key;
			if (!key.empty ()) {
				pFontInfo = static_cast<TFontInfo*>(m_ResInfo.m_CustomFonts.Find (key));
				if (pFontInfo && pFontInfo->hFont == hFont) break;
			}
		}
		if (!pFontInfo) {
			for (int i = 0; i < m_SharedResInfo.m_CustomFonts.GetSize (); i++) {
				faw::string_t key = m_SharedResInfo.m_CustomFonts.GetAt (i)->Key;
				if (!key.empty ()) {
					pFontInfo = static_cast<TFontInfo*>(m_SharedResInfo.m_CustomFonts.Find (key));
					if (pFontInfo && pFontInfo->hFont == hFont) break;
				}
			}
		}
		if (!pFontInfo) pFontInfo = GetDefaultFontInfo ();
		if (pFontInfo->tm.tmHeight == 0) {
			HFONT hOldFont = (HFONT) ::SelectObject (m_hDcPaint, pFontInfo->hFont);
			::GetTextMetrics (m_hDcPaint, &pFontInfo->tm);
			::SelectObject (m_hDcPaint, hOldFont);
		}
		return pFontInfo;
	}

	const TImageInfo* CPaintManagerUI::GetImage (faw::string_t bitmap) {
		TImageInfo* data = static_cast<TImageInfo*>(m_ResInfo.m_ImageHash.Find (bitmap));
		if (!data) data = static_cast<TImageInfo*>(m_SharedResInfo.m_ImageHash.Find (bitmap));
		return data;
	}

	const TImageInfo* CPaintManagerUI::GetImageEx (faw::string_t bitmap, faw::string_t type, DWORD mask, bool bUseHSL, HINSTANCE instance) {
		const TImageInfo* data = GetImage (bitmap);
		if (!data) {
			if (AddImage (bitmap, type, mask, bUseHSL, false, instance)) {
				if (m_bForceUseSharedRes) data = static_cast<TImageInfo*>(m_SharedResInfo.m_ImageHash.Find (bitmap));
				else data = static_cast<TImageInfo*>(m_ResInfo.m_ImageHash.Find (bitmap));
			}
		}

		return data;
	}

	const TImageInfo* CPaintManagerUI::AddImage (faw::string_t bitmap, faw::string_t type, DWORD mask, bool bUseHSL, bool bShared, HINSTANCE instance) {
		if (bitmap.empty ()) return nullptr;

		TImageInfo* data = nullptr;
		if (type.length () > 0) {
			if (isdigit (bitmap[0])) {
				int iIndex = _ttoi (bitmap.data ());
				data = CRenderEngine::LoadImage (iIndex, type.data (), mask, instance);
			}
		} else {
			data = CRenderEngine::LoadImage (bitmap, _T (""), mask, instance);
		}

		if (!data) {
			return nullptr;
		}
		data->bUseHSL = bUseHSL;
		if (!type.empty ()) data->sResType = type;
		data->dwMask = mask;
		if (data->bUseHSL) {
			data->pSrcBits = new BYTE[data->nX * data->nY * 4];
			::CopyMemory (data->pSrcBits, data->pBits, data->nX * data->nY * 4);
		} else data->pSrcBits = nullptr;
		if (m_bUseHSL) CRenderEngine::AdjustImage (true, data, m_H, m_S, m_L);
		if (data) {
			if (bShared || m_bForceUseSharedRes) {
				TImageInfo* pOldImageInfo = static_cast<TImageInfo*>(m_SharedResInfo.m_ImageHash.Find (bitmap));
				if (pOldImageInfo) {
					CRenderEngine::FreeImage (pOldImageInfo);
					m_SharedResInfo.m_ImageHash.Remove (bitmap);
				}

				if (!m_SharedResInfo.m_ImageHash.Insert (bitmap, data)) {
					CRenderEngine::FreeImage (data);
					data = nullptr;
				}
			} else {
				TImageInfo* pOldImageInfo = static_cast<TImageInfo*>(m_ResInfo.m_ImageHash.Find (bitmap));
				if (pOldImageInfo) {
					CRenderEngine::FreeImage (pOldImageInfo);
					m_ResInfo.m_ImageHash.Remove (bitmap);
				}

				if (!m_ResInfo.m_ImageHash.Insert (bitmap, data)) {
					CRenderEngine::FreeImage (data);
					data = nullptr;
				}
			}
		}

		return data;
	}

	const TImageInfo* CPaintManagerUI::AddImage (faw::string_t bitmap, HBITMAP hBitmap, int iWidth, int iHeight, bool bAlpha, bool bShared) {
		// 因无法确定外部HBITMAP格式，不能使用hsl调整
		if (bitmap.empty ()) return nullptr;
		if (!hBitmap || iWidth <= 0 || iHeight <= 0) return nullptr;

		TImageInfo* data = new TImageInfo;
		data->pBits = nullptr;
		data->pSrcBits = nullptr;
		data->hBitmap = hBitmap;
		data->pBits = nullptr;
		data->nX = iWidth;
		data->nY = iHeight;
		data->bAlpha = bAlpha;
		data->bUseHSL = false;
		data->pSrcBits = nullptr;
		data->dwMask = 0;

		if (bShared || m_bForceUseSharedRes) {
			if (!m_SharedResInfo.m_ImageHash.Insert (bitmap, data)) {
				CRenderEngine::FreeImage (data);
				data = nullptr;
			}
		} else {
			if (!m_ResInfo.m_ImageHash.Insert (bitmap, data)) {
				CRenderEngine::FreeImage (data);
				data = nullptr;
			}
		}

		return data;
	}

	const TImageInfo* CPaintManagerUI::AddImage (faw::string_t bitmap, HBITMAP *phBitmap, int iWidth, int iHeight, bool bAlpha, bool bShared) {
		// 因无法确定外部HBITMAP格式，不能使用hsl调整
		if (bitmap.empty ()) return nullptr;
		if (!phBitmap || !*phBitmap || iWidth <= 0 || iHeight <= 0) return nullptr;

		TImageInfo* data = new TImageInfo;
		data->pBits = nullptr;
		data->pSrcBits = nullptr;
		data->phBitmap = phBitmap;
		data->pBits = nullptr;
		data->nX = iWidth;
		data->nY = iHeight;
		data->bAlpha = bAlpha;
		data->bUseHSL = false;
		data->pSrcBits = nullptr;
		data->dwMask = 0;

		if (bShared || m_bForceUseSharedRes) {
			if (!m_SharedResInfo.m_ImageHash.Insert (bitmap, data)) {
				CRenderEngine::FreeImage (data);
				data = nullptr;
			}
		} else {
			if (!m_ResInfo.m_ImageHash.Insert (bitmap, data)) {
				CRenderEngine::FreeImage (data);
				data = nullptr;
			}
		}

		return data;
	}

	void CPaintManagerUI::RemoveImage (faw::string_t bitmap, bool bShared) {
		TImageInfo* data = nullptr;
		if (bShared) {
			data = static_cast<TImageInfo*>(m_SharedResInfo.m_ImageHash.Find (bitmap));
			if (data) {
				CRenderEngine::FreeImage (data);
				m_SharedResInfo.m_ImageHash.Remove (bitmap);
			}
		} else {
			data = static_cast<TImageInfo*>(m_ResInfo.m_ImageHash.Find (bitmap));
			if (data) {
				CRenderEngine::FreeImage (data);
				m_ResInfo.m_ImageHash.Remove (bitmap);
			}
		}
	}

	void CPaintManagerUI::RemoveAllImages (bool bShared) {
		if (bShared) {
			TImageInfo* data;
			for (int i = 0; i < m_SharedResInfo.m_ImageHash.GetSize (); i++) {
				faw::string_t key = m_SharedResInfo.m_ImageHash.GetAt (i)->Key;
				if (!key.empty ()) {
					data = static_cast<TImageInfo*>(m_SharedResInfo.m_ImageHash.Find (key, false));
					if (data) {
						CRenderEngine::FreeImage (data);
					}
				}
			}
			m_SharedResInfo.m_ImageHash.RemoveAll ();
		} else {
			TImageInfo* data;
			for (int i = 0; i < m_ResInfo.m_ImageHash.GetSize (); i++) {
				faw::string_t key = m_ResInfo.m_ImageHash.GetAt (i)->Key;
				if (!key.empty ()) {
					data = static_cast<TImageInfo*>(m_ResInfo.m_ImageHash.Find (key, false));
					if (data) {
						CRenderEngine::FreeImage (data);
					}
				}
			}
			m_ResInfo.m_ImageHash.RemoveAll ();
		}
	}

	void CPaintManagerUI::AdjustSharedImagesHSL () {
		TImageInfo* data;
		for (int i = 0; i < m_SharedResInfo.m_ImageHash.GetSize (); i++) {
			faw::string_t key = m_SharedResInfo.m_ImageHash.GetAt (i)->Key;
			if (!key.empty ()) {
				data = static_cast<TImageInfo*>(m_SharedResInfo.m_ImageHash.Find (key));
				if (data && data->bUseHSL) {
					CRenderEngine::AdjustImage (m_bUseHSL, data, m_H, m_S, m_L);
				}
			}
		}
	}

	void CPaintManagerUI::AdjustImagesHSL () {
		TImageInfo* data;
		for (int i = 0; i < m_ResInfo.m_ImageHash.GetSize (); i++) {
			faw::string_t key = m_ResInfo.m_ImageHash.GetAt (i)->Key;
			if (!key.empty ()) {
				data = static_cast<TImageInfo*>(m_ResInfo.m_ImageHash.Find (key));
				if (data && data->bUseHSL) {
					CRenderEngine::AdjustImage (m_bUseHSL, data, m_H, m_S, m_L);
				}
			}
		}
		Invalidate ();
	}

	void CPaintManagerUI::PostAsyncNotify () {
		if (!m_bAsyncNotifyPosted) {
			::PostMessage (m_hWndPaint, WM_APP + 1, 0, 0L);
			m_bAsyncNotifyPosted = true;
		}
	}
	void CPaintManagerUI::ReloadSharedImages () {
		TImageInfo *data = nullptr;
		TImageInfo *pNewData = nullptr;
		for (int i = 0; i < m_SharedResInfo.m_ImageHash.GetSize (); i++) {
			faw::string_t bitmap = m_SharedResInfo.m_ImageHash.GetAt (i)->Key;
			if (!bitmap.empty ()) {
				data = static_cast<TImageInfo*>(m_SharedResInfo.m_ImageHash.Find (bitmap));
				if (data) {
					if (!data->sResType.empty ()) {
						if (isdigit (bitmap[0])) {
							int iIndex = _ttoi (bitmap.c_str ());
							pNewData = CRenderEngine::LoadImage (iIndex, data->sResType.c_str (), data->dwMask);
						}
					} else {
						pNewData = CRenderEngine::LoadImage (bitmap, _T (""), data->dwMask);
					}
					if (!pNewData) continue;

					CRenderEngine::FreeImage (data, false);
					data->hBitmap = pNewData->hBitmap;
					data->phBitmap = pNewData->phBitmap;
					data->pBits = pNewData->pBits;
					data->nX = pNewData->nX;
					data->nY = pNewData->nY;
					data->bAlpha = pNewData->bAlpha;
					data->pSrcBits = nullptr;
					if (data->bUseHSL) {
						data->pSrcBits = new BYTE[data->nX * data->nY * 4];
						::CopyMemory (data->pSrcBits, data->pBits, data->nX * data->nY * 4);
					} else data->pSrcBits = nullptr;
					if (m_bUseHSL) CRenderEngine::AdjustImage (true, data, m_H, m_S, m_L);

					delete pNewData;
				}
			}
		}
	}

	void CPaintManagerUI::ReloadImages () {
		RemoveAllDrawInfos ();

		TImageInfo *data = nullptr;
		TImageInfo *pNewData = nullptr;
		for (int i = 0; i < m_ResInfo.m_ImageHash.GetSize (); i++) {
			faw::string_t bitmap = m_ResInfo.m_ImageHash.GetAt (i)->Key;
			if (!bitmap.empty ()) {
				data = static_cast<TImageInfo*>(m_ResInfo.m_ImageHash.Find (bitmap));
				if (data) {
					if (!data->sResType.empty ()) {
						if (isdigit (bitmap[0])) {
							int iIndex = _ttoi (bitmap.c_str ());
							pNewData = CRenderEngine::LoadImage (iIndex, data->sResType.c_str (), data->dwMask);
						}
					} else {
						pNewData = CRenderEngine::LoadImage (bitmap, _T (""), data->dwMask);
					}

					CRenderEngine::FreeImage (data, false);
					if (!pNewData) {
						m_ResInfo.m_ImageHash.Remove (bitmap);
						continue;
					}
					data->hBitmap = pNewData->hBitmap;
					data->phBitmap = pNewData->phBitmap;
					data->pBits = pNewData->pBits;
					data->nX = pNewData->nX;
					data->nY = pNewData->nY;
					data->bAlpha = pNewData->bAlpha;
					data->pSrcBits = nullptr;
					if (data->bUseHSL) {
						data->pSrcBits = new BYTE[data->nX * data->nY * 4];
						::CopyMemory (data->pSrcBits, data->pBits, data->nX * data->nY * 4);
					} else data->pSrcBits = nullptr;
					if (m_bUseHSL) CRenderEngine::AdjustImage (true, data, m_H, m_S, m_L);

					delete pNewData;
				}
			}
		}

		if (m_pRoot) m_pRoot->Invalidate ();
	}

	const TDrawInfo* CPaintManagerUI::GetDrawInfo (faw::string_t pStrImage, faw::string_t pStrModify) {
		faw::string_t sStrImage = pStrImage;
		faw::string_t sStrModify = pStrModify;
		faw::string_t sKey = sStrImage + sStrModify;
		TDrawInfo* pDrawInfo = static_cast<TDrawInfo*>(m_ResInfo.m_DrawInfoHash.Find (sKey));
		if (!pDrawInfo && !sKey.empty ()) {
			pDrawInfo = new TDrawInfo ();
			pDrawInfo->Parse (pStrImage, pStrModify, this);
			m_ResInfo.m_DrawInfoHash.Insert (sKey, pDrawInfo);
		}
		return pDrawInfo;
	}

	void CPaintManagerUI::RemoveDrawInfo (faw::string_t pStrImage, faw::string_t pStrModify) {
		faw::string_t sStrImage = pStrImage;
		faw::string_t sStrModify = pStrModify;
		faw::string_t sKey = sStrImage + sStrModify;
		TDrawInfo* pDrawInfo = static_cast<TDrawInfo*>(m_ResInfo.m_DrawInfoHash.Find (sKey));
		if (pDrawInfo) {
			m_ResInfo.m_DrawInfoHash.Remove (sKey);
			delete pDrawInfo;
			pDrawInfo = nullptr;
		}
	}

	void CPaintManagerUI::RemoveAllDrawInfos () {
		TDrawInfo* pDrawInfo = nullptr;
		for (int i = 0; i < m_ResInfo.m_DrawInfoHash.GetSize (); i++) {
			faw::string_t key = m_ResInfo.m_DrawInfoHash.GetAt (i)->Key;
			if (!key.empty ()) {
				pDrawInfo = static_cast<TDrawInfo*>(m_ResInfo.m_DrawInfoHash.Find (key, false));
				if (pDrawInfo) {
					delete pDrawInfo;
					pDrawInfo = nullptr;
				}
			}
		}
		m_ResInfo.m_DrawInfoHash.RemoveAll ();
	}

	void CPaintManagerUI::AddDefaultAttributeList (faw::string_t pStrControlName, faw::string_t pStrControlAttrList, bool bShared) {
		if (bShared || m_bForceUseSharedRes) {
			faw::string_t* pDefaultAttr = new faw::string_t (pStrControlAttrList);
			if (pDefaultAttr) {
				faw::string_t* pOldDefaultAttr = static_cast<faw::string_t*>(m_SharedResInfo.m_AttrHash.Set (pStrControlName, (LPVOID) pDefaultAttr));
				if (pOldDefaultAttr) delete pOldDefaultAttr;
			}
		} else {
			faw::string_t* pDefaultAttr = new faw::string_t (pStrControlAttrList);
			if (pDefaultAttr) {
				faw::string_t* pOldDefaultAttr = static_cast<faw::string_t*>(m_ResInfo.m_AttrHash.Set (pStrControlName, (LPVOID) pDefaultAttr));
				if (pOldDefaultAttr) delete pOldDefaultAttr;
			}
		}
	}

	faw::string_t CPaintManagerUI::GetDefaultAttributeList (faw::string_t pStrControlName) const {
		faw::string_t* pDefaultAttr = static_cast<faw::string_t*>(m_ResInfo.m_AttrHash.Find (pStrControlName));
		if (!pDefaultAttr) pDefaultAttr = static_cast<faw::string_t*>(m_SharedResInfo.m_AttrHash.Find (pStrControlName));
		if (pDefaultAttr) return *pDefaultAttr;
		return _T ("");
	}

	bool CPaintManagerUI::RemoveDefaultAttributeList (faw::string_t pStrControlName, bool bShared) {
		if (bShared) {
			faw::string_t* pDefaultAttr = static_cast<faw::string_t*>(m_SharedResInfo.m_AttrHash.Find (pStrControlName));
			if (!pDefaultAttr) return false;

			delete pDefaultAttr;
			return m_SharedResInfo.m_AttrHash.Remove (pStrControlName);
		} else {
			faw::string_t* pDefaultAttr = static_cast<faw::string_t*>(m_ResInfo.m_AttrHash.Find (pStrControlName));
			if (!pDefaultAttr) return false;

			delete pDefaultAttr;
			return m_ResInfo.m_AttrHash.Remove (pStrControlName);
		}
	}

	void CPaintManagerUI::RemoveAllDefaultAttributeList (bool bShared) {
		if (bShared) {
			faw::string_t* pDefaultAttr;
			for (int i = 0; i < m_SharedResInfo.m_AttrHash.GetSize (); i++) {
				faw::string_t key = m_SharedResInfo.m_AttrHash.GetAt (i)->Key;
				if (!key.empty ()) {
					pDefaultAttr = static_cast<faw::string_t*>(m_SharedResInfo.m_AttrHash.Find (key));
					if (pDefaultAttr) delete pDefaultAttr;
				}
			}
			m_SharedResInfo.m_AttrHash.RemoveAll ();
		} else {
			faw::string_t* pDefaultAttr;
			for (int i = 0; i < m_ResInfo.m_AttrHash.GetSize (); i++) {
				faw::string_t key = m_ResInfo.m_AttrHash.GetAt (i)->Key;
				if (!key.empty ()) {
					pDefaultAttr = static_cast<faw::string_t*>(m_ResInfo.m_AttrHash.Find (key));
					if (pDefaultAttr) delete pDefaultAttr;
				}
			}
			m_ResInfo.m_AttrHash.RemoveAll ();
		}
	}

	void CPaintManagerUI::AddWindowCustomAttribute (faw::string_t pstrName, faw::string_t pstrAttr) {
		if (pstrName.empty () || pstrAttr.empty ()) return;
		faw::string_t* pCostomAttr = new faw::string_t (pstrAttr);
		if (!m_mWindowCustomAttrHash.Find (pstrName))
			m_mWindowCustomAttrHash.Set (pstrName, (LPVOID) pCostomAttr);
		else
			delete pCostomAttr;
	}

	faw::string_t CPaintManagerUI::GetWindowCustomAttribute (faw::string_t pstrName) const {
		if (pstrName.empty ()) return _T ("");
		faw::string_t* pCostomAttr = static_cast<faw::string_t*>(m_mWindowCustomAttrHash.Find (pstrName));
		if (pCostomAttr) return *pCostomAttr;
		return _T ("");
	}

	bool CPaintManagerUI::RemoveWindowCustomAttribute (faw::string_t pstrName) {
		if (pstrName.empty ()) return false;
		faw::string_t* pCostomAttr = static_cast<faw::string_t*>(m_mWindowCustomAttrHash.Find (pstrName));
		if (!pCostomAttr) return false;

		delete pCostomAttr;
		return m_mWindowCustomAttrHash.Remove (pstrName);
	}

	void CPaintManagerUI::RemoveAllWindowCustomAttribute () {
		faw::string_t* pCostomAttr;
		for (int i = 0; i < m_mWindowCustomAttrHash.GetSize (); i++) {
			faw::string_t key = m_mWindowCustomAttrHash.GetAt (i)->Key;
			if (!key.empty ()) {
				pCostomAttr = static_cast<faw::string_t*>(m_mWindowCustomAttrHash.Find (key));
				delete pCostomAttr;
			}
		}
		m_mWindowCustomAttrHash.Resize ();
	}

	CControlUI* CPaintManagerUI::GetRoot () const {
		ASSERT (m_pRoot);
		return m_pRoot;
	}

	CControlUI* CPaintManagerUI::FindControl (POINT pt) const {
		ASSERT (m_pRoot);
		return m_pRoot->FindControl (__FindControlFromPoint, &pt, UIFIND_VISIBLE | UIFIND_HITTEST | UIFIND_TOP_FIRST);
	}

	CControlUI* CPaintManagerUI::FindControl (faw::string_t pstrName) const {
		ASSERT (m_pRoot);
		return static_cast<CControlUI*>(m_mNameHash.Find (pstrName));
	}

	CControlUI* CPaintManagerUI::FindSubControlByPoint (CControlUI* pParent, POINT pt) const {
		if (!pParent) pParent = GetRoot ();
		ASSERT (pParent);
		return pParent->FindControl (__FindControlFromPoint, &pt, UIFIND_VISIBLE | UIFIND_HITTEST | UIFIND_TOP_FIRST);
	}

	CControlUI* CPaintManagerUI::FindSubControlByName (CControlUI* pParent, faw::string_t pstrName) const {
		if (!pParent) pParent = GetRoot ();
		ASSERT (pParent);
		return pParent->FindControl (__FindControlFromName, (LPVOID) pstrName.data (), UIFIND_ALL);
	}

	CControlUI* CPaintManagerUI::FindSubControlByClass (CControlUI* pParent, faw::string_t pstrClass, int iIndex) {
		if (!pParent) pParent = GetRoot ();
		ASSERT (pParent);
		m_aFoundControls.Resize (iIndex + 1);
		return pParent->FindControl (__FindControlFromClass, (LPVOID) pstrClass.data (), UIFIND_ALL);
	}

	CStdPtrArray* CPaintManagerUI::FindSubControlsByClass (CControlUI* pParent, faw::string_t pstrClass) {
		if (!pParent) pParent = GetRoot ();
		ASSERT (pParent);
		m_aFoundControls.Empty ();
		pParent->FindControl (__FindControlsFromClass, (LPVOID) pstrClass.data (), UIFIND_ALL);
		return &m_aFoundControls;
	}

	CStdPtrArray* CPaintManagerUI::GetFoundControls () {
		return &m_aFoundControls;
	}

	CControlUI* CALLBACK CPaintManagerUI::__FindControlFromNameHash (CControlUI* pThis, LPVOID pData) {
		CPaintManagerUI* pManager = static_cast<CPaintManagerUI*>(pData);
		const faw::string_t sName = pThis->GetName ();
		if (sName.empty ()) return nullptr;
		// Add this control to the hash list
		pManager->m_mNameHash.Set (sName, pThis);
		return nullptr; // Attempt to add all controls
	}

	CControlUI* CALLBACK CPaintManagerUI::__FindControlFromCount (CControlUI* /*pThis*/, LPVOID pData) {
		int* pnCount = static_cast<int*>(pData);
		(*pnCount)++;
		return nullptr;  // Count all controls
	}

	CControlUI* CALLBACK CPaintManagerUI::__FindControlFromPoint (CControlUI* pThis, LPVOID pData) {
		LPPOINT pPoint = static_cast<LPPOINT>(pData);
		return ::PtInRect (&pThis->GetPos (), *pPoint) ? pThis : nullptr;
	}

	CControlUI* CALLBACK CPaintManagerUI::__FindControlFromTab (CControlUI* pThis, LPVOID pData) {
		FINDTABINFO* pInfo = static_cast<FINDTABINFO*>(pData);
		if (pInfo->pFocus == pThis) {
			if (pInfo->bForward) pInfo->bNextIsIt = true;
			return pInfo->bForward ? nullptr : pInfo->pLast;
		}
		if ((pThis->GetControlFlags () & UIFLAG_TABSTOP) == 0) return nullptr;
		pInfo->pLast = pThis;
		if (pInfo->bNextIsIt) return pThis;
		if (!pInfo->pFocus) return pThis;
		return nullptr;  // Examine all controls
	}

	CControlUI* CALLBACK CPaintManagerUI::__FindControlFromShortcut (CControlUI* pThis, LPVOID pData) {
		if (!pThis->IsVisible ()) return nullptr;
		FINDSHORTCUT* pFS = static_cast<FINDSHORTCUT*>(pData);
		if (pFS->ch == toupper (pThis->GetShortcut ())) pFS->bPickNext = true;
		if (pThis->GetClass ().find (_T ("LabelUI")) != faw::string_t::npos) return nullptr;   // Labels never get focus!
		return pFS->bPickNext ? pThis : nullptr;
	}

	CControlUI* CALLBACK CPaintManagerUI::__FindControlFromName (CControlUI* pThis, LPVOID pData) {
		faw::string_t pstrName = static_cast<LPCTSTR>(pData);
		const faw::string_t sName = pThis->GetName ();
		if (sName.empty ()) return nullptr;
		return (sName == pstrName ? pThis : nullptr);
	}

	CControlUI* CALLBACK CPaintManagerUI::__FindControlFromClass (CControlUI* pThis, LPVOID pData) {
		faw::string_t pstrType = static_cast<LPCTSTR>(pData);
		faw::string_t pType = pThis->GetClass ();
		CStdPtrArray* pFoundControls = pThis->GetManager ()->GetFoundControls ();
		if (pstrType == _T ("*") || pstrType == pType) {
			int iIndex = -1;
			while (pFoundControls->GetAt (++iIndex));
			if (iIndex < pFoundControls->GetSize ()) pFoundControls->SetAt (iIndex, pThis);
		}
		if (pFoundControls->GetAt (pFoundControls->GetSize () - 1)) return pThis;
		return nullptr;
	}

	CControlUI* CALLBACK CPaintManagerUI::__FindControlsFromClass (CControlUI* pThis, LPVOID pData) {
		faw::string_t pstrType = static_cast<LPCTSTR>(pData);
		faw::string_t pType = pThis->GetClass ();
		if (pstrType == _T ("*") || pstrType == pType)
			pThis->GetManager ()->GetFoundControls ()->Add ((LPVOID) pThis);
		return nullptr;
	}

	CControlUI* CALLBACK CPaintManagerUI::__FindControlsFromUpdate (CControlUI* pThis, LPVOID pData) {
		if (pThis->IsUpdateNeeded ()) {
			pThis->GetManager ()->GetFoundControls ()->Add ((LPVOID) pThis);
			return pThis;
		}
		return nullptr;
	}

	bool CPaintManagerUI::TranslateAccelerator (LPMSG pMsg) {
		for (int i = 0; i < m_aTranslateAccelerator.GetSize (); i++) {
			LRESULT lResult = static_cast<ITranslateAccelerator *>(m_aTranslateAccelerator[i])->TranslateAccelerator (pMsg);
			if (lResult == S_OK) return true;
		}
		return false;
	}

	bool CPaintManagerUI::TranslateMessage (const LPMSG pMsg) {
		// Pretranslate Message takes care of system-wide messages, such as
		// tabbing and shortcut key-combos. We'll look for all messages for
		// each window and any child control attached.
		UINT uStyle = GetWindowStyle (pMsg->hwnd);
		UINT uChildRes = uStyle & WS_CHILD;
		std::optional<LRESULT> lRes = 0;
		if (uChildRes != 0) {
			HWND hWndParent = ::GetParent (pMsg->hwnd);

			for (int i = 0; i < m_aPreMessages.GetSize (); i++) {
				CPaintManagerUI* pT = static_cast<CPaintManagerUI*>(m_aPreMessages[i]);
				HWND hTempParent = hWndParent;
				while (hTempParent) {
					if (pMsg->hwnd == pT->GetPaintWindow () || hTempParent == pT->GetPaintWindow ()) {
						if (pT->TranslateAccelerator (pMsg))
							return true;

						lRes = pT->PreMessageHandler (pMsg->message, pMsg->wParam, pMsg->lParam);
					}
					hTempParent = GetParent (hTempParent);
				}
			}
		} else {
			for (int i = 0; i < m_aPreMessages.GetSize (); i++) {
				CPaintManagerUI* pT = static_cast<CPaintManagerUI*>(m_aPreMessages[i]);
				if (pMsg->hwnd == pT->GetPaintWindow ()) {
					if (pT->TranslateAccelerator (pMsg))
						return true;

					lRes = pT->PreMessageHandler (pMsg->message, pMsg->wParam, pMsg->lParam);
					if (lRes.has_value ())
						return true;

					return false;
				}
			}
		}
		return false;
	}

	bool CPaintManagerUI::AddTranslateAccelerator (ITranslateAccelerator *pTranslateAccelerator) {
		ASSERT (m_aTranslateAccelerator.Find (pTranslateAccelerator) < 0);
		return m_aTranslateAccelerator.Add (pTranslateAccelerator);
	}

	bool CPaintManagerUI::RemoveTranslateAccelerator (ITranslateAccelerator *pTranslateAccelerator) {
		for (int i = 0; i < m_aTranslateAccelerator.GetSize (); i++) {
			if (static_cast<ITranslateAccelerator *>(m_aTranslateAccelerator[i]) == pTranslateAccelerator) {
				return m_aTranslateAccelerator.Remove (i);
			}
		}
		return false;
	}

	void CPaintManagerUI::UsedVirtualWnd (bool bUsed) {
		m_bUsedVirtualWnd = bUsed;
	}

	// 样式管理
	void CPaintManagerUI::AddStyle (faw::string_t pName, faw::string_t pDeclarationList, bool bShared) {
		faw::string_t* pStyle = new faw::string_t (pDeclarationList);

		if (bShared || m_bForceUseSharedRes) {
			if (!m_SharedResInfo.m_StyleHash.Insert (pName, pStyle)) {
				delete pStyle;
			}
		} else {
			if (!m_ResInfo.m_StyleHash.Insert (pName, pStyle)) {
				delete pStyle;
			}
		}
	}

	faw::string_t CPaintManagerUI::GetStyle (faw::string_t pName) const {
		faw::string_t* pStyle = static_cast<faw::string_t*>(m_ResInfo.m_StyleHash.Find (pName));
		if (!pStyle) pStyle = static_cast<faw::string_t*>(m_SharedResInfo.m_StyleHash.Find (pName));
		if (pStyle) return pStyle->c_str ();
		else return _T ("");
	}

	BOOL CPaintManagerUI::RemoveStyle (faw::string_t pName, bool bShared) {
		faw::string_t* pStyle = nullptr;
		if (bShared) {
			pStyle = static_cast<faw::string_t*>(m_SharedResInfo.m_StyleHash.Find (pName));
			if (pStyle) {
				delete pStyle;
				m_SharedResInfo.m_StyleHash.Remove (pName);
			}
		} else {
			pStyle = static_cast<faw::string_t*>(m_ResInfo.m_StyleHash.Find (pName));
			if (pStyle) {
				delete pStyle;
				m_ResInfo.m_StyleHash.Remove (pName);
			}
		}
		return true;
	}

	const CStdStringPtrMap& CPaintManagerUI::GetStyles (bool bShared) const {
		if (bShared) return m_SharedResInfo.m_StyleHash;
		else return m_ResInfo.m_StyleHash;
	}

	void CPaintManagerUI::RemoveAllStyle (bool bShared) {
		if (bShared) {
			faw::string_t* pStyle;
			for (int i = 0; i < m_SharedResInfo.m_StyleHash.GetSize (); i++) {
				faw::string_t key = m_SharedResInfo.m_StyleHash.GetAt (i)->Key;
				if (!key.empty ()) {
					pStyle = static_cast<faw::string_t*>(m_SharedResInfo.m_StyleHash.Find (key));
					delete pStyle;
				}
			}
			m_SharedResInfo.m_StyleHash.RemoveAll ();
		} else {
			faw::string_t* pStyle;
			for (int i = 0; i < m_ResInfo.m_StyleHash.GetSize (); i++) {
				faw::string_t key = m_ResInfo.m_StyleHash.GetAt (i)->Key;
				if (!key.empty ()) {
					pStyle = static_cast<faw::string_t*>(m_ResInfo.m_StyleHash.Find (key));
					delete pStyle;
				}
			}
			m_ResInfo.m_StyleHash.RemoveAll ();
		}
	}

	const TImageInfo* CPaintManagerUI::GetImageString (faw::string_t pStrImage, faw::string_t pStrModify) {
		faw::string_t sImageName = pStrImage;
		faw::string_t sImageResType = _T ("");
		DWORD dwMask = 0;
		for (size_t i = 0; i < 2; ++i) {
			std::map<faw::string_t, faw::string_t> m = FawTools::parse_keyvalue_pairs (i == 0 ? pStrImage : pStrModify);
			for (auto[str_key, str_value] : m) {
				if (str_key == _T ("file") || str_key == _T ("res")) {
					sImageName = str_value;
				} else if (str_key == _T ("restype")) {
					sImageResType = str_value;
				} else if (str_key == _T ("mask")) {
					dwMask = static_cast<decltype (dwMask)> (FawTools::parse_hex (str_value));
				}
			}
		}
		return GetImageEx (sImageName, sImageResType, dwMask);
	}

	bool CPaintManagerUI::InitDragDrop () {
		AddRef ();

		if (FAILED (RegisterDragDrop (m_hWndPaint, this))) //calls addref
		{
			DWORD dwError = GetLastError ();
			return false;
		} else Release (); //i decided to AddRef explicitly after new

		FORMATETC ftetc = { 0 };
		ftetc.cfFormat = CF_BITMAP;
		ftetc.dwAspect = DVASPECT_CONTENT;
		ftetc.lindex = -1;
		ftetc.tymed = TYMED_GDI;
		AddSuportedFormat (ftetc);
		ftetc.cfFormat = CF_DIB;
		ftetc.tymed = TYMED_HGLOBAL;
		AddSuportedFormat (ftetc);
		ftetc.cfFormat = CF_HDROP;
		ftetc.tymed = TYMED_HGLOBAL;
		AddSuportedFormat (ftetc);
		ftetc.cfFormat = CF_ENHMETAFILE;
		ftetc.tymed = TYMED_ENHMF;
		AddSuportedFormat (ftetc);
		return true;
	}
	static WORD DIBNumColors (void* pv) {
		int bits;
		LPBITMAPINFOHEADER  lpbi;
		LPBITMAPCOREHEADER  lpbc;
		lpbi = ((LPBITMAPINFOHEADER) pv);
		lpbc = ((LPBITMAPCOREHEADER) pv);
		/*  With the BITMAPINFO format headers, the size of the palette
		*  is in biClrUsed, whereas in the BITMAPCORE - style headers, it
		*  is dependent on the bits per pixel ( = 2 raised to the power of
		*  bits/pixel).
		*/
		if (lpbi->biSize != sizeof (BITMAPCOREHEADER)) {
			if (lpbi->biClrUsed != 0)
				return (WORD) lpbi->biClrUsed;
			bits = lpbi->biBitCount;
		} else
			bits = lpbc->bcBitCount;
		switch (bits) {
		case 1:
			return 2;
		case 4:
			return 16;
		case 8:
			return 256;
		default:
			/* A 24 bitcount DIB has no color table */
			return 0;
		}
	}
	//code taken from SEEDIB MSDN sample
	static WORD ColorTableSize (LPVOID lpv) {
		LPBITMAPINFOHEADER lpbih = (LPBITMAPINFOHEADER) lpv;

		if (lpbih->biSize != sizeof (BITMAPCOREHEADER)) {
			if (((LPBITMAPINFOHEADER) (lpbih))->biCompression == BI_BITFIELDS)
				/* Remember that 16/32bpp dibs can still have a color table */
				return (sizeof (DWORD) * 3) + (DIBNumColors (lpbih) * sizeof (RGBQUAD));
			else
				return (WORD) (DIBNumColors (lpbih) * sizeof (RGBQUAD));
		} else
			return (WORD) (DIBNumColors (lpbih) * sizeof (RGBTRIPLE));
	}

	bool CPaintManagerUI::OnDrop (FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect) {
		POINT ptMouse = { 0 };
		GetCursorPos (&ptMouse);
		::SendMessage (m_hTargetWnd, WM_LBUTTONUP, 0, MAKELPARAM (ptMouse.x, ptMouse.y));

		if (pFmtEtc->cfFormat == CF_DIB && medium.tymed == TYMED_HGLOBAL) {
			if (medium.hGlobal) {
				LPBITMAPINFOHEADER  lpbi = (BITMAPINFOHEADER*) GlobalLock (medium.hGlobal);
				if (lpbi) {
					HBITMAP hbm = NULL;
					HDC hdc = GetDC (nullptr);
					if (hdc) {
						int i = ((BITMAPFILEHEADER *) lpbi)->bfOffBits;
						hbm = CreateDIBitmap (hdc, (LPBITMAPINFOHEADER) lpbi,
							(LONG) CBM_INIT,
							(LPSTR) lpbi + lpbi->biSize + ColorTableSize (lpbi),
							(LPBITMAPINFO) lpbi, DIB_RGB_COLORS);

						::ReleaseDC (nullptr, hdc);
					}
					GlobalUnlock (medium.hGlobal);
					if (hbm != NULL)
						hbm = (HBITMAP) SendMessage (m_hTargetWnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hbm);
					if (hbm != NULL)
						DeleteObject (hbm);
					return true; //release the medium
				}
			}
		}
		if (pFmtEtc->cfFormat == CF_BITMAP && medium.tymed == TYMED_GDI) {
			if (medium.hBitmap) {
				HBITMAP hBmp = (HBITMAP) SendMessage (m_hTargetWnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) medium.hBitmap);
				if (hBmp)
					DeleteObject (hBmp);
				return false; //don't free the bitmap
			}
		}
		if (pFmtEtc->cfFormat == CF_ENHMETAFILE && medium.tymed == TYMED_ENHMF) {
			ENHMETAHEADER emh;
			GetEnhMetaFileHeader (medium.hEnhMetaFile, sizeof (ENHMETAHEADER), &emh);
			RECT rc = { 0 };//={0,0,EnhMetaHdr.rclBounds.right-EnhMetaHdr.rclBounds.left, EnhMetaHdr.rclBounds.bottom-EnhMetaHdr.rclBounds.top};
			HDC hDC = GetDC (m_hTargetWnd);
			//start code: taken from ENHMETA.EXE MSDN Sample
			//*ALSO NEED to GET the pallete (select and RealizePalette it, but i was too lazy*
			// Get the characteristics of the output device
			float PixelsX = (float) GetDeviceCaps (hDC, HORZRES);
			float PixelsY = (float) GetDeviceCaps (hDC, VERTRES);
			float MMX = (float) GetDeviceCaps (hDC, HORZSIZE);
			float MMY = (float) GetDeviceCaps (hDC, VERTSIZE);
			// Calculate the rect in which to draw the metafile based on the
			// intended size and the current output device resolution
			// Remember that the intended size is given in 0.01mm units, so
			// convert those to device units on the target device
			rc.top = (int) ((float) (emh.rclFrame.top) * PixelsY / (MMY*100.0f));
			rc.left = (int) ((float) (emh.rclFrame.left) * PixelsX / (MMX*100.0f));
			rc.right = (int) ((float) (emh.rclFrame.right) * PixelsX / (MMX*100.0f));
			rc.bottom = (int) ((float) (emh.rclFrame.bottom) * PixelsY / (MMY*100.0f));
			//end code: taken from ENHMETA.EXE MSDN Sample

			HDC hdcMem = CreateCompatibleDC (hDC);
			HGDIOBJ hBmpMem = CreateCompatibleBitmap (hDC, emh.rclBounds.right, emh.rclBounds.bottom);
			HGDIOBJ hOldBmp = ::SelectObject (hdcMem, hBmpMem);
			PlayEnhMetaFile (hdcMem, medium.hEnhMetaFile, &rc);
			HBITMAP hBmp = (HBITMAP)::SelectObject (hdcMem, hOldBmp);
			DeleteDC (hdcMem);
			ReleaseDC (m_hTargetWnd, hDC);
			hBmp = (HBITMAP) SendMessage (m_hTargetWnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hBmp);
			if (hBmp)
				DeleteObject (hBmp);
			return true;
		}
		if (pFmtEtc->cfFormat == CF_HDROP && medium.tymed == TYMED_HGLOBAL) {
			HDROP hDrop = (HDROP) GlobalLock (medium.hGlobal);
			if (hDrop) {
				TCHAR szFileName[MAX_PATH];
				UINT cFiles = DragQueryFile (hDrop, 0xFFFFFFFF, nullptr, 0);
				if (cFiles > 0) {
					DragQueryFile (hDrop, 0, szFileName, sizeof (szFileName));
					HBITMAP hBitmap = (HBITMAP) LoadImage (nullptr, szFileName, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
					if (hBitmap) {
						HBITMAP hBmp = (HBITMAP) SendMessage (m_hTargetWnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hBitmap);
						if (hBmp)
							DeleteObject (hBmp);
					}
				}
				//DragFinish(hDrop); // base class calls ReleaseStgMedium
			}
			GlobalUnlock (medium.hGlobal);
		}
		return true; //let base free the medium
	}
} // namespace DuiLib
