#ifndef __UIRESOURCEMANAGER_H__
#define __UIRESOURCEMANAGER_H__
#pragma once

namespace DuiLib {
	// 控件文字查询接口
	class UILIB_API IQueryControlText {
	public:
		virtual faw::string_t QueryControlText (faw::string_t lpstrId, faw::string_t lpstrType) = 0;
	};

	class UILIB_API CResourceManager {
	private:
		CResourceManager (void);
		virtual ~CResourceManager (void);

	public:
		static CResourceManager* GetInstance () {
			static CResourceManager * p = new CResourceManager;
			return p;
		};
		void Release (void) {
			delete this;
		}

	public:
		BOOL LoadResource (std::variant<UINT, faw::string_t> xml, faw::string_t type = _T (""));
		BOOL LoadResource (CMarkupNode Root);
		void ResetResourceMap ();
		faw::string_t GetImagePath (faw::string_t lpstrId);
		faw::string_t GetXmlPath (faw::string_t lpstrId);

	public:
		void SetLanguage (faw::string_t pstrLanguage) { m_sLauguage = pstrLanguage; }
		faw::string_t GetLanguage () { return m_sLauguage; }
		BOOL LoadLanguage (faw::string_t pstrXml);

	public:
		void SetTextQueryInterface (IQueryControlText* pInterface) {
			m_pQuerypInterface = pInterface;
		}
		faw::string_t GetText (faw::string_t lpstrId, faw::string_t lpstrType = _T (""));
		void ReloadText ();
		void ResetTextMap ();

	private:
		CStdStringPtrMap m_mTextResourceHashMap;
		IQueryControlText	*m_pQuerypInterface;
		CStdStringPtrMap m_mImageHashMap;
		CStdStringPtrMap m_mXmlHashMap;
		CMarkup m_xml;
		faw::string_t m_sLauguage;
		CStdStringPtrMap m_mTextHashMap;
	};

} // namespace DuiLib

#endif // __UIRESOURCEMANAGER_H__