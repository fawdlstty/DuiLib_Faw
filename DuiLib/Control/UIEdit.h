#ifndef __UIEDIT_H__
#define __UIEDIT_H__

#pragma once

namespace DuiLib {
	class CEditWnd;

	class UILIB_API CEditUI: public CLabelUI {
		DECLARE_DUICONTROL (CEditUI)
		friend class CEditWnd;
	public:
		CEditUI ();

		faw::string_t GetClass () const;
		LPVOID GetInterface (faw::string_t pstrName);
		UINT GetControlFlags () const;

		void SetEnabled (bool bEnable = true);
		void SetText (faw::string_t pstrText);
		void SetMaxChar (UINT uMax);
		UINT GetMaxChar ();
		void SetReadOnly (bool bReadOnly);
		bool IsReadOnly () const;
		void SetPasswordMode (bool bPasswordMode);
		bool IsPasswordMode () const;
		void SetPasswordChar (TCHAR cPasswordChar);
		TCHAR GetPasswordChar () const;
		void SetNumberOnly (bool bNumberOnly);
		bool IsNumberOnly () const;
		int GetWindowStyls () const;

		faw::string_t GetNormalImage ();
		void SetNormalImage (faw::string_t pStrImage);
		faw::string_t GetHotImage ();
		void SetHotImage (faw::string_t pStrImage);
		faw::string_t GetFocusedImage ();
		void SetFocusedImage (faw::string_t pStrImage);
		faw::string_t GetDisabledImage ();
		void SetDisabledImage (faw::string_t pStrImage);

		bool IsAutoSelAll ();
		void SetAutoSelAll (bool bAutoSelAll);
		void SetSel (long nStartChar, long nEndChar);
		void SetSelAll ();
		void SetReplaceSel (faw::string_t lpszReplace);

		void SetTipValue (faw::string_t pStrTipValue);
		faw::string_t GetTipValue ();
		void SetTipValueColor (faw::string_t pStrColor);
		DWORD GetTipValueColor ();

		void SetPos (RECT rc, bool bNeedInvalidate = true);
		void Move (SIZE szOffset, bool bNeedInvalidate = true);
		void SetVisible (bool bVisible = true);
		void SetInternVisible (bool bVisible = true);
		SIZE EstimateSize (SIZE szAvailable);
		void DoEvent (TEventUI& event);
		void SetAttribute (faw::string_t pstrName, faw::string_t pstrValue);

		void PaintStatusImage (HDC hDC);
		void PaintText (HDC hDC);

	protected:
		CEditWnd	*m_pWindow			= nullptr;

		UINT		m_uMaxChar			= 255;
		bool		m_bReadOnly			= false;
		bool		m_bPasswordMode		= false;
		bool		m_bAutoSelAll		= false;
		TCHAR		m_cPasswordChar		= _T ('*');
		UINT		m_uButtonState		= 0;
		faw::string_t	m_sNormalImage;
		faw::string_t	m_sHotImage;
		faw::string_t	m_sFocusedImage;
		faw::string_t	m_sDisabledImage;
		faw::string_t	m_sTipValue;
		DWORD		m_dwTipValueColor	= 0xFFBAC0C5;
		int			m_iWindowStyls		= 0;
	};
}
#endif // __UIEDIT_H__