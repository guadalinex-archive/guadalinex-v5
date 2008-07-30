
#include <Python.h>
#include <structmember.h>
#include <locale.h>
#include "ptools.h"
#include "Sync.h"
#include "AgendaLib.h"

#undef LOG
#ifndef NDEBUG
#define LOG(X) printf X
#else
#define LOG(X)
#endif

static std::vector<PyObject *> module_objects;

extern "C" struct emsync_Sync 
{
	PyObject_HEAD;
	CSync *sync;
	unsigned int State_Connecting;
	unsigned int State_InitSync;
	unsigned int State_Downloading;
	unsigned int State_Acknowledge;
	unsigned int State_Uploading;
	unsigned int State_Cancelled;
	unsigned int State_Finished;
	unsigned int State_FinishedWithError;
	unsigned int State_FinishedWithPartialDownload; // some contact downloads Ok. GetError() returns error.
	unsigned int Error_None;
	unsigned int Error_NoDB;
	unsigned int Error_CouldNotConnect;
	unsigned int Error_InvalidReply;
	unsigned int Error_BadAuth;
	unsigned int Error_Unknown; // (Toolkit C...?)
};

PyObject *new_emsync_Sync(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	emsync_Sync *sync;
	sync = (emsync_Sync *)type->tp_alloc(type, 0);
	if (sync!=NULL)
	{
		sync->sync = new CSync;
		if (sync->sync == NULL)
		{
			type->tp_dealloc((PyObject *)sync);
			sync = NULL;
		}
	}
	LOG( ("emsync_Sync: New Sync creation: %s.\n", (const char *)(sync!=NULL? "OK":"FAIL")) );
	return (PyObject *)sync;
}

static void dealloc_emsync_Sync(PyObject *self)
{
	LOG( ("emsync_Sync: Sync object destroyed.\n") );
	emsync_Sync *sync=(emsync_Sync *)self;
	if (sync!=NULL)
	{
		delete sync->sync;
		self->ob_type->tp_free(sync);
	}
}

static int init_emsync_Sync(PyObject *self, PyObject *args, PyObject *kwds)
{
	emsync_Sync *sync=(emsync_Sync *)self;
	sync->sync = new CSync;
	if (sync->sync == NULL)
		return -1;
	sync->State_Connecting=0;
	sync->State_InitSync=1;
	sync->State_Downloading=2;
	sync->State_Acknowledge=3;
	sync->State_Uploading=4;
	sync->State_Cancelled=5;
	sync->State_Finished=6;
	sync->State_FinishedWithError=7;
	sync->State_FinishedWithPartialDownload=8; // some contact downloads Ok. GetError() returns error.
	sync->Error_None=0;
	sync->Error_NoDB=1;
	sync->Error_CouldNotConnect=2;
	sync->Error_InvalidReply=3;
	sync->Error_BadAuth=4;
	sync->Error_Unknown=5; // (Toolkit C...?)
	return 0;
}

static PyMemberDef emsync_SyncMembers [] = 
{
	{ "State_Connecting", T_UINT, offsetof(emsync_Sync, State_Connecting), READONLY, "" },
	{ "State_InitSync", T_UINT, offsetof(emsync_Sync, State_InitSync), READONLY, "" },
	{ "State_Downloading", T_UINT, offsetof(emsync_Sync, State_Downloading), READONLY, "" },
	{ "State_Acknowledge", T_UINT, offsetof(emsync_Sync, State_Acknowledge), READONLY, "" },
	{ "State_Cancelled", T_UINT, offsetof(emsync_Sync, State_Cancelled), READONLY, "" },
	{ "State_Finished", T_UINT, offsetof(emsync_Sync, State_Finished), READONLY, "" },
	{ "State_FinishedWithError", T_UINT, offsetof(emsync_Sync, State_FinishedWithError), READONLY, "" },
	{ "State_FinishedWithPartialDownload", T_UINT, offsetof(emsync_Sync, State_FinishedWithPartialDownload), READONLY, "" },
	
	{ "Error_None", T_UINT, offsetof(emsync_Sync, Error_None), READONLY, "" },
	{ "Error_NoDB", T_UINT, offsetof(emsync_Sync, Error_NoDB), READONLY, "" },
	{ "Error_CouldNotConnect", T_UINT, offsetof(emsync_Sync, Error_CouldNotConnect), READONLY, "" },
	{ "Error_InvalidReply", T_UINT, offsetof(emsync_Sync, Error_InvalidReply), READONLY, "" },
	{ "Error_BadAuth", T_UINT, offsetof(emsync_Sync, Error_BadAuth), READONLY, "" },
	{ "Error_Unknown", T_UINT, offsetof(emsync_Sync, Error_Unknown), READONLY, "" },
	{NULL}
};

