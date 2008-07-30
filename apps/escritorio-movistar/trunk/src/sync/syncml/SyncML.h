
#ifndef _SYNCML_H
#define _SYNCML_H

#include <string>
#include <memory>

#include "vCard.h"

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

// #define FUNAMBOL 1

// template<class C> class CAutoLock
//
// clase para automatizar bloqueo de secciones cr�ticas:
// 
template<class C> class CAutoLock
{
	C &c;
public:
	CAutoLock(C &c): c(c) { Lock(); }
	~CAutoLock() { Unlock(); }
	void Lock() { c.Lock(); }
	void Unlock() { c.Unlock(); }
};

struct CSyncData;

// class CSYncML
//
// Clase C++ de encapsulaci�n ligera de la implementaci�n de referencia 
// Toolkit C SyncML.
// 
// Proceso general para realizar una sincronizaci�n SyncML
// usando esta clase:
// 
// 1) Init()
// 2) Connect()
// 3) StartMessage()
// 4) Enviar comandos SyncML usando las funciones:
//    Alert(), Sync(), Map(), Put(), ...
// 5) EndMessage()
// 6) Disconnect()
// 7) End()
// 8) Procesar mensaje de respuesta obtenido del servidor
//    usando las funciones getFirstReply*()
// 
// Notas:
// - Esta clase es una encapsulaci�n ligera ya que no oculta realmente
//   todos los mecanismos propios del protocolo SyncML. Concretamente
//   la nomenclatura de los comandos, as� como los c�digos de alerta
//   y de Status son num�ricos, y hacen referencia a los especificados
//   por el protocolo SyncML.
// - Se pueden reutilizar sesiones (para un m�ltiple intercambio 
//   de mensajes por id�ntica sesi�n) sin llamar Init()/End()
//   entre secuencias StartMessage/EndMessage. S� que es necesario
//   establecer comunicaciones m�ltiples bas�ndose en conexi�n, es decir, 
//   se debe construir un mensaje de intercambio por conexi�n (Connect()/Disconnect()).
// - Aunque Init() y End() no son est�ticas, s� que deben llamarse
//   una sola vez por aplicaci�n debido a que las funciones 
//   Toolkit C que estan por debajo estan orientadas a funcionar de 
//   esta forma. De este modo, la clase CSyncML realmente solo 
//   puede tener una �nica instancia por aplicac�n, a modo de singleton,
//   aunque este patr�n no ha sido programado expl�citamente.
// - Los n�meros de mensaje/comando que devuelven las funciones de comando 
//   (StartMessage(), Alert(), Sync(), etc) sirven para comprobar su referencia
//   en respuestas obtenidas en comandos tipo Status desde el servidor.
// - Esta clase no expone el interfaz de Toolkit C al usuario, para de esta forma 
//   poder f�cilmente incluir este m�dulo en una dll o una lib externa. 
//

class CSyncML
{
public:
	CSyncML();
	~CSyncML();

	// inicializa (por sesi�n):
	bool Init(unsigned memKB);

	// finaliza (por sesi�n)
	bool End();

	// conecta al servidor
	bool Connect(const std::tstring &host, const std::tstring &service, const std::tstring &idPC,
		const std::tstring &user = _T(""), const std::tstring &pass = _T(""));

	// desconecta
	bool Disconnect();

	// interrumpe el proceso de sincronizaci�n, sea cual sea el estado.
	// esta funci�n se puede llamar desde un hilo diferente desde el que
	// se realiza la sincronizaci�n.
	void Interrupt();

	// iniciar un mensaje, y devuelve el identificador num�rico que 
	// se ha enviado al servidor.
	int StartMessage(const std::tstring &devtype, const std::tstring &devid);

	// finaliza el mensaje, y devuelve el identificador num�rico que
	// se ha enviado al servidor
	int EndMessage();

	// obtiene el porcentaje de env�o de informaci�n http
	int GetReceivePercent();

	// obtiene el porcentaje de recepci�n de informaci�n http
	// este valor puede ser siempre 100, si el servidor
	// no nos ha enviado el tama�o del documento
	int GetSendPercent();

	// commandos SyncML
	//

	// tipos de alerta:
	// 200 TWO-WAY				Specifies a client-initiated, two-way sync.
	// 201 SLOW SYNC			Specifies a client-initiated, two-way slow-sync.
	// 202 ONE-WAY FROM CLIENT	Specifies the client-initiated, one-way only sync from the client to the server.
	// 203 REFRESH FROM CLIENT	Specifies the client-initiated, refresh operation for the oneway only sync from the client to the server.
	// 204 ONE-WAY FROM SERVER	Specifies the client-initiated, one-way only sync from the server to the client.
	// 205 REFRESH FROM	SERVER	Specifies the client-initiated, refresh operation of the oneway only sync from the server to the client.

