#ifndef __UIBUTTON_H__
#define __UIBUTTON_H__

#pragma once

namespace DuiLib {
	class UILIB_API CButtonUI: public CLabelUI {
		DECLARE_DUICONTROL (CButtonUI)

	public:
		CButtonUI ();

		faw::string_t GetClass () const;
		LPVOID GetInterface (faw::string_t pstrName);
		UINT GetControlFlags () const;

		bool Activate ();
		void SetEnabled (bool bEnable = true);
		void DoEvent (TEventUI& event);

		virtual faw::string_t GetNormalImage ();
		virtual void SetNormalImage (faw::string_t pStrImage);
		virtual faw::string_t GetHotImage ();
		virtual void SetHotImage (faw::string_t pStrImage);
		virtual faw::string_t GetPushedImage ();
		virtual void SetPushedImage (faw::string_t pStrImage);
		virtual faw::string_t GetFocusedImage ();
		virtual void SetFocusedImage (faw::string_t pStrImage);
		virtual faw::string_t GetDisabledImage ();
		virtual void SetDisabledImage (faw::string_t pStrImage);
		virtual faw::string_t GetHotForeImage ();
		virtual void SetHotForeImage (faw::string_t pStrImage);
		void SetStateCount (int nCount);
		int GetStateCount () const;
		virtual faw::string_t GetStateImage ();
		virtual void SetStateImage (faw::string_t pStrImage);

		void BindTabIndex (int _BindTabIndex);
		void BindTabLayoutName (faw::string_t _TabLayoutName);
		void BindTriggerTabSel (int _SetSelectIndex = -1);
		void RemoveBindTabIndex ();
		int	 GetBindTabLayoutIndex ();
		faw::string_t GetBindTabLayoutName ();

		void SetHotFont (int index);
		int GetHotFont () const;
		void SetPushedFont (int index);
		int GetPushedFont () const;
		void SetFocusedFont (int index);
		int GetFocusedFont () const;

		void SetHotBkColor (DWORD dwColor);
		DWORD GetHotBkColor () const;
		void SetPushedBkColor (DWORD dwColor);
		DWORD GetPushedBkColor () const;
		void SetDisabledBkColor (DWORD dwColor);
		DWORD GetDisabledBkColor () const;
		void SetHotTextColor (DWORD dwColor);
		DWORD GetHotTextColor () const;
		void SetPushedTextColor (DWORD dwColor);
		DWORD GetPushedTextColor () const;
		void SetFocusedTextColor (DWORD dwColor);
		DWORD GetFocusedTextColor () const;
		void SetAttribute (faw::string_t pstrName, faw::string_t pstrValue);

		void PaintText (HDC hDC);

		void PaintBkColor (HDC hDC);
		void PaintStatusImage (HDC hDC);
		void PaintForeImage (HDC hDC);

	protected:
		UINT		m_uButtonState			= 0;

		int			m_iHotFont				= -1;
		int			m_iPushedFont			= -1;
		int			m_iFocusedFont			= -1;

		DWORD		m_dwHotBkColor			= 0;
		DWORD		m_dwPushedBkColor		= 0;
		DWORD		m_dwDisabledBkColor		= 0;
		DWORD		m_dwHotTextColor		= 0;
		DWORD		m_dwPushedTextColor		= 0;
		DWORD		m_dwFocusedTextColor	= 0;

		faw::string_t	m_sNormalImage;
		faw::string_t	m_sHotImage;
		faw::string_t	m_sHotForeImage;
		faw::string_t	m_sPushedImage;
		faw::string_t	m_sPushedForeImage;
		faw::string_t	m_sFocusedImage;
		faw::string_t	m_sDisabledImage;
		int			m_nStateCount			= 0;
		faw::string_t	m_sStateImage;

		int			m_iBindTabIndex			= -1;
		faw::string_t	m_sBindTabLayoutName;
	};

}	// namespace DuiLib

#endif // __UIBUTTON_H__