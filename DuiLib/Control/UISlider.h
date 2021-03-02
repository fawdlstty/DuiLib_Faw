#ifndef __UISLIDER_H__
#define __UISLIDER_H__

#pragma once

namespace DuiLib {
	class UILIB_API CSliderUI: public CProgressUI {
		DECLARE_DUICONTROL (CSliderUI)
	public:
		CSliderUI ();

		faw::string_t GetClass () const;
		UINT GetControlFlags () const;
		LPVOID GetInterface (faw::string_t pstrName);

		void SetEnabled (bool bEnable = true);

		int GetChangeStep ();
		void SetChangeStep (int step);
		void SetThumbSize (SIZE szXY);
		RECT GetThumbRect () const;
		faw::string_t GetThumbImage () const;
		void SetThumbImage (faw::string_t pStrImage);
		faw::string_t GetThumbHotImage () const;
		void SetThumbHotImage (faw::string_t pStrImage);
		faw::string_t GetThumbPushedImage () const;
		void SetThumbPushedImage (faw::string_t pStrImage);

		void DoEvent (TEventUI& event);
		void SetAttribute (faw::string_t pstrName, faw::string_t pstrValue);
		void PaintForeImage (HDC hDC);

		void SetValue (int nValue);
		void SetCanSendMove (bool bCanSend);
		bool GetCanSendMove () const;
	protected:
		SIZE		m_szThumb		= { 0 };
		UINT		m_uButtonState	= 0;
		int			m_nStep			= 1;

		faw::string_t	m_sThumbImage;
		faw::string_t	m_sThumbHotImage;
		faw::string_t	m_sThumbPushedImage;

		faw::string_t	m_sImageModify;
		bool		m_bSendMove		= false;
	};
}

#endif // __UISLIDER_H__