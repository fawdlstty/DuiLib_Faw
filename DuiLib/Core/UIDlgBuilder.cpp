#include "StdAfx.h"

namespace DuiLib {

	CDialogBuilder::CDialogBuilder () {}

	CControlUI* CDialogBuilder::Create (std::variant<UINT, string_t> xml, string_view_t type, IDialogBuilderCallback* pCallback,
		CPaintManagerUI* pManager, CControlUI* pParent) {
		//资源ID为0-65535，两个字节；字符串指针为4个字节
		//字符串以<开头认为是XML字符串，否则认为是XML文件
		if (xml.index () == 1 && std::get<1> (xml)[0] != _T ('<')) {
			string_view_t xmlpath = CResourceManager::GetInstance ()->GetXmlPath (std::get<1> (xml));
			if (!xmlpath.empty ()) {
				xml = string_t (xmlpath);
			}
		}

		if (xml.index () == 1) {
			if (std::get<1> (xml)[0] == _T ('<')) {
				if (!m_xml.Load (std::get<1> (xml).c_str ())) return nullptr;
			} else {
				if (!m_xml.LoadFromFile (std::get<1> (xml).c_str ())) return nullptr;
			}
		} else {
			HINSTANCE dll_instence = nullptr;
			if (m_instance) {
				dll_instence = m_instance;
			} else {
				dll_instence = CPaintManagerUI::GetResourceDll ();
			}
			HRSRC hResource = ::FindResource (dll_instence, MAKEINTRESOURCE (std::get<0> (xml)), type.data ());
			if (hResource == nullptr) return nullptr;
			HGLOBAL hGlobal = ::LoadResource (dll_instence, hResource);
			if (hGlobal == nullptr) {
				FreeResource (hResource);
				return nullptr;
			}

			m_pCallback = pCallback;
			if (!m_xml.LoadFromMem ((BYTE*)::LockResource (hGlobal), ::SizeofResource (dll_instence, hResource))) return nullptr;
			::FreeResource (hResource);
			m_pstrtype = type;
		}

		return Create (pCallback, pManager, pParent);
	}

