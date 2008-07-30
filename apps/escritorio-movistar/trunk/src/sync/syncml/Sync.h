#ifndef _SYNC_H
#define _SYNC_H

#include <string>
#include <vector>
#ifndef _WIN32
#include <pthread.h>
#endif
#include "vCard.h"
// #include "AgendaLib.h"
#include "SyncML.h"

class IEMAddressbook;

// class CSync
//
// esto es para la funcionalidad completa:
// (con funcionalidad limitada no se nace)
//
// clase para realizar la sincronizaci�n y actualizaci�n de la base de datos
// de contactos. 
// El proceso para realizar una sincronizaci�n consta de los pasos:
// 1) Llamar a Start(), lo que inicia el hilo, que inmediatamente
//    intenta realizar la conexi�n.
// 2) Esperar que el estado el hilo sea State_Acknowledge o State_Finished
// 3) Cuando el estado sea State_Acknowledge, obtener la lista
//    de contactos recibidos por parte del servidor. Estos contactos 
//	  los elige el hilo automaticamente para confirmar respuesta del usuario.
//	  La funci�n GetVerifyContacts() obtiene la lista de contactos
//	  que se deben comprobar para actualizar en la base de datos, y enviar 
//	  confirmaci�n al servidor SyncML.
// 4) Dado el caso 3), establecer la lista de contactos que deben actualizarse 
//	  localmente llamando a UpdateContacts().
// 5) Cuando el estado es State_Finished, el proceso ha terminado.
//
//
// con funcionalidad limitada (el comportamiento actual):
// 1) Llamar Start
// 2) Esperar a que termine llamando Running()
// 3) Comprobar estado de error.
// * Los contactos, de bajarse sin error, se a�aden automaticamente a DB de EM.
// 
class CSync
{
public:
	CSync(): error(Error_None), running(false), sync(0)
	{
#ifdef _WIN32
		InitializeCriticalSection(&cs);
#else
		pthread_mutex_init(&mt, NULL);
#endif
	}
	~CSync()
	{
		Stop();
#ifdef _WIN32
		DeleteCriticalSection(&cs);
#else
		pthread_mutex_destroy(&mt);
#endif
	}

	// m�todo de acualizaci�n
	// 
	// indica al hilo el tipo de actualizaci�n de contactos locales
	// que debe hacerse una vez recibidos contactos.
	enum UpdateMode
	{
		Update_Always,
		Update_Never,
		Update_Ask
	};

	// iniciar el hilo de ejecuci�n
	bool Start(UpdateMode mode,
			   bool forceFullDownload,
			   const std::tstring addressBookPath,
			   const std::tstring &host, 
			   const std::tstring &service,
			   const std::tstring &db,
			   const std::tstring &idPC,
			   const std::tstring &user,
			   const std::tstring &password,
			   const std::tstring &telefono);

	// consulta del progreso de la sincronizaci�n
	int GetProgress();

	// solicita la finalizaci�n de la sincronizaci�n
	void RequestShutdown();

	// consulta el estado de finalizaci�n solicitado
	bool ShutdownRequested() { CAutoLock alock(*this); return _ShutdownRequested(); }

	// comprueba si esta ejecutandose el hilo de sincronizaci�n
	bool Running() { CAutoLock alock(*this); return running && state!=State_Finished; }

	// devuelve el m�todo de actualizaci�n
	UpdateMode GetUpdateMode() { CAutoLock alock(*this); return _GetUpdateMode(); }

	// estados del hilo de sincronizaci�n
	enum State
	{
		State_Connecting,
		State_InitSync,
		State_Downloading,
		State_Acknowledge,
		State_Uploading,
		State_Cancelled,
		State_Finished,
		State_FinishedWithError,
		State_FinishedWithPartialDownload // some contact downloads Ok. GetError() returns error.
	};

	// obtiene el estado del proceso de sync
	State GetState()
	{
		CAutoLock alock(*this);
		return _GetState();
	}

	// c�digos de error devueltos por el proceso de sync
	enum Error
	{
		Error_None,
		Error_NoDB,
		Error_CouldNotConnect,
		Error_InvalidReply,
		Error_BadAuth,
		Error_Unknown // (Toolkit C...?)
	};

	// obtiene el ultimo error del proceso de sync
	Error GetError()
	{
		CAutoLock alock(*this);
		return error;
	}

	// contacto de intercambio entre hilo y usuario del objeto a efectos
	// de confirmaci�n por parte del usuario (para el m�todo de actualizaci�n
	// Update_ask)
	struct SConfirmContact
	{
		std::tstring id;
		CVCard vcard;
		enum Op
		{
			Op_Replace,
			Op_Delete,
			Op_Leave
		} op;
	};

