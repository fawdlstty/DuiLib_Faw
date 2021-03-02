#ifndef FAWLIB__FILE_HPP__
#define FAWLIB__FILE_HPP__



#include <iostream>
#include <fstream>
#include <string>

#include <Windows.h>
#include <Shlwapi.h>
#pragma comment (lib, "shlwapi.lib")

#include "String.hpp"
#include "Encoding.hpp"



namespace faw {
	class File {
		File () {}
	public:
		static bool exist (String _path) {
			//DWORD _attr = ::GetFileAttributes (_path.c_str ());
			//return (_attr != INVALID_FILE_ATTRIBUTES && !(_attr & FILE_ATTRIBUTE_DIRECTORY));
			String _path1 = _path.replace (_T ('/'), _T ('\\'));
			return ::PathFileExists (_path.c_str ());
		}
		static bool remove (String _path) {
			if (!exist (_path))
				return true;
			::SetFileAttributes (_path.c_str (), FILE_ATTRIBUTE_NORMAL);
			return !!::DeleteFile (_path.c_str ());
		}
		static bool copy (String _src, String _dest) { return !!::CopyFile (_src.c_str (), _dest.c_str (), FALSE); }
		static bool move (String _src, String _dest) { return !!::MoveFile (_src.c_str (), _dest.c_str ()); }
		static bool write (String _path, String _data, String _encoding = _T ("utf-8"), std::ios::openmode _openmode = std::ios::binary) {
			MakeSureDirectoryPathExists (_path.stra ().c_str ());
			_encoding.lower_self ();
			if (_encoding == _T ("utf-8") || _encoding == _T ("utf8")) {
				std::ofstream ofs (_path.stra (), _openmode);
				if (!ofs.is_open ())
					return false;
				ofs << Encoding::T_to_utf8 (_data.str ());
				ofs.close ();
			} else if (_encoding == _T ("ucs-2") || _encoding == _T ("ucs2") || _encoding == _T ("utf-16") || _encoding == _T ("utf16")) {
				std::wofstream ofs (_path.strw (), _openmode);
				if (!ofs.is_open ())
					return false;
				ofs << Encoding::T_to_utf16 (_data.str ());
				ofs.close ();
			} else if (_encoding == _T ("gb18030") || _encoding == _T ("gbk") || _encoding == _T ("gb2312")) {
				std::ofstream ofs (_path.stra (), _openmode);
				if (!ofs.is_open ())
					return false;
				ofs << Encoding::T_to_gb18030 (_data.str ());
				ofs.close ();
			}
			return true;
		}
		static String read (String _path, String _encoding = _T ("utf-8")) {
			String _ret = _T ("");
			if (_encoding == _T ("utf-8") || _encoding == _T ("utf8")) {
				std::ifstream ifs (_path.stra (), std::ios::binary);
				if (!ifs.is_open ())
					return _ret;
				std::string _data ((std::istreambuf_iterator<char> (ifs)), std::istreambuf_iterator<char> ());
				_ret = Encoding::utf8_to_T (_data);
				ifs.close ();
			} else if (_encoding == _T ("ucs-2") || _encoding == _T ("ucs2") || _encoding == _T ("utf-16") || _encoding == _T ("utf16")) {
				std::wifstream ifs (_path.strw (), std::ios::binary);
				if (!ifs.is_open ())
					return _ret;
				std::wstring _data ((std::istreambuf_iterator<wchar_t> (ifs)), std::istreambuf_iterator<wchar_t> ());
				_ret = _data;
				ifs.close ();
			} else if (_encoding == _T ("gb18030") || _encoding == _T ("gbk") || _encoding == _T ("gb2312")) {
				std::ifstream ifs (_path.stra (), std::ios::binary);
				if (!ifs.is_open ())
					return _ret;
				std::string _data ((std::istreambuf_iterator<char> (ifs)), std::istreambuf_iterator<char> ());
				_ret = _data;
				ifs.close ();
			}
			return _ret;
		}
		static std::string read_binary (String _path) {
			std::ifstream ifs (_path.stra (), std::ios::binary);
			if (!ifs.is_open ())
				return "";
			return std::string ((std::istreambuf_iterator<char> (ifs)), std::istreambuf_iterator<char> ());
		}
		static void append_binary (std::string _path, const void *_data, size_t _len) {
			std::ofstream ofs (_path, std::ios::binary | std::ios::app);
			ofs.write ((const char *) _data, _len);
			ofs.close ();
		}
		static int64_t get_size (String _path) {
			std::ifstream ifs (_path.stra (), std::ios::binary | std::ios::in);
			if (!ifs.is_open ())
				return -1;
			ifs.seekg (0, ifs.end);
			return ifs.tellg ();
		}
	};
}



#endif //FAWLIB__FILE_HPP__
