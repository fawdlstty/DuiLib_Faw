#ifndef __FAW_TOOLS_HPP__
#define __FAW_TOOLS_HPP__

class FawTools {
public:
	// "1,2,3,4" -> RECT
	static RECT parse_rect_from_str (string_view_t str) {
		std::vector<size_t> v = split_number (str, 4);
		return { v[0], v[1], v[2], v[3] };
	}

	// "1,2" -> SIZE
	static SIZE parse_size_from_str (string_view_t str) {
		std::vector<size_t> v = split_number (str, 2);
		return { v[0], v[1] };
	}

	// #FFFF0000 or FF0000 -> size_t
	static size_t parse_hex (string_view_t str) {
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
		return n;
	}

	// "a"="a" "b"=b c="c" d=d -> std::map<string_t, string_t>
	static std::map<string_t, string_t> parse_keyvalue_pairs (string_view_t str) {
		std::map<string_t, string_t> m;
		size_t begin = 0;
		string_view_t str_key = _T (""), str_value = _T ("");
		while (begin < str.size ()) {
			// parse str_key
			TCHAR ch = str[begin];
			TCHAR sp = (ch == _T ('\"') || ch == _T ('\'')) ? ch : _T ('=');
			if (ch == sp)
				++begin;
			size_t p = str.find (sp, begin);
			if (p == string_t::npos)
				break;
			str_key = str.substr (begin, p - begin);
			if (p == string_t::npos)
				break;
			// parse str_value
			p = str.find (_T ('='), begin);
			if (p == string_t::npos)
				break;
			begin = p + 1;
			if (begin >= str.size ())
				break;
			ch = str[begin];
			sp = (ch == _T ('\"') || ch == _T ('\'')) ? ch : _T (' ');
			if (ch == sp) {
				if (begin + 2 >= str.size ())
					break;
				++begin;
			}
			p = str.find (sp, begin);
			str_value = str.substr (begin, p - begin);
			// reset
			m[string_t (str_key)] = string_t (str_value);
			if (p != string_t::npos) {
				begin = p + 1;
				if (begin >= str.size ())
					break;
				if (ch == sp && str[begin] == _T (' '))
					++begin;
			}
			str_key = str_value = _T ("");
		}
		if (!str_key.empty ())
			m[string_t (str_key)] = string_t (str_value);
		return m;
	}

	// "true" or "True" or "TRUE" -> true
	static bool parse_bool (string_view_t str) {
		if (_tcsicmp (_T ("true"), str.data ()) == 0 || _tcsicmp (_T ("on"), str.data ()) == 0 || _tcsicmp (_T ("yes"), str.data ()) == 0 || _tcsicmp (_T ("ok"), str.data ()) == 0 || _tcsicmp (_T ("1"), str.data ()) == 0) {
			return true;
		} else if (_tcsicmp (_T ("false"), str.data ()) == 0 || _tcsicmp (_T ("off"), str.data ()) == 0 || _tcsicmp (_T ("no"), str.data ()) == 0 || _tcsicmp (_T ("cancel"), str.data ()) == 0 || _tcsicmp (_T ("0"), str.data ()) == 0) {
			return false;
		} else {
			return str.length () > 0;
		}
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
	static std::vector<size_t> split_number (string_view_t str, size_t expect = string_t::npos) {
		std::vector<size_t> v;
		size_t n = 0;
		bool has_num = false;
		for (size_t i = 0; i < str.length (); ++i) {
			TCHAR ch = str[i];
			if (ch >= _T ('0') && ch <= _T ('9')) {
				n = n * 10 + ch - _T ('0');
				has_num = true;
			} else {
				v.push_back (n);
				n = 0;
				has_num = false;
			}
		}
		if (has_num)
			v.push_back (n);
		while (expect != string_t::npos && v.size () < expect)
			v.push_back (0);
		return v;
	}

	static std::string _conv_to_multi (std::wstring_view _old, UINT ToType) {
		int lenOld = lstrlenW (_old.data ());
		int lenNew = ::WideCharToMultiByte (ToType, 0, _old.data (), lenOld, nullptr, 0, nullptr, nullptr);
		std::string s;
		s.resize (lenNew);
		if (!::WideCharToMultiByte (ToType, 0, _old.data (), lenOld, const_cast<char*>(s.c_str ()), lenNew, nullptr, nullptr))
			return "";
		return s.c_str ();
	}
	static std::wstring _conv_to_wide (std::string_view _old, UINT ToType) {
		int lenOld = lstrlenA (_old.data ());
		int lenNew = ::MultiByteToWideChar (ToType, 0, _old.data (), lenOld, nullptr, 0);
		std::wstring s;
		s.resize (lenNew);
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
