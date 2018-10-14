#ifndef __UIDLGBUILDER_H__
#define __UIDLGBUILDER_H__

#pragma once

namespace DuiLib {

	class IDialogBuilderCallback {
	public:
		virtual CControlUI* CreateControl (string_view_t pstrClass) = 0;
	};


	class UILIB_API CDialogBuilder {
	public:
		CDialogBuilder ();
		CControlUI* Create (std::variant<UINT, string_t> xml, string_view_t type = nullptr, IDialogBuilderCallback* pCallback = nullptr,
			CPaintManagerUI* pManager = nullptr, CControlUI* pParent = nullptr);
		CControlUI* Create (IDialogBuilderCallback* pCallback = nullptr, CPaintManagerUI* pManager = nullptr,
			CControlUI* pParent = nullptr);

		CMarkup* GetMarkup ();

		void GetLastErrorMessage (LPTSTR pstrMessage, SIZE_T cchMax) const;
		void GetLastErrorLocation (LPTSTR pstrSource, SIZE_T cchMax) const;
		void SetInstance (HINSTANCE instance) { m_instance = instance; };
	private:
		CControlUI* _Parse (CMarkupNode* parent, CControlUI* pParent = nullptr, CPaintManagerUI* pManager = nullptr);

		CMarkup					m_xml;
		IDialogBuilderCallback	*m_pCallback	= nullptr;
		CDuiString				m_pstrtype		= _T ("");
		HINSTANCE				m_instance		= NULL;
	};

} // namespace DuiLib

#endif // __UIDLGBUILDER_H__
