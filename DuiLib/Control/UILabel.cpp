#include "StdAfx.h"
#include "UILabel.h"

#include <atlconv.h>
namespace DuiLib {
	IMPLEMENT_DUICONTROL (CLabelUI)

	CLabelUI::CLabelUI (): m_uTextStyle (DT_VCENTER | DT_SINGLELINE) {}

	CLabelUI::~CLabelUI () {}

	string_view_t CLabelUI::GetClass () const {
		return _T ("LabelUI");
	}

	LPVOID CLabelUI::GetInterface (string_view_t pstrName) {
		if (_tcsicmp (pstrName.data (), _T ("Label")) == 0) return static_cast<CLabelUI*>(this);
		return CControlUI::GetInterface (pstrName);
	}

	UINT CLabelUI::GetControlFlags () const {
		return IsEnabled () ? UIFLAG_SETCURSOR : 0;
	}
	void CLabelUI::SetTextStyle (UINT uStyle) {
		m_uTextStyle = uStyle;
		Invalidate ();
	}

	UINT CLabelUI::GetTextStyle () const {
		return m_uTextStyle;
	}

	void CLabelUI::SetTextColor (DWORD dwTextColor) {
		m_dwTextColor = dwTextColor;
		Invalidate ();
	}

	DWORD CLabelUI::GetTextColor () const {
		return m_dwTextColor;
	}

	void CLabelUI::SetDisabledTextColor (DWORD dwTextColor) {
		m_dwDisabledTextColor = dwTextColor;
		Invalidate ();
	}

	DWORD CLabelUI::GetDisabledTextColor () const {
		return m_dwDisabledTextColor;
	}

	void CLabelUI::SetFont (int index) {
		m_iFont = index;
		m_bNeedEstimateSize = true;
		Invalidate ();
	}

	int CLabelUI::GetFont () const {
		return m_iFont;
	}

	RECT CLabelUI::GetTextPadding () const {
		return m_rcTextPadding;
	}

	void CLabelUI::SetTextPadding (RECT rc) {
		m_rcTextPadding = rc;
		m_bNeedEstimateSize = true;
		Invalidate ();
	}

	bool CLabelUI::IsShowHtml () {
		return m_bShowHtml;
	}

	void CLabelUI::SetShowHtml (bool bShowHtml) {
		if (m_bShowHtml == bShowHtml) return;

		m_bShowHtml = bShowHtml;
		m_bNeedEstimateSize = true;
		Invalidate ();
	}

	SIZE CLabelUI::EstimateSize (SIZE szAvailable) {
		if (m_cxyFixed.cx > 0 && m_cxyFixed.cy > 0) return m_cxyFixed;

		if ((szAvailable.cx != m_szAvailableLast.cx || szAvailable.cy != m_szAvailableLast.cy)) {
			m_bNeedEstimateSize = true;
		}

		if (m_bNeedEstimateSize) {
			CDuiString sText = GetText ();
			m_bNeedEstimateSize = false;
			m_szAvailableLast = szAvailable;
			m_cxyFixedLast = m_cxyFixed;
			if ((m_uTextStyle & DT_SINGLELINE) != 0) {
				if (m_cxyFixedLast.cy == 0) {
					m_cxyFixedLast.cy = m_pManager->GetFontInfo (m_iFont)->tm.tmHeight + 8;
					m_cxyFixedLast.cy += GetManager ()->GetDPIObj ()->Scale (m_rcTextPadding.top + m_rcTextPadding.bottom);
				}
				if (m_cxyFixedLast.cx == 0) {
					if (m_bAutoCalcWidth) {
						RECT rcText = { 0, 0, 9999, m_cxyFixedLast.cy };
						if (m_bShowHtml) {
							int nLinks = 0;
							CRenderEngine::DrawHtmlText (m_pManager->GetPaintDC (), m_pManager, rcText, sText, 0, NULL, NULL, nLinks, m_iFont, DT_CALCRECT | m_uTextStyle & ~DT_RIGHT & ~DT_CENTER);
						} else {
							CRenderEngine::DrawText (m_pManager->GetPaintDC (), m_pManager, rcText, sText, 0, m_iFont, DT_CALCRECT | m_uTextStyle & ~DT_RIGHT & ~DT_CENTER);
						}
						m_cxyFixedLast.cx = rcText.right - rcText.left + GetManager ()->GetDPIObj ()->Scale (m_rcTextPadding.left + m_rcTextPadding.right);
					} else {
						m_cxyFixedLast.cx = szAvailable.cx;
					}
				}
			} else {
				if (m_cxyFixedLast.cx == 0) {
					m_cxyFixedLast.cx = szAvailable.cx;
				}
				if (m_cxyFixedLast.cy == 0) {
					if (m_bAutoCalcHeight) {
						RECT rcText = { 0, 0, m_cxyFixedLast.cx, 9999 };
						rcText.left += m_rcTextPadding.left;
						rcText.right -= m_rcTextPadding.right;
						if (m_bShowHtml) {
							int nLinks = 0;
							CRenderEngine::DrawHtmlText (m_pManager->GetPaintDC (), m_pManager, rcText, sText, 0, NULL, NULL, nLinks, m_iFont, DT_CALCRECT | m_uTextStyle & ~DT_RIGHT & ~DT_CENTER);
						} else {
							CRenderEngine::DrawText (m_pManager->GetPaintDC (), m_pManager, rcText, sText, 0, m_iFont, DT_CALCRECT | m_uTextStyle & ~DT_RIGHT & ~DT_CENTER);
						}
						m_cxyFixedLast.cy = rcText.bottom - rcText.top + GetManager ()->GetDPIObj ()->Scale (m_rcTextPadding.top + m_rcTextPadding.bottom);
					} else {
						//m_cxyFixedLast.cy = szAvailable.cy;
					}
				}
			}
		}
		return m_cxyFixedLast;
	}

