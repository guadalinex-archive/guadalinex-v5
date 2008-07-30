
#include "stdafx.h"
#include <assert.h>
#include "ptools.h"

#ifdef _WIN32

#define ARRSIZE(v) (sizeof(v)/sizeof(v[0]))
///////////////////////////////////////////////////////////////////////////////
std::wstring toWideString(const char* pStr, int len, UINT codePage /*= CP_ACP*/)
{
	assert( pStr!=NULL );
    assert( len>=0 || len==-1 );
	WCHAR buffer[64];
	WCHAR *ptr;

    // figure out how many wide characters we are going to get 
	if (len==-1) len = (int)strlen(pStr);
	int nChars = MultiByteToWideChar(codePage, 0, pStr, len, NULL, 0);
    if (nChars == 0)
        return L"";

    // convert the narrow string to a wide string 
	if (ARRSIZE(buffer)>=(nChars+1))
		ptr = buffer;
	else
		ptr = new WCHAR[nChars+1];
	memset(ptr, 0, (nChars+1) * sizeof(WCHAR));

	MultiByteToWideChar(codePage, 0, pStr, len, ptr, nChars);
	std::wstring result(ptr);
	if (ptr!=buffer)
		delete ptr;

    return result;
}

///////////////////////////////////////////////////////////////////////////////
std::string toNarrowString(const WCHAR* pStr, int len, UINT codePage /*= CP_ACP*/)
{
    assert( pStr!=NULL );
    assert( len>=0 || len==-1 );
	char buffer[64];
	char *ptr;

    // figure out how many narrow characters we are going to get 
	if (len==-1) len = (int)wcslen(pStr);
	int nChars = WideCharToMultiByte(codePage, 0, pStr, len, NULL, 0, NULL, NULL);
    if (nChars == 0)
        return "";

    // convert the wide string to a narrow string
	if (ARRSIZE(buffer)>=(nChars+1))
		ptr = buffer;
	else
		ptr = new char[nChars+1];
	memset(ptr, 0, (nChars+1) * sizeof(char));

    WideCharToMultiByte(codePage, 0, pStr, len, ptr, nChars, NULL, NULL);
	std::string result(&ptr[0]);
	if (ptr!=buffer)
		delete ptr;

    return result;
}
#else

#include <iconv.h>
#include <langinfo.h>
#include <pthread.h>

static void get_locale_encoding(std::string &enc);