	// devuelve los contactos recibidos que deben verificarse 
	// (por parte del usuario)
	void GetVerificationContacts(std::vector<SConfirmContact> &confirm)
	{
		ASSERT(GetState() == State_Acknowledge);
		CAutoLock alock(*this);
		confirm = CSync::confirm;
	}

	// establece los contactos ya verificados por el usuario
	// para que los procese el hilo
	void UpdateContacts(const std::vector<SConfirmContact> &update)
	{
		CAutoLock alock(*this);
		ASSERT(_GetState()==State_Acknowledge);
		int u;
		for (u=0; u<(int)confirm.size(); u++)
			confirm[u].op = SConfirmContact::Op_Leave;
		for (int u=0; u<(int)update.size(); u++)
			for (int u2=0; u2<(int)confirm.size(); u2++)
				if (confirm[u2].id==update[u].id)
					confirm[u2].op = update[u].op;
		_SetState(State_Uploading);
	}

private:

	// finalizaci�n bloqueante del hilo
	void Stop()
	{
		RequestShutdown();
		while (Running())
		{
#ifdef _WIN32
			Sleep(0);
#else
			timespec ts, tr;
			ts.tv_sec=0;
			ts.tv_nsec=20*1000000; // 20 ms -> nano
			nanosleep(&ts, &tr);
#endif
		}
	}

	// establecer el estado
	void SetState(State state)
	{
		CAutoLock alock(*this);
		_SetState(state);
	}

	// obtener el estado (as�ncrono)
	State _GetState()
	{
		return state;
	}

	// establece el estado (as�ncrono)
	void _SetState(State state)
	{
		CSync::state = state;
	}

	// obtiene el modo de actualizaci�n (as�ncrono)
	UpdateMode _GetUpdateMode()
	{
		return updateMode;
	}

	// pone el c�digo de error
	void SetError(Error error)
	{
		CAutoLock alock(*this);
		_SetError(error);
	}

	// pone el c�digo de error (as�ncrono)
	void _SetError(Error error)
	{
		CSync::error = error;
	}

	// devuelve el c�digo de error (as�ncrono)
	Error _GetError()
	{
		return error;
	}

	// consulta el estado de petici�n de terminar hilo (as�ncrono)
	bool _ShutdownRequested() { return doShutdown; }

	// objeto de sincronizaci�n multihilo
	typedef ::CAutoLock<CSync> CAutoLock;

	Error error; // c�digo de error resultado del proceso
	UpdateMode updateMode; // m�todo de actualizaci�n de contactos
	bool forceFull; // forzar descarga completa
	std::wstring addressBookPath; // fichero de base de datos de contactos locales
#ifdef _WIN32
	CRITICAL_SECTION cs; // objeto seccion cr�tica del objeto
#else
	pthread_mutex_t mt;
#endif
	bool running, doShutdown; // estado de ejecuci�n, petici�n de finalizaci�n
	State state; // estado del proceso de sync.
	std::tstring host, service, db, idPC, user, password, telefono; // datos varios de sync.
	std::vector<SConfirmContact> confirm; // contactos a confirmar/confirmados para actualizaci�n de contactos
	int faseSync; // para progreso

	void *sync; // objeto de encapsulaci�n Toolkit C para manejo SyncML (opaco)

	// hilo de ejecuci�n
#ifdef _WIN32
	static unsigned __stdcall threadProc(void *);
#else
	static void *threadProc(void *);
#endif

	// a�adir contacto a base de datos local
	static std::string AddContact(IEMAddressbook *abook, const CVCard &vcard, bool sim);

	// sustituir contacto en la base de datos local
	static bool ReplaceContact(IEMAddressbook *abook, std::string id, const CVCard &vcard);

	// eliminar contacto en la base de datos local
	static bool DeleteContact(IEMAddressbook *abook, std::string id);

protected:
	// bloquear objeto
#ifdef _WIN32
	void Lock() { EnterCriticalSection(&cs); }
#else
	void Lock() { pthread_mutex_lock(&mt); }
#endif

	// desbloquear objeto
#ifdef _WIN32
	void Unlock() { LeaveCriticalSection(&cs); }
#else
	void Unlock() { pthread_mutex_unlock(&mt); }
#endif

	friend class ::CAutoLock<CSync>;

public:
	static bool mismoTelefono(std::string t1, std::string t2, const std::tstring &countryCode);
	static void filtrarContactosAgenda(IEMAddressbook *abook, const std::tstring &countryCode);
};


#endif // _SYNC_H
