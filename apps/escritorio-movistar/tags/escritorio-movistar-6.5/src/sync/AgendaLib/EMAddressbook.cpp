
#ifdef _WIN32
#include "StdAfx.h"
#else
#include <exception>
#include "EMAddressbookAPI.h"
#include <assert.h>
#endif
#include "EMAddressbook.h"
#include "sqlite3.h"
using namespace std;

#include <limits.h>

#define EMSTMT_SEARCH_COLUMNS_LIKE (0x1)
#define EMSTMT_SEARCH_COLUMN_IS    (0x2)
#define EMSTMT_SELECT_BY_ID        (0x3)
#define EMSTMT_REMOVE_BY_ID        (0x4)
#define EMSTMT_SELECT_ALL          (0x5)

#ifdef HAVE_CXX_EXCEPTION_CHAR_P_CTOR
typedef std::exception exception;
#else
class exception: public std::exception
{
	std::string cause;
public:
	exception(const std::string &cause): cause(cause) { }
    virtual ~exception() throw() { }
    virtual const char* what() const throw() { return cause.c_str(); }
};
#endif

/*
void TraceThreadID()
{
	DWORD id = GetCurrentThreadId();
	char buf[256];
	_snprintf(buf, 256, "ThreadID: 0x%08u\r\n", id);
	OutputDebugStringA(buf);
}
*/

EMAddressbook::EMAddressbook(const char* dbPath):
	_callbackenabled(true)
{
	//Open data base and create tables if needed
	static const char* CREATE_SQL =
		"CREATE TABLE IF NOT EXISTS contacts ( id INTEGER PRIMARY KEY AUTOINCREMENT , name TEXT , phone TEXT, email TEXT , copia_agenda_id TEXT ,modification_stringdate TEXT );";
		
	int rc = sqlite3_open(dbPath,&_db);
	if (rc != SQLITE_OK) {
#ifdef _DEBUG
		OutputDebugString(TEXT("Error en db\n"));	
#endif
		throw ::exception("No se puede crear la DB");
	}
	rc = sqlite3_exec(_db,CREATE_SQL,NULL,NULL,NULL);
	if (rc != SQLITE_OK){
#ifdef _DEBUG
		OutputDebugString(TEXT("Error creando tablas"));
#endif
		sqlite3_close(_db);
		_db = NULL;
		// Excepcion
		throw ::exception("No se puede crear la tabla contacts");
	}

	//Creo los statement
	//_insertStmt = NULL;
	//_updateByIdStmt =NULL;
	_currentHandle = 0;
}


EMAddressbook::~EMAddressbook(void)
{
	//if (_insertStmt){
	//	sqlite3_finalize(_insertStmt);
	//	_insertStmt = NULL;
	//}
	//if (_updateByIdStmt){
	//	sqlite3_finalize(_updateByIdStmt);
	//	_updateByIdStmt =NULL;
	//}
	statements_map::iterator it = m_map_stmts.begin();
	while (it!=m_map_stmts.end())
	{
		sqlite3_finalize(it->second);
		it++;
	}

	sqlite3_close(_db);
	_db = NULL;
}

string EMAddressbook::readStringInResult(int fieldIndex,sqlite3_stmt* stmt)
{
	string result = "";	
	const char * value = (const char *) sqlite3_column_text(stmt,fieldIndex);
	if (value){
		result = value;
	}
	return result	;
}

vector<EMContact> EMAddressbook::allContacts(void)
{	
//TraceThreadID();
	static const char *SELECT_ALL_SQL = 
		"SELECT id,name,phone,email,copia_agenda_id,modification_stringdate FROM contacts ORDER BY id;";
	vector<EMContact> result;
	statements_map_key key(EMSTMT_SELECT_ALL, 0);
	sqlite3_stmt *stmt = GetStmt(key, SELECT_ALL_SQL);
	executeSearchStmt(stmt,&result);
	return  result;
}