PyObject *start_emsync_Sync(PyObject *self, PyObject *args)
{
	const char *abook, *host, *service, *db, *idPC, *user, *password, *telefono;
	
	PyArg_ParseTuple(args, "ssssssss",
		&abook,
		&host,
		&service, 
		&db,
		&idPC, 
		&user,
		&password,
		&telefono
		);

	emsync_Sync *sync=(emsync_Sync *)self;
	if (sync!=0 && sync->sync!=0)
	{
		bool ok;
		ok=sync->sync->Start(
			CSync::Update_Always,
			true,
			abook,
			host,
			service,
			db,
			idPC,
			user,
			password,
			telefono);
		return Py_BuildValue("i", ok? 1:0);
	}
	return Py_BuildValue("i", 0);
}

PyObject *get_progress_emsync_Sync(PyObject *self, PyObject *args)
{
	emsync_Sync *sync=(emsync_Sync *)self;
	if (sync!=0 && sync->sync!=0)
		return Py_BuildValue("i", sync->sync->GetProgress());
	return Py_BuildValue("z", NULL);
}

PyObject *request_shutdown_emsync_Sync(PyObject *self, PyObject *args)
{
	emsync_Sync *sync=(emsync_Sync *)self;
	if (sync!=0 && sync->sync!=0)
		sync->sync->RequestShutdown();
	return Py_BuildValue("z", NULL);
}

PyObject *shutdown_requested_emsync_Sync(PyObject *self, PyObject *args)
{
	emsync_Sync *sync=(emsync_Sync *)self;
	if (sync!=0 && sync->sync!=0)
		return Py_BuildValue("i", sync->sync->ShutdownRequested()? 1:0);
	return Py_BuildValue("z", NULL);
}

PyObject *running_emsync_Sync(PyObject *self, PyObject *args)
{
	emsync_Sync *sync=(emsync_Sync *)self;
	if (sync!=0 && sync->sync!=0)
		return Py_BuildValue("i", sync->sync->Running()? 1:0);
	return Py_BuildValue("z", NULL);
}

PyObject *get_state_emsync_Sync(PyObject *self, PyObject *args)
{
	emsync_Sync *sync=(emsync_Sync *)self;
	if (sync!=0 && sync->sync!=0)
		return Py_BuildValue("i", sync->sync->GetState());
	return Py_BuildValue("z", NULL);
}

PyObject *get_error_emsync_Sync(PyObject *self, PyObject *args)
{
	emsync_Sync *sync=(emsync_Sync *)self;
	if (sync!=0 && sync->sync!=0)
		return Py_BuildValue("i", sync->sync->GetError());
	return Py_BuildValue("z", NULL);
}

static PyMethodDef emsync_SyncMethods [] = 
{
		{ "start", (PyCFunction)start_emsync_Sync, METH_VARARGS, "Start synchronization process."},
		{ "get_progress", (PyCFunction)get_progress_emsync_Sync, METH_NOARGS, "Get synchronization progress."},
		{ "request_shutdown", (PyCFunction)request_shutdown_emsync_Sync, METH_NOARGS, "Request that sync process terminates as soon as possible."},
		{ "shutdown_requested", (PyCFunction)shutdown_requested_emsync_Sync, METH_NOARGS, "Check if a shutdown has been requested."},
		{ "running", (PyCFunction)running_emsync_Sync, METH_NOARGS, "Check if sync process is running."},
		{ "get_state", (PyCFunction)get_state_emsync_Sync, METH_NOARGS, "Get current sync state."},
		{ "get_error", (PyCFunction)get_error_emsync_Sync, METH_NOARGS, "Get error code on failed sync."},
		{NULL}
};

