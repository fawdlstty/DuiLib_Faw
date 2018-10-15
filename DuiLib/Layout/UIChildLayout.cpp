#include "StdAfx.h"
#include "UIChildLayout.h"

namespace DuiLib {
	IMPLEMENT_DUICONTROL (CChildLayoutUI)

	CChildLayoutUI::CChildLayoutUI () {}

	void CChildLayoutUI::Init () {
		if (!m_pstrXMLFile.empty ()) {
			CDialogBuilder builder;
			CContainerUI* pChildWindow = static_cast<CContainerUI*>(builder.Create (m_pstrXMLFile, (UINT) 0, nullptr, m_pManager));
			if (pChildWindow) {
				this->Add (pChildWindow);
			} else {
				this->RemoveAll ();
			}
		}
	}

	void CChildLayoutUI::SetAttribute (string_view_t pstrName, string_view_t pstrValue) {
		if (_tcsicmp (pstrName.data (), _T ("xmlfile")) == 0)
			SetChildLayoutXML (pstrValue);
		else
			CContainerUI::SetAttribute (pstrName, pstrValue);
	}

	void CChildLayoutUI::SetChildLayoutXML (DuiLib::CDuiString pXML) {
		m_pstrXMLFile = pXML;
	}

	DuiLib::CDuiString CChildLayoutUI::GetChildLayoutXML () {
		return m_pstrXMLFile;
	}

	LPVOID CChildLayoutUI::GetInterface (string_view_t pstrName) {
		if (_tcsicmp (pstrName.data (), DUI_CTR_CHILDLAYOUT) == 0) return static_cast<CChildLayoutUI*>(this);
		return CControlUI::GetInterface (pstrName);
	}

	string_view_t CChildLayoutUI::GetClass () const {
		return _T ("ChildLayoutUI");
	}
} // namespace DuiLib
