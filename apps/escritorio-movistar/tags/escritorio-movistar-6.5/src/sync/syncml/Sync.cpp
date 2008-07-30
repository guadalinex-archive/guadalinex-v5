
#include "stdafx.h"

#include "Sync.h"
#include "SyncML.h"
#ifdef _WIN32
#include <process.h>
#endif
#include <sstream>
#include "ptools.h"
// #ifdef _WIN32
#include "AgendaLib.h"
// #endif
// #include "EMAddressBook.h"

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

bool CSync::Start(
	UpdateMode mode,
	bool forceFullDownload,
    const std::tstring addressBookPath,
	const std::tstring &host, 
	const std::tstring &service,
	const std::tstring &db,
	const std::tstring &idPC,
	const std::tstring &user,
	const std::tstring &password,
	const std::tstring &telefono)
{
	if (Running())
		Stop();
	CAutoLock alock(*this);

	//CSync::updateMode = mode;

	CSync::updateMode = Update_Always;

	CSync::forceFull = forceFullDownload;
	CSync::addressBookPath = toWideString(addressBookPath);
	CSync::host = host;
	CSync::service = service;
	CSync::db = db;
	CSync::idPC = idPC;
	CSync::user = user;
	CSync::password = password;
	CSync::telefono = telefono;

	faseSync = 0;

	doShutdown = false;
	_SetError(Error_None);
	_SetState(State_Connecting);
	
#ifdef _WIN32
	uintptr_t thrd = _beginthreadex(NULL, 0, &CSync::threadProc, (void *)this, 0, NULL);
#else	
	pthread_t thrd;
	pthread_create(&thrd, NULL, &CSync::threadProc, (void *)this);
#endif
	
	return running = thrd!=0;
}

void CSync::RequestShutdown()
{
	CAutoLock alock(*this);
	doShutdown=true;
	CSyncML *sync = (CSyncML *)CSync::sync;
	if (sync!=0)
		sync->Interrupt();
}

int CSync::GetProgress()
{
	CAutoLock alock(*this);
	CSyncML *sync = (CSyncML *)CSync::sync;
	if (sync!=0)
	{
		return faseSync*50 + (sync->GetReceivePercent()*95/100+sync->GetSendPercent()*5/100)/2;
	}
	return 100;
}

struct SContact: public CSync::SConfirmContact
{
	std::tstring remoteID;
	std::tstring cmd, msg;
	std::tstring target, source;
	std::tstring cmdName;
};