static PyTypeObject emsync_SyncType = 
{
		PyObject_HEAD_INIT(NULL)
	    0,                         /*ob_size*/
	    "emsync.Sync",             /*tp_name*/
	    sizeof(emsync_Sync), /*tp_basicsize*/
	    0,                         /*tp_itemsize*/
	    dealloc_emsync_Sync,	   /*tp_dealloc*/
	    0,                         /*tp_print*/
	    0,                         /*tp_getattr*/
	    0,                         /*tp_setattr*/
	    0,                         /*tp_compare*/
	    0,                         /*tp_repr*/
	    0,                         /*tp_as_number*/
	    0,                         /*tp_as_sequence*/
	    0,                         /*tp_as_mapping*/
	    0,                         /*tp_hash */
	    0,                         /*tp_call*/
	    0,                         /*tp_str*/
	    0,                         /*tp_getattro*/
	    0,                         /*tp_setattro*/
	    0,                         /*tp_as_buffer*/
	    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
	    
	    /* tp_doc: */
	    "Sync object to perform Copiagenda syncronization",
	    
	    0,		               /* tp_traverse */
	    0,		               /* tp_clear */
	    0,		               /* tp_richcompare */
	    0,		               /* tp_weaklistoffset */
	    0,		               /* tp_iter */
	    0,		               /* tp_iternext */
	    emsync_SyncMethods,	   /* tp_methods */
	    emsync_SyncMembers,    /* tp_members */
	    0,           		   /* tp_getset */
	    0,                         /* tp_base */
	    0,                         /* tp_dict */
	    0,                         /* tp_descr_get */
	    0,                         /* tp_descr_set */
	    0,                         /* tp_dictoffset */
	    (initproc)init_emsync_Sync,	/* tp_init */
	    0,                         /* tp_alloc */
	    new_emsync_Sync,                 /* tp_new */ 
};

static PyObject *global_init(PyObject *self, PyObject *args);
static PyObject *global_end(PyObject *self, PyObject *args);
static PyObject *filter_contacts(PyObject *self, PyObject *args);

static PyMethodDef EMSyncMethods[] = 
{
	{ "global_init", global_init, METH_VARARGS, 
			"One time application sync module initialization. " 
			"Call from main app thread, before any other threads creation."
	},
	{ "global_end", global_end, METH_VARARGS, 
			"One time application sync module termination. " 
			"Call from main app thread, after al threads exited."
	},
	{ "filter_contacts", filter_contacts, METH_VARARGS, 
			"Filters sqlite3 database contacts for duplicates. " 
			"Call after modifying contacts database (sync, add, ...)."
	},
	{ NULL, NULL, 0, NULL }
};

static PyObject *global_init(PyObject *self, PyObject *args)
{
	LOG( ("emsync module global_init\n") );
	ptoolsInit();
	return Py_BuildValue("");
}

static PyObject *global_end(PyObject *self, PyObject *args)
{
	LOG( ("emsync module global_end\n") );
	ptoolsEnd();
	return Py_BuildValue("");
}

static PyObject *filter_contacts(PyObject *self, PyObject *args)
{
	LOG( ("emsync module filter_contacts\n") );
	
	const char *abook, *country_code;
	
	if (PyArg_ParseTuple(args, "ss",
		&abook,
		&country_code))
	{
		IEMAddressbook *book=create_EMAddressbook(abook);	
		if (book!=0)
			CSync::filtrarContactosAgenda(book, country_code);
		book->destroy();
	}	
	return Py_BuildValue("");
}

PyMODINIT_FUNC
initemsync(void)
{	
	if (PyType_Ready(&emsync_SyncType)>=0)
	{
		PyObject *module=Py_InitModule3("emsync", EMSyncMethods, "EM Synchronization module.");
		
		Py_INCREF(&emsync_SyncType);
		PyModule_AddObject(module, "Sync", (PyObject *)&emsync_SyncType);
	}
}
