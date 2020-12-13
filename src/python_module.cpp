#include <Python.h>
#include "osmium_import.hpp"
#include "output.hpp"
#include <fstream>

#define START_FUNC try {
#define ENDFUNC \
    } catch (const std::exception &e) {\
        PyErr_SetString(PyExc_IOError, e.what());\
        return NULL;\
    } catch (...) {\
        PyErr_SetString(PyExc_IOError, "unknown exception");\
        return NULL;\
    }



#define parseOSMdata_doc \
    "Takes 3 parameters" \
    "inputFilename" \
    "geojsonOutputFilename - can be None" \
    "internalFormatFilename - can be None"
PyObject * parseOSMdata(PyObject *self, PyObject *args) {
    const char *inputFilename, *geojsonOutputFilename, *internalFormatFilename;
    if (!PyArg_ParseTuple(args, "szz", &inputFilename, &geojsonOutputFilename, &internalFormatFilename)) {
        return NULL;
    }
    if(NULL != geojsonOutputFilename && NULL != internalFormatFilename) {
        std::ofstream geojson(geojsonOutputFilename);
        std::ofstream paths(internalFormatFilename, std::ios_base::binary);
        importAndOutputFile(inputFilename, geojson, paths);
    } else if(NULL != geojsonOutputFilename) {
        NullStream geojson;
        std::ofstream paths(internalFormatFilename, std::ios_base::binary);
        importAndOutputFile(inputFilename, geojson, paths);
    } else {
        NullStream geojson;
        NullStream paths;
        importAndOutputFile(inputFilename, geojson, paths);
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(module_doc, "Path finding backend");

static struct PyMethodDef methods[] = {
    {"parseOSMdata", &parseOSMdata, METH_VARARGS, parseOSMdata_doc},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef backendmodule = {PyModuleDef_HEAD_INIT, "backend", module_doc, -1, methods, NULL, NULL, NULL, NULL};

extern "C" {
    PyMODINIT_FUNC PyInit_backend(void);
}

PyMODINIT_FUNC PyInit_backend(void) {
    PyObject * m = PyModule_Create(&backendmodule);
    return m;
}
