
#ifdef _WIN32
#ifdef AGENDALIB_EXPORTS
	#define AGENDALIB_API __declspec(dllexport)
#else
	#define AGENDALIB_API __declspec(dllimport)
#endif
#else
#define AGENDALIB_API
#endif

/**
 * \defgroup c_api  API en C
 */
/*@{*/


/** Puntero opaco a un objeto EMAddressbook */
typedef  void* EMContactHandle;
/** Puntero opaco a un objeto EMAddressbook */
typedef  void* EMContactListHandle;

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


/** Crea una agenda en el path especificado*/ 
AGENDALIB_API EMAddressbookHandle em_addressbook_create(const char *dbPath);
/** \brief Destruye el objeto agenda	
	No destruey el fichero BD;
*/ 
AGENDALIB_API void em_addressbook_destroy(EMAddressbookHandle);

/**
 * \defgroup addressbook_search  Busqueda de   contactos
 */
/*@{*/

/** \brief Obtiene todos los contactos de la agenda	
	@param book El puntero al objeto agenda
	@return un puntero a una lista de contactos
*/
AGENDALIB_API EMContactListHandle em_addressbook_search_all_contacts(EMAddressbookHandle book);
/** \brief Busca contactos en la agenda	
	@param book El puntero al objeto agenda
	@param searchString la cadena a buscar
	@return un puntero a una lista de contactos que continen searchString en alguno de sus campos
*/ 
AGENDALIB_API EMContactListHandle em_addressbook_search_contacts(EMAddressbookHandle book,const char* searchString);
/** \brief Busca contactos en la agenda	 usando un telefono
	@param book El puntero al objeto agenda
	@param phone la cadena a buscar
	@return un puntero a una lista de contactos que continen searchString en alguno de sus campos de telefono.
*/
AGENDALIB_API EMContactListHandle em_addressbook_search_by_phone_contacts(EMAddressbookHandle book,const char *phone);


/** \brief Registra un callback de cambio en la agenda
	@param book El puntero al objeto agenda
	@param callback puntero al callback
	@return un id que se puede usar para desactivar el callback posteriormente.
	El callback registrado sera llamado cada vez que se modifique la agenda.
*/
/*@}*/


/**
 * \defgroup addressbook_callback  Notificac�n de cabios en BD
 */
/*@{*/
AGENDALIB_API EMAddressbookCallbackHandle em_addressbook_add_change_callback(EMAddressbookHandle book,EMAddressbookCallback callback, DWORD_PTR dwParam);

/** \brief Elimina un callback de cambio de la agenda
	@param book El puntero al objeto agenda
	@param callbackHandle handle delvuelto por  EMAddressbookCallbackHandle em_addressbook_add_change_callback(EMAddressbookCallbackHandle book,EMAddressbookCallback callback)
	Una vez retirado ,el callback ya no sera llamado cuando se realize un cambio
*/
AGENDALIB_API void em_addressbook_remove_change_callback(EMAddressbookHandle book, EMAddressbookCallbackHandle callbackHandle);
/*@}*/


/**
 * \defgroup lista_contacto  Manejo de listas de contactos
 */
/*@{*/

/**\brief Retorna el numero de elementos de una lista de contactos
	@param list puntero a la lista de contactosd
	@return el numero de elemento de la lista , (unsigned int)-1 si hay un error.
	*/
AGENDALIB_API unsigned int em_contactlist_count(EMContactListHandle list);
/**\brief Retorna el un handle al contacto en la posicion especificada.
	@param list puntero a la lista de contactosd
	@return Retorna el un handle al contacto en la posicion especificada , NULL si hay un error.
*/
AGENDALIB_API EMContactHandle em_contactlist_contact_at_index(EMContactListHandle list, unsigned int index);
/** 
	\brief Destruye la lista de contactos.
	@param list puntero a la lista de contactos.
*/
AGENDALIB_API void em_contactlist_destroy(EMContactListHandle list);
/*@}*/


/**
 * \defgroup contacto Manejo de las propiedades de los Contactos
 */
/*@{*/

/**  Obtine el nombre del contacto  */
AGENDALIB_API  char * em_contact_name(EMContactHandle contact);
/**  Obtine el Tel�fono m�vil del contacto */
AGENDALIB_API  char * em_contact_phone(EMContactHandle contact);
/**  Obtine el Mail del contacto */
AGENDALIB_API  char * em_contact_email(EMContactHandle contact);
/** Destruye el contacto 

Libera la memoria del objeto per no lo borra de la BD 

*/
AGENDALIB_API void em_contact_destroy(EMContactHandle contact);
/** Libera la memoria de las cadenas devueltas por las demas funciones em_contact_ */
AGENDALIB_API  void em_string_destroy(char *str);

/*@}*/