	void CLabelUI::DoEvent (TEventUI& event) {
		if (event.Type == UIEVENT_SETFOCUS) {
			m_bFocused = true;
			return;
		}
		if (event.Type == UIEVENT_KILLFOCUS) {
			m_bFocused = false;
			return;
		}
		CControlUI::DoEvent (event);
	}

	void CLabelUI::SetAttribute (string_view_t pstrName, string_view_t pstrValue) {
		if (_tcsicmp (pstrName.data (), _T ("align")) == 0) {
			if (_tcsstr (pstrValue.data (), _T ("left")) != nullptr) {
				m_uTextStyle &= ~(DT_CENTER | DT_RIGHT);
				m_uTextStyle |= DT_LEFT;
			}
			if (_tcsstr (pstrValue.data (), _T ("center")) != nullptr) {
				m_uTextStyle &= ~(DT_LEFT | DT_RIGHT);
				m_uTextStyle |= DT_CENTER;
			}
			if (_tcsstr (pstrValue.data (), _T ("right")) != nullptr) {
				m_uTextStyle &= ~(DT_LEFT | DT_CENTER);
				m_uTextStyle |= DT_RIGHT;
			}
		} else if (_tcsicmp (pstrName.data (), _T ("valign")) == 0) {
			if (_tcsstr (pstrValue.data (), _T ("top")) != nullptr) {
				m_uTextStyle &= ~(DT_BOTTOM | DT_VCENTER | DT_WORDBREAK);
				m_uTextStyle |= (DT_TOP | DT_SINGLELINE);
			}
			if (_tcsstr (pstrValue.data (), _T ("vcenter")) != nullptr) {
				m_uTextStyle &= ~(DT_TOP | DT_BOTTOM | DT_WORDBREAK);
				m_uTextStyle |= (DT_VCENTER | DT_SINGLELINE);
			}
			if (_tcsstr (pstrValue.data (), _T ("bottom")) != nullptr) {
				m_uTextStyle &= ~(DT_TOP | DT_VCENTER | DT_WORDBREAK);
				m_uTextStyle |= (DT_BOTTOM | DT_SINGLELINE);
			}
		} else if (_tcsicmp (pstrName.data (), _T ("endellipsis")) == 0) {
			if (_tcsicmp (pstrValue.data (), _T ("true")) == 0) m_uTextStyle |= DT_END_ELLIPSIS;
			else m_uTextStyle &= ~DT_END_ELLIPSIS;
		} else if (_tcsicmp (pstrName.data (), _T ("wordbreak")) == 0) {
			if (FawTools::parse_bool (pstrValue)) {
				m_uTextStyle &= ~DT_SINGLELINE;
				m_uTextStyle |= DT_WORDBREAK | DT_EDITCONTROL;
			} else {
				m_uTextStyle &= ~DT_WORDBREAK & ~DT_EDITCONTROL;
				m_uTextStyle |= DT_SINGLELINE;
			}
		} else if (_tcsicmp (pstrName.data (), _T ("noprefix")) == 0) {
			if (FawTools::parse_bool (pstrValue)) {
				m_uTextStyle |= DT_NOPREFIX;
			} else {
				m_uTextStyle = m_uTextStyle & ~DT_NOPREFIX;
			}
		} else if (_tcsicmp (pstrName.data (), _T ("font")) == 0) SetFont (FawTools::parse_dec (pstrValue));
		else if (_tcsicmp (pstrName.data (), _T ("textcolor")) == 0) {
			DWORD clrColor = (DWORD) FawTools::parse_hex (pstrValue);
			SetTextColor (clrColor);
		} else if (_tcsicmp (pstrName.data (), _T ("disabledtextcolor")) == 0) {
			DWORD clrColor = (DWORD) FawTools::parse_hex (pstrValue);
			SetDisabledTextColor (clrColor);
		} else if (_tcsicmp (pstrName.data (), _T ("textpadding")) == 0) {
			RECT rcTextPadding = FawTools::parse_rect (pstrValue);
			SetTextPadding (rcTextPadding);
		} else if (_tcsicmp (pstrName.data (), _T ("showhtml")) == 0) {
			SetShowHtml (FawTools::parse_bool (pstrValue));
		} else if (_tcsicmp (pstrName.data (), _T ("autocalcwidth")) == 0) {
			SetAutoCalcWidth (FawTools::parse_bool (pstrValue));
		} else if (_tcsicmp (pstrName.data (), _T ("autocalcheight")) == 0) {
			SetAutoCalcHeight (FawTools::parse_bool (pstrValue));
		} else {
			CControlUI::SetAttribute (pstrName, pstrValue);
		}
	}

