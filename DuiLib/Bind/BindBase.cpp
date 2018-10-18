#include "StdAfx.h"



namespace DuiLib {
	std::map<string_t, BindCtrlBase*> BindCtrlBase::s_bind_ctrls;
	std::map<string_t, BindVarSuperBase*> BindVarSuperBase::s_bind_vars;



	void BindVarSuperBase::init_binding (CPaintManagerUI *pm) {
		for (auto[ctrl_name, obj] : s_bind_vars) {
			obj->m_ctrl = pm->FindControl (ctrl_name);
			if (obj->m_ctrl == nullptr)
				continue;
			obj->m_ctrl_class_name = obj->m_ctrl->GetClass ();
			if (obj->GetVarType () == _T ("string_t")) {
				static std::map<string_t, std::vector<string_t>> smv {
					{ _T ("string_t"), { _T ("ComboUI"), _T ("ComboBoxUI"), _T ("DateTimeUI"), _T ("EditUI"), _T ("GroupBoxUI"), _T ("IPAddressUI"), _T ("IPAddressExUI"), _T ("LabelUI"), _T ("OptionUI"), _T ("RichEditUI"), _T ("TextUI") } },
					{ _T ("size_t"), { _T ("EditUI"), _T ("GroupBoxUI"), _T ("LabelUI"), _T ("OptionUI"), _T ("RichEditUI"), _T ("TextUI") } },
					{ _T ("int64_t"), { _T ("EditUI"), _T ("GroupBoxUI"), _T ("LabelUI"), _T ("OptionUI"), _T ("RichEditUI"), _T ("TextUI") } },
					{ _T ("bool"), { _T ("CheckBox") } }
				};
				if (smv.find (obj->GetVarType ().data ()) == smv.end ()) {
					DUITRACE (_T ("不支持的模板类型：%s"), obj->GetVarType ().data ());
					ASSERT (false);
				} else if (FawTools::find (smv[obj->GetVarType ().data ()], obj->m_ctrl_class_name) == string_t::npos) {
					DUITRACE (_T ("%s 无法绑定非 %s 类型变量"), obj->m_ctrl_class_name.data (), obj->GetVarType ().data ());
					ASSERT (false);
				}
			}
		}
	}

	template <typename T>
	string_view_t BindVarBase<T>::GetVarType () {
		using base_T = std::decay<T>;
		if constexpr (std::is_same<base_T, string_t>::value) {
			return _T ("string_t");
		} else if constexpr (std::is_same<base_T, size_t>::value) {
			return _T ("size_t");
		} else if constexpr (std::is_same<base_T, int64_t>::value) {
			return _T ("int64_t");
		} else if constexpr (std::is_same<base_T, bool>::value) {
			return _T ("bool");
		} else {
			return _T ("");
		}
	}
}
