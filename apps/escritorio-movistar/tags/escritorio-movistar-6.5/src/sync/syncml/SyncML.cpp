
#include "stdafx.h"

#include "SyncML.h"
#include "vCard.h"

extern "C"
{
#include "sml.h"
#include "xpt.h"
#include "xpt-b64.h"
#include "smlmetinfdtd.h"
#include "xpt-http.h"
#include "libmem.h"
}

#include "ptools.h"
#include <sstream>
#include <list>
#ifdef _WIN32
#include <mmsystem.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

//<Put>
//  <CmdID>2</CmdID>
//  <Meta>
//    <Type xmlns='syncml:metinf'>application/vnd.syncml-devinf+xml</Type>
//  </Meta>
//  <Item>
//    <Source>
//      <LocURI>./devinf10</LocURI>
//    </Source>
//     "<Data>" 
//

#define SYNCML_DEF_INFO \
      "<DevInf xmlns='syncml:devinf'> " \
	    "<VerDTD>1.0</VerDTD>" \
        "<Man>Movistar</Man> " \
        "<Mod>EM</Mod> " \
        "<OEM>TID</OEM> " \
        "<FwV>6.1</FwV> " \
        "<SwV>6.1</SwV> " \
        "<HwV>6.1</HwV> " \
        "<DevID>$DEVID</DevID> " \
        "<DevTyp>$DEVTYP</DevTyp> " \
        "<DataStore> " \
          "<SourceRef>./local-pab</SourceRef> " \
          "<DisplayName>Phonebook</DisplayName> " \
          "<MaxGUIDSize>256</MaxGUIDSize> " \
          "<Rx-Pref> " \
            "<CTType>text/x-vcard </CTType> " \
            "<VerCT>2.1</VerCT> " \
          "</Rx-Pref> " \
          "<Tx-Pref> " \
            "<CTType>text/x-vcard</CTType> " \
            "<VerCT>2.1</VerCT> " \
          "</Tx-Pref> " \
          " <SyncCap> " \
            " <SyncType>01</SyncType> " \
            " <SyncType>02</SyncType> " \
            " <SyncType>05</SyncType> " \
            " <SyncType>06</SyncType> " \
          " </SyncCap> " \
        "</DataStore> " \
        "<CTCap> " \
          "<CTType>text/x-vcard</CTType> " \
          "<PropName>BEGIN</PropName> " \
          "<ValEnum>VCARD</ValEnum> " \
          "<PropName>END</PropName> " \
          "<ValEnum>VCARD</ValEnum> " \
          "<PropName>VERSION</PropName> " \
          "<ValEnum>2.1</ValEnum> " \
          "<PropName>N</PropName> " \
          "<PropName>TEL</PropName> " \
		  "<ParamName>MSG</ParamName> " \
		  "<ParamName>CELL</ParamName> " \
		  "<ParamName>VOICE</ParamName> " \
		  "<ParamName>FAX</ParamName> " \
          "<PropName>EMAIL</PropName> " \
		  "<ParamName>INTERNET</ParamName> " \
		  "<ParamName>HOME</ParamName> " \
		  "<ParamName>WORK</ParamName> " \
        "</CTCap> " \
      "</DevInf>"


          //"<ParamName>VOICE</ParamName> " 
          //"<ParamName>CELL</ParamName> " 
          //"<ParamName>WORK</ParamName> " 
          //"<ParamName>HOME</ParamName> " 

    //"</Data>" 
//  </Item>
//</Put>



// �apa:
extern "C" Ret_t mgrResetWorkspace (InstanceID_t id);

// otra �apa (para progreso):
extern "C"
{
#ifdef _WIN32
	CRITICAL_SECTION g_syncml_progress_cs;
#else
	pthread_mutex_t g_syncml_progress_mt;
#endif
	static int g_syncMLRecvPercent, g_syncMLSendPercent;

	void __syncml_setSendPercent(int p)
	{
#ifdef _WIN32
		EnterCriticalSection(&g_syncml_progress_cs);
		g_syncMLSendPercent=p;
		LeaveCriticalSection(&g_syncml_progress_cs);
#else
		pthread_mutex_lock(&g_syncml_progress_mt);
		g_syncMLSendPercent=p;
		pthread_mutex_unlock(&g_syncml_progress_mt);
#endif
	}

	void __syncml_setReceivePercent(int p)
	{
#ifdef _WIN32
		EnterCriticalSection(&g_syncml_progress_cs);
		g_syncMLRecvPercent=p;
		LeaveCriticalSection(&g_syncml_progress_cs);
#else
		pthread_mutex_lock(&g_syncml_progress_mt);
		g_syncMLRecvPercent=p;
		pthread_mutex_unlock(&g_syncml_progress_mt);
#endif
	}
	int __syncml_getSendPercent()
	{
#ifdef _WIN32
		EnterCriticalSection(&g_syncml_progress_cs);
		int p = g_syncMLSendPercent;
		LeaveCriticalSection(&g_syncml_progress_cs);
#else
		pthread_mutex_lock(&g_syncml_progress_mt);
		int p = g_syncMLSendPercent;
		pthread_mutex_unlock(&g_syncml_progress_mt);
#endif
		return p;
	}

	int __syncml_getReceivePercent()
	{
#ifdef _WIN32
		EnterCriticalSection(&g_syncml_progress_cs);
		int p = g_syncMLRecvPercent;
		LeaveCriticalSection(&g_syncml_progress_cs);
#else
		pthread_mutex_lock(&g_syncml_progress_mt);
		int p = g_syncMLRecvPercent;
		pthread_mutex_unlock(&g_syncml_progress_mt);
#endif
		return p;
	}
}

static Ret_t __DummyCmdFunc(InstanceID_t id, VoidPtr_t userData, VoidPtr_t pContent);
static Ret_t __StartMessageCmdFunc (InstanceID_t id, VoidPtr_t userData, SmlSyncHdrPtr_t pContent);
static Ret_t __AlertCmdFunc(InstanceID_t id, VoidPtr_t userData, SmlAlertPtr_t pContent);
static Ret_t __AddCmdFunc(InstanceID_t id, VoidPtr_t userData, SmlAddPtr_t pContent);
static Ret_t __ReplaceCmdFunc(InstanceID_t id, VoidPtr_t userData, SmlReplacePtr_t pContent);
static Ret_t __DeleteCmdFunc(InstanceID_t id, VoidPtr_t userData, SmlDeletePtr_t pContent);
static Ret_t __StatusCmdFunc(InstanceID_t id, VoidPtr_t userData, SmlStatusPtr_t pContent);