	void CLabelUI::PaintText (HDC hDC) {
		if (m_dwTextColor == 0) m_dwTextColor = m_pManager->GetDefaultFontColor ();
		if (m_dwDisabledTextColor == 0) m_dwDisabledTextColor = m_pManager->GetDefaultDisabledColor ();

		RECT rc = m_rcItem;
		RECT _rcTextPadding = CLabelUI::m_rcTextPadding;
		GetManager ()->GetDPIObj ()->Scale (&_rcTextPadding);
		rc.left += _rcTextPadding.left;
		rc.right -= _rcTextPadding.right;
		rc.top += _rcTextPadding.top;
		rc.bottom -= _rcTextPadding.bottom;

		CDuiString sText = GetText ();
		if (sText.empty ()) return;
		int nLinks = 0;
		if (IsEnabled ()) {
			if (m_bShowHtml)
				CRenderEngine::DrawHtmlText (hDC, m_pManager, rc, sText, m_dwTextColor, \
					nullptr, nullptr, nLinks, m_iFont, m_uTextStyle);
			else
				CRenderEngine::DrawText (hDC, m_pManager, rc, sText, m_dwTextColor, \
					m_iFont, m_uTextStyle);
		} else {
			if (m_bShowHtml)
				CRenderEngine::DrawHtmlText (hDC, m_pManager, rc, sText, m_dwDisabledTextColor, \
					nullptr, nullptr, nLinks, m_iFont, m_uTextStyle);
			else
				CRenderEngine::DrawText (hDC, m_pManager, rc, sText, m_dwDisabledTextColor, \
					m_iFont, m_uTextStyle);
		}
	}

	bool CLabelUI::GetAutoCalcWidth () const {
		return m_bAutoCalcWidth;
	}

	void CLabelUI::SetAutoCalcWidth (bool bAutoCalcWidth) {
		m_bAutoCalcWidth = bAutoCalcWidth;
	}

	bool CLabelUI::GetAutoCalcHeight () const {
		return m_bAutoCalcHeight;
	}

	void CLabelUI::SetAutoCalcHeight (bool bAutoCalcHeight) {
		m_bAutoCalcHeight = bAutoCalcHeight;
	}

	void CLabelUI::SetText (string_view_t pstrText) {
		CControlUI::SetText (pstrText);
		if (GetAutoCalcWidth () || GetAutoCalcHeight ()) {
			NeedParentUpdate ();
		}
	}
}