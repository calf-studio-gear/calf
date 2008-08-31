#ifndef _TTLDATA_H
#define _TTLDATA_H

#include <iostream>
#include <sstream>
#include <string>
#include <Python.h>

class TTLLexer: public yyFlexLexer
{
public:
    std::string strctx;
    PyObject *pylist;

    TTLLexer(std::istream *istr) : yyFlexLexer(istr), pylist(PyList_New(0)) {}
    void add(const std::string &type, const std::string &value) { 
        PyList_Append(pylist, Py_BuildValue("(ss)", type.c_str(), value.c_str()));
        // printf("Type %s, Value %s\n", type.c_str(), value.c_str());
    }
    void add(const std::string &type, PyObject *value) { 
        PyList_Append(pylist, Py_BuildValue("(sO)", type.c_str(), value));
        PyObject *str = PyObject_Str(value);
        // printf("Type %s, Repr Value %s\n", type.c_str(), PyString_AsString(str));
        Py_DECREF(str);
    }
    PyObject *grab() {
        PyObject *tmp = pylist;
        pylist = NULL;
        return tmp;
    }
    ~TTLLexer() {
        Py_XDECREF(pylist);
    }
};

#define LEXER_DATA (dynamic_cast<TTLLexer *>(this))

#endif