int EMAddressbook::saveContact(EMContact& c)
{
	int result = EM_ADDRESSBOOK_ERROR;
	//Es insert o update
	if (strcmp(c.getId(), NO_SAVED_ID)==0){
		result = this->insertContact(c);						
	}else{
		result = this->updateContact(c);
	}
	
	// notifico a los listeners que la db ha cambiado
	if (result == EM_ADDRESSBOOK_OK){
		this->notifyDatabaseChange();
	}
	return result;
}

int EMAddressbook::insertContact(EMContact& c)
{
	static const char *INSERT_SQL_TEMPLATE = 
		"INSERT INTO contacts(name,phone,email,copia_agenda_id,modification_stringdate) VALUES(?,?,?,?,DATETIME('now','localtime'));";
	sqlite3_stmt *insertStmt=NULL;
	if (sqlite3_prepare_v2(_db,INSERT_SQL_TEMPLATE,-1,&insertStmt,NULL) != SQLITE_OK ){
		return EM_ADDRESSBOOK_ERROR;
	}
	
	//Bind de parametros
	sqlite3_bind_text(insertStmt,1,c.getName(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(insertStmt,2,c.getPhone(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(insertStmt,3,c.getEmail(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(insertStmt,4,c.getCopiaAgendaId(),-1,SQLITE_TRANSIENT);

	// ejecuto
	int nResult;
	if (sqlite3_step(insertStmt) != SQLITE_DONE)
	{
		nResult = EM_ADDRESSBOOK_ERROR;
	}
	else
	{
		nResult = EM_ADDRESSBOOK_OK;
		sqlite_int64 nID = sqlite3_last_insert_rowid(_db);
		char szID[128];
#ifdef HAVE_SNPRINTF
		snprintf(szID, 128, "%llu", (unsigned long long)nID);
#elif defined HAVE__SNPRINTF
		_snprintf(szID, 128, "%llu", (unsigned long long)nID);
#else
		sprintf(szID, "%llu", (unsigned long long)nID);
#endif
		c.setId(szID);
	}
	sqlite3_finalize(insertStmt);

	return nResult;
}

int EMAddressbook::updateContact(const EMContact& c)
{
	static const char *UPDATE_SQL_TEMPLATE = 
		"UPDATE contacts SET name=?,phone=?,email=?,copia_agenda_id=?,modification_stringdate=DATETIME('now','localtime') WHERE id=?;";
	sqlite3_stmt *updateByIdStmt=NULL;
	int prepareResult =sqlite3_prepare_v2(_db,UPDATE_SQL_TEMPLATE,-1,&updateByIdStmt,NULL);
	if (prepareResult!= SQLITE_OK ){
		return EM_ADDRESSBOOK_ERROR;
	}

	sqlite3_bind_text(updateByIdStmt,1,c.getName(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(updateByIdStmt,2,c.getPhone(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(updateByIdStmt,3,c.getEmail(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(updateByIdStmt,4,c.getCopiaAgendaId(),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(updateByIdStmt,5,c.getId(),-1,SQLITE_TRANSIENT);
	
	// ejecuto
	int nResult;
	if (sqlite3_step(updateByIdStmt) != SQLITE_DONE) {
		nResult = EM_ADDRESSBOOK_ERROR;
	}
	else {
		nResult = EM_ADDRESSBOOK_OK;
	}
	sqlite3_finalize(updateByIdStmt);

	return nResult;
}

int EMAddressbook::removeContact(const EMContact& c)
{
	return removeContactByID(c.getId());
}

int EMAddressbook::removeContactByID(const char* szID)
{
	static const char *REMOVE_SQL_TEMPLATE = 
		"DELETE FROM contacts WHERE id=?;";
	statements_map_key key(EMSTMT_REMOVE_BY_ID, 0);
	sqlite3_stmt *stmt = GetStmt(key, REMOVE_SQL_TEMPLATE);

	int result = EM_ADDRESSBOOK_ERROR;
	if (strcmp(szID, NO_SAVED_ID)!=0)
	{
		sqlite3_bind_text(stmt,1,szID,-1,SQLITE_TRANSIENT);
		if (sqlite3_step(stmt) == SQLITE_DONE)
		{
			result = EM_ADDRESSBOOK_OK;
			// notifico a los listeners que la db ha cambiado
			notifyDatabaseChange();
		}
		sqlite3_reset(stmt);
	}

	return result;
}

void EMAddressbook::notifyDatabaseChange(void)
{
	if (_callbackenabled) {
		callbacks_map::const_iterator iter;
		for(iter =_callbacks.begin(); iter != _callbacks.end(); iter++){
			const callbacks_data& data = iter->second;
			(*data.first)(this, data.second);
		}
	}
}

void EMAddressbook::enableCallback(bool value)
{
	_callbackenabled = value;
}

bool EMAddressbook::getCallbackEnabled()
{
	return _callbackenabled;
}

EMContact EMAddressbook::contactFromRow(sqlite3_stmt* stmt)
{
	int i = 0;
	string id = this->readStringInResult(i++,stmt);
	string name = this->readStringInResult(i++,stmt);
	string phone = this->readStringInResult(i++,stmt);
	string email = this->readStringInResult(i++,stmt);
	string copia_agenda_id = this->readStringInResult(i++,stmt);
	string modification_stringdate = this->readStringInResult(i++,stmt);
	return EMContact(id.c_str(),
		name.c_str(),
		phone.c_str(),
		email.c_str(),
		copia_agenda_id.c_str(),
		modification_stringdate.c_str());
}

void EMAddressbook::executeSearchStmt(sqlite3_stmt *stmt,vector<EMContact> *results)
{

	int lastResult;	
	while ((lastResult = sqlite3_step(stmt)) == SQLITE_ROW ){
		EMContact contact = this->contactFromRow(stmt);
		results->push_back(contact);
	}
	sqlite3_reset(stmt);
	if (lastResult != SQLITE_DONE){
		throw ::exception("Error en la select");
	}

}

vector<EMContact> EMAddressbook::search(const char* searchText)
{
	return searchColumnsLike(searchText, EMADDRB_COLUMN_ALL);
}

vector<EMContact> EMAddressbook::searchColumnsLike(const char* searchText, unsigned int columns)
{	
//TraceThreadID();
	static const char select_search[] = 
		"SELECT id,name,phone,email,copia_agenda_id,modification_stringdate FROM contacts WHERE ";

	vector<EMContact> result;
	unsigned int fields = (columns&EMADDRB_COLUMN_ALL);
	if (!fields)
		return result;

	statements_map_key key(EMSTMT_SEARCH_COLUMNS_LIKE, fields);
	sqlite3_stmt *stmt;
	statements_map::iterator it = m_map_stmts.find(key);
	if (it==m_map_stmts.end())
	{
		std::string statement;
		statement.append(select_search);
		if (fields&EMADDRB_COLUMN_NAME)
		{
			statement.append("name LIKE '%' || ?1 || '%'");
			fields&=~EMADDRB_COLUMN_NAME;
			if (fields) statement.append(" OR ");
		}
		if (fields&EMADDRB_COLUMN_PHONE)
		{
			statement.append("phone LIKE '%' || ?1 || '%'");
			fields&=~EMADDRB_COLUMN_PHONE;
			if (fields) statement.append(" OR ");
		}
		if (fields&EMADDRB_COLUMN_EMAIL)
		{
			statement.append("email LIKE '%' || ?1 || '%'");
			fields&=~EMADDRB_COLUMN_EMAIL;
			if (fields) statement.append(" OR ");
		}
		if (fields&EMADDRB_COLUMN_COPIAAGENDAID)
		{
			statement.append("copia_agenda_id LIKE '%' || ?1 || '%'");
		}
		statement.append(";");
		if (sqlite3_prepare(_db,statement.c_str(),-1,&stmt,NULL) != SQLITE_OK)
		{
			throw ::exception(
				(std::string("Error preparando select de b�squeda:")+statement).c_str());
		}
		m_map_stmts.insert(statements_map::value_type(key, stmt));
	}
	else
	{
		stmt = it->second;
	}
	sqlite3_bind_text(stmt, 1, searchText, -1, SQLITE_TRANSIENT);
	executeSearchStmt(stmt,&result);
	return result;
}

std::vector<EMContact> EMAddressbook::searchColumnIs(const char* searchText, unsigned int column)
{
//TraceThreadID();
	static const char select_search[] = 
		"SELECT id,name,phone,email,copia_agenda_id,modification_stringdate FROM contacts WHERE ";

	vector<EMContact> result;
	unsigned int field = (column&EMADDRB_COLUMN_ALL);
	if (!field)
		return result;
	
	statements_map_key key(EMSTMT_SEARCH_COLUMN_IS, field);
	sqlite3_stmt *stmt;
	statements_map::iterator it = m_map_stmts.find(key);
	if (it==m_map_stmts.end())
	{
		std::string statement;
		statement.append(select_search);
		if (field&EMADDRB_COLUMN_NAME)
		{
			statement.append("name");
		}
		else if (field&EMADDRB_COLUMN_PHONE)
		{
			statement.append("phone");
		}
		else if (field&EMADDRB_COLUMN_EMAIL)
		{
			statement.append("email");
		}
		else if (field&EMADDRB_COLUMN_COPIAAGENDAID)
		{
			statement.append("copia_agenda_id");
		}
		statement.append("=?;");
		if (sqlite3_prepare(_db,statement.c_str(),-1,&stmt,NULL) != SQLITE_OK)
		{
			throw ::exception(
				(std::string("Error preparando select de b�squeda:")+statement).c_str());
		}
		m_map_stmts.insert(statements_map::value_type(key, stmt));
	}
	else
	{
		stmt = it->second;
	}
	sqlite3_bind_text(stmt, 1, searchText, -1, SQLITE_TRANSIENT);
	executeSearchStmt(stmt,&result);
	return result;
}


EMAddressbookCallbackHandle EMAddressbook::addChangeCallback(EMAddressbookCallback callback, DWORD_PTR dwParam)
{
	
	EMAddressbookCallbackHandle handle = _currentHandle;
	_callbacks[handle] = callbacks_data(callback, dwParam);
	_currentHandle++;
	return handle;
	
}

void EMAddressbook::removeChangeCallback(EMAddressbookCallbackHandle handle)
{	
	_callbacks.erase(handle);
}


sqlite3_stmt* EMAddressbook::GetStmt(const statements_map_key key, const char* statement)
{
	sqlite3_stmt* stmt;
	statements_map::iterator it = m_map_stmts.find(key);
	if (it==m_map_stmts.end())
	{
		if (sqlite3_prepare(_db,statement,-1,&stmt,NULL) != SQLITE_OK)
		{
			throw ::exception(
				(std::string("Error preparando select de b�squeda:")+statement).c_str());
		}
		m_map_stmts.insert(statements_map::value_type(key, stmt));
	}
	else
	{
		stmt = it->second;
	}
	return stmt;
}

int EMAddressbook::getContact(const char* szID, EMContact& contact)
{
	static const char *SELECT_BY_ID_TEMPLATE = 
		"SELECT id,name,phone,email,copia_agenda_id,modification_stringdate FROM contacts WHERE id=?;";
	statements_map_key key(EMSTMT_SELECT_BY_ID, 0);
	sqlite3_stmt *stmt = GetStmt(key, SELECT_BY_ID_TEMPLATE);

	int result = EM_ADDRESSBOOK_ERROR;
	if (strcmp(szID, NO_SAVED_ID)!=0)
	{
		sqlite3_bind_text(stmt,1,szID,-1,SQLITE_TRANSIENT);
		if (sqlite3_step(stmt) == SQLITE_ROW)
		{
			contact = contactFromRow(stmt);
			result = EM_ADDRESSBOOK_OK;
		}
		sqlite3_reset(stmt);
	}
	return result;
}

#ifdef _WIN32
AGENDALIB_API IEMAddressbook* __stdcall create_EMAddressbook(const char* db_path)
#else
AGENDALIB_API IEMAddressbook* create_EMAddressbook(const char* db_path)
#endif
{
	EMAddressbook* addrbook = NULL;
	try {
		addrbook = new EMAddressbook(db_path);
	}
	catch(const ::exception& excp) {
#ifdef _WIN32
		OutputDebugStringA(excp.what());
#else
		assert(false); // code this!
#endif
	}
	return addrbook;
}

