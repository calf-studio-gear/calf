#include <Python.h>
#include "ttl.h"
#include "ttldata.h"
#include <list>
#include <map>
#include <iostream>
#include <fstream>
#include <jack/jack.h>

//////////////////////////////////////////////////// PyJackClient

struct PyJackPort;

typedef std::map<jack_port_t *, PyJackPort *> PortHandleMap;

struct AsyncOperationInfo
{
    enum OpCode { INVALID_OP, PORT_REGISTERED, PORT_UNREGISTERED, PORTS_CONNECTED, PORTS_DISCONNECTED };
    
    OpCode opcode;
    jack_port_id_t port, port2;
    
    AsyncOperationInfo() { opcode = INVALID_OP; }
    AsyncOperationInfo(OpCode _opcode, jack_port_id_t _port, jack_port_id_t _port2 = 0)
    : opcode(_opcode), port(_port), port2(_port2) {}
    inline int argc() {
        if (opcode == PORT_REGISTERED || opcode == PORT_UNREGISTERED) return 1;
        if (opcode == PORTS_CONNECTED || opcode == PORTS_DISCONNECTED) return 2;
        return 0;
    }
};

typedef std::list<AsyncOperationInfo> PortOperationList;

struct PyJackClient
{
    PyObject_HEAD
    jack_client_t *client;
    PortHandleMap *port_handle_map;
    pthread_mutex_t port_op_mutex;
    PortOperationList *port_op_queue;
};

static PyTypeObject jackclient_type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "calfpytools.JackClient",  /*tp_name*/
    sizeof(PyJackClient),      /*tp_basicsize*/
};

struct PyJackPort
{
    PyObject_HEAD
    PyJackClient *client;
    jack_port_t *port;
};

static PyTypeObject jackport_type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "calfpytools.JackPort",  /*tp_name*/
    sizeof(PyJackPort),      /*tp_basicsize*/
};

static void register_cb(jack_port_id_t port, int registered, void *arg) 
{
    PyJackClient *self = (PyJackClient *)arg;
    pthread_mutex_lock(&self->port_op_mutex);
    self->port_op_queue->push_back(AsyncOperationInfo(registered ? AsyncOperationInfo::PORT_REGISTERED : AsyncOperationInfo::PORT_UNREGISTERED, port));
    pthread_mutex_unlock(&self->port_op_mutex);
}

static void connect_cb(jack_port_id_t port1, jack_port_id_t port2, int connected, void *arg) 
{
    PyJackClient *self = (PyJackClient *)arg;
    pthread_mutex_lock(&self->port_op_mutex);
    self->port_op_queue->push_back(AsyncOperationInfo(connected ? AsyncOperationInfo::PORTS_CONNECTED : AsyncOperationInfo::PORTS_DISCONNECTED, port1, port2));
    pthread_mutex_unlock(&self->port_op_mutex);
}

static PyObject *jackclient_open(PyJackClient *self, PyObject *args)
{
    const char *name;
    int options = 0;
    jack_status_t status = (jack_status_t)0;
    
    if (!PyArg_ParseTuple(args, "s|i:open", &name, &options))
        return NULL;
    
    self->client = jack_client_open(name, (jack_options_t)options, &status);
    self->port_handle_map = new PortHandleMap;    
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
    pthread_mutex_init(&self->port_op_mutex, &attr);
    self->port_op_queue = new PortOperationList;
    
    jack_set_port_registration_callback(self->client, register_cb, self);
    jack_set_port_connect_callback(self->client, connect_cb, self);
    jack_activate(self->client);
    
    return Py_BuildValue("i", status);
}

static int jackclient_dealloc(PyJackPort *self)
{
    if (self->client)
    {
        PyObject_CallMethod((PyObject *)self, strdup("close"), NULL);
        assert(!self->client);
    }
    
    return 0;
}

#define CHECK_CLIENT if (!self->client) { PyErr_SetString(PyExc_ValueError, "Client not opened"); return NULL; }


static PyObject *jackclient_get_name(PyJackClient *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":get_name"))
        return NULL;
    
    CHECK_CLIENT
    
    return Py_BuildValue("s", jack_get_client_name(self->client));
}

