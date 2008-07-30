#include "AgendaLib.h"
#include "EMAddressbookAPI.h"
#include <string.h>

using namespace std;

#ifdef _DEBUG
#include <stdio.h>
#include <wtypes.h>

#define ARRSIZE(x) (sizeof(x)/sizeof(x[0]))
void TrazaA(const char* szEntry, ...)
{
	char buffer[1024];
	va_list args;
	int nCount;

	va_start(args, szEntry);
	nCount = _vsnprintf(buffer, ARRSIZE(buffer), szEntry, args);
	va_end(args);
	if (nCount!=-1) 
	{
		if (nCount<(ARRSIZE(buffer)-2)) {
			strcpy(&buffer[nCount], "\r\n");
		}
		OutputDebugStringA(buffer);
	}
}
class MemLeaks{
public:
	MemLeaks():_get_phone(0), _get_email(0), _get_name(0){}
	int _get_phone;
	int _get_email;
	int _get_name;
	int _frees;

	~MemLeaks()
	{
		TrazaA("phone string get number:%i", _get_phone);
		TrazaA("email string get number:%i", _get_email);
		TrazaA("name string get number:%i", _get_name);
		TrazaA("releases string number:%i", _frees);
		TrazaA("Diff: %i", _get_phone+_get_email+_get_name-_frees);
	}
	static MemLeaks& instance();
};

MemLeaks& MemLeaks::instance()
{
	static MemLeaks the_instance;
	return the_instance;
}
#endif // _DEBUG

EMAddressbookHandle em_addressbook_create(const char *dbPath)
{
	return create_EMAddressbook(dbPath); 
}

void em_addressbook_destroy(EMAddressbookHandle handle)
{
	if (handle){
		static_cast<IEMAddressbook*>(handle)->destroy();
	}
}
EMContactListHandle em_addressbook_search_all_contacts(EMAddressbookHandle book)
{
	if (!book){
		return NULL;
	}
	IEMAddressbook *adbook =  reinterpret_cast<IEMAddressbook *>(book);
	vector<EMContact> *result = new vector<EMContact>(adbook->allContacts());
	return result;
}

EMContactListHandle em_addressbook_search_contacts(EMAddressbookHandle book,const char* searchString)
{
	if (!book){
		return NULL;
	}

	IEMAddressbook *adbook =  reinterpret_cast<IEMAddressbook *>(book);	
	vector<EMContact> *result = new vector<EMContact>(adbook->search(searchString));
	return result;

}


EMContactListHandle em_addressbook_search_by_phone_contacts(EMAddressbookHandle book,const char *phone)
{
	if (!book){
		return NULL;
	}
	IEMAddressbook *adbook =  reinterpret_cast<IEMAddressbook *>(book);
	
	vector<EMContact> *result = new vector<EMContact>(
		adbook->searchColumnIs(phone, EMADDRB_COLUMN_PHONE));
	return result;
}


EMAddressbookCallbackHandle em_addressbook_add_change_callback(EMAddressbookHandle book,EMAddressbookCallback callback, DWORD_PTR dwParam)
{
	if (!book){
		return NULL;
	}
	IEMAddressbook *adbook =  reinterpret_cast<IEMAddressbook *>(book);
	return adbook->addChangeCallback(callback, dwParam);
}

void em_addressbook_remove_change_callback(EMAddressbookHandle book, EMAddressbookCallbackHandle callbackHandle)
{
	if (!book){
		return ;
	}
	IEMAddressbook *adbook =  reinterpret_cast<IEMAddressbook *>(book);
	adbook->removeChangeCallback(callbackHandle);
}

unsigned int em_contactlist_count(EMContactListHandle list)
{
	if (!list){
		return (unsigned int)-1;
	}
	vector<EMContact> *v = reinterpret_cast<vector<EMContact> *>(list);
	return (unsigned int)v->size();
}


EMContactHandle em_contactlist_contact_at_index(EMContactListHandle list , unsigned int index)
{
	if (!list){
		return NULL;
	}
	vector<EMContact> *v = reinterpret_cast<vector<EMContact> *>(list);
	if (index >= v->size()){
		return NULL;
	}

	EMContact *c = new EMContact(); 
	*c = (*v)[index];
	return c;
}

void em_contactlist_destroy(EMContactListHandle list)
{
	if(list){
		delete (vector<EMContact> *) list;
	}
}

char *read_property(EMContactHandle contact,const char* (EMContact::*getter)(void) const )
{
	if (!contact){
		return NULL;
	}
	EMContact *c =  reinterpret_cast<EMContact *>(contact); 
	const char *origin = (c->*getter)();
	size_t size = strlen(origin);
	char *result = new char[size+1];
	strcpy(result,origin);
	return result;
	
}
char* em_contact_name(EMContactHandle contact)
{ 
#ifdef _DEBUG
	MemLeaks::instance()._get_name++;
#endif
	return read_property(contact,&EMContact::getName);
}

char * em_contact_phone(EMContactHandle contact)
{
#ifdef _DEBUG
	MemLeaks::instance()._get_phone++;
#endif
	return read_property(contact,&EMContact::getPhone);
}

char * em_contact_email(EMContactHandle contact)
{
#ifdef _DEBUG
	MemLeaks::instance()._get_email++;
#endif
	return read_property(contact,&EMContact::getEmail);
}

void em_string_destroy(char *str)
{
#ifdef _DEBUG
	MemLeaks::instance()._frees++;
#endif
	delete [] str;
}

void em_contact_destroy(EMContactHandle contact)
{
	if(contact)
		delete (EMContact *)contact;

}


