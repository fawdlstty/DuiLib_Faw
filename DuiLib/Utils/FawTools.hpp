#ifndef __FAW_TOOLS_HPP__
#define __FAW_TOOLS_HPP__

class FawTools {
public:
	// "1,2,3,4" -> RECT
	template <typename T>
	static RECT parse_rect (T &str) { auto v = split_number (str, 4); return { (LONG) v[0], (LONG) v[1], (LONG) v[2], (LONG) v[3] }; }

	// "1,2" -> SIZE
	template <typename T>
	static SIZE parse_size (T &str) { auto v = split_number (str, 2); return { (LONG) v[0], (LONG) v[1] }; }

	// "#FFFF0000" or "FF0000" -> size_t
	template <typename T>
	static size_t parse_hex (T &str) {
		size_t n = 0, i = (str.length () > 0 && str[0] == _T ('#')) ? 1 : 0;
		for (; i < str.length (); ++i) {
			TCHAR ch = str[i];
			if (ch >= _T ('0') && ch <= _T ('9')) {
				n = n * 16 + ch - _T ('0');
			} else if (ch >= _T ('A') && ch <= _T ('Z')) {
				n = n * 16 + ch - _T ('A') + 10;
			} else if (ch >= _T ('a') && ch <= _T ('z')) {
				n = n * 16 + ch - _T ('a') + 10;
			} else {
				break;
			}
		}
		if constexpr (!std::is_const<T>::value)
			str = str.substr (i);
		return n;
	}

	// "12345" -> size_t
	template <typename T>
	static int parse_dec (T &str) {
		int n = 0;
		bool is_sign = (str.size () > 0 && str[0] == _T ('-'));
		size_t i = is_sign ? 1 : 0;
		for (; i < str.length (); ++i) {
			TCHAR ch = str[i];
			if (ch >= _T ('0') && ch <= _T ('9')) {
				n = n * 10 + ch - _T ('0');
			} else {
				break;
			}
		}
		if constexpr (!std::is_const<T>::value)
			str = str.substr (i);
		return is_sign ? 0 - n : n;
	}

	// "true" or "True" or "TRUE" -> true
	static bool parse_bool (string_view_t str) {
		static std::map<string_view_t, bool> mbool { { _T ("true"), true }, { _T ("false"), false }, { _T ("on"), true }, { _T ("off"), false }, { _T ("yes"), true }, { _T ("no"), false }, { _T ("ok"), true }, { _T ("cancel"), false }, { _T ("1"), true }, { _T ("0"), false } };
		if (mbool.find (str) != mbool.end ())
			return mbool[str];
		return !str.empty ();
	}

	// "a"="a" "b"=b c="c" d=d -> std::map<string_t, string_t>
	static std::map<string_t, string_t> parse_keyvalue_pairs (string_view_t str) {
		std::map<string_t, string_t> m;
		size_t begin = 0;
		string_view_t str_key = _T (""), str_value = _T ("");
		while (begin < str.size ()) {
			// parse str_key
			TCHAR ch = str[begin];
			TCHAR sp = (ch == _T ('\"') || ch == _T ('\'') ? ch : _T ('='));
			if (ch == sp) ++begin;
			size_t p = str.find (sp, begin);
			if (p == string_t::npos) break;
			str_key = str.substr (begin, p - begin);
			if (p == string_t::npos) break;
			// parse str_value
			p = str.find (_T ('='), begin);
			if (p == string_t::npos) break;
			begin = p + 1;
			if (begin >= str.size ()) break;
			ch = str[begin];
			sp = (ch == _T ('\"') || ch == _T ('\'')) ? ch : _T (' ');
			if (ch == sp) {
				if (begin + 2 >= str.size ()) break;
				++begin;
			}
			p = str.find (sp, begin);
			if (p == string_t::npos) break;
			str_value = str.substr (begin, p - begin);
			// reset
			m[string_t (str_key)] = string_t (str_value);
			if (p != string_t::npos) {
				begin = p + 1;
				if (begin >= str.size ()) break;
				if (ch == sp && str[begin] == _T (' ')) ++begin;
			}
			str_key = str_value = _T ("");
		}
		if (!str_key.empty ())
			m[string_t (str_key)] = string_t (str_value);
		return m;
	}