static PyObject *create_jack_port(PyJackClient *client, jack_port_t *port)
{
    PortHandleMap::iterator it = client->port_handle_map->find(port);
    if (it != client->port_handle_map->end())
    {
        Py_INCREF(it->second);
        return Py_BuildValue("O", it->second);
    }
    if (port)
    {
        PyObject *cobj = PyCObject_FromVoidPtr(port, NULL);
        PyObject *args = Py_BuildValue("OO", client, cobj);
        PyObject *newobj = _PyObject_New(&jackport_type);
        jackport_type.tp_init(newobj, args, NULL);
        (*client->port_handle_map)[port] = (PyJackPort *)newobj;
        Py_DECREF(args);
        return newobj; 
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *jackclient_register_port(PyJackClient *self, PyObject *args)
{
    const char *name, *type = JACK_DEFAULT_AUDIO_TYPE;
    unsigned long flags = 0, buffer_size = 0;
    if (!PyArg_ParseTuple(args, "s|sii:register_port", &name, &type, &flags, &buffer_size))
        return NULL;
    
    CHECK_CLIENT
        
    jack_port_t *port = jack_port_register(self->client, name, type, flags, buffer_size);
    return create_jack_port(self, port);
}

static PyObject *jackclient_get_port(PyJackClient *self, PyObject *args)
{
    const char *name;
    if (!PyArg_ParseTuple(args, "s:get_port", &name))
        return NULL;
    
    CHECK_CLIENT
        
    jack_port_t *port = jack_port_by_name(self->client, name);
    return create_jack_port(self, port);
}

static PyObject *jackclient_get_cobj(PyJackClient *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":get_cobj"))
        return NULL;
    
    CHECK_CLIENT
    
    return PyCObject_FromVoidPtr((void *)self->client, NULL);
}

static PyObject *jackclient_get_ports(PyJackClient *self, PyObject *args)
{
    const char *name = NULL, *type = NULL;
    unsigned long flags = 0;
    if (!PyArg_ParseTuple(args, "|ssi:get_ports", &name, &type, &flags))
        return NULL;
    
    CHECK_CLIENT
    
    const char **p = jack_get_ports(self->client, name, type, flags);
    PyObject *res = PyList_New(0);
    if (!p)
        return res;
    
    for (const char **q = p; *q; q++)
    {
        PyList_Append(res, PyString_FromString(*q));
        // free(q);
    }
    free(p);
    
    return res;
}

static PyObject *jackclient_connect(PyJackClient *self, PyObject *args)
{
    char *from_port = NULL, *to_port = NULL;
    if (!PyArg_ParseTuple(args, "ss:connect", &from_port, &to_port))
        return NULL;
    
    CHECK_CLIENT
    
    int result = jack_connect(self->client, from_port, to_port);
    
    switch(result)
    {
        case 0:
            Py_INCREF(Py_True);
            return Py_True;
        case EEXIST:
            Py_INCREF(Py_False);
            return Py_False;
        default:
            PyErr_SetString(PyExc_RuntimeError, "Connection error");
            return NULL;
    }
}

static PyObject *create_jack_port_by_id(PyJackClient *self, jack_port_id_t id)
{
    return create_jack_port(self, jack_port_by_id(self->client, id));
}

static PyObject *jackclient_get_message(PyJackClient *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":get_message"))
        return NULL;
    
    CHECK_CLIENT
    
    PyObject *obj = NULL;
    AsyncOperationInfo info;
    pthread_mutex_lock(&self->port_op_mutex);
    if (self->port_op_queue->empty())
    {
        obj = Py_None;
        Py_INCREF(Py_None);
    }
    else
    {
        info = self->port_op_queue->front();
        self->port_op_queue->pop_front();
    }
    pthread_mutex_unlock(&self->port_op_mutex);
    if (!obj)
    {
        if (info.argc() == 1)
            obj = Py_BuildValue("iO", info.opcode, create_jack_port_by_id(self, info.port));
        else
        if (info.argc() == 2)
            obj = Py_BuildValue("iOO", info.opcode, create_jack_port_by_id(self, info.port), create_jack_port_by_id(self, info.port2));
    }
    return obj;
}