struct Reply
{
	virtual ~Reply() { }
	CSyncML::ReplyType type;
	Reply(CSyncML::ReplyType type): type(type) { }
};

struct SyncHdrReply: public Reply
{
	std::string target, source;
	std::string msg;
	bool noResp;
	SyncHdrReply(bool noResp, const std::string &target, const std::string &source, const std::string &msg):
		Reply(CSyncML::Reply_SyncHeader), noResp(noResp), target(target), source(source), msg(msg) { }
};

struct AlertReply: public Reply
{
	int alertCode;
	std::string last, next;
	std::string target, source;
	std::string msg, cmd;
	bool noResp;
	AlertReply(int alert, bool noResp, 
		const std::string &lastAnchor, const std::string &nextAnchor,
		const std::string &target, const std::string &source,
		const std::string &msg, const std::string &cmd):
		Reply(CSyncML::Reply_Alert), alertCode(alert), noResp(noResp), 
		target(target), source(source),
		last(lastAnchor), next(nextAnchor),
		msg(msg), cmd(cmd) { }
};

struct StatusReply: public Reply
{
	int statusCode;
	std::string targetRef, sourceRef;
	std::string msg, cmd;
	int msgRef, cmdRef;
	StatusReply(int statusCode, 
		const std::string &targetRef, const std::string &sourceRef,
		int msgRef, int cmdRef, const std::string &msg, const std::string &cmd):
		Reply(CSyncML::Reply_Status), statusCode(statusCode),
		targetRef(targetRef), sourceRef(sourceRef), 
		msgRef(msgRef), cmdRef(cmdRef), msg(msg), cmd(cmd) { }
};

struct AddReply: public Reply
{
	CVCard vcard;
	std::string msg, cmd;
	//std::string target, source;
	std::string sourceItem;
	AddReply(const CVCard &vcard, const std::string &sourceItem,
		const std::string &cmd, const std::string &msg):
		Reply(CSyncML::Reply_Add), vcard(vcard), sourceItem(sourceItem), cmd(cmd), msg(msg) { }
};

struct ReplaceReply: public Reply
{
	CVCard vcard;
	std::string msg, cmd;
	std::string targetItem;
	ReplaceReply(const CVCard &vcard, const std::string &targetItem,
		const std::string &cmd, const std::string &msg):
		Reply(CSyncML::Reply_Replace), vcard(vcard), targetItem(targetItem), cmd(cmd), msg(msg) { }
};

struct DeleteReply: public Reply
{
	std::string msg, cmd;
	std::string targetItem;
	DeleteReply(const std::string &targetItem,
		const std::string &cmd, const std::string &msg):
		Reply(CSyncML::Reply_Delete), targetItem(targetItem), cmd(cmd), msg(msg) { }
};

struct CSyncData
{
	CSyncData()
	{
		insID = 0;
		protInfo = 0;
		serviceId = 0;
		commId = 0;
		inMsg = false;
	}
	~CSyncData()
	{
		for (std::list<Reply *>::iterator it=replies.begin(); it != replies.end(); it++)
			delete *it;
		replies.clear();

	}
	// los datos de instancia de C Toolkit:
	InstanceID_t insID;
	const XptProtocolInfo *protInfo;
	XptServiceID_t serviceId;
	XptCommunicationID_t commId;
	bool inMsg;

	// la URI a usar para sesiones m�ltiples de intercambio de mensajes:
	std::string session;

	// los comandos de respuesta obtenidos del servidor para 
	// un intercambio:
	std::list<Reply *> replies;

	// aunque nosotros enviamos IDs de mensajes num�ricos, en teor�a 
	// se nos pueden devolver IDs alfanum�ricos:
	std::string curRecvMsg;

#ifdef _WIN32
	HANDLE heap;
#endif
};

CSyncML::CSyncML()
{
	syncMLData.reset(new CSyncData);
#ifdef _WIN32
	InitializeCriticalSection(&cs);
	InitializeCriticalSection(&g_syncml_progress_cs);
#else
	pthread_mutex_init(&mt, NULL);
	pthread_mutex_init(&g_syncml_progress_mt, NULL);
#endif
}

CSyncML::~CSyncML()
{
#ifdef _WIN32
	DeleteCriticalSection(&cs);
	DeleteCriticalSection(&g_syncml_progress_cs);
#else
	pthread_mutex_destroy(&mt);
	pthread_mutex_destroy(&g_syncml_progress_mt);
#endif
}

bool CSyncML::Init(unsigned mem) // KB
{
	ASSERT(data().insID == 0);
	if (data().insID != 0)
		End();

#ifdef _WIN32
	data().heap = HeapCreate(0, 0, 0);
	smlSetHeap(&data().heap);
#endif

	maxMem = mem;

	SmlOptions_t initOptions;
	initOptions.defaultPrintFunc = 0;
	initOptions.maxWorkspaceAvailMem = mem*1024;
	bool ok = smlInit(&initOptions)==SML_ERR_OK;

	SmlCallbacks_t callbacks;
	callbacks.addCmdFunc		= __AddCmdFunc;
	callbacks.alertCmdFunc		= __AlertCmdFunc;
	callbacks.copyCmdFunc		= (smlCopyCmdFunc)__DummyCmdFunc;
	callbacks.deleteCmdFunc		= __DeleteCmdFunc;
	callbacks.endAtomicFunc		= (smlEndAtomicFunc)__DummyCmdFunc;
	callbacks.endMessageFunc	= (smlEndMessageFunc)__DummyCmdFunc;
	callbacks.endSequenceFunc	= (smlEndSequenceFunc)__DummyCmdFunc;
	callbacks.endSyncFunc		= (smlEndSyncFunc)__DummyCmdFunc;
	callbacks.execCmdFunc		= (smlExecCmdFunc)__DummyCmdFunc;
	callbacks.getCmdFunc		= (smlGetCmdFunc)__DummyCmdFunc;
	callbacks.handleErrorFunc	= (smlHandleErrorFunc)__DummyCmdFunc;
	callbacks.mapCmdFunc		= (smlMapCmdFunc)__DummyCmdFunc;
	callbacks.putCmdFunc		= (smlPutCmdFunc)__DummyCmdFunc;
	callbacks.replaceCmdFunc	= __ReplaceCmdFunc;
	callbacks.resultsCmdFunc	= (smlResultsCmdFunc)__DummyCmdFunc;
	callbacks.searchCmdFunc		= (smlSearchCmdFunc)__DummyCmdFunc;
	callbacks.startAtomicFunc	= (smlStartAtomicFunc)__DummyCmdFunc;
	callbacks.startMessageFunc	= __StartMessageCmdFunc;
	callbacks.startSequenceFunc	= (smlStartSequenceFunc)__DummyCmdFunc;
	callbacks.startSyncFunc		= (smlStartSyncFunc)__DummyCmdFunc;
	callbacks.statusCmdFunc		= __StatusCmdFunc;
	callbacks.transmitChunkFunc	= (smlTransmitChunkFunc)__DummyCmdFunc;

	SmlInstanceOptions_t insOptions;
	insOptions.encoding = SML_XML;
	insOptions.workspaceName = "MyDB";
	insOptions.workspaceSize = mem*1024;

	ok = ok && smlInitInstance(&callbacks, &insOptions, (VoidPtr_t)&data(), &data().insID)==SML_ERR_OK;

	if (!ok)
	{
		smlTerminate();
		data().insID = 0;
	}
	else
	{
#ifdef _WIN32
		GUID guid;
		CoCreateGuid(&guid);

		char buff[512];
		sprintf(buff, "%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x",
				guid.Data1, guid.Data2, guid.Data3, 
				guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], 
				guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
		session = buff;
#else
		session = "";
#endif
		
		
	}

	currentMessage = 0;

	data().session = "";

	__syncml_setReceivePercent(0);
	__syncml_setSendPercent(0);

	return ok;
}

