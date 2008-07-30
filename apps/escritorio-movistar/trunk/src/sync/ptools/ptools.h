#ifndef PTOOLS_H_
#define PTOOLS_H_

#include <string.h>
#include <string>
#ifdef _WIN32
//#include <afx.h>
#include <windows.h>
#include <tchar.h>
#endif

#ifdef UNICODE
# define tstring wstring
# define tstringstream wstringstream
# define tregex wregex
# define toTString toWideString
#ifndef tchar
#define tchar wchar_t
#endif
#else
# define tstring string
# define tstringstream stringstream
# define tregex regex
# define toTString toNarrowString
#ifndef tchar
#define tchar char
#endif
#endif

#ifndef _WIN32
#define BYTE uint8_t
#define WORD uint16_t
#define DWORD uint32_t
#define UINT32 uint32_t
#define UINT64 uint64_t
#endif

#ifndef _T
#define _T(X) (X)
#endif
#ifndef ASSERT
#include <assert.h>
#define ASSERT(X) assert(X)
#endif
#ifndef TRACE
#define TRACE(X) 
#endif

#ifdef _WIN32
std::wstring toWideString(const CHAR* pStr, int len, UINT codePage = CP_ACP);
std::wstring inline toWideString(const WCHAR* pStr, int len, UINT codePage = CP_ACP)
	{return std::wstring(pStr, len);}
std::string  toNarrowString(const WCHAR* pStr, int len, UINT codePage = CP_ACP);
std::string inline toNarrowString(const CHAR* pStr, int len, UINT codePage = CP_ACP)
	{return std::string(pStr, len);}
std::wstring inline toWideString(const std::string& str, UINT codePage = CP_ACP)
	{return toWideString(str.c_str(), (int)str.size(), codePage);}
std::wstring inline toWideString(const std::wstring& str, UINT codePage = CP_ACP)
	{return str;}
std::string  inline toNarrowString(const std::wstring& str, UINT codePage = CP_ACP)
	{return toNarrowString(str.c_str(), (int)str.size(), codePage);}
std::string inline toNarrowString(const std::string& str, UINT codePage = CP_ACP)
	{return str;}
std::string  inline WideToUTF8(const WCHAR* pStr, int len=-1)
	{return toNarrowString(pStr, len, CP_UTF8);}
std::wstring  inline UTF8ToWide(const CHAR* pStr, int len=-1)
	{return toWideString(pStr, len, CP_UTF8);}
std::string  inline WideToUTF8(const std::wstring& str)
	{return toNarrowString(str.c_str(), (int)str.length(), CP_UTF8);}
std::wstring  inline UTF8ToWide(const std::string& str)
	{return toWideString(str.c_str(), (int)str.length(), CP_UTF8);}
std::string  inline NarrowToUTF8(const CHAR* pStr, int len=-1)
	{return toNarrowString(pStr, len, CP_UTF8);}
std::string  inline NarrowToUTF8(const std::string& str)
	{return NarrowToUTF8(str.c_str(), (int)str.length());}
std::string  inline UTF8ToNarrow(const CHAR* pStr, int len=-1)
	{return toNarrowString(pStr, len, CP_UTF8);}
std::string  inline UTF8ToNarrow(const std::string& str)
	{return UTF8ToNarrow(str.c_str(), (int)str.length());}
#else
#ifndef CHAR
#define CHAR char
#endif
#ifndef WCHAR
#define WCHAR wchar_t
#endif
std::wstring toWideString(const CHAR* pStr, int len);
std::wstring inline toWideString(const WCHAR* pStr, int len)
	{return std::wstring(pStr, len);}
std::string toNarrowString(const WCHAR* pStr, int len);
std::string inline toNarrowString(const CHAR* pStr, int len)
	{return std::string(pStr, len);}
std::wstring inline toWideString(const std::string& str)
	{return toWideString(str.c_str(), str.size());}
std::wstring inline toWideString(const std::wstring& str)
	{return str;}
std::string  inline toNarrowString(const std::wstring& str)
	{return toNarrowString(str.c_str(), str.size());}
std::string inline toNarrowString(const std::string& str)
	{return str;}
std::string WideToUTF8(const WCHAR* pStr, int len=-1);
std::wstring UTF8ToWide(const CHAR* pStr, int len=-1);
std::string  inline WideToUTF8(const std::wstring& str)
	{return WideToUTF8(str.c_str(), str.length());}
std::wstring  inline UTF8ToWide(const std::string& str)
	{return UTF8ToWide(str.c_str(), str.length());}
std::string NarrowToUTF8(const CHAR* pStr, int len=-1);
std::string UTF8ToNarrow(const CHAR* pStr, int len=-1);
std::string  inline NarrowToUTF8(const std::string& str)
	{return NarrowToUTF8(str.c_str(), str.length());}
std::string  inline UTF8ToNarrow(const std::string& str)
	{return UTF8ToNarrow(str.c_str(), str.length());}

using std::min;
using std::max;

void ptoolsInit();
void ptoolsEnd();
#endif

inline std::string toUTF8(const std::wstring &ws) { return WideToUTF8(ws); }
inline std::string toUTF8(const std::string &s) { return NarrowToUTF8(s); }

#endif /*PTOOLS_H_*/
