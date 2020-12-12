#include <Python.h>

PyDoc_STRVAR(module_doc, "Path finding backend");

static struct PyModuleDef backendmodule = {PyModuleDef_HEAD_INIT, "backend", module_doc, -1, NULL, NULL, NULL, NULL, NULL};

extern "C" {
    PyMODINIT_FUNC PyInit_backend(void);
}

PyMODINIT_FUNC PyInit_backend(void) {
    PyObject * m = PyModule_Create(&backendmodule);
    return m;
}
