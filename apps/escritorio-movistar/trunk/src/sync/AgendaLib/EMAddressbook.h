#pragma once

#include <string> 
#include <vector>
#include <map>
#include "sqlite3.h"
#include "EMContact.h"
#include "AgendaLib.h"


/** \defgroup cpp_api  API en C++
*/

/**
	\brief  Encapsula el acceso a la agenda.
	\ingroup cpp_api
	La agenda del escritorio reside en una base de datos SQLite 3. Esta class permite acceder 
	de una forma sencilla (e independiente de la BD) a los datos almacenados, crear nuevos 
	contactos y modificar o borrar los ya existenetes

	Tambien es posible registrar callbacks para que no sean notificados los cambios en la BD.

	<h3> Metodos por Categoria </h3>
	- Modificac�n de la Agenda \n
		- int saveContact(EMContact c)
		- int removeContact(EMContact c)

	- Busqueda \n
		- vector<EMContact>allContacts(void)
		- vector<EMContact> search(string searchText)
		- vector<EMContact> searchByPhone(string phone);

	- Noficaciones de cambio en la BD\n
		- EMAddressbookCallbackHandle addChangeCallback(EMAddressbookCallback callback)
		- void removeChangeCallback(EMAddressbookCallbackHandle callbackHandle)		

		This is the \ref c_api link" to this group.
*/
class EMAddressbook: public IEMAddressbook
{
protected:
	sqlite3 *_db;
	bool _callbackenabled;

	//sqlite3_stmt *_insertStmt;
	//sqlite3_stmt *_updateByIdStmt;

	typedef std::pair<unsigned short, unsigned int> statements_map_key;
	typedef std::map<statements_map_key, sqlite3_stmt*> statements_map;
	statements_map m_map_stmts;
	
	typedef std::pair<EMAddressbookCallback, DWORD_PTR> callbacks_data;
	typedef std::map <EMAddressbookCallbackHandle, callbacks_data> callbacks_map;
	callbacks_map _callbacks;
	EMAddressbookCallbackHandle _currentHandle;

	sqlite3_stmt* GetStmt(const statements_map_key key, const char* statement);

	std::string readStringInResult(int fieldIndex, sqlite3_stmt* stmt);
	int insertContact(EMContact& c);
	int updateContact(const EMContact& c);
	EMContact contactFromRow(sqlite3_stmt* stmt);
	void executeSearchStmt(sqlite3_stmt *stmt,std::vector<EMContact> *results);

public:
	
	/**
		Contructor de la EMAddressbook
		@param  dbPath El path al fichero donde gardar la base de datos.\n
		Si el fichero no existe lo crea;
	*/

	EMAddressbook(const char* dbPath);
	/**
		Destructor de la EMAddressbook		
	*/
	
	~EMAddressbook(void);
	void destroy() {delete this;}
	
	/**
		Persiste el contacto proporcionado en la BD
		@param c El contacto que se desea guardar en BD
		@return EM_ADDRESSBOOK_OK en caso de exito, EM_ADDRESSBOOK_ERROR.

		Esta funci�n utiliza el campo id del contacto para dete si c es un nuevo contacto (id == NO_SAVED_ID).\n
		Si es un nuevo contacto realizara un insert en la DB en caso contrario realizara un update.
		
		NOTA: Tras esta funcion el contacto debera obtenerse denuevo de la db para obtener los datos actualizados. 

	*/
	int saveContact(EMContact& c);
	
	/**
		Elimina un contacto de la BD
		@param c El contacto que va ha ser eliminado
		@return EM_ADDRESSBOOK_OK en caso de exito, EM_ADDRESSBOOK_ERROR.

		Elimina el contacto de forma permanente de la BD , si este no estaba en la BD no hace nada.
	*/
	int removeContact(const EMContact& c);

	/**
		Elimina un contacto de la BD
		@param ID Identificador del contacto que va ha ser eliminado
		@return EM_ADDRESSBOOK_OK en caso de exito, EM_ADDRESSBOOK_ERROR.

		Elimina el contacto de forma permanente de la BD , si este no estaba en la BD no hace nada.
	*/
	int removeContactByID(const char* ID);
	
	/**
		Obtiene todos los contactos de la BD
		@return un vector con todos lso contactos que hay en la BD
	*/
	std::vector<EMContact>allContacts(void);	
	
	/**
		Obtiene todos los contactos de la BD
		@param searchText La cadena a buscar
		@return un vector con los contactos  de la Bd que contienen searchText en cualquiera de sus campos.
	*/
	std::vector<EMContact> search(const char* searchText);
	std::vector<EMContact> searchColumnsLike(const char* searchText, unsigned int columns);
	
	/**
		Obtiene todos los contactos de la BD
		@param searchText La cadena a buscar
		@param column Columna donde buscar
		@return un vector con los contactos  de la Bd que contienen phone en cualquiera de los campos de telefono.
	*/
	std::vector<EMContact> searchColumnIs(const char* searchText, unsigned int column);

	/**
		Registra un callback para que sea llamado cuando se realizan cambios en  BD.
		@param callback El callback a registrar
		@return un id que puede ser usado posteriormente para eliminar el resgitro callback.		
	*/
	EMAddressbookCallbackHandle addChangeCallback(EMAddressbookCallback callback, DWORD_PTR dwParam);

	/**
		Elimina el resgistro de un callback
		@param callbackHandle

		Elimina el callback identificado por callbackHandle, esta callback no volvera sear llamado
		cuando la BD cambie.

		Si el callbackHandle es invalido este metodo no hace nada.
	*/
	void removeChangeCallback(EMAddressbookCallbackHandle callbackHandle);

	/**
		Recupera un contacto de la BD
		@param ID
		@param contacto
		@return EM_ADDRESSBOOK_OK en caso de exito, EM_ADDRESSBOOK_ERROR.

		Recupera el contacto en la BD a partir de su identificador.
	*/
	int getContact(const char* ID, EMContact& c);
	/**
		Notifica un cambio en la base de datos.

		Para notificar cambios "externos" en la BD, por ejemplo desde otra conexi�n.
	*/
	void notifyDatabaseChange();
	void enableCallback(bool value);
	bool getCallbackEnabled();
};
