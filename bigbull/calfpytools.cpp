#include "Python.h"
#include <jack/jack.h>

//////////////////////////////////////////////////// PyJackClient

struct PyJackClient
{
    PyObject_HEAD
    jack_client_t *client;
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

static PyObject *jackclient_open(PyJackClient *self, PyObject *args)
{
    const char *name;
    int options = 0;
    jack_status_t status = (jack_status_t)0;
    
    if (!PyArg_ParseTuple(args, "s|i:open", &name, &options))
        return NULL;
    
    self->client = jack_client_open(name, (jack_options_t)options, &status);
    
    return Py_BuildValue("i", status);
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
    if (port)
    {
        PyObject *cobj = PyCObject_FromVoidPtr(port, NULL);
        PyObject *args = Py_BuildValue("OO", client, cobj);
        PyObject *newobj = _PyObject_New(&jackport_type);
        jackport_type.tp_init(newobj, args, NULL);
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

static PyObject *jackclient_close(PyJackClient *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":close"))
        return NULL;
    
    CHECK_CLIENT
    
    jack_client_close(self->client);
    self->client = NULL;
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef jackclient_methods[] = {
    {"open", (PyCFunction)jackclient_open, METH_VARARGS, "Open a client"},
    {"close", (PyCFunction)jackclient_close, METH_VARARGS, "Close a client"},
    {"get_name", (PyCFunction)jackclient_get_name, METH_VARARGS, "Retrieve client name"},
    {"get_port", (PyCFunction)jackclient_get_port, METH_VARARGS, "Create port object from name of existing JACK port"},
    {"register_port", (PyCFunction)jackclient_register_port, METH_VARARGS, "Register a new port and return an object that represents it"},
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

#define CHECK_PORT_CLIENT if (!self->client || !self->client->client) { PyErr_SetString(PyExc_ValueError, "Client not opened"); return NULL; }
#define CHECK_PORT if (!self->port) { PyErr_SetString(PyExc_ValueError, "The port is not valid"); return NULL; }

static PyObject *jackport_get_full_name(PyJackPort *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":get_full_name"))
        return NULL;
    
    CHECK_PORT_CLIENT
    CHECK_PORT
    
    return Py_BuildValue("s", jack_port_name(self->port));
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

static PyObject *jackport_get_aliases(PyJackPort *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":get_aliases"))
        return NULL;

    CHECK_PORT_CLIENT
    CHECK_PORT
    
    char buf1[256], buf2[256];
    char *const aliases[2] = { buf1, buf2 };
    int count = jack_port_get_aliases(self->port, aliases);
    
    if (count == 0)
        return Py_BuildValue("[]");
    if (count == 1)
        return Py_BuildValue("[s]", aliases[0]);
    return Py_BuildValue("[ss]", aliases[0], aliases[1]);
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
    
    jack_port_unregister(self->client->client, self->port);
    self->port = NULL;
    self->client = NULL;
    
    Py_DECREF(client);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef jackport_methods[] = {
    {"unregister", (PyCFunction)jackport_unregister, METH_VARARGS, "Unregister a port"},
    {"is_valid", (PyCFunction)jackport_is_valid, METH_VARARGS, "Checks if the port object is valid (registered)"},
    {"is_mine", (PyCFunction)jackport_is_mine, METH_VARARGS, "Checks if the port object is valid (registered)"},
    {"get_full_name", (PyCFunction)jackport_get_full_name, METH_VARARGS, "Retrieve full port name (including client name)"},
    {"get_name", (PyCFunction)jackport_get_name, METH_VARARGS, "Retrieve short port name (without client name)"},
    {"set_name", (PyCFunction)jackport_set_name, METH_VARARGS, "Set short port name"},
    {"get_aliases", (PyCFunction)jackport_get_aliases, METH_VARARGS, "Retrieve two port aliases"},
    {NULL, NULL, 0, NULL}
};


//////////////////////////////////////////////////// calfpytools

/*
static PyObject *calfpytools_test(PyObject *self, PyObject *args)
{
    return Py_BuildValue("i", 42);
}
*/

static PyMethodDef module_methods[] = {
//    {"test", calfpytools_test, METH_VARARGS, "Do nothing, return 42"},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initcalfpytools()
{
    jackclient_type.tp_new = PyType_GenericNew;
    jackclient_type.tp_flags = Py_TPFLAGS_DEFAULT;
    jackclient_type.tp_doc = "JACK client object";
    jackclient_type.tp_methods = jackclient_methods;
    if (PyType_Ready(&jackclient_type) < 0)
        return;

    jackport_type.tp_new = PyType_GenericNew;
    jackport_type.tp_flags = Py_TPFLAGS_DEFAULT;
    jackport_type.tp_doc = "JACK port object (created by client)";
    jackport_type.tp_methods = jackport_methods;
    jackport_type.tp_init = (initproc)jackport_init;
    if (PyType_Ready(&jackport_type) < 0)
        return;
    
        PyObject *mod = Py_InitModule3("calfpytools", module_methods, "Python utilities for Calf");
    Py_INCREF(&jackclient_type);
    PyModule_AddObject(mod, "JackClient", (PyObject *)&jackclient_type);
    PyModule_AddObject(mod, "JackPort", (PyObject *)&jackport_type);    
}