bool CSyncML::End()
{
	ASSERT(data().protInfo==0);
	if (data().protInfo!=0)
		Disconnect();

	if (data().insID==0)
		return false;

	xptCleanUp();

	smlTerminateInstance(data().insID);
	smlTerminate();

#ifdef _WIN32
	smlResetHeap();
	HeapDestroy(data().heap);
	data().heap = NULL;
#endif

	data().insID = 0;

	return true;
}

bool CSyncML::Connect(const std::tstring &host, const std::tstring &service, const std::tstring &idPC,
					const std::tstring &user, const std::tstring &pass)
{
	CAutoLock alock(*this);

	ASSERT(data().protInfo==0);
	if (data().protInfo!=0)
		Disconnect();

	bool ok;

	CSyncML::user = user;
	CSyncML::pass = pass;

	ok = xptGetProtocol("HTTP", &data().protInfo)==SML_ERR_OK;

	std::tstringstream ss;

	ss << "HOST=" << host;

	ok = ok && xptSelectProtocol(data().protInfo->id, toNarrowString(ss.str()).c_str(), XPT_CLIENT, &data().serviceId)==SML_ERR_OK;
	if (!ok)
	{
		data().serviceId = 0;
		data().protInfo = 0;
	}

	alock.Unlock();
	XptCommunicationID_t commId;
	ok = ok && xptOpenCommunication(data().serviceId, XPT_REQUEST_SENDER, &commId)==SML_ERR_OK;
	alock.Lock();
	data().commId = commId;
	if (!ok)
	{
		if (data().serviceId!=0)
			xptDeselectProtocol(data().serviceId);
		data().protInfo = 0;
		data().commId = 0;
	}

	CSyncML::host = host;
	CSyncML::service = service;
	CSyncML::idPC = idPC;

	return ok;
}

bool CSyncML::Disconnect()
{
	CAutoLock alock(*this);

	mgrResetWorkspace(data().insID); // �apa para poder re-utilizar instancias de SyncML Toolkit

	xptCloseCommunication(data().commId);

	xptDeselectProtocol(data().serviceId);

	data().serviceId = 0;
	data().commId = 0;
	data().protInfo = 0;

	return true;
}

void CSyncML::Interrupt()
{
	CAutoLock alock(*this);
	if (data().serviceId!=0 && data().commId!=0)
		xptCancelCommunicationAsync(data().serviceId, data().commId);
}

int CSyncML::StartMessage(const std::tstring &devtype, const std::tstring &devid)
{
	ASSERT(!data().inMsg);
	if (data().inMsg)
		EndMessage();

	bool ok;

	CSyncML::devtype = devtype;
	CSyncML::devid = devid;

	ok = xptBeginExchange(data().commId)==SML_ERR_OK;

	SmlSyncHdr_t syncHdr;
	SmlSource_t sourceMsg;
	SmlTarget_t targetMsg;

	std::stringstream ss;

	syncHdr.elementType = SML_PE_HEADER;
	syncHdr.version = smlString2Pcdata("1.0");
	syncHdr.proto = smlString2Pcdata("SyncML/1.0");
	syncHdr.sessionID = smlString2Pcdata((String_t)session.c_str());	
	syncHdr.respURI = NULL;

	ss.str("");
	
#ifdef _WIN32
	ss<<(int)(timeGetTime()&0x00FFFF);
#else
	ss<<(int)(clock()&0x00FFFF);
#endif	
	
	syncHdr.sessionID = smlString2Pcdata((String_t)ss.str().c_str());

	ss.str("");
	ss << ++currentMessage;
	syncHdr.msgID = smlString2Pcdata((String_t)ss.str().c_str());

	syncHdr.flags = 0; // SmlNoResp_f;

	ss.str("");
	ss << "http://" << toNarrowString(host) << "/" << toNarrowString(service);

	targetMsg.locURI = smlString2Pcdata((String_t)ss.str().c_str()); // "http://localhost:8080/funambol");
	targetMsg.locName = NULL; // smlString2Pcdata("SyncML Server");

	ss.str("");
	// ss << "EM:" << toNarrowString(idPC);
	// ss << "prueba_devid"; // "IMEI:00440015202000000";
	ss << toNarrowString(devid);
	sourceMsg.locURI =  smlString2Pcdata((String_t)ss.str().c_str());
	sourceMsg.locName = NULL; // smlString2Pcdata("MyDB");

	syncHdr.target = &targetMsg;
	syncHdr.source = &sourceMsg;

	syncHdr.respURI = NULL;
	ss.str("");
	ss << "<MaxMsgSize xmlns='syncml:metinf'>" << maxMem*1024 << "</MaxMsgSize>";
	syncHdr.meta = smlString2Pcdata((String_t)ss.str().c_str()); // NULL;
	syncHdr.cred = NULL;

	 // preparar credenciales:
	if (user!=_T(""))
	{
		SmlCred_t cred;
		cred.meta = smlString2Pcdata("<Type xmlns='syncml:metinf'>syncml:auth-basic</Type>");

		std::tstringstream ss;
		ss<<user<<_T(':')<<pass;
		BufferSize_t encodedLen = base64GetSize(ss.str().length());
		DataBuffer_t encodedAuth = (DataBuffer_t)new char[encodedLen];
		BufferSize_t authLen = ss.str().length();
		DataBuffer_t auth = (DataBuffer_t)new char[authLen];
		std::string authNarrow = toNarrowString(ss.str().c_str());
		strncpy((char *)auth, authNarrow.c_str(), authLen);
		BufferSize_t off;
		unsigned char save[3]={0};

		BufferSize_t encodedLen2 = base64Encode(encodedAuth, encodedLen, auth, &authLen, &off, 1, save);
		
		std::string auth64((char *)encodedAuth, encodedLen2);

		cred.data = smlString2Pcdata((String_t)auth64.c_str());
		syncHdr.cred = &cred;

		delete [] (char *)encodedAuth;
		delete [] (char *)auth;
	}
	else
		syncHdr.cred = NULL;

	ok = ok && smlStartMessageExt(data().insID, &syncHdr, SML_VERS_1_0)==SML_ERR_OK;

	smlFreePcdata(syncHdr.version);
	smlFreePcdata(syncHdr.proto);
	smlFreePcdata(syncHdr.sessionID);
	smlFreePcdata(syncHdr.msgID);
	smlFreePcdata(targetMsg.locURI);
	smlFreePcdata(sourceMsg.locURI);
	smlFreePcdata(syncHdr.meta);
	if (syncHdr.cred!=NULL)
	{
		smlFreePcdata(syncHdr.cred->meta);
		smlFreePcdata(syncHdr.cred->data);
	}

	currentCommand = 0;

	if (ok)
		data().inMsg = true;
	else
		--currentMessage;

	return ok? currentMessage:-1;
}

