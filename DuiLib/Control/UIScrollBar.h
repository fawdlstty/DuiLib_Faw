#ifndef __UISCROLLBAR_H__
#define __UISCROLLBAR_H__

#pragma once

namespace DuiLib {
	class UILIB_API CScrollBarUI: public CControlUI {
		DECLARE_DUICONTROL (CScrollBarUI)
	public:
		CScrollBarUI ();

		faw::string_t GetClass () const;
		LPVOID GetInterface (faw::string_t pstrName);

		CContainerUI* GetOwner () const;
		void SetOwner (CContainerUI* pOwner);

		void SetVisible (bool bVisible = true);
		void SetEnabled (bool bEnable = true);
		void SetFocus ();

		bool IsHorizontal ();
		void SetHorizontal (bool bHorizontal = true);
		int GetScrollRange () const;
		void SetScrollRange (int nRange);
		int GetScrollPos () const;
		void SetScrollPos (int nPos);
		int GetLineSize () const;
		void SetLineSize (int nSize);

		bool GetShowButton1 ();
		void SetShowButton1 (bool bShow);
		faw::string_t GetButton1NormalImage ();
		void SetButton1NormalImage (faw::string_t pStrImage);
		faw::string_t GetButton1HotImage ();
		void SetButton1HotImage (faw::string_t pStrImage);
		faw::string_t GetButton1PushedImage ();
		void SetButton1PushedImage (faw::string_t pStrImage);
		faw::string_t GetButton1DisabledImage ();
		void SetButton1DisabledImage (faw::string_t pStrImage);

		bool GetShowButton2 ();
		void SetShowButton2 (bool bShow);
		faw::string_t GetButton2NormalImage ();
		void SetButton2NormalImage (faw::string_t pStrImage);
		faw::string_t GetButton2HotImage ();
		void SetButton2HotImage (faw::string_t pStrImage);
		faw::string_t GetButton2PushedImage ();
		void SetButton2PushedImage (faw::string_t pStrImage);
		faw::string_t GetButton2DisabledImage ();
		void SetButton2DisabledImage (faw::string_t pStrImage);

		faw::string_t GetThumbNormalImage ();
		void SetThumbNormalImage (faw::string_t pStrImage);
		faw::string_t GetThumbHotImage ();
		void SetThumbHotImage (faw::string_t pStrImage);
		faw::string_t GetThumbPushedImage ();
		void SetThumbPushedImage (faw::string_t pStrImage);
		faw::string_t GetThumbDisabledImage ();
		void SetThumbDisabledImage (faw::string_t pStrImage);

		faw::string_t GetRailNormalImage ();
		void SetRailNormalImage (faw::string_t pStrImage);
		faw::string_t GetRailHotImage ();
		void SetRailHotImage (faw::string_t pStrImage);
		faw::string_t GetRailPushedImage ();
		void SetRailPushedImage (faw::string_t pStrImage);
		faw::string_t GetRailDisabledImage ();
		void SetRailDisabledImage (faw::string_t pStrImage);

		faw::string_t GetBkNormalImage ();
		void SetBkNormalImage (faw::string_t pStrImage);
		faw::string_t GetBkHotImage ();
		void SetBkHotImage (faw::string_t pStrImage);
		faw::string_t GetBkPushedImage ();
		void SetBkPushedImage (faw::string_t pStrImage);
		faw::string_t GetBkDisabledImage ();
		void SetBkDisabledImage (faw::string_t pStrImage);

		void SetPos (RECT rc, bool bNeedInvalidate = true);
		void DoEvent (TEventUI& event);
		void SetAttribute (faw::string_t pstrName, faw::string_t pstrValue);

		bool DoPaint (HDC hDC, const RECT& rcPaint, CControlUI* pStopControl);

		void PaintBk (HDC hDC);
		void PaintButton1 (HDC hDC);
		void PaintButton2 (HDC hDC);
		void PaintThumb (HDC hDC);
		void PaintRail (HDC hDC);

	protected:

		enum {
			DEFAULT_SCROLLBAR_SIZE = 16,
			DEFAULT_TIMERID = 10,
		};

		bool			m_bHorizontal			= false;
		int64_t			m_nRange				= 0;
		int64_t			m_nScrollPos			= 0;
		int				m_nLineSize				= 8;
		CContainerUI	*m_pOwner				= nullptr;
		POINT			m_ptLastMouse			= { 0, 0 };
		int				m_nLastScrollPos		= 0;
		int				m_nLastScrollOffset		= 0;
		int				m_nScrollRepeatDelay	= 0;

		faw::string_t		m_sBkNormalImage;
		faw::string_t		m_sBkHotImage;
		faw::string_t		m_sBkPushedImage;
		faw::string_t		m_sBkDisabledImage;

		bool			m_bShowButton1			= true;
		RECT			m_rcButton1				= { 0 };
		UINT			m_uButton1State			= 0;
		faw::string_t		m_sButton1NormalImage;
		faw::string_t		m_sButton1HotImage;
		faw::string_t		m_sButton1PushedImage;
		faw::string_t		m_sButton1DisabledImage;

		bool			m_bShowButton2			= true;
		RECT			m_rcButton2				= { 0 };
		UINT			m_uButton2State			= 0;
		faw::string_t		m_sButton2NormalImage;
		faw::string_t		m_sButton2HotImage;
		faw::string_t		m_sButton2PushedImage;
		faw::string_t		m_sButton2DisabledImage;

		RECT			m_rcThumb				= { 0 };
		UINT			m_uThumbState			= 0;
		faw::string_t		m_sThumbNormalImage;
		faw::string_t		m_sThumbHotImage;
		faw::string_t		m_sThumbPushedImage;
		faw::string_t		m_sThumbDisabledImage;

		faw::string_t		m_sRailNormalImage;
		faw::string_t		m_sRailHotImage;
		faw::string_t		m_sRailPushedImage;
		faw::string_t		m_sRailDisabledImage;

		faw::string_t		m_sImageModify;
	};
}

#endif // __UISCROLLBAR_H__