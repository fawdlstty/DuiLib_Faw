#ifndef __BIND_CTRLS_HPP__
#define __BIND_CTRLS_HPP__

#pragma once

namespace DuiLib {
#define DEF_BINDCTRL(ctrl_name)													\
	class Bind##ctrl_name##UI: public BindCtrlBase {											\
	public:																						\
		C##ctrl_name##UI *operator-> () { return static_cast<C##ctrl_name##UI*> (m_ctrl); }		\
	protected:																					\
		string_view_t GetClassType () const override { return _T (#ctrl_name##"UI"); }			\
	}
	DEF_BINDCTRL (ActiveX);
	//DEF_BINDCTRL (Animation);
	DEF_BINDCTRL (Button);
	DEF_BINDCTRL (ColorPalette);
	DEF_BINDCTRL (Combo);
	DEF_BINDCTRL (ComboBox);
	DEF_BINDCTRL (DateTime);
	DEF_BINDCTRL (Edit);
	DEF_BINDCTRL (FadeButton);
	DEF_BINDCTRL (Flash);
	DEF_BINDCTRL (GifAnim);
	//DEF_BINDCTRL (GifAnimEx);
	DEF_BINDCTRL (GroupBox);
	DEF_BINDCTRL (HotKey);
	DEF_BINDCTRL (IPAddress);
	DEF_BINDCTRL (IPAddressEx);
	DEF_BINDCTRL (Label);
	DEF_BINDCTRL (List);
	DEF_BINDCTRL (ListEx);
	DEF_BINDCTRL (Menu);
	DEF_BINDCTRL (Option);
	DEF_BINDCTRL (Progress);
	DEF_BINDCTRL (RichEdit);
	DEF_BINDCTRL (Ring);
	DEF_BINDCTRL (RollText);
	DEF_BINDCTRL (ScrollBar);
	DEF_BINDCTRL (Slider);
	DEF_BINDCTRL (Text);
	DEF_BINDCTRL (TreeView);
	DEF_BINDCTRL (WebBrowser);
}

#endif //__BIND_CTRLS_HPP__