int CSyncML::EndMessage()
{
	bool ok;

	data().inMsg = false;

	ok = smlEndMessage(data().insID, true)==SML_ERR_OK;

	if (!ok)
		return -1;

	MemPtr_t readPos;
	MemSize_t size;
	bool lock = (ok = ok && smlLockReadBuffer(data().insID, &readPos, &size)==SML_ERR_OK);

	XptCommunicationInfo_t commInfo;
	commInfo.cbSize = sizeof(commInfo);
	commInfo.cbLength = size;
	strcpy(commInfo.mimeType, "application/vnd.syncml+xml");
	// strcpy(commInfo.docName, toNarrowString(service).c_str());
	std::string respURI;
	if (data().session != "")
		respURI = data().session;
	else
		respURI = toNarrowString(service);
	strcpy(commInfo.docName, toNarrowString(respURI).c_str());

	commInfo.hmacInfo = NULL;
	commInfo.auth = NULL;

	ok = ok && xptSetDocumentInfo(data().commId, &commInfo)==SML_ERR_OK;

	// problema con el toolkit:
	// copiamos nuestros datos ya que, aunque hemos bloqueado el buffer como lectura unicamente,
	// el Toolkit modifica el buffer si �ste es superior a 2000 bytes:
	// en principio esto nos afecta en que no podemos sacar por debug la cadena enviada correctamente,
	// aunque estrictamente es un error de la implementaci�n del protocolo HTTP SyncML,
	// ya que estos datos no los deberia modificar (entre otras cosas porque el par�metro es 
	// (const void *) ).
	//
	char *dataToSend = new char[size];
	memcpy(dataToSend, readPos, size);
	size_t sent = 0;
	ok = ok && xptSendData(data().commId, dataToSend, size, &sent)==SML_ERR_OK; // readPos, size, &sent)==SML_ERR_OK;
	delete [] dataToSend;

#ifdef DEBUG
	{
		TRACE("\n***********************************\n");
		TRACE("              SENT\n");
		TRACE("***********************************\n");
		for (int c=0; c<(int)sent; c+=64)
		{
			std::string s((char *)readPos+c, min(64,(int)sent-c)); // received);
			for (int c=0; c<s.length(); c++)
				if (s[c]=='%')
					s.insert(c++, "%");
			TRACE(s.c_str());
		}
		TRACE("\n***********************************\n");
	}
#endif

	ok = ok && xptSendComplete(data().commId)==SML_ERR_OK;

	ok = lock && smlUnlockReadBuffer(data().insID, size)==SML_ERR_OK && ok;

	XptCommunicationInfo_t commInfo2;
	commInfo2.cbSize = sizeof(commInfo2);

	ok = ok && xptGetDocumentInfo(data().commId, &commInfo2)==SML_ERR_OK;

	size_t received = 0;

#ifdef DEBUG
	{
		TRACE("\n***********************************\n");
		TRACE("            RECEIVED\n");
		TRACE("***********************************\n");
	}
#endif

	bool okRec;
	unsigned totalReceived=0;

	__syncml_setReceivePercent(0);

	do
	{
		ok = ok && (lock = smlLockWriteBuffer(data().insID, &readPos, &size)==SML_ERR_OK);

		received = 0;
		Ret_t ret = ok? xptReceiveData(data().commId, readPos, size, &received):SML_ERR_UNSPECIFIC;

		if (ret==SML_ERR_A_XPT_INVALID_STATE)
		{
			received = 0;
			okRec = false;
		}
		else
			okRec = (ok = ok && ret==SML_ERR_OK) && received!=0;

		totalReceived+=received;

		if (commInfo2.cbLength!=-1)
			__syncml_setReceivePercent(commInfo2.cbLength!=0? totalReceived*100/commInfo2.cbLength:0);

#ifdef DEBUG
	if (ok)
	{
		for (int c=0; c<(int)received; c+=64)
		{
			std::string s((char *)readPos+c, min(64,(int)received-c));
			for (int c=0; c<s.length(); c++)
				if (s[c]=='%')
					s.insert(c++, "%");
			TRACE(s.c_str());
		}
	}
#endif

		ok = lock && smlUnlockWriteBuffer(data().insID, received)==SML_ERR_OK && ok;
	} while (okRec);

	__syncml_setReceivePercent(100);

#ifdef DEBUG
	{
		TRACE("\n***********************************\n");
	}
#endif

	ok = ok && smlProcessData(data().insID, SML_ALL_COMMANDS)==SML_ERR_OK;

	ok = ok && xptEndExchange(data().commId)==SML_ERR_OK;

	return ok? currentMessage:-1;
}

