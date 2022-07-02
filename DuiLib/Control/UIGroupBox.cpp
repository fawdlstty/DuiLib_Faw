#include "StdAfx.h"
#include "UIGroupBox.h"

namespace DuiLib {
	IMPLEMENT_DUICONTROL (CGroupBoxUI)

	//////////////////////////////////////////////////////////////////////////
	//
	CGroupBoxUI::CGroupBoxUI (): m_uTextStyle (DT_SINGLELINE | DT_VCENTER | DT_CENTER) {
		SetInset ({ 20, 25, 20, 20 });
	}

	CGroupBoxUI::~CGroupBoxUI () {}

	faw::string_t CGroupBoxUI::GetClass () const {
		return _T ("GroupBoxUI");
	}

	LPVOID CGroupBoxUI::GetInterface (faw::string_t pstrName) {
		if (pstrName == _T ("GroupBox")) return static_cast<CGroupBoxUI*>(this);
		return CVerticalLayoutUI::GetInterface (pstrName);
	}
	void CGroupBoxUI::SetTextColor (DWORD dwTextColor) {
		m_dwTextColor = dwTextColor;
		Invalidate ();
	}

	DWORD CGroupBoxUI::GetTextColor () const {
		return m_dwTextColor;
	}
	void CGroupBoxUI::SetDisabledTextColor (DWORD dwTextColor) {
		m_dwDisabledTextColor = dwTextColor;
		Invalidate ();
	}

	DWORD CGroupBoxUI::GetDisabledTextColor () const {
		return m_dwDisabledTextColor;
	}
	void CGroupBoxUI::SetFont (int index) {
		m_iFont = index;
		Invalidate ();
	}

	int CGroupBoxUI::GetFont () const {
		return m_iFont;
	}
	void CGroupBoxUI::PaintText (HDC hDC) {
		faw::string_t sText = GetText ();
		if (sText.empty ()) {
			return;
		}

		if (m_dwTextColor == 0) m_dwTextColor = m_pManager->GetDefaultFontColor ();
		if (m_dwDisabledTextColor == 0) m_dwDisabledTextColor = m_pManager->GetDefaultDisabledColor ();
		if (sText.empty ()) return;

		RECT rcText = m_rcItem;
		SIZE szAvailable = { m_rcItem.right - m_rcItem.left - 10 , m_rcItem.bottom - m_rcItem.top - 10 };
		SIZE sz = CalcrectSize(szAvailable);
		::InflateRect(&rcText, -sz.cy / 2, -sz.cy / 2);

		//计算文字区域
		rcText.left = rcText.left + 15;
		rcText.top = rcText.top - sz.cy / 2;
		rcText.right = rcText.left + sz.cx;
		rcText.bottom = rcText.top + sz.cy;

		DWORD dwTextColor = m_dwTextColor;
		if (!IsEnabled ()) dwTextColor = m_dwDisabledTextColor;
		CRenderEngine::DrawText (hDC, m_pManager, rcText, sText, dwTextColor, m_iFont, m_uTextStyle, GetAdjustColor (m_dwBackColor));
	}
	void CGroupBoxUI::PaintBorder (HDC hDC) {
		int nBorderSize;
		SIZE cxyBorderRound = { 0 };
		RECT rcBorderSize = { 0 };
		if (m_pManager) {
			nBorderSize = GetManager ()->GetDPIObj ()->Scale (m_nBorderSize);
			cxyBorderRound = GetManager ()->GetDPIObj ()->Scale (m_cxyBorderRound);
			rcBorderSize = GetManager ()->GetDPIObj ()->Scale (m_rcBorderSize);
		} else {
			nBorderSize = m_nBorderSize;
			cxyBorderRound = m_cxyBorderRound;
			rcBorderSize = m_rcBorderSize;
		}

		if (nBorderSize > 0) {
			Gdiplus::Bitmap gb (m_rcItem.right - m_rcItem.left, m_rcItem.bottom - m_rcItem.top, PixelFormat32bppARGB);
			Gdiplus::Graphics gg (&gb);
			RECT rcItem = m_rcItem;
			DWORD dwColor = GetAdjustColor ((IsFocused () && m_dwFocusBorderColor != 0) ? m_dwFocusBorderColor : m_dwBorderColor);
			HDC gghdc = gg.GetHDC ();
			if (cxyBorderRound.cx > 0 || cxyBorderRound.cy > 0) {
				CRenderEngine::DrawRoundRect (gghdc, rcItem, nBorderSize, cxyBorderRound.cx, cxyBorderRound.cy, dwColor);
			} else {
				CRenderEngine::DrawRect (gghdc, rcItem, nBorderSize, dwColor);
			}
			gg.ReleaseHDC (gghdc);

			// 可用空间
			SIZE szAvailable = { m_rcItem.right - m_rcItem.left - 10, m_rcItem.bottom - m_rcItem.top - 10 };
			// 文字大小
			SIZE sz = CalcrectSize(szAvailable);
			// 边框大小,边框缩放到文字腰部
			::InflateRect(&rcItem, -sz.cy / 2, -sz.cy / 2);
			// 文字位置,边框原点左移15,再上移使边框到其腰部
			RECT rcItemText = rcItem;
			rcItemText.left = rcItemText.left + 15;
			rcItemText.top = rcItemText.top - sz.cy / 2;
			rcItemText.right = rcItemText.left + sz.cx;
			rcItemText.bottom = rcItemText.top + sz.cy;
			// 清除文字位置的边框
			PaintGroupBorder(hDC, rcItem.left, rcItem.top, rcItem.right - rcItem.left - 1, rcItem.bottom - rcItem.top - 1, cxyBorderRound.cx, nBorderSize,
				Gdiplus::Color(GetAdjustColor(m_dwFocusBorderColor)), rcItemText, Gdiplus::Color(GetAdjustColor(m_dwBackColor)));
		}

		PaintText (hDC);
	}