static PyObject *jackclient_disconnect(PyJackClient *self, PyObject *args)
{
    char *from_port = NULL, *to_port = NULL;
    if (!PyArg_ParseTuple(args, "ss:disconnect", &from_port, &to_port))
        return NULL;
    
    CHECK_CLIENT
    
    int result = jack_disconnect(self->client, from_port, to_port);
    
    switch(result)
    {
        case 0:
            Py_INCREF(Py_None);
            return Py_None;
        default:
            PyErr_SetString(PyExc_RuntimeError, "Disconnection error");
            return NULL;
    }
}

static PyObject *jackclient_close(PyJackClient *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":close"))
        return NULL;
    
    CHECK_CLIENT
    
    jack_deactivate(self->client);
    jack_client_close(self->client);
    self->client = NULL;
    delete self->port_handle_map;
    delete self->port_op_queue;
    pthread_mutex_destroy(&self->port_op_mutex);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef jackclient_methods[] = {
    {"open", (PyCFunction)jackclient_open, METH_VARARGS, "Open a client"},
    {"close", (PyCFunction)jackclient_close, METH_VARARGS, "Close a client"},
    {"get_name", (PyCFunction)jackclient_get_name, METH_VARARGS, "Retrieve client name"},
    {"get_port", (PyCFunction)jackclient_get_port, METH_VARARGS, "Create port object from name of existing JACK port"},
    {"get_ports", (PyCFunction)jackclient_get_ports, METH_VARARGS, "Get a list of port names based on specified name, type and flag filters"},
    {"register_port", (PyCFunction)jackclient_register_port, METH_VARARGS, "Register a new port and return an object that represents it"},
    {"get_cobj", (PyCFunction)jackclient_get_cobj, METH_VARARGS, "Retrieve jack_client_t pointer for the client as CObject"},
    {"get_message", (PyCFunction)jackclient_get_message, METH_VARARGS, "Retrieve next port registration/connection message from the message queue"},
    {"connect", (PyCFunction)jackclient_connect, METH_VARARGS, "Connect two ports with given names"},
    {"disconnect", (PyCFunction)jackclient_disconnect, METH_VARARGS, "Disconnect two ports with given names"},
    {NULL, NULL, 0, NULL}
};

//////////////////////////////////////////////////// PyJackPort

static int jackport_init(PyJackPort *self, PyObject *args, PyObject *kwds)
{
    PyJackClient *client = NULL;
    PyObject *cobj = NULL;
    
    if (!PyArg_ParseTuple(args, "O!O:__init__", &jackclient_type, &client, &cobj))
        return 0;
    if (!PyCObject_Check(cobj))
    {
        PyErr_SetString(PyExc_TypeError, "Port constructor cannot be called explicitly");
        return 0;
    }
    Py_INCREF(client);

    self->client = client;
    self->port = (jack_port_t *)PyCObject_AsVoidPtr(cobj);
    
    return 0;
}

static int jackport_dealloc(PyJackPort *self)
{
    // if not unregistered, decref (unregister decrefs automatically)
    if (self->client) {
        self->client->port_handle_map->erase(self->port);
        Py_DECREF(self->client);
    }
    
    return 0;
}

#define CHECK_PORT_CLIENT if (!self->client || !self->client->client) { PyErr_SetString(PyExc_ValueError, "Client not opened"); return NULL; }
#define CHECK_PORT if (!self->port) { PyErr_SetString(PyExc_ValueError, "The port is not valid"); return NULL; }

static PyObject *jackport_strrepr(PyJackPort *self, int quotes)
{
    CHECK_PORT_CLIENT
    CHECK_PORT
    
    int flags = jack_port_flags(self->port);
    int flags_io = flags & (JackPortIsInput | JackPortIsOutput);
    return PyString_FromFormat("<calfpytools.JackPort, name=%s%s%s, flags=%s%s%s%s>", 
        quotes ? "\"" : "",
        jack_port_name(self->port), 
        quotes ? "\"" : "",
        (flags_io == JackPortIsInput) ? "in" : (flags_io == JackPortIsOutput ? "out" : "?"),
        flags & JackPortIsPhysical ? "|physical" : "",
        flags & JackPortCanMonitor ? "|can_monitor" : "",
        flags & JackPortIsTerminal ? "|terminal" : "");
}

static PyObject *jackport_str(PyJackPort *self)
{
    return jackport_strrepr(self, 0);
}