int CSyncML::GetReceivePercent()
{
	return __syncml_getReceivePercent();
}

int CSyncML::GetSendPercent()
{
	return __syncml_getSendPercent();
}

int CSyncML::Alert(int alertCode, const std::tstring &service, const std::tstring &lastAnchor, const std::tstring &nextAnchor)
{
	std::stringstream ss;
	ss<<alertCode;
	std::string cmd = ss.str();

	SmlAlert_t alert;
	alert.elementType = SML_PE_ALERT;

	ss.str("");
	ss << ++currentCommand;
	alert.cmdID = smlString2Pcdata((String_t)ss.str().c_str());
	alert.flags = 0; // SmlNoResp_f;
	alert.cred = NULL;
	alert.data = smlString2Pcdata((String_t)cmd.c_str());
	alert.itemList = NULL;

	SmlItemList_t list;
	SmlItem_t item;

	SmlTarget_t targetList;
	SmlSource_t sourceList;
	targetList.locURI = smlString2Pcdata((String_t)toNarrowString(service).c_str()); // contacts_server");
	targetList.locName = NULL; // smlString2Pcdata("Sync Server DB");
	sourceList.locURI = smlString2Pcdata("./local-pab");
	sourceList.locName = NULL; // smlString2Pcdata("Sync Client DB");
	item.source = &sourceList;
	item.target = &targetList;
	item.data = NULL;
	item.flags = SmlMoreData_f;

	std::string last, next;
	last=toNarrowString(lastAnchor);
	next=toNarrowString(nextAnchor);

	SmlMetInfAnchor_t anchor;
	anchor.last = last!=""? smlString2Pcdata((String_t)last.c_str()):NULL;
	anchor.next = next!=""? smlString2Pcdata((String_t)next.c_str()):NULL;
	SmlMetInfMetInf_t meta;
	meta.format = NULL;
	meta.type = NULL;
	meta.mark = NULL;
	meta.size = NULL;
	meta.nextnonce = NULL;
	meta.version = NULL;
	meta.maxmsgsize = NULL;
	meta.maxobjsize = NULL;
	meta.mem = NULL;
	meta.emi = NULL;
	meta.anchor = &anchor;

	SmlPcdata_t metaData;
	metaData.contentType = SML_PCDATA_EXTENSION;
	metaData.extension = SML_EXT_METINF;
	metaData.content = &meta;
	metaData.length = sizeof(meta);
	item.meta = last!="" || next!=""? &metaData:NULL;

	list.item = &item;
	list.next = NULL;
	
	alert.itemList = &list;

	bool ok = smlAlertCmd(data().insID, &alert)==SML_ERR_OK;

	smlFreePcdata(alert.cmdID);
	smlFreePcdata(alert.data);
	smlFreePcdata(targetList.locURI);
	smlFreePcdata(sourceList.locURI);
	smlFreePcdata(anchor.last);
	smlFreePcdata(anchor.next);

	if (!ok)
		--currentCommand;

	return ok? currentCommand:-1;
}

int CSyncML::Sync(const std::tstring &service)
{
	Sync(true, toNarrowString(service));
	return Sync(false, toNarrowString(service));
}

int CSyncML::Sync(bool start, const std::string &service)
{
	bool ok;
	if (start)
	{
		std::stringstream ss;

		SmlSync_t sync;
		sync.elementType = SML_PE_SYNC_START;
		ss.str("");
		ss << ++currentCommand;
		sync.cmdID = smlString2Pcdata((String_t)ss.str().c_str());
		sync.flags = 0; // SmlNoResp_f;
		sync.cred = NULL;

		SmlTarget_t target;
		SmlSource_t source;
		target.locURI = smlString2Pcdata((String_t)service.c_str()); // contacts_server");
		target.locName = NULL; // smlString2Pcdata("Sync Server DB");
		source.locURI = smlString2Pcdata("./local-pab");
		source.locName = NULL; // smlString2Pcdata("Sync Client DB");
		sync.source = &source;
		sync.target = &target;

		sync.meta = NULL;
		sync.noc = NULL;

		ok = smlStartSync(data().insID, &sync)==SML_ERR_OK;

		smlFreePcdata(sync.cmdID);
		smlFreePcdata(target.locURI);
		smlFreePcdata(source.locURI);

		if (!ok)
			--currentCommand;
	}
	else
		ok = smlEndSync(data().insID)==SML_ERR_OK;
	return ok? currentCommand:-1;
}

int CSyncML::Status(int statusCode, const std::tstring &msgRef, const std::tstring &cmdRef,
				   const std::tstring &targetRef, const std::tstring &sourceRef, const std::tstring &cmd,
				   const std::tstring &nextAnchor)
{
	SmlStatus_t status;

	std::stringstream ss;
	status.elementType = SML_PE_STATUS;
	ss.str("");
	ss << ++currentCommand;
	status.cmdID = smlString2Pcdata((String_t)ss.str().c_str());
	status.msgRef = smlString2Pcdata((String_t)toNarrowString(msgRef).c_str());
	status.cmdRef = smlString2Pcdata((String_t)toNarrowString(cmdRef).c_str());

	SmlTargetRefList_t target;
	SmlSourceRefList_t source;
	target.targetRef = targetRef!=_T("")? smlString2Pcdata((String_t)toNarrowString(targetRef).c_str()):NULL;
	target.next = NULL;
	source.sourceRef = sourceRef!=_T("")? smlString2Pcdata((String_t)toNarrowString(sourceRef).c_str()):NULL;
	source.next = NULL;
	status.targetRefList = targetRef!=_T("")? &target:NULL;
	status.sourceRefList = sourceRef!=_T("")? &source:NULL;

	status.cmd = smlString2Pcdata((String_t)toNarrowString(cmd).c_str());

	status.cred = NULL;
	status.chal = NULL;

	ss.str("");
	ss << statusCode;
	status.data = smlString2Pcdata((String_t)ss.str().c_str());

	SmlItemList_t itemList;
	SmlItem_t item;
	item.source = NULL;
	item.target = NULL;
	item.flags = 0;
	item.meta = NULL;

	SmlMetInfAnchor_t anchor;
	anchor.last = NULL;
	anchor.next = smlString2Pcdata((String_t)toNarrowString(nextAnchor).c_str());
	SmlMetInfMetInf_t meta;
	meta.format = NULL;
	meta.type = NULL;
	meta.mark = NULL;
	meta.size = NULL;
	meta.nextnonce = NULL;
	meta.version = NULL;
	meta.maxmsgsize = NULL;
	meta.maxobjsize = NULL;
	meta.mem = NULL;
	meta.emi = NULL;
	meta.anchor = &anchor;

	SmlPcdata_t metaData;
	metaData.contentType = SML_PCDATA_EXTENSION;
	metaData.extension = SML_EXT_METINF;
	metaData.content = &meta;
	metaData.length = sizeof(meta);
	item.data = &metaData;

	itemList.item = nextAnchor==_T("")? NULL:&item;
	itemList.next = NULL;

	status.itemList = &itemList; 

	bool ok = smlStatusCmd(data().insID, &status)==SML_ERR_OK;

	smlFreePcdata(status.cmdID);
	smlFreePcdata(status.msgRef);
	smlFreePcdata(status.cmdRef);
	if (target.targetRef!=NULL)
		smlFreePcdata(target.targetRef);
	if (source.sourceRef)
		smlFreePcdata(source.sourceRef);
	smlFreePcdata(status.cmd);
	smlFreePcdata(status.data);
	smlFreePcdata(anchor.next);

	if (!ok)
		--currentCommand;

	return ok? currentCommand:-1;
}

