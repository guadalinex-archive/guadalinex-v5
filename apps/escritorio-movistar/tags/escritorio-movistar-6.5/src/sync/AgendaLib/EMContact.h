#pragma once
#include <string>
//#include "AgendaLib.h"

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

/*
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#pragma warning (disable : 4275 4231)
	EXPIMP_TEMPLATE template class AGENDALIB_API std::allocator<char>;
	EXPIMP_TEMPLATE template class AGENDALIB_API std::basic_string<char,std::char_traits<char>,std::allocator<char>>;
	EXPIMP_TEMPLATE template class AGENDALIB_API std::_String_val<char,std::allocator<char>>;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define NO_SAVED_ID "-1" 

/**
	\brief Representa un contacto de la agenda.
	\ingroup cpp_api
	Esta clase es un simple contenedor de las propiedades de los contactos
*/
class AGENDALIB_API EMContact
{

protected :
	typedef struct {
		std::string  _id;
		std::string _name;
		std::string _phone;
		std::string _email;
		std::string _copia_agenda_id;
		std::string _modification_stringDate;
	} data;
	data* _data;

public:
	/**
		Constructor
	*/
	EMContact();
	EMContact(const EMContact &other);

	EMContact(const char*  id,
			  const char* name,
			  const char* phone,
			  const char* email,
			  const char* copia_agenda_id,
			  const char* modificationStringdate);
			
	EMContact(const char* name,
			  const char* phone,
			  const char* email);

	EMContact& operator=(const EMContact &other);
	~EMContact(void);

	/** getter  del id del contacto en la BD*/
	const char* getId(void) const;
	/** setter del id del contacto en la BD*/
	void setId(const char* newId);
	/** getter */
	const char* getName(void) const;
	/** setter */
	void setName(const char* newValue);	
	/** getter */
	const char* getPhone(void)const;
	/** setter */
	void setPhone(const char* newValue);
	/** getter */
	const char* getEmail(void)const;
	/** setter */
	void setEmail(const char* newValue);
	/** getter  del id del contacto en la copiagenda si lo tiene*/
	const char* getCopiaAgendaId(void) const;
	/** setter del id del contacto en la copiagenda NO ESTABLECER A MANO*/
	void setCopiaAgendaId(const char* newValue);
	/** getter Fecha del ultimo save en la BD*/
	const char* getModificationDate(void)const ;  
	/** setter del ultimo save en la BD  NO ESTABLECER A MANO lo actualiza automaticamente EMAddressbook::saveContact(EMContact c)*/
	void setModificationDate(const char* newDate);  
};