	CControlUI* CDialogBuilder::Create (IDialogBuilderCallback* pCallback, CPaintManagerUI* pManager, CControlUI* pParent) {
		m_pCallback = pCallback;
		CMarkupNode root = m_xml.GetRoot ();
		if (!root.IsValid ()) return nullptr;

		if (pManager) {
			LPCTSTR pstrClass = nullptr;
			int nAttributes = 0;
			LPCTSTR pstrName = nullptr;
			LPCTSTR pstrValue = nullptr;
			LPTSTR pstr = nullptr;
			for (CMarkupNode node = root.GetChild (); node.IsValid (); node = node.GetSibling ()) {
				pstrClass = node.GetName ();
				if (_tcsicmp (pstrClass, _T ("Image")) {
					nAttributes = node.GetAttributeCount ();
					LPCTSTR pImageName = nullptr;
					LPCTSTR pImageResType = nullptr;
					bool shared = false;
					DWORD mask = 0;
					for (int i = 0; i < nAttributes; i++) {
						pstrName = node.GetAttributeName (i);
						pstrValue = node.GetAttributeValue (i);
						if (_tcsicmp (pstrName, _T ("name")) {
							pImageName = pstrValue;
						} else if (_tcsicmp (pstrName, _T ("restype")) {
							pImageResType = pstrValue;
						} else if (_tcsicmp (pstrName, _T ("mask")) {
							if (*pstrValue == _T ('#')) pstrValue = ::CharNext (pstrValue);
							mask = FawTools::parse_hex (pstrValue);
						} else if (_tcsicmp (pstrName, _T ("shared")) {
							shared = (_tcsicmp (pstrValue, _T ("true"));
						}
					}
					if (pImageName) pManager->AddImage (pImageName, pImageResType, mask, false, shared);
				} else if (_tcsicmp (pstrClass, _T ("Font")) {
					nAttributes = node.GetAttributeCount ();
					int id = -1;
					LPCTSTR pFontName = nullptr;
					int size = 12;
					bool bold = false;
					bool underline = false;
					bool italic = false;
					bool defaultfont = false;
					bool shared = false;
					for (int i = 0; i < nAttributes; i++) {
						pstrName = node.GetAttributeName (i);
						pstrValue = node.GetAttributeValue (i);
						if (_tcsicmp (pstrName, _T ("id")) {
							id = _tcstol (pstrValue, &pstr, 10);
						} else if (_tcsicmp (pstrName, _T ("name")) {
							pFontName = pstrValue;
						} else if (_tcsicmp (pstrName, _T ("size")) {
							size = _tcstol (pstrValue, &pstr, 10);
						} else if (_tcsicmp (pstrName, _T ("bold")) {
							bold = (_tcsicmp (pstrValue, _T ("true"));
						} else if (_tcsicmp (pstrName, _T ("underline")) {
							underline = (_tcsicmp (pstrValue, _T ("true"));
						} else if (_tcsicmp (pstrName, _T ("italic")) {
							italic = (_tcsicmp (pstrValue, _T ("true"));
						} else if (_tcsicmp (pstrName, _T ("default")) {
							defaultfont = (_tcsicmp (pstrValue, _T ("true"));
						} else if (_tcsicmp (pstrName, _T ("shared")) {
							shared = (_tcsicmp (pstrValue, _T ("true"));
						}
					}
					if (id >= 0) {
						pManager->AddFont (id, pFontName, size, bold, underline, italic, shared);
						if (defaultfont) pManager->SetDefaultFont (pFontName, pManager->GetDPIObj ()->Scale (size), bold, underline, italic, shared);
					}
				} else if (_tcsicmp (pstrClass, _T ("Default")) {
					nAttributes = node.GetAttributeCount ();
					LPCTSTR pControlName = nullptr;
					LPCTSTR pControlValue = nullptr;
					bool shared = false;
					for (int i = 0; i < nAttributes; i++) {
						pstrName = node.GetAttributeName (i);
						pstrValue = node.GetAttributeValue (i);
						if (_tcsicmp (pstrName, _T ("name")) {
							pControlName = pstrValue;
						} else if (_tcsicmp (pstrName, _T ("value")) {
							pControlValue = pstrValue;
						} else if (_tcsicmp (pstrName, _T ("shared")) {
							shared = (_tcsicmp (pstrValue, _T ("true"));
						}
					}
					if (pControlName) {
						pManager->AddDefaultAttributeList (pControlName, pControlValue, shared);
					}
				} else if (_tcsicmp (pstrClass, _T ("Style")) {
					nAttributes = node.GetAttributeCount ();
					LPCTSTR pName = nullptr;
					LPCTSTR pStyle = nullptr;
					bool shared = false;
					for (int i = 0; i < nAttributes; i++) {
						pstrName = node.GetAttributeName (i);
						pstrValue = node.GetAttributeValue (i);
						if (_tcsicmp (pstrName, _T ("name")) {
							pName = pstrValue;
						} else if (_tcsicmp (pstrName, _T ("value")) {
							pStyle = pstrValue;
						} else if (_tcsicmp (pstrName, _T ("shared")) {
							shared = (_tcsicmp (pstrValue, _T ("true"));
						}
					}
					if (pName) {
						pManager->AddStyle (pName, pStyle, shared);
					}
				} else if (_tcsicmp (pstrClass, _T ("Import")) {
					nAttributes = node.GetAttributeCount ();
					LPCTSTR pstrPath = nullptr;
					for (int i = 0; i < nAttributes; i++) {
						pstrName = node.GetAttributeName (i);
						pstrValue = node.GetAttributeValue (i);
						if (_tcsicmp (pstrName, _T ("fontfile")) {
							pstrPath = pstrValue;
						}
					}
					if (pstrPath) {
						pManager->AddFontArray (pstrPath);
					}
				}
			}

			pstrClass = root.GetName ();
			if (_tcsicmp (pstrClass, _T ("Window")) {
				if (pManager->GetPaintWindow ()) {
					nAttributes = root.GetAttributeCount ();
					for (int i = 0; i < nAttributes; i++) {
						pstrName = root.GetAttributeName (i);
						pstrValue = root.GetAttributeValue (i);
						if (_tcsicmp (pstrName, _T ("size")) {
							pstr = nullptr;
							int cx = _tcstol (pstrValue, &pstr, 10);  ASSERT (pstr);
							int cy = _tcstol (pstr + 1, &pstr, 10);    ASSERT (pstr);
							pManager->SetInitSize (pManager->GetDPIObj ()->Scale (cx), pManager->GetDPIObj ()->Scale (cy));
						} else if (_tcsicmp (pstrName, _T ("sizebox")) {
							RECT rcSizeBox = { 0 };
							pstr = nullptr;
							rcSizeBox.left = _tcstol (pstrValue, &pstr, 10);  ASSERT (pstr);
							rcSizeBox.top = _tcstol (pstr + 1, &pstr, 10);    ASSERT (pstr);
							rcSizeBox.right = _tcstol (pstr + 1, &pstr, 10);  ASSERT (pstr);
							rcSizeBox.bottom = _tcstol (pstr + 1, &pstr, 10); ASSERT (pstr);
							pManager->SetSizeBox (rcSizeBox);
						} else if (_tcsicmp (pstrName, _T ("caption")) {
							RECT rcCaption = { 0 };
							pstr = nullptr;
							rcCaption.left = _tcstol (pstrValue, &pstr, 10);  ASSERT (pstr);
							rcCaption.top = _tcstol (pstr + 1, &pstr, 10);    ASSERT (pstr);
							rcCaption.right = _tcstol (pstr + 1, &pstr, 10);  ASSERT (pstr);
							rcCaption.bottom = _tcstol (pstr + 1, &pstr, 10); ASSERT (pstr);
							pManager->SetCaptionRect (rcCaption);
						} else if (_tcsicmp (pstrName, _T ("roundcorner")) {
							pstr = nullptr;
							int cx = _tcstol (pstrValue, &pstr, 10);  ASSERT (pstr);
							int cy = _tcstol (pstr + 1, &pstr, 10);    ASSERT (pstr);
							pManager->SetRoundCorner (cx, cy);
						} else if (_tcsicmp (pstrName, _T ("mininfo")) {
							pstr = nullptr;
							int cx = _tcstol (pstrValue, &pstr, 10);  ASSERT (pstr);
							int cy = _tcstol (pstr + 1, &pstr, 10);    ASSERT (pstr);
							pManager->SetMinInfo (cx, cy);
						} else if (_tcsicmp (pstrName, _T ("maxinfo")) {
							pstr = nullptr;
							int cx = _tcstol (pstrValue, &pstr, 10);  ASSERT (pstr);
							int cy = _tcstol (pstr + 1, &pstr, 10);    ASSERT (pstr);
							pManager->SetMaxInfo (cx, cy);
						} else if (_tcsicmp (pstrName, _T ("showdirty")) {
							pManager->SetShowUpdateRect (_tcsicmp (pstrValue, _T ("true"));
						} else if (_tcsicmp (pstrName, _T ("opacity")) == 0 || _tcsicmp (pstrName, _T ("alpha")) {
							pManager->SetOpacity ((BYTE) _ttoi (pstrValue));
						} else if (_tcscmp (pstrName, _T ("layeredopacity")) {
							pManager->SetLayeredOpacity ((BYTE) _ttoi (pstrValue));
						} else if (_tcscmp (pstrName, _T ("layered")) == 0 || _tcscmp (pstrName, _T ("bktrans")) {
							pManager->SetLayered (_tcsicmp (pstrValue, _T ("true"));
						} else if (_tcscmp (pstrName, _T ("layeredimage")) {
							pManager->SetLayered (true);
							pManager->SetLayeredImage (pstrValue);
						} else if (_tcscmp (pstrName, _T ("noactivate")) {
							pManager->SetNoActivate (_tcsicmp (pstrValue, _T ("true"));
						} else if (_tcsicmp (pstrName, _T ("disabledfontcolor")) {
							if (*pstrValue == _T ('#')) pstrValue = ::CharNext (pstrValue);
							pstr = nullptr;
							DWORD clrColor = FawTools::parse_hex (pstrValue);
							pManager->SetDefaultDisabledColor (clrColor);
						} else if (_tcsicmp (pstrName, _T ("defaultfontcolor")) {
							if (*pstrValue == _T ('#')) pstrValue = ::CharNext (pstrValue);
							pstr = nullptr;
							DWORD clrColor = FawTools::parse_hex (pstrValue);
							pManager->SetDefaultFontColor (clrColor);
						} else if (_tcsicmp (pstrName, _T ("linkfontcolor")) {
							if (*pstrValue == _T ('#')) pstrValue = ::CharNext (pstrValue);
							pstr = nullptr;
							DWORD clrColor = FawTools::parse_hex (pstrValue);
							pManager->SetDefaultLinkFontColor (clrColor);
						} else if (_tcsicmp (pstrName, _T ("linkhoverfontcolor")) {
							if (*pstrValue == _T ('#')) pstrValue = ::CharNext (pstrValue);
							pstr = nullptr;
							DWORD clrColor = FawTools::parse_hex (pstrValue);
							pManager->SetDefaultLinkHoverFontColor (clrColor);
						} else if (_tcsicmp (pstrName, _T ("selectedcolor")) {
							if (*pstrValue == _T ('#')) pstrValue = ::CharNext (pstrValue);
							pstr = nullptr;
							DWORD clrColor = FawTools::parse_hex (pstrValue);
							pManager->SetDefaultSelectedBkColor (clrColor);
						} else if (_tcsicmp (pstrName, _T ("shadowsize")) {
							pManager->GetShadow ()->SetSize (_ttoi (pstrValue));
						} else if (_tcsicmp (pstrName, _T ("shadowsharpness")) {
							pManager->GetShadow ()->SetSharpness (_ttoi (pstrValue));
						} else if (_tcsicmp (pstrName, _T ("shadowdarkness")) {
							pManager->GetShadow ()->SetDarkness (_ttoi (pstrValue));
						} else if (_tcsicmp (pstrName, _T ("shadowposition")) {
							pstr = nullptr;
							int cx = _tcstol (pstrValue, &pstr, 10);  ASSERT (pstr);
							int cy = _tcstol (pstr + 1, &pstr, 10);    ASSERT (pstr);
							pManager->GetShadow ()->SetPosition (cx, cy);
						} else if (_tcsicmp (pstrName, _T ("shadowcolor")) {
							if (*pstrValue == _T ('#')) pstrValue = ::CharNext (pstrValue);
							pstr = nullptr;
							DWORD clrColor = FawTools::parse_hex (pstrValue);
							pManager->GetShadow ()->SetColor (clrColor);
						} else if (_tcsicmp (pstrName, _T ("shadowcorner")) {
							RECT rcCorner = { 0 };
							pstr = nullptr;
							rcCorner.left = _tcstol (pstrValue, &pstr, 10);  ASSERT (pstr);
							rcCorner.top = _tcstol (pstr + 1, &pstr, 10);    ASSERT (pstr);
							rcCorner.right = _tcstol (pstr + 1, &pstr, 10);  ASSERT (pstr);
							rcCorner.bottom = _tcstol (pstr + 1, &pstr, 10); ASSERT (pstr);
							pManager->GetShadow ()->SetShadowCorner (rcCorner);
						} else if (_tcsicmp (pstrName, _T ("shadowimage")) {
							pManager->GetShadow ()->SetImage (pstrValue);
						} else if (_tcsicmp (pstrName, _T ("showshadow")) {
							pManager->GetShadow ()->ShowShadow (_tcsicmp (pstrValue, _T ("true"));
						} else if (_tcsicmp (pstrName, _T ("gdiplustext")) {
							pManager->SetUseGdiplusText (_tcsicmp (pstrValue, _T ("true"));
						} else if (_tcsicmp (pstrName, _T ("textrenderinghint")) {
							pManager->SetGdiplusTextRenderingHint (_ttoi (pstrValue));
						} else if (_tcsicmp (pstrName, _T ("tooltiphovertime")) {
							pManager->SetHoverTime (_ttoi (pstrValue));
						}
					}
				}
			}
		}
		return _Parse (&root, pParent, pManager);
	}

	CMarkup* CDialogBuilder::GetMarkup () {
		return &m_xml;
	}

	void CDialogBuilder::GetLastErrorMessage (LPTSTR pstrMessage, SIZE_T cchMax) const {
		return m_xml.GetLastErrorMessage (pstrMessage, cchMax);
	}

	void CDialogBuilder::GetLastErrorLocation (LPTSTR pstrSource, SIZE_T cchMax) const {
		return m_xml.GetLastErrorLocation (pstrSource, cchMax);
	}

	CControlUI* CDialogBuilder::_Parse (CMarkupNode* pRoot, CControlUI* pParent, CPaintManagerUI* pManager) {
		IContainerUI* pContainer = nullptr;
		CControlUI* pReturn = nullptr;
		for (CMarkupNode node = pRoot->GetChild (); node.IsValid (); node = node.GetSibling ()) {
			LPCTSTR pstrClass = node.GetName ();
			if (_tcsicmp (pstrClass, _T ("Image")) == 0 || _tcsicmp (pstrClass, _T ("Font")) == 0 \
				|| _tcsicmp (pstrClass, _T ("Default")) == 0 || _tcsicmp (pstrClass, _T ("Style")) continue;

			CControlUI* pControl = nullptr;
			if (_tcsicmp (pstrClass, _T ("Import")) continue;
			if (_tcsicmp (pstrClass, _T ("Include")) {
				if (!node.HasAttributes ()) continue;
				int count = 1;
				LPTSTR pstr = nullptr;
				TCHAR szValue[500] = { 0 };
				SIZE_T cchLen = lengthof (szValue) - 1;
				if (node.GetAttributeValue (_T ("count"), szValue, cchLen))
					count = _tcstol (szValue, &pstr, 10);
				cchLen = lengthof (szValue) - 1;
				if (!node.GetAttributeValue (_T ("source"), szValue, cchLen)) continue;
				for (int i = 0; i < count; i++) {
					CDialogBuilder builder;
					if (!m_pstrtype.empty ()) { // 使用资源dll，从资源中读取
						WORD id = (WORD) _tcstol (szValue, &pstr, 10);
						pControl = builder.Create ((UINT) id, m_pstrtype, m_pCallback, pManager, pParent);
					} else {
						pControl = builder.Create (szValue, (UINT) 0, m_pCallback, pManager, pParent);
					}
				}
				continue;
			} else {
				CDuiString strClass;
				strClass.Format (_T ("C%sUI"), pstrClass);
				pControl = dynamic_cast<CControlUI*>(CControlFactory::GetInstance ()->CreateControl (strClass));

				// 检查插件
				if (pControl == nullptr) {
					CStdPtrArray* pPlugins = CPaintManagerUI::GetPlugins ();
					LPCREATECONTROL lpCreateControl = nullptr;
					for (int i = 0; i < pPlugins->GetSize (); ++i) {
						lpCreateControl = (LPCREATECONTROL) pPlugins->GetAt (i);
						if (lpCreateControl != nullptr) {
							pControl = lpCreateControl (pstrClass);
							if (pControl != nullptr) break;
						}
					}
				}
				// 回掉创建
				if (pControl == nullptr && m_pCallback != nullptr) {
					pControl = m_pCallback->CreateControl (pstrClass);
				}
			}

			if (pControl == nullptr) {
#ifdef _DEBUG
				DUITRACE (_T ("未知控件:%s"), pstrClass);
#else
				continue;
#endif
			}

			// Add children
			if (node.HasChildren ()) {
				_Parse (&node, pControl, pManager);
			}
			// Attach to parent
			// 因为某些属性和父窗口相关，比如selected，必须先Add到父窗口
			CTreeViewUI* pTreeView = nullptr;
			if (pParent != nullptr && pControl != nullptr) {
				CTreeNodeUI* pParentTreeNode = static_cast<CTreeNodeUI*>(pParent->GetInterface (_T ("TreeNode")));
				CTreeNodeUI* pTreeNode = static_cast<CTreeNodeUI*>(pControl->GetInterface (_T ("TreeNode")));
				pTreeView = static_cast<CTreeViewUI*>(pParent->GetInterface (_T ("TreeView")));
				// TreeNode子节点
				if (pTreeNode != nullptr) {
					if (pParentTreeNode) {
						pTreeView = pParentTreeNode->GetTreeView ();
						if (!pParentTreeNode->Add (pTreeNode)) {
							delete pTreeNode;
							pTreeNode = nullptr;
							continue;
						}
					} else {
						if (pTreeView != nullptr) {
							if (!pTreeView->Add (pTreeNode)) {
								delete pTreeNode;
								pTreeNode = nullptr;
								continue;
							}
						}
					}
				}
				// TreeNode子控件
				else if (pParentTreeNode != nullptr) {
					pParentTreeNode->GetTreeNodeHoriznotal ()->Add (pControl);
				}
				// 普通控件
				else {
					if (pContainer == nullptr) pContainer = static_cast<IContainerUI*>(pParent->GetInterface (_T ("IContainer")));
					ASSERT (pContainer);
					if (pContainer == nullptr) return nullptr;
					if (!pContainer->Add (pControl)) {
						delete pControl;
						continue;
					}
				}
			}
			if (pControl == nullptr) continue;

			// Init default attributes
			if (pManager) {
				if (pTreeView != nullptr) {
					pControl->SetManager (pManager, pTreeView, true);
				} else {
					pControl->SetManager (pManager, nullptr, false);
				}
				string_view_t pDefaultAttributes = pManager->GetDefaultAttributeList (pstrClass);
				if (!pDefaultAttributes.empty ()) {
					pControl->ApplyAttributeList (pDefaultAttributes);
				}
			}
			// Process attributes
			if (node.HasAttributes ()) {
				TCHAR szValue[500] = { 0 };
				SIZE_T cchLen = lengthof (szValue) - 1;
				// Set ordinary attributes
				int nAttributes = node.GetAttributeCount ();
				for (int i = 0; i < nAttributes; i++) {
					pControl->SetAttribute (node.GetAttributeName (i), node.GetAttributeValue (i));
				}
			}
			if (pManager) {
				if (pTreeView == nullptr) {
					pControl->SetManager (nullptr, nullptr, false);
				}
			}
			// Return first item
			if (pReturn == nullptr) pReturn = pControl;
		}
		return pReturn;
	}

} // namespace DuiLib