int CSyncML::Map(const std::tstring &target, const std::tstring &source,
				const std::tstring &sourceItem, const std::tstring &targetItem)
{
	std::stringstream ss;

	SmlMap_t map;

	map.elementType = SML_PE_MAP;

	ss.str("");
	ss << ++currentCommand;
	map.cmdID = smlString2Pcdata((String_t)ss.str().c_str());

	SmlTarget_t targ;
	SmlSource_t sour;
	targ.locURI = smlString2Pcdata((String_t)toNarrowString(target).c_str());
	targ.locName = NULL; // smlString2Pcdata("");
	sour.locURI = smlString2Pcdata((String_t)toNarrowString(source).c_str());
	sour.locName = NULL; // smlString2Pcdata("");

	map.target = &targ;
	map.source = &sour;

	map.cred = NULL;
	map.meta = NULL;

	SmlMapItemList_t itemList;
	SmlMapItem_t item;
	itemList.mapItem = &item;
	itemList.next = NULL;

	SmlTarget_t targItem;
	SmlSource_t sourItem;
	targItem.locURI = smlString2Pcdata((String_t)toNarrowString(targetItem).c_str());
	targItem.locName = NULL; // smlString2Pcdata(""); // NULL;
	sourItem.locURI = smlString2Pcdata((String_t)toNarrowString(sourceItem).c_str());
	sourItem.locName = NULL; // smlString2Pcdata(""); // NULL;

	item.target = &targItem;
	item.source = &sourItem;

	map.mapItemList = &itemList;

	bool ok = smlMapCmd(data().insID, &map)==SML_ERR_OK;

	smlFreePcdata(map.cmdID);
	smlFreePcdata(targ.locURI);
	smlFreePcdata(sour.locURI);
	smlFreePcdata(targItem.locURI);
	smlFreePcdata(sourItem.locURI);

	if (!ok)
		--currentCommand;

	return ok? currentCommand:-1;
}

int CSyncML::Put(const std::tstring &dataStr)
{
	std::stringstream ss;

	SmlPut_t put;
	put.elementType = SML_PE_PUT;

	ss.str("");
	ss << ++currentCommand;
	put.cmdID = smlString2Pcdata((String_t)ss.str().c_str());
	put.flags = 0; // NoResp
	put.lang = NULL;
	put.cred = NULL;
	put.meta = smlString2Pcdata("<Type xmlns='syncml:metinf'>application/vnd.syncml-devinf+xml</Type>");;

	SmlItemList_t itemList;
	SmlItem_t item;

	SmlSource_t source;
	source.locURI = smlString2Pcdata("./devinf10");
	source.locName = NULL;
	item.source = &source;
	item.target = NULL;
	item.data = smlString2Pcdata((String_t)toNarrowString(dataStr).c_str());
	item.flags = 0;
	item.meta = NULL;

	itemList.item = &item;
	itemList.next = NULL;

	put.itemList = &itemList;

	bool ok = smlPutCmd(data().insID, &put)==SML_ERR_OK;

	smlFreePcdata(put.cmdID);
	smlFreePcdata(put.meta);
	smlFreePcdata(source.locURI);
	smlFreePcdata(item.data);

	if (!ok)
		--currentCommand;

	return ok? currentCommand:-1;
}

int CSyncML::PutDevInfo(const std::tstring &data)
{
	if (data==_T(""))
	{
		std::tstring devid(toTString(SYNCML_DEF_INFO));
		const std::tstring tagDevId=_T("$DEVID");
		const std::tstring tagDevTyp=_T("$DEVTYP");
		std::tstring::size_type pos;

		pos=devid.find(tagDevId);
		if (pos!=std::tstring::npos)
		{
			devid.erase(devid.begin()+pos, devid.begin()+pos+tagDevId.length());
			devid.insert(devid.begin()+pos, CSyncML::devid.begin(), CSyncML::devid.end());
		}
		pos=devid.find(tagDevTyp);
		if (pos!=std::tstring::npos)
		{
			devid.erase(devid.begin()+pos, devid.begin()+pos+tagDevTyp.length());
			devid.insert(devid.begin()+pos, CSyncML::devtype.begin(), CSyncML::devtype.end());
		}

		return Put(devid);
	}
	return Put(data);
}

bool CSyncML::haveReply()
{
	return data().replies.size()>0;
}

void CSyncML::removeFirstReply()
{
	if (data().replies.size()>0)
	{
		delete data().replies.front();
		data().replies.erase(data().replies.begin());
	}
}

CSyncML::ReplyType CSyncML::getFirstReplyType()
{
	ASSERT(data().replies.size()>0);
	return data().replies.front()->type;
}

