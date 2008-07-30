#ifdef _WIN32
#include "StdAfx.h"
#endif
#include "EMContact.h"
using namespace std;

EMContact::EMContact()
{
	_data = new data;
	_data->_id =  NO_SAVED_ID;
	_data->_name =  "";
	_data->_phone = "";
	_data->_email = "";
	_data->_copia_agenda_id = "";
	_data->_modification_stringDate = "";
}


EMContact::EMContact(const char*  id,
			  const char* name, 
			  const char* phone,
			  const char* email,
			  const char* copia_agenda_id,
			  const char* modification_stringDate)
{
	_data = new data;
	_data->_id = id;
	_data->_name = name;
	_data->_phone = phone;
	_data->_email = email;
	_data->_copia_agenda_id = copia_agenda_id;
	_data->_modification_stringDate = modification_stringDate;
}

EMContact::EMContact(const EMContact &other)
{
	_data = new data;
	_data->_id = other.getId();
	_data->_name = other.getName();
	_data->_phone = other.getPhone();
	_data->_email = other.getEmail();
	_data->_copia_agenda_id = other.getCopiaAgendaId();
	_data->_modification_stringDate = other.getModificationDate();
}

EMContact::EMContact(const char* name,
			  const char* phone,
			  const char* email)
{
	_data = new data;
	_data->_id = NO_SAVED_ID;
	_data->_name = name;
	_data->_phone = phone;
	_data->_email = email;
	_data->_copia_agenda_id = "";
	_data->_modification_stringDate = "";
}

EMContact& EMContact::operator=(const EMContact &other)
{
	_data->_id = other.getId();
	_data->_name = other.getName();
	_data->_phone = other.getPhone();
	_data->_email = other.getEmail();
	_data->_copia_agenda_id = other.getCopiaAgendaId();
	_data->_modification_stringDate = other.getModificationDate();
	return *this;
}

EMContact::~EMContact(void)
{
	delete _data;
}

const char* EMContact::getId()const
{
	return _data->_id.c_str();
}

	
void EMContact::setId(const char* newId)
{
	_data->_id = newId;

}

const char* EMContact::getName() const
{
	
	return _data->_name.c_str();
}

void EMContact::setName(const char* newValue)
{
	_data->_name = newValue;
}

const char* EMContact::getPhone() const
{
	return _data->_phone.c_str();
}

void EMContact::setPhone(const char* newValue)
{
	_data->_phone = newValue;
}

const char* EMContact::getEmail(void)const
{
	return _data->_email.c_str();

}

void EMContact::setEmail(const char* newValue)
{
	_data->_email = newValue;
}


const char* EMContact::getCopiaAgendaId(void) const 
{
	return _data->_copia_agenda_id.c_str();
}

void EMContact::setCopiaAgendaId(const char* newValue)
{
	_data->_copia_agenda_id = newValue;
}


const char* EMContact::getModificationDate() const 
{
	return _data->_modification_stringDate.c_str();
}
		   

void EMContact::setModificationDate(const char* newDate)
{
	_data->_modification_stringDate = newDate;
}
	