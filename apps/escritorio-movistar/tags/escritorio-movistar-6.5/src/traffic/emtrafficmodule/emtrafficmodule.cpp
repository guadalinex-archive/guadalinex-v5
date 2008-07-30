
#include <Python.h>
#include <structmember.h>
#include <locale.h>
// #include "ptools.h"
#include "Traffic.h"

#undef LOG
#ifndef NDEBUG
#define LOG(X) printf X
#else
#define LOG(X)
#endif

static std::vector<PyObject *> module_objects;

extern "C" struct emtraffic_Traffic 
{
	PyObject_HEAD;
	CTraffic *traffic;
};

PyObject *new_emtraffic_Traffic(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	emtraffic_Traffic *traffic;
	traffic = (emtraffic_Traffic *)type->tp_alloc(type, 0);
	if (traffic!=NULL)
	{
		traffic->traffic = new CTraffic;
		if (traffic->traffic == NULL)
		{
			type->tp_dealloc((PyObject *)traffic);
			traffic = NULL;
		}
	}
	LOG( ("emtraffic_Traffic: New Traffic creation: %s.\n", (const char *)(traffic!=NULL? "OK":"FAIL")) );
	return (PyObject *)traffic;
}

static void dealloc_emtraffic_Traffic(PyObject *self)
{
	LOG( ("emtraffic_Traffic: Traffic object destroyed.\n") );
	emtraffic_Traffic *traffic=(emtraffic_Traffic *)self;
	if (traffic!=NULL)
	{
		delete traffic->traffic;
		self->ob_type->tp_free(traffic);
	}
}

static int init_emtraffic_Traffic(PyObject *self, PyObject *args, PyObject *kwds)
{
	emtraffic_Traffic *traffic=(emtraffic_Traffic *)self;
	traffic->traffic = new CTraffic;
	if (traffic->traffic == NULL)
		return -1;
	return 0;
}

static PyMemberDef emtraffic_TrafficMembers [] = 
{
	{NULL}
};

PyObject *set_type_emtraffic_Traffic(PyObject *self, PyObject *args)
{
	const char *type;
	PyArg_ParseTuple(args, "s", &type);
	emtraffic_Traffic *traffic=(emtraffic_Traffic *)self;
	if (traffic!=0 && traffic->traffic!=0)
		traffic->traffic->SetType(type);
	return Py_BuildValue("z", NULL);
}

PyObject *get_consumo_historico_pendiente_emtraffic_Traffic(PyObject *self, PyObject *args)
{
	emtraffic_Traffic *traffic=(emtraffic_Traffic *)self;
	if (traffic!=0 && traffic->traffic!=0)
	{
		CTraffic::SHistorico h;
		h=traffic->traffic->GetConsumoHistoricoPendiente();		
		PyObject *date=Py_BuildValue("[i,i]", (int)h.mes, (int)h.anno);
		PyObject *traffic=Py_BuildValue("[K,K]", (unsigned long long)h.up, (unsigned long long)h.down);
		PyObject *item=PyList_New(2);
		Py_IncRef(item);
		PyList_SetItem(item, 0, date);
		PyList_SetItem(item, 1, traffic);
		return item;
	}
	return Py_BuildValue("z", NULL);
}

PyObject *procesa_historico_emtraffic_Traffic(PyObject *self, PyObject *args)
{
	unsigned up, down, tick;
	int fin;
	emtraffic_Traffic *traffic=(emtraffic_Traffic *)self;
	if (PyArg_ParseTuple(args, "IIIi", &up, &down, &tick, &fin))
		if (traffic!=0 && traffic->traffic!=0)
			traffic->traffic->ProcesaHistorico(up, down, tick, fin!=0? true:false);
	return Py_BuildValue("z", NULL);
}

static PyMethodDef emtraffic_TrafficMethods [] = 
{
		{ "set_type", (PyCFunction)set_type_emtraffic_Traffic, METH_VARARGS, "Set type of traffic (4 char word max., like 'DUN', 'DUNR')."},
		{ "get_pending_traffic_history", (PyCFunction)get_consumo_historico_pendiente_emtraffic_Traffic, METH_NOARGS, "Get current accumulated traffic in self history object."},
		{ "process_history_traffic", (PyCFunction)procesa_historico_emtraffic_Traffic, METH_VARARGS, "Add traffic history data that has ocurred recently."},
		{NULL}
};