static PyObject *jackport_repr(PyJackPort *self)
{
    return jackport_strrepr(self, 1);
}

static PyObject *jackport_get_full_name(PyJackPort *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":get_full_name"))
        return NULL;
    
    CHECK_PORT_CLIENT
    CHECK_PORT
    
    return Py_BuildValue("s", jack_port_name(self->port));
}

static PyObject *jackport_get_flags(PyJackPort *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":get_flags"))
        return NULL;
    
    CHECK_PORT_CLIENT
    CHECK_PORT
    
    return PyInt_FromLong(jack_port_flags(self->port));
}

static PyObject *jackport_is_valid(PyJackPort *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":is_valid"))
        return NULL;
    
    return PyBool_FromLong(self->client && self->client->client && self->port);
}

static PyObject *jackport_is_mine(PyJackPort *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":is_mine"))
        return NULL;
    
    CHECK_PORT_CLIENT
    CHECK_PORT
    
    return PyBool_FromLong(self->client && self->client->client && self->port && jack_port_is_mine(self->client->client, self->port));
}

static PyObject *jackport_get_name(PyJackPort *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":get_name"))
        return NULL;
    
    CHECK_PORT_CLIENT
    CHECK_PORT
    
    return Py_BuildValue("s", jack_port_short_name(self->port));
}

static PyObject *jackport_get_type(PyJackPort *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":get_type"))
        return NULL;
    
    CHECK_PORT_CLIENT
    CHECK_PORT
    
    return Py_BuildValue("s", jack_port_type(self->port));
}

static PyObject *jackport_get_cobj(PyJackPort *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":get_cobj"))
        return NULL;
    
    CHECK_PORT_CLIENT
    CHECK_PORT
    
    return PyCObject_FromVoidPtr((void *)self->port, NULL);
}

static PyObject *jackport_get_aliases(PyJackPort *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":get_aliases"))
        return NULL;

    CHECK_PORT_CLIENT
    CHECK_PORT
    
    char buf1[256], buf2[256];
    char *const aliases[2] = { buf1, buf2 };
    int count = jack_port_get_aliases(self->port, aliases);
    
    PyObject *alist = PyList_New(0);
    for (int i = 0; i < count; i++)
        PyList_Append(alist, PyString_FromString(aliases[i]));
    return alist;
}

static PyObject *jackport_get_connections(PyJackPort *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":get_aliases"))
        return NULL;

    CHECK_PORT_CLIENT
    CHECK_PORT
    
    const char **conns = jack_port_get_all_connections(self->client->client, self->port);
    
    PyObject *res = PyList_New(0);
    if (conns)
    {
        for (const char **p = conns; *p; p++)
            PyList_Append(res, PyString_FromString(*p));
    }
    
    return res;
}

static PyObject *jackport_set_name(PyJackPort *self, PyObject *args)
{
    const char *name;
    if (!PyArg_ParseTuple(args, "s:set_name", &name))
        return NULL;
    
    CHECK_PORT
    
    jack_port_set_name(self->port, name);
    
    return Py_BuildValue("s", jack_port_short_name(self->port));
}

static PyObject *jackport_unregister(PyJackPort *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":unregister"))
        return NULL;
    
    CHECK_PORT
    
    PyJackClient *client = self->client;
    
    client->port_handle_map->erase(self->port);
    jack_port_unregister(self->client->client, self->port);
    self->port = NULL;
    self->client = NULL;
    
    Py_DECREF(client);
    client = NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef jackport_methods[] = {
    {"unregister", (PyCFunction)jackport_unregister, METH_VARARGS, "Unregister a port"},
    {"is_valid", (PyCFunction)jackport_is_valid, METH_VARARGS, "Checks if the port object is valid (registered)"},
    {"is_mine", (PyCFunction)jackport_is_mine, METH_VARARGS, "Checks if the port object is valid (registered)"},
    {"get_full_name", (PyCFunction)jackport_get_full_name, METH_VARARGS, "Retrieve full port name (including client name)"},
    {"get_name", (PyCFunction)jackport_get_name, METH_VARARGS, "Retrieve short port name (without client name)"},
    {"get_type", (PyCFunction)jackport_get_type, METH_VARARGS, "Retrieve port type name"},
    {"get_flags", (PyCFunction)jackport_get_flags, METH_VARARGS, "Retrieve port flags (defined in module, ie. calfpytools.JackPortIsInput)"},
    {"set_name", (PyCFunction)jackport_set_name, METH_VARARGS, "Set short port name"},
    {"get_aliases", (PyCFunction)jackport_get_aliases, METH_VARARGS, "Retrieve a list of port aliases"},
    {"get_connections", (PyCFunction)jackport_get_connections, METH_VARARGS, "Retrieve a list of ports the port is connected to"},
    {"get_cobj", (PyCFunction)jackport_get_cobj, METH_VARARGS, "Retrieve jack_port_t pointer for the port"},
    {NULL, NULL, 0, NULL}
};