	void CGroupBoxUI::PaintGroupBorder(HDC hDC, float x, float y, float width, float height, float arcSize, float lineWidth, Gdiplus::Color lineColor, RECT rcItemText, Gdiplus::Color fillColor)
	{
		float arcDiameter = arcSize * 2;
		// 创建GDI+对象
		Gdiplus::Graphics  g(hDC);
		//设置画图时的滤波模式为消除锯齿现象
		g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

		// 绘图路径
		Gdiplus::GraphicsPath roundRectPath;

		roundRectPath.AddLine(x + 15 + (rcItemText.right - rcItemText.left), y, x + width - arcSize, y);  // 顶部横线文字右侧
		roundRectPath.AddArc(x + width - arcDiameter, y, arcDiameter, arcDiameter, 270, 90); // 右上圆角

		roundRectPath.AddLine(x + width, y + arcSize, x + width, y + height - arcSize);  // 右侧竖线
		roundRectPath.AddArc(x + width - arcDiameter, y + height - arcDiameter, arcDiameter, arcDiameter, 0, 90); // 右下圆角

		roundRectPath.AddLine(x + width - arcSize, y + height, x + arcSize, y + height);  // 底部横线
		roundRectPath.AddArc(x, y + height - arcDiameter, arcDiameter, arcDiameter, 90, 90); // 左下圆角

		roundRectPath.AddLine(x, y + height - arcSize, x, y + arcSize);  // 左侧竖线
		roundRectPath.AddArc(x, y, arcDiameter, arcDiameter, 180, 90); // 左上圆角
		if (arcSize < 15)
			roundRectPath.AddLine(x + arcSize, y,x + 15 , y);  // 顶部横线文字左侧

		//创建画笔
		Gdiplus::Pen pen(Gdiplus::Color(GetAdjustColor(m_dwBorderColor)), m_nBorderSize);
		// 绘制矩形
		g.DrawPath(&pen, &roundRectPath);
	}

	SIZE CGroupBoxUI::CalcrectSize (SIZE szAvailable) {
		SIZE cxyFixed = GetFixedXY ();
		RECT rcText = { 0, 0, MAX (szAvailable.cx, cxyFixed.cx), 20 };

		faw::string_t sText = GetText ();

		CRenderEngine::DrawText (m_pManager->GetPaintDC (), m_pManager, rcText, sText, m_dwTextColor, m_iFont, DT_CALCRECT | m_uTextStyle);
		SIZE cXY = { rcText.right - rcText.left, rcText.bottom - rcText.top };
		return cXY;
	}
	void CGroupBoxUI::SetAttribute (faw::string_t pstrName, faw::string_t pstrValue) {
		if (pstrName == _T ("textcolor")) {
			SetTextColor ((DWORD) FawTools::parse_hex (pstrValue));
		} else if (pstrName == _T ("disabledtextcolor")) {
			SetDisabledTextColor ((DWORD) FawTools::parse_hex (pstrValue));
		} else if (pstrName == _T ("font")) {
			SetFont (FawTools::parse_dec (pstrValue));
		}

		CVerticalLayoutUI::SetAttribute (pstrName, pstrValue);
	}
}
