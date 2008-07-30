// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the AGENDALIB_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// AGENDALIB_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
//

#include <stdio.h>
#include <string.h>

#pragma once

#ifdef _WIN32
#ifdef AGENDALIB_EXPORTS
	#define AGENDALIB_API __declspec(dllexport)
	#define EXPIMP_TEMPLATE 
#else
	#define AGENDALIB_API __declspec(dllimport)
	#define EXPIMP_TEMPLATE extern
#endif
#else
#define AGENDALIB_API
#define EXPIMP_TEMPLATE
#endif

#include <vector>
#include "EMContact.h"

#if !(defined DWORD_PTR)
  #if defined(_WIN64)
	typedef unsigned __int64 DWORD_PTR;
  #else
    typedef unsigned long DWORD_PTR;
  #endif
#endif

/**
	C�digo de retorno para operaci�n con exito.
*/
#define EM_ADDRESSBOOK_OK  0
/**
	C�digo de retorno para operaci�n fallida.
*/
#define EM_ADDRESSBOOK_ERROR  -1

/** Puntero opaco a un objeto EMAddressbook */
typedef  void* EMAddressbookHandle;
/** cookie */
typedef int EMAddressbookCallbackHandle;
/** Callback */
#ifdef _WIN32
typedef void (__stdcall *EMAddressbookCallback)(EMAddressbookHandle, DWORD_PTR);
#else
typedef void (*EMAddressbookCallback)(EMAddressbookHandle, DWORD_PTR);
#endif

#ifdef _WIN32
#ifdef _DEBUG
class AGENDALIB_API std::_Container_base;
#else
struct AGENDALIB_API std::_Container_base;
#endif
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4231)
#endif

EXPIMP_TEMPLATE template class AGENDALIB_API std::allocator<EMContact>;
EXPIMP_TEMPLATE template class AGENDALIB_API std::vector<EMContact, std::allocator<EMContact> >;


#define EMADDRB_COLUMN_NAME           (1<<0)
#define EMADDRB_COLUMN_PHONE          (1<<1)
#define EMADDRB_COLUMN_EMAIL          (1<<2)
#define EMADDRB_COLUMN_COPIAAGENDAID  (1<<3)
#define EMADDRB_COLUMN_ALL            (0x0F)

class IEMAddressbook
{
protected:
	virtual ~IEMAddressbook(){}; // Use destroy()
public:
	virtual void destroy()=0;
	virtual int saveContact(EMContact& c)=0;
	virtual int removeContact(const EMContact& c)=0;
	virtual int removeContactByID(const char* ID)=0;
	virtual std::vector<EMContact> allContacts(void)=0;	
	virtual std::vector<EMContact> search(const char* searchText)=0;
	virtual std::vector<EMContact> searchColumnsLike(const char* searchText, unsigned int columns)=0;
	virtual std::vector<EMContact> searchColumnIs(const char* searchText, unsigned int column)=0;
	virtual EMAddressbookCallbackHandle addChangeCallback(EMAddressbookCallback callback, DWORD_PTR dwParam)=0;
	virtual void removeChangeCallback(EMAddressbookCallbackHandle callbackHandle)=0;
	virtual int getContact(const char* ID, EMContact& c)=0;
	virtual void notifyDatabaseChange()=0;
	virtual void enableCallback(bool value)=0;
	virtual bool getCallbackEnabled()=0;
};

#ifdef _WIN32
AGENDALIB_API IEMAddressbook* __stdcall create_EMAddressbook(const char* db_path);
#else
AGENDALIB_API IEMAddressbook* create_EMAddressbook(const char* db_path);
#endif
//AGENDALIB_API IEMContact* __stdcall create_EMContact();
