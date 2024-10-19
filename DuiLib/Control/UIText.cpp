﻿#include "StdAfx.h"
#include "UIText.h"

namespace DuiLib {
	IMPLEMENT_DUICONTROL (CTextUI)

	CTextUI::CTextUI () {
		m_uTextStyle = DT_WORDBREAK;
		m_rcTextPadding.left = 2;
		m_rcTextPadding.right = 2;
		::ZeroMemory (m_rcLinks, sizeof (m_rcLinks));
	}

	CTextUI::~CTextUI () {}

	faw::string_t CTextUI::GetClass () const {
		return _T ("TextUI");
	}

	LPVOID CTextUI::GetInterface (faw::string_t pstrName) {
		if (pstrName == DUI_CTRL_TEXT) return static_cast<CTextUI*>(this);
		return CLabelUI::GetInterface (pstrName);
	}

	UINT CTextUI::GetControlFlags () const {
		if (IsEnabled () && m_nLinks > 0) return UIFLAG_SETCURSOR;
		else return 0;
	}

	faw::string_t* CTextUI::GetLinkContent (int iIndex) {
		if (iIndex >= 0 && iIndex < m_nLinks) return &m_sLinks[iIndex];
		return nullptr;
	}

	void CTextUI::DoEvent (TEventUI& event) {
		if (!IsMouseEnabled () && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND) {
			if (m_pParent) m_pParent->DoEvent (event);
			else CLabelUI::DoEvent (event);
			return;
		} else if (event.Type == UIEVENT_SETCURSOR) {
			for (int i = 0; i < m_nLinks; i++) {
				if (::PtInRect (&m_rcLinks[i], event.ptMouse)) {
					::SetCursor (::LoadCursor (nullptr, IDC_HAND));
					return;
				}
			}
		} else if (event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_DBLCLICK && IsEnabled ()) {
			for (int i = 0; i < m_nLinks; i++) {
				if (::PtInRect (&m_rcLinks[i], event.ptMouse)) {
					Invalidate ();
					return;
				}
			}
		} else if (event.Type == UIEVENT_BUTTONUP && IsEnabled ()) {
			for (int i = 0; i < m_nLinks; i++) {
				if (::PtInRect (&m_rcLinks[i], event.ptMouse)) {
					m_pManager->SendNotify (this, DUI_MSGTYPE_LINK, i);
					return;
				}
			}
		} else if (event.Type == UIEVENT_CONTEXTMENU) {
			return;
		} else if (m_nLinks > 0 && event.Type == UIEVENT_MOUSEMOVE && IsEnabled ()) {
			int nHoverLink = -1;
			for (int i = 0; i < m_nLinks; i++) {
				if (::PtInRect (&m_rcLinks[i], event.ptMouse)) {
					nHoverLink = i;
					break;
				}
			}

			if (m_nHoverLink != nHoverLink) {
				m_nHoverLink = nHoverLink;
				Invalidate ();
				return;
			}
		} else if (event.Type == UIEVENT_MOUSELEAVE) {
			if (m_nLinks > 0 && IsEnabled ()) {
				if (m_nHoverLink != -1) {
					m_nHoverLink = -1;
					Invalidate ();
					return;
				}
			}
		}

		CLabelUI::DoEvent (event);
	}

	SIZE CTextUI::EstimateSize (SIZE szAvailable) {
		faw::string_t sText = GetText ();
		RECT _rcTextPadding = GetTextPadding ();

		RECT rcText = { 0, 0, m_bAutoCalcWidth ? 9999 : GetManager ()->GetDPIObj ()->Scale (m_cxyFixed.cx), 9999 };
		rcText.left += _rcTextPadding.left;
		rcText.right -= _rcTextPadding.right;

		if (m_bShowHtml) {
			int nLinks = 0;
			CRenderEngine::DrawHtmlText (m_pManager->GetPaintDC (), m_pManager, rcText, sText, m_dwTextColor, nullptr, nullptr, nLinks, m_iFont, DT_CALCRECT | m_uTextStyle);
		} else {
			CRenderEngine::DrawText (m_pManager->GetPaintDC (), m_pManager, rcText, sText, m_dwTextColor, m_iFont, DT_CALCRECT | m_uTextStyle);
		}
		SIZE cXY = { rcText.right - rcText.left + _rcTextPadding.left + _rcTextPadding.right,
			rcText.bottom - rcText.top + _rcTextPadding.top + _rcTextPadding.bottom };

		if (m_bAutoCalcWidth) {
			m_cxyFixed.cx = MulDiv (cXY.cx, 100, GetManager ()->GetDPIObj ()->GetScale ());
		}
		if (m_bAutoCalcHeight) {
			m_cxyFixed.cy = MulDiv (cXY.cy, 100, GetManager ()->GetDPIObj ()->GetScale ());
		}

		return CControlUI::EstimateSize (szAvailable);
	}

	void CTextUI::PaintText (HDC hDC) {
		faw::string_t sText = GetText ();
		if (sText.empty ()) {
			m_nLinks = 0;
			return;
		}

		if (m_dwTextColor == 0) m_dwTextColor = m_pManager->GetDefaultFontColor ();
		if (m_dwDisabledTextColor == 0) m_dwDisabledTextColor = m_pManager->GetDefaultDisabledColor ();

		m_nLinks = lengthof (m_rcLinks);
		RECT rc = m_rcItem;
		RECT rcTextPadding = GetTextPadding();
		rc.left += rcTextPadding.left;
		rc.right -= rcTextPadding.right;
		rc.top += rcTextPadding.top;
		rc.bottom -= rcTextPadding.bottom;
		if (IsEnabled ()) {
			if (m_bShowHtml)
				CRenderEngine::DrawHtmlText (hDC, m_pManager, rc, sText, m_dwTextColor, \
					m_rcLinks, m_sLinks, m_nLinks, m_iFont, m_uTextStyle);
			else
				CRenderEngine::DrawText (hDC, m_pManager, rc, sText, m_dwTextColor, \
					m_iFont, m_uTextStyle);
		} else {
			if (m_bShowHtml)
				CRenderEngine::DrawHtmlText (hDC, m_pManager, rc, sText, m_dwDisabledTextColor, \
					m_rcLinks, m_sLinks, m_nLinks, m_iFont, m_uTextStyle);
			else
				CRenderEngine::DrawText (hDC, m_pManager, rc, sText, m_dwDisabledTextColor, \
					m_iFont, m_uTextStyle);
		}
	}
}
