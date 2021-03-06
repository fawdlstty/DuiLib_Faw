﻿#ifndef __UIDLGBUILDER_H__
#define __UIDLGBUILDER_H__

#pragma once

namespace DuiLib {

	class IDialogBuilderCallback {
	public:
		virtual CControlUI* CreateControl (faw::string_t pstrClass) = 0;
	};


	class UILIB_API CDialogBuilder {
	public:
		CDialogBuilder ();
		CControlUI* Create (std::variant<UINT, faw::string_t> xml, faw::string_t type = _T (""), IDialogBuilderCallback* pCallback = nullptr, CPaintManagerUI* pManager = nullptr, CControlUI* pParent = nullptr);
		CControlUI* Create (IDialogBuilderCallback* pCallback = nullptr, CPaintManagerUI* pManager = nullptr, CControlUI* pParent = nullptr);

		CMarkup* GetMarkup ();

		faw::string_t GetLastErrorMessage () const;
		faw::string_t GetLastErrorLocation () const;
		void SetInstance (HINSTANCE instance) { m_instance = instance; };
	private:
		CControlUI* _Parse (CMarkupNode* parent, CControlUI* pParent = nullptr, CPaintManagerUI* pManager = nullptr);

		CMarkup					m_xml;
		IDialogBuilderCallback	*m_pCallback	= nullptr;
		faw::string_t				m_pstrtype		= _T ("");
		HINSTANCE				m_instance		= NULL;
	};

} // namespace DuiLib

#endif // __UIDLGBUILDER_H__