#ifdef _WIN32
unsigned CSync::threadProc(void *data)
#else
void *CSync::threadProc(void *data)
#endif
{
	CSync *oSync = (CSync *)data;
	CSyncML sync;
	bool ok = true;
	bool contactsAdded = false;

	oSync->Lock();
	oSync->sync = &sync;
	bool forceFull = oSync->forceFull;
	std::tstring host, service, db, idPC, user, password, telefono;
	std::wstring addressBookPath;
	addressBookPath = oSync->addressBookPath;
	host = oSync->host;
	service = oSync->service;
	db = oSync->db;
	idPC = oSync->idPC;
	user = oSync->user;
	password = oSync->password;
	telefono = oSync->telefono;

	IEMAddressbook *abook = create_EMAddressbook(WideToUTF8(addressBookPath).c_str());
	ASSERT(abook!=0);
	if (abook==0)
	{
		oSync->SetError(Error_NoDB);
		oSync->SetState(State_FinishedWithError);
		oSync->sync = 0;
		oSync->Unlock();
#ifdef _WIN32
		return -1;
#else
		return 0;
#endif
	}
	oSync->Unlock();

	// estado inicial:
	oSync->SetState(State_InitSync);

	// comprobar si deberiamos hacer un download completo o incremental:
	bool fullDownload = forceFull;

	//if (ok && !fullDownload && !oSync->ShutdownRequested())
	//{




	//
	//}

	
	fullDownload = true;


	// pass 0 para primer download,
	// pass 1 para segundo download, despu�s de confirmaci�n de usuario
	bool pass1ok = false, pass2ok = false;
	for (int pass = 0; ok && !oSync->ShutdownRequested() && pass<2; pass++)
	{
		oSync->Lock();
		oSync->faseSync = pass;
		oSync->Unlock();

		int initMsg, alertCmd, syncCmd;
		bool connected = false;

		ok = sync.Init(10000);
		if (!ok)
			oSync->SetError(Error_Unknown);

		// conectar:
		if (ok && !oSync->ShutdownRequested())
		{
			oSync->SetState(State_Connecting);

			ok = connected = ok && sync.Connect(host, service, idPC, user, password);
			if (!ok)
				oSync->SetError(Error_CouldNotConnect);
		}

		// realizar petici�n de sync (recibir cambios, o full download de servidor):
		if (ok && !oSync->ShutdownRequested())
		{
			oSync->SetState(State_Downloading);

			std::tstring devtype, devid;
			if (pass==0)
			{
				devtype=_T("sim");
				devid=std::tstring(_T("EMS:"))+telefono;
			}
			else
			{
				devtype=_T("phone"); // movil");
				devid=std::tstring(_T("EMP:"))+telefono;
			}

			initMsg = sync.StartMessage(devtype, devid);

			int devI;
#ifndef FUNAMBOL
			devI = sync.PutDevInfo();
#endif
			//int last, next;
			//last = rand();
			//next = rand();
			std::tstringstream ss1, ss2;
			ss1.str(_T(""));
			ss1<<_T("0"); // last;
#ifdef _AFX
			CTime time;
			time=CTime::GetCurrentTime();
			CString timeFmt=time.Format("EM%Y%m%d%H%M%S");
			ss2.str(_T(""));
			//ss2<<next;
			ss2<<std::tstring(timeFmt.GetBuffer());
#else
			time_t tt;
			tt=time(NULL);
			struct tm *t;
			if (tt!=(time_t)(-1) && (t=localtime(&tt))!=NULL)
			{
				std::stringstream sstime;
				sstime<<"EM"<<1900+t->tm_year<<t->tm_mon+1<<t->tm_mday<<
					t->tm_hour<<t->tm_min<<t->tm_sec;
				ss2<<toTString(sstime.str());				
			}
#endif
			
			alertCmd = sync.Alert(fullDownload? 205:204, db, ss1.str(), ss2.str());
 			syncCmd = sync.Sync(db);

#ifndef FUNAMBOL
			ok = ok && alertCmd!=-1 && devI!=-1 && syncCmd!=-1;
#else
			ok = ok && alertCmd!=-1 && syncCmd!=-1;
#endif
			if (!ok || initMsg==-1)
				oSync->SetError(Error_Unknown);
			else
			{
				ok = initMsg!=-1 && sync.EndMessage()!=-1 && ok;
				if (!ok)
					oSync->SetError(Error_InvalidReply);
			}
		}

		if (connected)
		{
			if (!sync.Disconnect())
			{
				oSync->SetError(Error_Unknown);
				ok = false;
			}
		}

		// procesar respuesta (mensajes de servidor, actualizar nuestra
		// base de datos, etc.):
		if (ok && !oSync->ShutdownRequested())
		{
			std::vector<SContact> addCt;
			//std::vector<SContact> replCt;
			//std::vector<SContact> delCt;
			bool gotAlert = false;
			std::tstring alertTarget, alertSource, alertReplyMsg, alertReplyCmd;
			bool gotHeader = false;
			std::tstring headerTarget, headerSource, headerReplyMsg;
			bool statusOk = true;
			bool badAuth = false;

			// procesar respuestas de servidor:
			while (sync.haveReply())
			{
				if (sync.getFirstReplyType() == CSyncML::Reply_Status)
				{
					CSyncML::StatusReply status = sync.getFirstReplyStatus();
					if (statusOk)
					{
						if (status.code==401) // invalid authentication
							badAuth = true;
						if (status.code!=200)
							statusOk=false;
					}
				}
				else if (sync.getFirstReplyType() == CSyncML::Reply_SyncHeader)
				{
					CSyncML::SyncHeaderReply hdr = sync.getFirstReplySyncHeader();
					headerTarget = hdr.target;
					headerSource = hdr.source;
					headerReplyMsg = hdr.msg;
					gotHeader = true;
				}
				else if (sync.getFirstReplyType() == CSyncML::Reply_Alert && !gotAlert)
				{
					CSyncML::AlertReply alert = sync.getFirstReplyAlert();
					int code=alert.code;
					if (code==200 || code==201 || code==204 || code==205) // alertas soportadas
					{
						alertTarget = alert.target;
						alertSource = alert.source;
						alertReplyMsg = alert.msg;
						alertReplyCmd = alert.cmd;
						gotAlert = true;
					}
				}
				else if (sync.getFirstReplyType() == CSyncML::Reply_Add)
				{
					CSyncML::AddReply add = sync.getFirstReplyAdd();
					addCt.push_back(SContact());
					addCt.back().id = _T("");
					addCt.back().vcard = add.vcard;
					addCt.back().op = SConfirmContact::Op_Leave;
					addCt.back().remoteID = add.sourceItem;
					addCt.back().cmd = add.cmd;
					addCt.back().msg = add.msg;
					addCt.back().target = _T("");
					addCt.back().source = add.sourceItem;
					addCt.back().cmdName = _T("Add");
				}
				//else if (sync.getFirstReplyType() == CSyncML::Reply_Replace)
				//{
				//	CSyncML::ReplaceReply repl = sync.getFirstReplyReplace();
				//	replCt.push_back(SContact());
				//	replCt.back().id = repl.targetItem;
				//	replCt.back().vcard = repl.vcard;
				//	replCt.back().op = SConfirmContact::Op_Replace;
				//	replCt.back().remoteID = _T("");
				//	replCt.back().cmd = repl.cmd;
				//	replCt.back().msg = repl.msg;
				//	replCt.back().target = repl.targetItem;
				//	replCt.back().source = _T("");
				//	replCt.back().cmdName = _T("Replace");
				//}
				//else if (sync.getFirstReplyType() == CSyncML::Reply_Delete)
				//{
				//	CSyncML::DeleteReply del = sync.getFirstReplyDelete();
				//	delCt.push_back(SContact());
				//	delCt.back().id = del.targetItem;
				//	delCt.back().op = SConfirmContact::Op_Delete;
				//	delCt.back().remoteID = _T("");
				//	delCt.back().cmd = del.cmd;
				//	delCt.back().msg = del.msg;
				//	delCt.back().target = del.targetItem;
				//	delCt.back().source = _T("");
				//	delCt.back().cmdName = _T("Delete");
				//}
				sync.removeFirstReply();
			}

			// los nuevos a a�adir, se a�aden y se confirman inmediatamente
			// (los modificados o a eliminar, depende de la pol�tica always/never/ask,
			// o de si estamos en la segunda pasada, en cuyo caso ya tenemos confirmaci�n
			// del usuario, al menos de los replace/delete obtenidos anteriormente):

			// TODO: (para versi�n completa) �qu� hacer si entremedias ha habido modificaciones en servidor???

			if (ok && (!gotAlert || !gotHeader || !statusOk))
			{
				ok = false;
				if (badAuth)
					oSync->SetError(Error_BadAuth);
				else
					oSync->SetError(Error_InvalidReply);
			}

			//if (ok && !oSync->ShutdownRequested() &&
			//	(addCt.size()>0 || 
			//	 (replCt.size()>0 || delCt.size()>0) && 
			//	 (oSync->GetUpdateMode()==Update_Always || pass==1)))
			if (ok && !oSync->ShutdownRequested() &&
				addCt.size()>0)
			{
				// eliminar contactos de agenda local que sean de copiagenda:
				{
					std::vector<EMContact> bookcts=abook->allContacts();
					for (int c=0; c<(int)bookcts.size(); c++)
					{
						const EMContact &ct(bookcts[c]);
						std::string id(ct.getCopiaAgendaId());
						if (id=="1" && pass==0 || id=="2" && pass==1)
						{
							abook->removeContactByID(ct.getId());
						}
					}
				}

				oSync->SetState(State_Uploading);

				//// conectar (nota: no hay sync.End()/sync.Init(); reutilizamos sesi�n
				//// para responder a los comandos del servidor):
				//if (ok && !oSync->ShutdownRequested())
				//{
				//	ok = connected = ok && sync.Connect(host, service, idPC, user, password);
				//}
				// realizar petici�n:
				if (ok && !oSync->ShutdownRequested())
				{
					//initMsg = sync.StartMessage();

					//ASSERT(gotHeader && gotAlert);
					//if (gotHeader)
					//	sync.Status(200, headerReplyMsg, _T("0"), headerTarget, headerSource, _T("SyncHdr"), _T(""));
					//if (gotAlert)
					//	sync.Status(200, alertReplyMsg, alertReplyCmd, alertTarget, alertSource, _T("Alert"), _T(""));

					int i;

					// a�adir nuevos contactos:
					for (i=0; i<(int)addCt.size(); i++)
					{
						const SContact &ct(addCt[i]);
						/*std::string id =*/ AddContact(abook, ct.vcard, pass==0);

						contactsAdded = true;

						//if (id!="")
						//{
						//	std::tstringstream ss;
						//	ss << toTString(id);
						//	sync.Status(200, ct.msg, ct.cmd, ct.target, ct.source, ct.cmdName, _T(""));
						//	sync.Map(alertTarget, alertSource, ct.source, ss.str());
						//}
						//else
						//{
						//	// no se ha a�adido el contacto a base de datos, 
						//	// indicarselo al servidor con un error:
						//	//
						//	// 510 Data store failure. An error occurred while processing the request.
						//	//     The error is related to a failure in the recipient data store.
						//	sync.Status(510, ct.msg, ct.cmd, _T(""), ct.source, ct.cmdName, _T(""));
						//}
					}

					//if (oSync->GetUpdateMode()==Update_Always || pass==1)
					//{
					//	for (i=0; i<(int)replCt.size(); i++)
					//	{
					//		const SContact &ct(addCt[i]);
					//		if (ReplaceContact(abook, toNarrowString(ct.id), ct.vcard))
					//		{
					//			sync.Status(200, ct.msg, ct.cmd, ct.target, ct.source, ct.cmdName, _T(""));
					//			sync.Map(alertTarget, alertSource, ct.source, ct.id);
					//		}
					//		else
					//		{
					//			// no se ha modificado el contacto a base de datos, 
					//			// indicarselo al servidor con un error:
					//			//
					//			// 510 Data store failure. An error occurred while processing the request.
					//			//     The error is related to a failure in the recipient data store.
					//			sync.Status(510, ct.msg, ct.cmd, _T(""), ct.source, ct.cmdName, _T(""));
					//		}
					//	}
					//	for (i=0; i<(int)delCt.size(); i++)
					//	{
					//		const SContact &ct(addCt[i]);
					//		if (DeleteContact(abook, toNarrowString(ct.id)))
					//		{
					//			sync.Status(200, ct.msg, ct.cmd, ct.target, ct.source, ct.cmdName, _T(""));
					//			sync.Map(alertTarget, alertSource, ct.source, ct.id);
					//		}
					//		else
					//		{
					//			// no se ha modificado el contacto a base de datos, 
					//			// indicarselo al servidor con un error:
					//			//
					//			// 510 Data store failure. An error occurred while processing the request.
					//			//     The error is related to a failure in the recipient data store.
					//			sync.Status(510, ct.msg, ct.cmd, _T(""), ct.source, ct.cmdName, _T(""));
					//		}
					//	}
					//}

					//ok = initMsg!=-1 && sync.EndMessage()!=-1 && ok;
				}

				//ok = connected && sync.Disconnect();
			}
			//if (ok && !oSync->ShutdownRequested() &&
			//	pass==0 && (replCt.size()>0 || delCt.size()>0) &&
			//	oSync->GetUpdateMode()==Update_Ask)
			//{
			//	oSync->Lock();
			//	oSync->confirm.clear();
			//	int i;
			//	for (i=0; i<(int)replCt.size(); i++)
			//		oSync->confirm.push_back(replCt[i]);
			//	for (i=0; i<(int)delCt.size(); i++)
			//		oSync->confirm.push_back(delCt[i]);
			//	oSync->_SetState(State_Acknowledge);
			//	oSync->Unlock();

			//	// esperar confirmaci�n de usuario:
			//	while (ok && !oSync->ShutdownRequested() && oSync->GetState()==State_Acknowledge)
			//	{
			//		Sleep(100);
			//	}
			//}
		}

		sync.End();

		if (pass==0)
			pass1ok = ok;
		else if (pass==1)
			pass2ok = ok;
	}

	abook->destroy();
	abook = 0;

	if (oSync->ShutdownRequested() && !contactsAdded)
	{
		oSync->Lock();
		oSync->_SetError(Error_None);
		oSync->_SetState(State_Cancelled);
		oSync->running = false;
		oSync->sync = 0;
		oSync->Unlock();
#ifdef _WIN32
		return -1;
#else
		return 0;
#endif
	}

	if (pass1ok!=pass2ok && (pass1ok || pass2ok))
	{
		oSync->Lock();
		if (oSync->_GetError()==Error_None)
			oSync->_SetError(Error_Unknown);
		oSync->_SetState(State_FinishedWithPartialDownload);
		oSync->running = false;
		oSync->sync = 0;
		oSync->Unlock();
#ifdef _WIN32
		return -1;
#else
		return 0;
#endif
	}
	if (!pass1ok && !pass2ok)
	{
		oSync->Lock();
		if (oSync->_GetError()==Error_None)
			oSync->_SetError(Error_Unknown);
		oSync->_SetState(State_FinishedWithError);
		oSync->running = false;
		oSync->sync = 0;
		oSync->Unlock();
#ifdef _WIN32
		return -1;
#else
		return 0;
#endif
	}
	oSync->Lock();
	oSync->_SetError(Error_None);
	oSync->_SetState(State_Finished);
	oSync->running = false;
	oSync->sync = 0;
	oSync->Unlock();
	return 0;
}

