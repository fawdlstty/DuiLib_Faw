#ifndef __BIND_BASE_HPP__
#define __BIND_BASE_HPP__

#pragma once



namespace DuiLib {
	class BindCtrlBase {
	public:
		BindCtrlBase (string_view_t ctrl_name): m_ctrl_name (ctrl_name) {}
		virtual ~BindCtrlBase () = default;

	protected:
		virtual string_view_t GetClassType () = 0;

	private:
		string_view_t m_ctrl_name;

		static std::map<string_t, BindCtrlBase*> s_bind_ctrls;
	};



	class BindVarSuperBase {
	public:
		BindVarSuperBase (string_view_t ctrl_name): m_ctrl_name (ctrl_name) { s_bind_vars[ctrl_name.data ()] = this; }
		virtual ~BindVarSuperBase () = default;

	protected:
		virtual string_view_t GetVarType () = 0;

	private:
		string_t m_ctrl_name = _T ("");
		string_t m_ctrl_class_name = _T ("");
		CControlUI *m_ctrl = nullptr;

		static void init_binding (CPaintManagerUI *pm) {
			for (auto[ctrl_name, obj] : s_bind_vars) {
				obj->m_ctrl = pm->FindControl (ctrl_name);
				if (obj->m_ctrl == nullptr)
					continue;
				obj->m_ctrl_class_name = obj->m_ctrl->GetClass ();
				if (obj->GetVarType () == _T ("string_t")) {
					static std::vector<string_t> sv_string_t { _T ("ComboUI"), _T ("ComboBoxUI"), _T ("DateTimeUI"), _T ("EditUI"), _T ("GroupBoxUI"), _T ("IPAddressUI"), _T ("IPAddressExUI"), _T ("LabelUI"), _T ("OptionUI"), _T ("RichEditUI"), _T ("TextUI") };
					if (FawTools::find (sv_string_t, obj->m_ctrl_class_name) == string_t::npos) {
						DUITRACE (_T ("%s 无法绑定非 %s 类型变量"), obj->m_ctrl_class_name.data (), obj->GetVarType ().data ());
						ASSERT (false);
					}
				}
			}
		}
		static std::map<string_t, BindVarSuperBase*> s_bind_vars;
	};

	template <typename T>
	class BindVarBase: public BindVarSuperBase {
	public:
		BindVarBase (string_view_t ctrl_name): BindVarBase (ctrl_name) {}
		virtual ~BindVarBase () = default;

		string_view_t GetVarType () override {
			using base_T = std::decay<T>::type;
			if constexpr (std::is_same<base_T, string_t>::value) {
				return _T ("string_t");
			} else if constexpr (std::is_same<base_T, size_t>::value) {
				return _T ("size_t");
			} else if constexpr (std::is_same<base_T, int32_t>::value) {
				return _T ("int32_t");
			} else if constexpr (std::is_same<base_T, int64_t>::value) {
				return _T ("int64_t");
			} else if constexpr (std::is_same<base_T, bool>::value) {
				return _T ("bool");
			} else constexpr {
				return _T ("");
			}
		}
	};



	inline std::map<string_t, BindCtrlBase*> BindCtrlBase::s_bind_ctrls;
	inline std::map<string_t, BindVarSuperBase*> BindVarSuperBase::s_bind_vars;
}

#endif //__BIND_BASE_HPP__
