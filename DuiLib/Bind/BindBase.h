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

		static void init_binding (CPaintManagerUI *pm);
		static std::map<string_t, BindVarSuperBase*> s_bind_vars;
	};

	template <typename T>
	class BindVarBase: public BindVarSuperBase {
	public:
		BindVarBase (string_view_t ctrl_name): BindVarBase (ctrl_name) {}
		virtual ~BindVarBase () = default;

		string_view_t GetVarType () override;
	};
}

#endif //__BIND_BASE_HPP__