	// prepara el comando Alert que le indica al servidor que se desea 
	// realizar una sincronizaci�n, y el tipo de la misma que se desea.
	int Alert(int alertCode, const std::tstring &service, const std::tstring &lastAnchor=_T(""), const std::tstring &nextAnchor=_T(""));

	// prepara el comando Sync para enviar comandos de datos de sincronizaci�n
	int Sync(const std::tstring &service);

	// prepara el comando Status para informar al servidor del estado 
	// de un comando/mensaje previo de sincronizaci�n recibido del servidor
	int Status(int statusCode, const std::tstring &msgRef, const std::tstring &cmdRef,
		const std::tstring &targetRef, const std::tstring &sourceRef, const std::tstring &cmd,
		const std::tstring &nextAnchor);

	// prepara el comando Map para identificar contactos remotos con locales
	// (el servidor mantiene una referencia de �stos)
	int Map(const std::tstring &target, const std::tstring &source,
		const std::tstring &sourceItem, const std::tstring &targetItem);

	// prepara el comando Put para env�o de datos extra al servidor
	int Put(const std::tstring &data);

	// env�a la informaci�n del dispositivo en un comando Put
	int PutDevInfo(const std::tstring &data=_T(""));

	// gesti�n respuestas del servidor:
	//
	enum ReplyType
	{
		Reply_SyncHeader,
		Reply_Status,
		Reply_Alert,
		Reply_Add,
		Reply_Replace,
		Reply_Delete
	};

	// indica si tenemos alguna respuesta al mensaje enviado
	bool haveReply();

	// elimina la primera respuesta de la lista interna
	void removeFirstReply();

	// obtiene el tipo de la primera respuesta
	ReplyType getFirstReplyType();

	// obtiene la primera respuesta como tipo SyncHdr
	struct SyncHeaderReply
	{
		bool noResp;
		std::tstring msg;
		std::tstring target, source;
	};
	SyncHeaderReply getFirstReplySyncHeader();

	// obtiene la primera respuesta como tipo Alert
	struct AlertReply
	{
		int code; 
		std::tstring lastAnchor, nextAnchor;
		bool noResp;
		std::tstring msg, cmd;
		std::tstring target, source;
	};
	AlertReply getFirstReplyAlert();

	// obtiene la primera respuesta como tipo Status
	struct StatusReply
	{
		int code;
		std::tstring targetRef, sourceRef;
		int msgRef, cmdRef;
		std::tstring msg, cmd;
	};
	StatusReply getFirstReplyStatus();

	// obtiene la primera respuesta como tipo Add
	struct AddReply
	{
		//std::tstring target, source;
		std::tstring sourceItem, cmd, msg; // sourceItem es el GUID temporal del servidor (no se puede guardar)
		CVCard vcard;
	};
	AddReply getFirstReplyAdd();

	// obtiene la primera respuesta como tipo Replace
	struct ReplaceReply
	{
		std::tstring targetItem, cmd, msg; // targetItem es el LUID del cliente que el servidor desea que modifiquemos
		CVCard vcard;
	};
	ReplaceReply getFirstReplyReplace();

	// obtiene la primera respuesta como tipo Delete
	struct DeleteReply
	{
		std::tstring targetItem, cmd, msg; // targetItem es el LUID del cliente que el servidor desea que eliminemos
	};
	DeleteReply getFirstReplyDelete();

private:

	std::auto_ptr<CSyncData> syncMLData;

	std::tstring host, service, user, pass, idPC, devtype, devid;
	int currentCommand, currentMessage;
	std::string session;
	unsigned maxMem; // KB

#ifdef _WIN32
	CRITICAL_SECTION cs;
#else
	pthread_mutex_t mt;
#endif

	CSyncData &data() { return *syncMLData; }

	int Sync(bool start, const std::string &service);

protected:
#ifdef _WIN32
	void Lock() { EnterCriticalSection(&cs); }
	void Unlock() { LeaveCriticalSection(&cs); }
#else
	void Lock() { pthread_mutex_lock(&mt); }
	void Unlock() { pthread_mutex_unlock(&mt); }
#endif

	typedef ::CAutoLock<CSyncML> CAutoLock;
	friend class ::CAutoLock<CSyncML>;
};

#endif // _SYNCML_H
