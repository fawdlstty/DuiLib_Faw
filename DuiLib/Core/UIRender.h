﻿#ifndef __UIRENDER_H__
#define __UIRENDER_H__

#pragma once

#ifdef USE_XIMAGE_EFFECT
class CxImage;
#endif
namespace DuiLib {
	/////////////////////////////////////////////////////////////////////////////////////
	//

	class UILIB_API CRenderClip {
	public:
		virtual ~CRenderClip ();
		RECT rcItem = { 0 };
		HDC hDC;
		HRGN hRgn;
		HRGN hOldRgn;

		static void GenerateClip (HDC hDC, RECT rc, CRenderClip& clip);
		static void GenerateRoundClip (HDC hDC, RECT rc, RECT rcItem, int width, int height, CRenderClip& clip);
		static void UseOldClipBegin (HDC hDC, CRenderClip& clip);
		static void UseOldClipEnd (HDC hDC, CRenderClip& clip);
	};

	/////////////////////////////////////////////////////////////////////////////////////
	//

	class UILIB_API CRenderEngine {
	public:
		static DWORD AdjustColor (DWORD dwColor, short H, short S, short L);
		static HBITMAP CreateARGB32Bitmap (HDC hDC, int cx, int cy, BYTE** pBits);
		static void AdjustImage (bool bUseHSL, TImageInfo* imageInfo, short H, short S, short L);
		static TImageInfo* LoadImage (std::variant<UINT, faw::string_t> bitmap, faw::string_t type = _T (""), DWORD mask = 0, HINSTANCE instance = NULL);
#ifdef USE_XIMAGE_EFFECT
		static CxImage *LoadGifImageX (std::variant<UINT, faw::string_t> bitmap, faw::string_t type = _T (""), DWORD mask = 0);
#endif
		static void FreeImage (TImageInfo* bitmap, bool bDelete = true);
		static TImageInfo* LoadImage (faw::string_t pStrImage, faw::string_t type = _T (""), DWORD mask = 0, HINSTANCE instance = NULL);
		static TImageInfo* LoadImage (UINT nID, faw::string_t type = _T (""), DWORD mask = 0, HINSTANCE instance = NULL);

		static Gdiplus::Image	*GdiplusLoadImage (faw::string_t pstrPath);
		static Gdiplus::Image* GdiplusLoadImage (LPVOID pBuf, size_t dwSize);
		static HICON GdiplusLoadIcon(LPVOID pBuf, size_t dwSize);
		static HICON GdiplusLoadIcon(faw::string_t pstrPath1);

		static bool DrawIconImageString (HDC hDC, CPaintManagerUI* pManager, const RECT& rcItem, const RECT& rcPaint, \
			faw::string_t pStrImage, faw::string_t pStrModify = _T (""));
		static bool MakeFitIconDest (const RECT& rcControl, const SIZE& szIcon, const faw::string_t& sAlign, RECT& rcDest);

		static void DrawText (HDC hDC, CPaintManagerUI* pManager, RECT& rc, faw::string_t pstrText, DWORD dwTextColor, \
			int iFont, UINT uStyle, DWORD dwTextBKColor);

		static void DrawImage (HDC hDC, HBITMAP hBitmap, const RECT& rc, const RECT& rcPaint, \
			const RECT& rcBmpPart, const RECT& rcCorners, bool bAlpha, BYTE uFade = 255,
			bool hole = false, bool xtiled = false, bool ytiled = false);

		static bool DrawImageInfo (HDC hDC, CPaintManagerUI* pManager, const RECT& rcItem, const RECT& rcPaint, const TDrawInfo* pDrawInfo, HINSTANCE instance = NULL);
		static bool DrawImageString (HDC hDC, CPaintManagerUI* pManager, const RECT& rcItem, const RECT& rcPaint, faw::string_t pStrImage, faw::string_t pStrModify = _T (""), HINSTANCE instance = NULL);

		static void DrawColor (HDC hDC, const RECT& rc, DWORD color);
		static void DrawGradient (HDC hDC, const RECT& rc, DWORD dwFirst, DWORD dwSecond, bool bVertical, int nSteps);

		// 以下函数中的颜色参数alpha值无效
		static void DrawLine (HDC hDC, const RECT& rc, int nSize, DWORD dwPenColor, int nStyle = PS_SOLID);
		static void DrawRect (HDC hDC, const RECT& rc, int nSize, DWORD dwPenColor, int nStyle = PS_SOLID);
		static void DrawRoundRectangle(HDC hDC, float x, float y, float width, float height, float arcSize, float lineWidth, Gdiplus::Color lineColor, bool fillPath, DWORD fillColor);
		static void DrawRoundRect (HDC hDC, const RECT& rc, int width, int height, int nSize, DWORD dwPenColor, int nStyle = PS_SOLID);
		static void DrawText (HDC hDC, CPaintManagerUI* pManager, RECT& rc, faw::string_t pstrText, \
			DWORD dwTextColor, int iFont, UINT uStyle);
		static void DrawHtmlText (HDC hDC, CPaintManagerUI* pManager, RECT& rc, faw::string_t pstrText,
			DWORD dwTextColor, RECT* pLinks, faw::string_t* sLinks, int& nLinkRects, int iFont, UINT uStyle);
		static HBITMAP GenerateBitmap (CPaintManagerUI* pManager, RECT rc, CControlUI* pStopControl = nullptr, DWORD dwFilterColor = 0);
		static HBITMAP GenerateBitmap (CPaintManagerUI* pManager, CControlUI* pControl, RECT rc, DWORD dwFilterColor = 0);
		static SIZE GetTextSize (HDC hDC, CPaintManagerUI* pManager, faw::string_t pstrText, int iFont, UINT uStyle);

		//alpha utilities
		static void CheckAlphaColor (DWORD& dwColor);
	};

} // namespace DuiLib

#endif // __UIRENDER_H__