std::string CSync::AddContact(IEMAddressbook *abook, const CVCard &vcard, bool sim) // sim vs. phone
{
	std::tstring name=vcard.getDetail(_T("N"));
	if (name!=_T(""))
	{
		std::tstring first, last;
		int i=0;
		for (std::tstring::size_type pos; (pos=name.find(';')), name!=_T(""); i++)
		{
			std::tstring n = name.substr(0, pos);
			if (pos==std::tstring::npos)
				pos = name.length();
			else
				pos++;
			name.erase(name.begin(), name.begin()+pos);
			if (i==1)
				first = n;
			else if (i==0)
				last = n;
		}
		name = first;
		//if (last!=_T("") && first!=_T(""))
		//	name += _T(", ");
		if (first!=_T("") && last!=_T(""))
			name += _T(" ");
		name += last;
	}

	std::tstring dname=vcard.getDetailTag(_T("N"));
	std::tstring charset, encoding;
	std::tstring::size_type pos;
	for ( ; (pos=dname.find(';')), dname!=_T(""); )
	{
		if (pos==std::tstring::npos)
			pos=dname.length();
		std::tstring val = dname.substr(0, pos);
		if (val==_T("ENCODING=QUOTED-PRINTABLE"))
			encoding=_T("QUOTED-PRINTABLE");
		else if (val==_T("CHARSET=UTF-8"))
			charset=_T("UTF-8");
		if (pos==dname.length())
			pos--;
		dname.erase(dname.begin(), dname.begin()+pos+1);
	}
	std::string narrowName;
	if (encoding==_T("QUOTED-PRINTABLE"))
	{
		narrowName = toNarrowString(name);
		CVCard::QuotedPrintableDecode(narrowName);
		name = toTString(narrowName);

	}
	if (charset==_T("UTF-8"))
	{
		narrowName = toNarrowString(name);
		narrowName = toNarrowString(UTF8ToWide(narrowName));
	}
	else
		narrowName = toNarrowString(name);

	while ((pos=narrowName.find(";"))!=std::string::npos)
	{
		narrowName.erase(pos, 1);
		narrowName.insert(pos, ", ");
	}

	std::vector<std::tstring> tels(vcard.getDetails(_T("TEL")));
	std::vector<std::tstring> tags(vcard.getDetailTags(_T("TEL")));
	ASSERT(tels.size()==tags.size());
	std::tstring tel;
	if (tels.size()==tags.size())
	{
		for (int i=0; i<(int)tels.size(); i++)
		{
			if (tags[i].find(_T(";CELL"))!=std::tstring::npos)
			{
				tel=tels[i];
				break;
			}
		}
	}
	if (tel==_T(""))
	{
		for (int i=0; i<(int)tels.size(); i++)
		{
			if (tels[i]!=_T(""))
			{
				tel=tels[i];
				break;
			}
		}
	}

	//TRACE("A�ADIDO %s\n", (char *) toNarrowString(tel).c_str());

	EMContact c(
		NO_SAVED_ID,
		WideToUTF8(toWideString(narrowName)).c_str(),
		toUTF8(tel).c_str(),
		toUTF8(vcard.getDetail(_T("EMAIL"))).c_str(),
		sim? "1":"2", // indicador de que el contacto es de copiagenda (1 para sim, 2 para phone).
		"");
	abook->saveContact(c);
	return c.getId();
}

