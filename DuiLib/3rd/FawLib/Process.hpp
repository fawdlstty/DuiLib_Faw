#ifndef FAWLIB__PROCESS_HPP__
#define FAWLIB__PROCESS_HPP__



#include <thread>
#include <functional>

#include <Windows.h>
#include <TlHelp32.h>
#include <shellapi.h>
#include <Psapi.h>

#include "String.hpp"

#pragma comment (lib, "psapi.lib")



namespace faw {
	class Process {
		Process () {}
	public:
		static bool create (String _cmd_line, std::function<void ()> _on_exit = nullptr) {
			STARTUPINFO si = { sizeof (STARTUPINFO) };
			PROCESS_INFORMATION pi = { 0 };
			bool bRet = !!::CreateProcess (nullptr, &_cmd_line [0], nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
			if (!bRet)
				return false;
			CloseHandle (pi.hThread);
			if (_on_exit) {
				HANDLE _handle = pi.hProcess;
				std::thread ([_on_exit, _handle] () {
					try {
						DWORD _exit_code = STILL_ACTIVE;
						while (_exit_code == STILL_ACTIVE) {
							std::this_thread::sleep_for (std::chrono::milliseconds (100));
							if (!::GetExitCodeProcess (_handle, &_exit_code))
								break;
						}
						CloseHandle (_handle);
						_on_exit ();
					} catch (...) {
					}
				}).detach ();
			} else {
				CloseHandle (pi.hProcess);
			}
			return true;
		}

		static void shell (String _url) {
			::ShellExecute (NULL, _T ("open"), _url.c_str (), nullptr, nullptr, SW_SHOW);
		}

		// 判断进程是否存在
		static bool process_exist (String file) {
			HANDLE hSnapShot = ::CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
			PROCESSENTRY32 pe32 { sizeof (PROCESSENTRY32) };
			std::map<DWORD, String> m;
			if (::Process32First (hSnapShot, &pe32)) {
				do {
					String _path = pe32.szExeFile;
					_path.replace_self (_T ('/'), _T ('\\'));
					size_t p = _path.rfind (_T ('\\'));
					if (_path.substr (p + 1) == file) {
						::CloseHandle (hSnapShot);
						return true;
					}
				} while (::Process32Next (hSnapShot, &pe32));
			}
			::CloseHandle (hSnapShot);
			return false;
		}

		static void open_url (String _url, HWND _hWnd = NULL) {
			// 使用 ShellExecute 打开 URL 容易有崩溃问题，崩溃点在 IEFrame 里
			if (_url == _T (""))
				return;
			String _shell = faw::Register::get_path_value<std::wstring> (L"HKEY_CLASSES_ROOT\\http\\shell\\open\\command");
			//String _shell = _T ("\"C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe\" -- \"%1\"");
			if (_shell != _T ("")) {
				// 创建进程
				_shell.replace_self (_T ("%1"), _url.str_view ());
				if (_shell.find (_url.str_view ())) {
					if (create (_shell))
						return;
				}
			}
			::MessageBox (_hWnd, _T ("请到蜜桃开播界面下载最新版本"), _T ("提示"), MB_ICONINFORMATION | MB_TOPMOST);
			//// 复制到剪贴板
			//String _info = String::format (_T ("无法打开您电脑的默认浏览器，是否将此网址复制到剪贴板？\r\n%s"), _url.c_str ());
			//if (IDYES == ::MessageBox (_hWnd, _info.c_str (), _T ("提示"), MB_ICONQUESTION | MB_YESNO | MB_TOPMOST)) {
			//	bool _ret = false;
			//	if (::OpenClipboard (_hWnd)) {
			//		::EmptyClipboard ();
			//		std::string _aurl = _url.stra ();
			//		size_t _size = _aurl.length () + 1;
			//		if (HANDLE _hData = ::GlobalAlloc (GMEM_MOVEABLE, _size)) {
			//			if (LPTSTR _pData = (LPTSTR) ::GlobalLock (_hData)) {
			//				memcpy (_pData, _aurl.c_str (), _size);
			//				_ret = true;
			//				::GlobalUnlock (_hData);
			//			}
			//			if (_ret)
			//				_ret = !!::SetClipboardData (CF_TEXT, _hData);
			//			if (!_ret)
			//				::GlobalFree (_hData);
			//		}
			//	}
			//	::CloseClipboard ();
			//	if (_ret) {
			//		::MessageBox (_hWnd, _T ("复制网址成功，请手动打开浏览器粘贴访问"), _T ("提示"), MB_ICONINFORMATION | MB_TOPMOST);
			//	} else {
			//		::MessageBox (_hWnd, _T ("复制网址到剪贴板失败！"), _T ("提示"), MB_ICONHAND | MB_TOPMOST);
			//	}
			//}
		}
	};
}



#endif //FAWLIB__PROCESS_HPP__
