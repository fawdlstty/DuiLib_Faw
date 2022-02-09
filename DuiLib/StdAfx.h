#ifndef __DUILIB_FAW__STDAFX_H__
#define __DUILIB_FAW__STDAFX_H__



#pragma warning (disable: 4251)

#define _CRT_SECURE_NO_WARNINGS

#ifdef __GNUC__
// 怎么都没找到min，max的头文件-_-
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#endif

#ifndef __FILET__
#include <tchar.h>
#define __FILET__	_T (__FILE__)
#define __FUNCTIONT__	_T (__FUNCTION__)
#endif

#include "UIlib.h"
#include <olectl.h>

#define lengthof(x) (sizeof(x)/sizeof(*x))
#define MAX max
#define MIN min
#define CLAMP(x,a,b) (MIN(b,MAX(a,x)))

#include <optional>

#include "Utils/FawTools.hpp"
#include <format>



#endif //__DUILIB_FAW__STDAFX_H__
