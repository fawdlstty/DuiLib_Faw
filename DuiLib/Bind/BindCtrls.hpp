#ifndef __BIND_CTRLS_HPP__
#define __BIND_CTRLS_HPP__

#pragma once

namespace DuiLib {
#define DEF_BINDCTRL(ctrl_name, ctrl_name_str)													\
	class Bind##ctrl_name##UI: public BindCtrlBase {											\
	public:																						\
		C##ctrl_name##UI *operator-> () { return static_cast<C##ctrl_name##UI*> (m_ctrl); }		\
	protected:																					\
		string_view_t GetClassType () const override { return _T (ctrl_name_str"UI"); }			\
	}
	DEF_BINDCTRL (ActiveX,			"ActiveX");
	//DEF_BINDCTRL (Animation,		"Animation");
	DEF_BINDCTRL (Button,			"Button");
	DEF_BINDCTRL (ColorPalette,		"ColorPalette");
	DEF_BINDCTRL (Combo,			"Combo");
	DEF_BINDCTRL (ComboBox,			"ComboBox");
	DEF_BINDCTRL (DateTime,			"DateTime");
	DEF_BINDCTRL (Edit,				"Edit");
	DEF_BINDCTRL (FadeButton,		"FadeButton");
	DEF_BINDCTRL (Flash,			"Flash");
	DEF_BINDCTRL (GifAnim,			"GifAnim");
	//DEF_BINDCTRL (GifAnimEx,		"GifAnimEx");
	DEF_BINDCTRL (GroupBox,			"GroupBox");
	DEF_BINDCTRL (HotKey,			"HotKey");
	DEF_BINDCTRL (IPAddress,		"IPAddress");
	DEF_BINDCTRL (IPAddressEx,		"IPAddressEx");
	DEF_BINDCTRL (Label,			"Label");
	DEF_BINDCTRL (List,				"List");
	DEF_BINDCTRL (ListEx,			"ListEx");
	DEF_BINDCTRL (Menu,				"Menu");
	DEF_BINDCTRL (Option,			"Option");
	DEF_BINDCTRL (Progress,			"Progress");
	DEF_BINDCTRL (RichEdit,			"RichEdit");
	DEF_BINDCTRL (Ring,				"Ring");
	DEF_BINDCTRL (RollText,			"RollText");
	DEF_BINDCTRL (ScrollBar,		"ScrollBar");
	DEF_BINDCTRL (Slider,			"Slider");
	DEF_BINDCTRL (Text,				"Text");
	DEF_BINDCTRL (TreeView,			"TreeView");
	DEF_BINDCTRL (WebBrowser,		"WebBrowser");
}

#endif //__BIND_CTRLS_HPP__