	//
	// ×Ö·û´®´¦Àí
	//

	static std::wstring gb18030_to_utf16 (std::string_view _old) { return _conv_to_wide (_old, CP_ACP); }
	static std::string utf16_to_gb18030 (std::wstring_view _old) { return _conv_to_multi (_old, CP_ACP); }
	static std::wstring utf8_to_utf16 (std::string_view _old) { return _conv_to_wide (_old, CP_UTF8); }
	static std::string utf16_to_utf8 (std::wstring_view _old) { return _conv_to_multi (_old, CP_UTF8); }
	static std::string gb18030_to_utf8 (std::string_view _old) { return utf16_to_utf8 (gb18030_to_utf16 (_old)); }
	static std::string utf8_to_gb18030 (std::string_view _old) { return utf16_to_gb18030 (utf8_to_utf16 (_old)); }
#ifdef UNICODE
	static std::string get_gb18030 (string_view_t _old) { return utf16_to_gb18030 (_old); }
	static std::wstring get_utf16 (string_view_t _old) { return std::wstring (_old); }
	static string_t get_T (std::string_view _old) { return gb18030_to_utf16 (_old); }
	static string_t get_T (std::wstring_view _old) { return std::wstring (_old); }
#else
	static std::string get_gb18030 (string_view_t _old) { return std::string (_old); }
	static std::wstring get_utf16 (string_view_t _old) { return gb18030_to_utf16 (_old); }
	static string_t get_T (std::string_view _old) { return std::string (_old); }
	static string_t get_T (std::wstring_view _old) { return utf16_to_gb18030 (_old); }
#endif

private:
	// "1,2,3,4" -> std::vector<size_t>
	template <typename T>
	static std::vector<size_t> split_number (T &str, size_t expect = string_t::npos) {
		std::vector<size_t> v;
		size_t n = 0, i = 0;
		bool has_num = false;
		for (; i < str.length (); ++i) {
			TCHAR ch = str[i];
			if (ch >= _T ('0') && ch <= _T ('9')) {
				n = n * 10 + ch - _T ('0');
				has_num = true;
			} else {
				v.push_back (n);
				n = 0;
				has_num = false;
				if (expect != string_t::npos && v.size () >= expect)
					break;
			}
		}
		if (has_num)
			v.push_back (n);
		while (expect != string_t::npos && v.size () < expect)
			v.push_back (0);
		if constexpr (!std::is_const<T>::value)
			str = str.substr (i);
		return v;
	}

	static std::string _conv_to_multi (std::wstring_view _old, UINT ToType) {
		int lenOld = lstrlenW (_old.data ());
		int lenNew = ::WideCharToMultiByte (ToType, 0, _old.data (), lenOld, nullptr, 0, nullptr, nullptr);
		std::string s ((size_t) (lenNew + 1), '\0');
		if (!::WideCharToMultiByte (ToType, 0, _old.data (), lenOld, const_cast<char*>(s.c_str ()), lenNew, nullptr, nullptr))
			return "";
		return s.c_str ();
	}
	static std::wstring _conv_to_wide (std::string_view _old, UINT ToType) {
		int lenOld = lstrlenA (_old.data ());
		int lenNew = ::MultiByteToWideChar (ToType, 0, _old.data (), lenOld, nullptr, 0);
		std::wstring s ((size_t) (lenNew + 1), L'\0');
		if (!::MultiByteToWideChar (ToType, 0, _old.data (), lenOld, const_cast<wchar_t*>(s.c_str ()), lenNew))
			return L"";
		return s.c_str ();
	}

	//static std::vector<string_t> split (string_view_t str, TCHAR sp) {
	//	std::vector<string_t> v;
	//	return v;
	//}
};

#endif //__FAW_TOOLS_HPP__