//////////////////////////////////////////////////// calfpytools

static PyObject *calfpytools_scan_ttl_file(PyObject *self, PyObject *args)
{
    char *ttl_name = NULL;
    if (!PyArg_ParseTuple(args, "s:scan_ttl_file", &ttl_name))
        return NULL;
    
    std::ifstream istr(ttl_name, std::ifstream::in);
    TTLLexer lexer(&istr);
    lexer.yylex();
    return lexer.grab();
}

static PyObject *calfpytools_scan_ttl_string(PyObject *self, PyObject *args)
{
    char *data = NULL;
    if (!PyArg_ParseTuple(args, "s:scan_ttl_string", &data))
        return NULL;
    
    std::string data_str = data;
    std::stringstream str(data_str);
    TTLLexer lexer(&str);
    lexer.yylex();
    return lexer.grab();
}

static PyMethodDef module_methods[] = {
    {"scan_ttl_file", calfpytools_scan_ttl_file, METH_VARARGS, "Scan a TTL file, return a list of token tuples"},
    {"scan_ttl_string", calfpytools_scan_ttl_string, METH_VARARGS, "Scan a TTL string, return a list of token tuples"},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initcalfpytools()
{
    jackclient_type.tp_new = PyType_GenericNew;
    jackclient_type.tp_flags = Py_TPFLAGS_DEFAULT;
    jackclient_type.tp_doc = "JACK client object";
    jackclient_type.tp_methods = jackclient_methods;
    jackclient_type.tp_dealloc = (destructor)jackclient_dealloc;
    if (PyType_Ready(&jackclient_type) < 0)
        return;

    jackport_type.tp_new = PyType_GenericNew;
    jackport_type.tp_flags = Py_TPFLAGS_DEFAULT;
    jackport_type.tp_doc = "JACK port object (created by client)";
    jackport_type.tp_methods = jackport_methods;
    jackport_type.tp_init = (initproc)jackport_init;
    jackport_type.tp_dealloc = (destructor)jackport_dealloc;
    jackport_type.tp_str = (reprfunc)jackport_str;
    jackport_type.tp_repr = (reprfunc)jackport_repr;
    if (PyType_Ready(&jackport_type) < 0)
        return;
    
    PyObject *mod = Py_InitModule3("calfpytools", module_methods, "Python utilities for Calf");
    Py_INCREF(&jackclient_type);
    Py_INCREF(&jackport_type);
    PyModule_AddObject(mod, "JackClient", (PyObject *)&jackclient_type);
    PyModule_AddObject(mod, "JackPort", (PyObject *)&jackport_type);
    
    PyModule_AddObject(mod, "JackPortIsInput", PyInt_FromLong(JackPortIsInput));
    PyModule_AddObject(mod, "JackPortIsOutput", PyInt_FromLong(JackPortIsOutput));
    PyModule_AddObject(mod, "JackPortIsPhysical", PyInt_FromLong(JackPortIsPhysical));
    PyModule_AddObject(mod, "JackPortCanMonitor", PyInt_FromLong(JackPortCanMonitor));
    PyModule_AddObject(mod, "JackPortIsTerminal", PyInt_FromLong(JackPortIsTerminal));
    PyModule_AddObject(mod, "JACK_DEFAULT_AUDIO_TYPE", PyString_FromString(JACK_DEFAULT_AUDIO_TYPE));
    PyModule_AddObject(mod, "JACK_DEFAULT_MIDI_TYPE", PyString_FromString(JACK_DEFAULT_MIDI_TYPE));
}