static PyTypeObject emtraffic_TrafficType = 
{
		PyObject_HEAD_INIT(NULL)
	    0,                         /*ob_size*/
	    "emtraffic.Traffic",             /*tp_name*/
	    sizeof(emtraffic_Traffic), /*tp_basicsize*/
	    0,                         /*tp_itemsize*/
	    dealloc_emtraffic_Traffic,	   /*tp_dealloc*/
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
	    "traffic object to store/query traffic information (currently history). ",
	    
	    0,		               /* tp_traverse */
	    0,		               /* tp_clear */
	    0,		               /* tp_richcompare */
	    0,		               /* tp_weaklistoffset */
	    0,		               /* tp_iter */
	    0,		               /* tp_iternext */
	    emtraffic_TrafficMethods,	   /* tp_methods */
	    emtraffic_TrafficMembers,    /* tp_members */
	    0,           		   /* tp_getset */
	    0,                         /* tp_base */
	    0,                         /* tp_dict */
	    0,                         /* tp_descr_get */
	    0,                         /* tp_descr_set */
	    0,                         /* tp_dictoffset */
	    (initproc)init_emtraffic_Traffic,	/* tp_init */
	    0,                         /* tp_alloc */
	    new_emtraffic_Traffic,                 /* tp_new */ 
};

static PyObject *global_init(PyObject *self, PyObject *args);
static PyObject *global_end(PyObject *self, PyObject *args);
static PyObject *set_db_file(PyObject *self, PyObject *args);
static PyObject *set_version(PyObject *self, PyObject *args);
static PyObject *get_consumo_historico(PyObject *self, PyObject *args);

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
	{ "set_db_file", set_db_file, METH_VARARGS, 
			"Set the sqlite3 db file path. " 
	},
	{ "set_version", set_version, METH_VARARGS, 
			"Set the EM version for db store. " 
	},
	{ "get_traffic_history", get_consumo_historico, METH_VARARGS, 
			"Obtains traffic history stored in database. " 
	},
	{ NULL, NULL, 0, NULL }
};

static PyObject *global_init(PyObject *self, PyObject *args)
{
	LOG( ("emtraffic module global_init\n") );
	ptoolsInit();
	return Py_BuildValue("");
}

static PyObject *global_end(PyObject *self, PyObject *args)
{
	LOG( ("emtraffic module global_end\n") );
	ptoolsEnd();
	return Py_BuildValue("");
}

PyObject *set_db_file(PyObject *self, PyObject *args)
{
	const char *db;
	if (PyArg_ParseTuple(args, "s", &db))
		CTraffic::SetDbFile(db);
	return Py_BuildValue("z", NULL);
}

PyObject *set_version(PyObject *self, PyObject *args)
{
	const char *str;
	int maj, min, upd;
	if (PyArg_ParseTuple(args, "iiis", &maj, &min, &upd, &str))
		CTraffic::SetVer(maj, min, upd, str);
	return Py_BuildValue("z", NULL);
}

PyObject *get_consumo_historico(PyObject *self, PyObject *args)
{
	const char *tag;
	if (!PyArg_ParseTuple(args, "s", &tag))
		tag="";

	std::vector<CTraffic::SHistorico> hist;
	hist=CTraffic::GetConsumoHistorico(tag);
	
	PyObject *list=PyList_New(hist.size());
	
	for (int i=0; i<hist.size(); i++)
	{
		const CTraffic::SHistorico &h(hist[i]);
		
		PyObject *date=Py_BuildValue("[i,i]", (int)h.mes, (int)h.anno);
		PyObject *traffic=Py_BuildValue("[K,K]", (unsigned long long)h.up, (unsigned long long)h.down);

		PyObject *item=PyList_New(2);
		Py_IncRef(item);
		PyList_SetItem(item, 0, date);
		PyList_SetItem(item, 1, traffic);
		
		PyList_SetItem(list, i, item);			
	}		
	Py_IncRef(list);
	return list;
}

PyMODINIT_FUNC
initemtraffic(void)
{	
	if (PyType_Ready(&emtraffic_TrafficType)>=0)
	{
		PyObject *module=Py_InitModule3("emtraffic", EMSyncMethods, "EM Traffic module.");
		
		Py_INCREF(&emtraffic_TrafficType);
		PyModule_AddObject(module, "Traffic", (PyObject *)&emtraffic_TrafficType);
	}
}
