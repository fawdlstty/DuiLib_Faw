﻿#ifndef __UIGROUPBOX_H__
#define __UIGROUPBOX_H__

#pragma once

namespace DuiLib {

	class UILIB_API CGroupBoxUI: public CVerticalLayoutUI {
		DECLARE_DUICONTROL (CGroupBoxUI)
	public:
		CGroupBoxUI ();
		virtual ~CGroupBoxUI ();
		faw::string_t GetClass () const;
		LPVOID GetInterface (faw::string_t pstrName);
		void SetTextColor (DWORD dwTextColor);
		DWORD GetTextColor () const;
		void SetDisabledTextColor (DWORD dwTextColor);
		DWORD GetDisabledTextColor () const;
		void SetFont (int index);
		int GetFont () const;

	protected:
		//Paint
		virtual void PaintText (HDC hDC);
		virtual void PaintBorder (HDC hDC);
		void PaintGroupBorder(HDC hDC, float x, float y, float width, float height, float arcSize, float lineWidth, Gdiplus::Color lineColor, RECT rcItemText, Gdiplus::Color fillColor);
		virtual void SetAttribute (faw::string_t pstrName, faw::string_t pstrValue);

	private:
		SIZE CalcrectSize (SIZE szAvailable);

	protected:
		DWORD	m_dwTextColor			= 0;
		DWORD	m_dwDisabledTextColor	= 0;
		int		m_iFont					= -1;
		UINT	m_uTextStyle;
	};
}
#endif // __UIGROUPBOX_H__