CSyncML::SyncHeaderReply CSyncML::getFirstReplySyncHeader()
{
	ASSERT(data().replies.size()>0 &&
		data().replies.front()->type==CSyncML::Reply_SyncHeader);
	SyncHeaderReply sync;
	sync.noResp = ((::SyncHdrReply *)data().replies.front())->noResp;
	sync.msg = toTString(((::SyncHdrReply *)data().replies.front())->msg);
	sync.target = toTString(((::SyncHdrReply *)data().replies.front())->target);
	sync.source = toTString(((::SyncHdrReply *)data().replies.front())->source);
	return sync;	
}

CSyncML::AlertReply CSyncML::getFirstReplyAlert()
{
	ASSERT(data().replies.size()>0 &&
		data().replies.front()->type==CSyncML::Reply_Alert);
	AlertReply alert;
	alert.code = ((::AlertReply *)data().replies.front())->alertCode;
	alert.lastAnchor = toTString(((::AlertReply *)data().replies.front())->last);
	alert.nextAnchor = toTString(((::AlertReply *)data().replies.front())->next);
	alert.noResp = ((::AlertReply *)data().replies.front())->noResp;
	alert.cmd = toTString(((::AlertReply *)data().replies.front())->cmd);
	alert.msg = toTString(((::AlertReply *)data().replies.front())->msg);
	alert.target = toTString(((::AlertReply *)data().replies.front())->target);
	alert.source = toTString(((::AlertReply *)data().replies.front())->source);
	return alert;	
}

CSyncML::StatusReply CSyncML::getFirstReplyStatus()
{
	ASSERT(data().replies.size()>0 &&
		data().replies.front()->type==CSyncML::Reply_Status);
	StatusReply status;
	status.code = ((::StatusReply *)data().replies.front())->statusCode;
	status.targetRef = toTString(((::StatusReply *)data().replies.front())->targetRef);
	status.sourceRef = toTString(((::StatusReply *)data().replies.front())->sourceRef);
	status.msgRef = ((::StatusReply *)data().replies.front())->msgRef;
	status.cmdRef = ((::StatusReply *)data().replies.front())->cmdRef;
	return status;	
}

CSyncML::AddReply CSyncML::getFirstReplyAdd()
{
	ASSERT(data().replies.size()>0 &&
		data().replies.front()->type==CSyncML::Reply_Add);
	AddReply add;
	add.vcard = ((::AddReply *)data().replies.front())->vcard;
	add.sourceItem = toTString(((::AddReply *)data().replies.front())->sourceItem);
	add.cmd = toTString(((::AddReply *)data().replies.front())->cmd);
	add.msg = toTString(((::AddReply *)data().replies.front())->msg);
	return add;	
}

CSyncML::ReplaceReply CSyncML::getFirstReplyReplace()
{
	ASSERT(data().replies.size()>0 &&
		data().replies.front()->type==CSyncML::Reply_Replace);
	ReplaceReply replace;
	replace.vcard = ((::ReplaceReply *)data().replies.front())->vcard;
	replace.targetItem = toTString(((::ReplaceReply *)data().replies.front())->targetItem);
	replace.cmd = toTString(((::ReplaceReply *)data().replies.front())->cmd);
	replace.msg = toTString(((::ReplaceReply *)data().replies.front())->msg);
	return replace;	
}

CSyncML::DeleteReply CSyncML::getFirstReplyDelete()
{
	ASSERT(data().replies.size()>0 &&
		data().replies.front()->type==CSyncML::Reply_Delete);
	DeleteReply del;
	del.targetItem = toTString(((::DeleteReply *)data().replies.front())->targetItem);
	del.cmd = toTString(((::DeleteReply *)data().replies.front())->cmd);
	del.msg = toTString(((::DeleteReply *)data().replies.front())->msg);
	return del;	
}

static Ret_t __DummyCmdFunc(InstanceID_t id, VoidPtr_t userData, VoidPtr_t pContent)
{
	return SML_ERR_OK;
}

static Ret_t __StartMessageCmdFunc (InstanceID_t id, VoidPtr_t userData, SmlSyncHdrPtr_t pContent)
{
	CSyncData *sd= (CSyncData *)userData;
	const char *str = 0;
	sd->curRecvMsg = (str = smlPcdata2String(pContent->msgID));
	if (str!=0)
	{
		smlLibFree((void *)str);
		str=0;
	}
	sd->session = pContent->respURI!=NULL? (str = smlPcdata2String(pContent->respURI)):"";
	if (str!=0)
	{
		smlLibFree((void *)str);
		str=0;
	}

	//int pos = sd->session.find("jsessionid=");
	//if (pos!=std::string::npos)
	//{
	//	std::string sss;
	//	sss=sd->session.substr(pos+strlen("jsessionid="));
	//	sd->session=sss;
	//}

	std::string target;
	std::string source;
	if (pContent->target != NULL)
	{
		target = (str = smlPcdata2String(pContent->target->locURI));
		if (str!=0)
		{
			smlLibFree((void *)str);
			str=0;
		}
	}
	if (pContent->source != NULL)
	{
		source = (str = smlPcdata2String(pContent->source->locURI));
		if (str!=0)
		{
			smlLibFree((void *)str);
			str=0;
		}
	}

	bool noResp = (pContent->flags&SmlNoResp_f)!=0;

	sd->replies.push_back(new SyncHdrReply(noResp, target, source, sd->curRecvMsg));

	return SML_ERR_OK;
}