///////////////////////////////////////////////////////////////////////////////
std::wstring toWideString(const CHAR* pStr, int len)
{
	if (len==-1)
		len=strlen(pStr);
	std::wstring ws;
	std::string enc;
	get_locale_encoding(enc);
	iconv_t ic=iconv_open("WCHAR_T", enc.c_str());
	if (ic!=(iconv_t)-1)
	{
		wchar_t *wc=new wchar_t[len+1]; // len*4*4+1];
		char *c=(char *)wc;
		char *inc=(char *)pStr;
		size_t inlen, outlen;
		inlen=len;
		outlen=(len+1)*sizeof(wchar_t);
		if (iconv(ic, &inc, &inlen, &c, &outlen)==0)
		{
			if (outlen>=sizeof(wchar_t))
			{
				*(wchar_t *)c=L'\000';
				ws=wc;
			}
		}
		iconv_close(ic);
	}
	return ws;
//	const char *curloc=setlocale(LC_CTYPE, NULL);
//	setlocale(LC_CTYPE, "");
//	wchar_t *wc=new wchar_t[len*2*4+1];
//	int ret=mbsrtowcs(wc, &pStr, len*2*4+1, NULL);
////	wc[len]=L'\000';
//	std::wstring ws(wc);
//	delete [] wc;
//	setlocale(LC_CTYPE, curloc);
//	return ws;
}
///////////////////////////////////////////////////////////////////////////////
std::string  toNarrowString(const WCHAR* pStr, int len)
{
	if (len==-1)
		len=wcslen(pStr);
	std::string s;
	std::string enc;
	get_locale_encoding(enc);
	iconv_t ic=iconv_open(enc.c_str(), "WCHAR_T");
	if (ic!=(iconv_t)-1)
	{
		char *c=new char[len*4*4+1];
		char *inc=(char *)pStr;
		char *outc=c;
		size_t inlen, outlen;
		inlen=len*sizeof(wchar_t);
		outlen=len*4*4+1;
		if (iconv(ic, &inc, &inlen, &outc, &outlen)==0)
		{
			if (outlen>=sizeof(char))
			{
				*(char *)outc='\000';
				s=c;
			}
		}
		iconv_close(ic);
	}
	return s;
//	const char *curloc=setlocale(LC_CTYPE, NULL);
//	setlocale(LC_CTYPE, "");
//	char *c=new char[len*2*4+1];
//	wcsrtombs(c, &pStr, len*2*4+1, NULL);
////	c[len]='\000';
//	std::string s(c);
//	delete [] c;
//	setlocale(LC_CTYPE, curloc);
//	return s;	
}
///////////////////////////////////////////////////////////////////////////////
std::string WideToUTF8(const WCHAR* pStr, int len)
{
	if (len==-1)
		len=wcslen(pStr);
	std::string s;
	iconv_t ic=iconv_open("UTF-8", "WCHAR_T");
	if (ic!=(iconv_t)-1)
	{
		char *c=new char[len*4*4+1];
		char *inc=(char *)pStr;
		char *outc=c;
		size_t inlen, outlen;
		inlen=len*sizeof(wchar_t);
		outlen=len*4*4+1;
		if (iconv(ic, &inc, &inlen, &outc, &outlen)==0)
		{
			if (outlen>=sizeof(char))
			{
				*(char *)outc='\000';
				s=c;
			}
		}
		iconv_close(ic);
	}
	return s;
	
//	char *curloc=setlocale(LC_CTYPE, NULL);
//	setlocale(LC_CTYPE, ""); // "UTF-8");
//	std::string s=toNarrowString(pStr, len);
//	setlocale(LC_CTYPE, curloc);
//	return s;
}
///////////////////////////////////////////////////////////////////////////////
std::wstring UTF8ToWide(const CHAR* pStr, int len)
{
	if (len==-1)
		len=strlen(pStr);
	std::wstring ws;
	iconv_t ic=iconv_open("WCHAR_T", "UTF-8");
	if (ic!=(iconv_t)-1)
	{
		wchar_t *wc=new wchar_t[len+1]; // len*4*4+1];
		char *c=(char *)wc;
		char *inc=(char *)pStr;
		size_t inlen, outlen;
		inlen=len;
		outlen=(len+1)*sizeof(wchar_t);
		if (iconv(ic, &inc, &inlen, &c, &outlen)==0)
		{
			if (outlen>=sizeof(wchar_t))
			{
				*(wchar_t *)c=L'\000';
				ws=wc;
			}
		}
		iconv_close(ic);
	}
	return ws;
//	char *curloc=setlocale(LC_CTYPE, NULL);
//	setlocale(LC_CTYPE, ""); // "UTF-8");
//	std::wstring ws=toWideString(pStr, len);
//	setlocale(LC_CTYPE, curloc);
//	return ws;	
}
///////////////////////////////////////////////////////////////////////////////
std::string NarrowToUTF8(const CHAR* pStr, int len)
{
	std::string s(pStr, len);
	std::wstring ws(toWideString(s));
	s=WideToUTF8(ws);
	return s;	
}
///////////////////////////////////////////////////////////////////////////////
std::string UTF8ToNarrow(const CHAR* pStr, int len)
{
	std::wstring ws(UTF8ToWide(pStr, len));
	return toNarrowString(ws);
}

///////////////////////////////////////////////////////////////////////////////
static pthread_mutex_t ptools_locale_mt;
static std::string ptools_locale_encoding;
static bool ptools_locale_encoding_init=false;

///////////////////////////////////////////////////////////////////////////////
static void get_locale_encoding(std::string &enc)
{
	assert(ptools_locale_encoding_init);
	pthread_mutex_lock(&ptools_locale_mt);
	enc = ptools_locale_encoding;
	pthread_mutex_unlock(&ptools_locale_mt);
}

///////////////////////////////////////////////////////////////////////////////
void ptoolsInit()
{
	if (!ptools_locale_encoding_init)
	{
		pthread_mutex_init(&ptools_locale_mt, NULL);
		
		pthread_mutex_lock(&ptools_locale_mt);
		
		const char *curloc=setlocale(LC_CTYPE, NULL);
		setlocale(LC_CTYPE, "");
		const char *enc=nl_langinfo(CODESET);
		ptools_locale_encoding = enc;
		setlocale(LC_CTYPE, curloc);
		
		ptools_locale_encoding_init=true;
		
		pthread_mutex_unlock(&ptools_locale_mt);
	}
}

///////////////////////////////////////////////////////////////////////////////
void ptoolsEnd()
{
	if (ptools_locale_encoding_init)
	{
		pthread_mutex_lock(&ptools_locale_mt);
		ptools_locale_encoding="";
		ptools_locale_encoding_init=false;
		pthread_mutex_unlock(&ptools_locale_mt);
		pthread_mutex_destroy(&ptools_locale_mt);
	}
}

#endif