bool CSync::ReplaceContact(IEMAddressbook *abook, std::string id, const CVCard &vcard)
{
	ASSERT(false); // esto no puede ocurrir ya
	return false;
}

bool CSync::DeleteContact(IEMAddressbook *abook, std::string id)
{
	ASSERT(false); // esto no puede ocurrir ya
	return false;
}

bool CSync::mismoTelefono(std::string t1, std::string t2, const std::tstring &countryCode)
{
	int i;
	for (i=0; i<(int)t1.size(); i++)
		if (isspace(t1[i]) || t1[i]==_T('-') || t1[i]==_T('/') || t1[i]==_T('\\'))
			t1.erase(t1.begin()+i--);
	for (i=0; i<(int)t2.size(); i++)
		if (isspace(t2[i]) || t2[i]==_T('-') || t2[i]==_T('/') || t2[i]==_T('\\'))
			t2.erase(t2.begin()+i--);
	const std::string narrowCountryCode = toNarrowString(countryCode);
	if (t1.find(narrowCountryCode)==0)
		t1.erase(0, narrowCountryCode.length());
	if (t2.find(narrowCountryCode)==0)
		t2.erase(0, narrowCountryCode.length());
	return t1==t2;
}

void CSync::filtrarContactosAgenda(IEMAddressbook *abook, const std::tstring &countryCode)
{
	if (abook==0)
		return;
	
	// filtrar contactos de copiagenda por n�mero de tel�fono para evitar duplicados:
	std::vector<EMContact> contacts=abook->allContacts();
	
	// recorremos todos los contactos:
	// - eliminamos los que tienen telefono vac�o, y son remotos.
	// - para cada contacto comparamos con todos los demas que tienen telefono
	//   para ver si hay conflicto que resolver
	//
	for (int c1=0; c1<(int)contacts.size(); c1++)
	{
		EMContact ct1(contacts[c1]);
		std::string id1(contacts[c1].getCopiaAgendaId());
	
		if ((std::string(ct1.getPhone())=="" || std::string(ct1.getPhone()).find(' ')!=std::string::npos) &&
			(id1=="1" || id1=="2"))
		{
//			TRACE("ELIMINADO %s\n", (char *)toNarrowString(UTF8ToWide(ct1.getPhone())).c_str());
			abook->removeContactByID(ct1.getId());
			contacts.erase(contacts.begin()+c1);
			c1--;
			continue;
		}
	
		for (int c2=0; c2<(int)contacts.size(); c2++)
		{
			if (c1!=c2)
			{
				const EMContact &ct2(contacts[c2]);
				const std::string id2(contacts[c2].getCopiaAgendaId());
				if (std::string(ct2.getPhone())!="" &&
					(id1!="1" && id1!="2" && (id2=="1" || id2=="2") ||
					 id1=="1" && id2=="2"))
				{
	
					// ver si los n�meros de tel�fono son similares:
	
					std::string t1, t2;
					t1=ct1.getPhone();
					t2=ct2.getPhone();
					if (mismoTelefono(t1, t2, countryCode))
					{
//						TRACE("ELIMINADO2 %s\n", (char *)toNarrowString(UTF8ToWide(ct2.getPhone())).c_str());
						abook->removeContactByID(ct2.getId());
						contacts.erase(contacts.begin()+c2);
						if (c1>=c2)
						{
							c1--;
							ct1 = contacts[c1];
							id1 = contacts[c1].getCopiaAgendaId();
						}
						c2--;
					}
				}
			}
		}
//		TRACE("MANTENIDO %s\n", (char *)toNarrowString(UTF8ToWide(contacts[c1].getPhone())).c_str());
	}
}