static Ret_t __AlertCmdFunc(InstanceID_t id, VoidPtr_t userData, SmlAlertPtr_t pContent)
{
	CSyncData *sd= (CSyncData *)userData;

	String_t str = 0;
	std::string codeStr = (str = smlPcdata2String(pContent->data));
	if (str!=0)
	{
		smlLibFree((void *)str);
		str=0;
	}
	std::string cmdStr = (str = smlPcdata2String(pContent->cmdID));
	if (str!=0)
	{
		smlLibFree((void *)str);
		str=0;
	}

	SmlItemListPtr_t itemList = pContent->itemList;
	ASSERT(itemList->next == NULL && itemList->item != NULL); // nosotros solamente enviamos un item, debe devolverse 1 tb.

	std::string last, next, target, source;

	if (itemList->item!=NULL)
	{
		if (itemList->item->target!=NULL)
		{
			target = (str = smlPcdata2String(itemList->item->target->locURI));
			if (str!=0)
			{
				smlLibFree((void *)str);
				str=0;
			}
		}
		if (itemList->item->source!=NULL)
		{
			source = (str = smlPcdata2String(itemList->item->source->locURI));
			if (str!=0)
			{
				smlLibFree((void *)str);
				str=0;
			}
		}
	}

	if (itemList->item != NULL && itemList->item->meta != 0)
	{
		ASSERT(itemList->item->meta->contentType == SML_PCDATA_EXTENSION && 
			itemList->item->meta->extension == SML_EXT_METINF && 
			itemList->item->meta->content != NULL);

		SmlMetInfMetInfPtr_t meta = (SmlMetInfMetInfPtr_t)itemList->item->meta->content;

		if (meta->anchor != NULL)
		{
			SmlMetInfAnchorPtr_t anchor = meta->anchor;

			if (anchor->last != NULL)
			{
				last = (str = smlPcdata2String(anchor->last));
				if (str!=0)
				{
					smlLibFree((void *)str);
					str=0;
				}
			}
			if (anchor->next != NULL)
			{
				next = (str = smlPcdata2String(anchor->next));
				if (str!=0)
				{
					smlLibFree((void *)str);
					str=0;
				}
			}
		}
	}

	bool noResp = (pContent->flags&SmlNoResp_f)!=0;

	sd->replies.push_back(new AlertReply(atoi(codeStr.c_str()), noResp, last, next, target, source, sd->curRecvMsg, cmdStr));

	return SML_ERR_OK;
}

static Ret_t __AddCmdFunc(InstanceID_t id, VoidPtr_t userData, SmlAddPtr_t pContent)
{
	CSyncData *sd= (CSyncData *)userData;

	String_t str = 0;
	std::string cmdStr = (str = smlPcdata2String(pContent->cmdID));
	if (str!=0)
	{
		smlLibFree((void *)str);
		str=0;
	}

	for (SmlItemListPtr_t item = pContent->itemList; item != NULL; item = item->next)
	{
		SmlItemPtr_t it = item->item;

		std::string sourceNarrow = (str = smlPcdata2String(it->source->locURI));
		if (str!=0)
		{
			smlLibFree((void *)str);
			str=0;
		}
		// std::tstring source = toTString(sourceNarrow);

		std::string dataNarrow = (str = smlPcdata2String(it->data));
		if (str!=0)
		{
			smlLibFree((void *)str);
			str=0;
		}
		std::tstring data = toTString(dataNarrow);

		CVCard card;
		card.fromString(data);

		sd->replies.push_back(new AddReply(card, sourceNarrow, cmdStr, sd->curRecvMsg));
	}

	return SML_ERR_OK;
}

static Ret_t __ReplaceCmdFunc(InstanceID_t id, VoidPtr_t userData, SmlReplacePtr_t pContent)
{
	CSyncData *sd= (CSyncData *)userData;

	String_t str = 0;
	std::string cmdStr = (str = smlPcdata2String(pContent->cmdID));
	if (str!=0)
	{
		smlLibFree((void *)str);
		str=0;
	}

	for (SmlItemListPtr_t item = pContent->itemList; item != NULL; item = item->next)
	{
		SmlItemPtr_t it = item->item;

		std::string targetNarrow = (str = smlPcdata2String(it->target->locURI));
		if (str!=0)
		{
			smlLibFree((void *)str);
			str=0;
		}

		std::string dataNarrow = (str = smlPcdata2String(it->data));
		if (str!=0)
		{
			smlLibFree((void *)str);
			str=0;
		}
		std::tstring data = toTString(dataNarrow);

		CVCard card;
		card.fromString(data);

		sd->replies.push_back(new ReplaceReply(card, targetNarrow, cmdStr, sd->curRecvMsg));
	}

	return SML_ERR_OK;
}

static Ret_t __DeleteCmdFunc(InstanceID_t id, VoidPtr_t userData, SmlDeletePtr_t pContent)
{
	CSyncData *sd= (CSyncData *)userData;

	String_t str = 0;
	std::string cmdStr = (str = smlPcdata2String(pContent->cmdID));
	if (str!=0)
	{
		smlLibFree((void *)str);
		str=0;
	}

	for (SmlItemListPtr_t item = pContent->itemList; item != NULL; item = item->next)
	{
		SmlItemPtr_t it = item->item;

		std::string targetNarrow = (str = smlPcdata2String(it->target->locURI));
		if (str!=0)
		{
			smlLibFree((void *)str);
			str=0;
		}

		std::string dataNarrow = (str = smlPcdata2String(it->data));
		if (str!=0)
		{
			smlLibFree((void *)str);
			str=0;
		}
		std::tstring data = toTString(dataNarrow);

		sd->replies.push_back(new DeleteReply(targetNarrow, cmdStr, sd->curRecvMsg));
	}

	return SML_ERR_OK;
}

static Ret_t __StatusCmdFunc(InstanceID_t id, VoidPtr_t userData, SmlStatusPtr_t pContent)
{
	CSyncData *sd= (CSyncData *)userData;

	String_t str = 0;
	std::string codeStr = (str = smlPcdata2String(pContent->data));
	if (str!=0)
	{
		smlLibFree((void *)str);
		str=0;
	}
	std::string cmdStr = (str = smlPcdata2String(pContent->cmdID));
	if (str!=0)
	{
		smlLibFree((void *)str);
		str=0;
	}
	std::string msgRef = (str = smlPcdata2String(pContent->msgRef));
	if (str!=0)
	{
		smlLibFree((void *)str);
		str=0;
	}
	std::string cmdRef = (str = smlPcdata2String(pContent->cmdRef));
	if (str!=0)
	{
		smlLibFree((void *)str);
		str=0;
	}

	std::string targetRef;
	std::string sourceRef;
	if (pContent->targetRefList != NULL && 
		pContent->targetRefList->targetRef != NULL)
	{
		targetRef = (str = smlPcdata2String(pContent->targetRefList->targetRef));
		if (str!=0)
		{
			smlLibFree((void *)str);
			str=0;
		}
	}
	if (pContent->sourceRefList != NULL && 
		pContent->sourceRefList->sourceRef != NULL)
	{
		sourceRef = (str = smlPcdata2String(pContent->sourceRefList->sourceRef));
		if (str!=0)
		{
			smlLibFree((void *)str);
			str=0;
		}
	}

	sd->replies.push_back(new StatusReply(atoi(codeStr.c_str()), targetRef, sourceRef, atoi(msgRef.c_str()), atoi(cmdRef.c_str()), sd->curRecvMsg, cmdStr));

	return SML_ERR_OK;
}

