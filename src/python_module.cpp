#include <Python.h>
#include "osmium_import.hpp"
#include "output.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include "graph.hpp"
#include "import_paths.hpp"
#include "helper.hpp"
#include "output_geojson.hpp"

#define MODULE_NAME "backend"
#define START_FUNC try {
#define ENDFUNC \
    } catch (const std::exception &e) {\
        PyErr_SetString(PyExc_Exception, e.what());\
        return NULL;\
    } catch (...) {\
        PyErr_SetString(PyExc_Exception, "unknown exception");\
        return NULL;\
    }

#define ALLOC_CHECK(o)  \
    if(NULL == o) { \
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate data"); \
        return NULL; \
    }


// ---------------- METHODS ------------
// MODULE
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
    START_FUNC;
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
    ENDFUNC;
    Py_RETURN_NONE;
}

#define calculate_distance_doc \
    "Calculate the distance between 2 points on the earths sphere. Arguments in order (x1, y1, x2, y2)"
PyObject * calculate_distance(PyObject *self, PyObject *args) {
    float x1, y1, x2, y2;
    if (!PyArg_ParseTuple(args, "ffff", &x1, &y1, &x2, &y2)) {
        return NULL;
    }
    START_FUNC;
    ClipperLib::IntPoint a{toInt(x1), toInt(y1)};
    ClipperLib::IntPoint b{toInt(x2), toInt(y2)};
    float distance = calculate_distance(a, b);
    return PyFloat_FromDouble(distance);
    ENDFUNC;
}

// --------------- TYPES ---------------

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    Graph *graph;
} PyGraph;

PyObject * pyGraphOutput(PyObject *pself, PyObject *args) {
    const char * filename;
    if (!PyArg_ParseTuple(args, "s", &filename)) {
        return NULL;
    }
    START_FUNC;
    auto self = reinterpret_cast<PyGraph *>(pself);
    self->graph->output(filename);
    ENDFUNC;
    Py_RETURN_NONE;
}

PyObject * pyGraphOutput_geojson(PyObject *pself, PyObject *args) {
    const char * filename;
    if (!PyArg_ParseTuple(args, "s", &filename)) {
        return NULL;
    }
    START_FUNC;
    auto self = reinterpret_cast<PyGraph *>(pself);
    self->graph->output_geojson(filename);
    ENDFUNC;
    Py_RETURN_NONE;
}

PyObject * pyGraphPath(PyObject *pself, PyObject *args) {
    float x1, y1, x2, y2;
    int algorithm = ALGORITHM_A_STAR;
    if (!PyArg_ParseTuple(args, "ffffi", &x1, &y1, &x2, &y2, &algorithm)) {
        return NULL;
    }
    START_FUNC;
    auto self = reinterpret_cast<PyGraph *>(pself);

    PathData p = findPath(self->graph, x1, y1, x2, y2, algorithm);

    std::stringstream out(std::ios_base::out);
    geojson_output::outputPathsStart(out);
    if(p.path.size() > 0) {
        geojson_output::outputFeaturePoint(out, p.path[0]);
        for(size_t i = 1; i < p.path.size(); i ++) {
            out << ",";
            geojson_output::outputLine(out, p.path[i - 1], p.path[i]);
            out << ",";
            geojson_output::outputFeaturePoint(out, p.path[i]);
        }
    }
    geojson_output::outputPathsEnd(out);

    std::string geojson = out.str();

    // From now on no exceptions should be thrown else the cleanup gets annoying;
    PyObject* dict = PyDict_New();
    ALLOC_CHECK(dict);

    PyObject * data_geojson = PyUnicode_FromString(geojson.c_str());
    ALLOC_CHECK(data_geojson);
    if(PyDict_SetItemString(dict, "geojson", data_geojson) < 0) {
        Py_DECREF(data_geojson);
        Py_DECREF(dict);
        return NULL;
    }
    Py_DECREF(data_geojson);

    PyObject * length = PyLong_FromSize_t(p.length);
    ALLOC_CHECK(length);
    if(PyDict_SetItemString(dict, "length", length) < 0) {
        Py_DECREF(length);
        Py_DECREF(dict);
        return NULL;
    }
    Py_DECREF(length);

    PyObject * time_taken = PyLong_FromSize_t(p.duration);
    ALLOC_CHECK(time_taken);
    if(PyDict_SetItemString(dict, "time_taken", time_taken) < 0) {
        Py_DECREF(time_taken);
        Py_DECREF(dict);
        return NULL;
    }
    Py_DECREF(time_taken);

    PyObject * heap_accesses = PyLong_FromSize_t(p.heap_accesses);
    ALLOC_CHECK(heap_accesses);
    if(PyDict_SetItemString(dict, "heap_accesses", heap_accesses) < 0) {
        Py_DECREF(heap_accesses);
        Py_DECREF(dict);
        return NULL;
    }
    Py_DECREF(heap_accesses);

    PyObject * pyDistance = PyFloat_FromDouble(p.distance);
    ALLOC_CHECK(pyDistance);
    if(PyDict_SetItemString(dict, "distance", pyDistance) < 0) {
        Py_DECREF(pyDistance);
        Py_DECREF(dict);
        return NULL;
    }
    Py_DECREF(pyDistance);
    return dict;

    ENDFUNC;
    Py_RETURN_NONE;
}

static struct PyMethodDef PyGraphMethods[] = {
    {"output", &pyGraphOutput, METH_VARARGS, ""},
    {"output_geojson", &pyGraphOutput_geojson, METH_VARARGS, ""},
    {"find_path", &pyGraphPath, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL},
};

static PyObject *PyGraphNew(PyTypeObject *type, PyObject *args, PyObject *kwds) {
        PyGraph *self;
        self = (PyGraph *)type->tp_alloc(type, 0);
        if (self != NULL) {
            self->graph = nullptr;
        }
        return (PyObject *)self;
}

static int PyGraphInit(PyObject *pself, PyObject *args, PyObject *kwds)
{
    auto self = reinterpret_cast<PyGraph *>(pself);
    const char *filename;
    long long node_count = -1;
    if (!PyArg_ParseTuple(args, "s|L", &filename, &node_count)) return -1;

    Graph *new_graph = nullptr;
    try{
        if (node_count > 0) {
            std::list<ClipperLib::Path> paths;
            paths_import::readIn(paths, filename);
            if(checkForJumpCrossing(paths)) {
                PyErr_SetString(PyExc_ValueError, "There are polygon edges that cross the 180 meridian");
                return -1;
            }
            if(node_count < 2000) {
                OutlineHolderSimple outline_holder(paths);
                new_graph = new Graph(outline_holder, node_count);
            } else {
                TreeOutlineHolder outline_holder(paths);
                new_graph = new Graph(outline_holder, node_count);
            }
        } else { // TODO
            new_graph = new Graph(filename);
        }
    } catch (const std::exception &e) {\
        PyErr_SetString(PyExc_Exception, e.what());\
        return -1;\
    } catch (...) {\
        PyErr_SetString(PyExc_Exception, "unknown exception");\
        return -1;\
    }
    delete self->graph;
    self->graph = new_graph;
    return 0;
}

static void PyGraphDealloc(PyObject *pself) {
    auto self = reinterpret_cast<PyGraph *>(pself);
    delete self->graph;
    Py_TYPE(self)->tp_free(pself);
}

PyTypeObject GraphType = [] {
    PyTypeObject r = {PyVarObject_HEAD_INIT(NULL, 0)};
    r.tp_name = MODULE_NAME ".Graph";
    r.tp_basicsize = sizeof(PyGraph);

    r.tp_itemsize = 0;
    r.tp_dealloc = PyGraphDealloc;
    r.tp_flags = Py_TPFLAGS_DEFAULT;
    r.tp_doc = "Graph";

    r.tp_methods = PyGraphMethods;
    r.tp_new = PyGraphNew;
    r.tp_init = PyGraphInit;
    return r;
}();




// --------------- MODULE --------------

PyDoc_STRVAR(module_doc, "Path finding backend");

static struct PyMethodDef moduleMethods[] = {
    {"parseOSMdata", &parseOSMdata, METH_VARARGS, parseOSMdata_doc},
    {"calculate_distance", &calculate_distance, METH_VARARGS, calculate_distance_doc},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef backendmodule = {PyModuleDef_HEAD_INIT, MODULE_NAME, module_doc, -1, moduleMethods, NULL, NULL, NULL, NULL};

extern "C" {
    PyMODINIT_FUNC PyInit_backend(void);
}

PyObject* getAlgorithmTuple() {
    PyObject* dijkstra = Py_BuildValue("(is)", ALGORITHM_DIJKSTRA, "Dijkstra");
    if(NULL == dijkstra) {
        return NULL;
    }
    PyObject* a_star = Py_BuildValue("(is)", ALGORITHM_A_STAR, "A*");
    if(NULL == a_star) {
        Py_DecRef(dijkstra);
        return NULL;
    }
    PyObject* ch_dijsktra = Py_BuildValue("(is)", ALGORITHM_CH_DIJSKTRA, "CH_DIJKSTRA");
    if(NULL == a_star) {
        Py_DecRef(ch_dijsktra);
        return NULL;
    }
    PyObject* tuple = Py_BuildValue("(NNN)", dijkstra, a_star, ch_dijsktra);
    if(NULL == tuple) {
        Py_DecRef(dijkstra);
        Py_DecRef(a_star);
        Py_DecRef(ch_dijsktra);
        return NULL;
    }
    return tuple;
}

PyMODINIT_FUNC PyInit_backend(void) {
    PyObject * m = PyModule_Create(&backendmodule);
    if(PyType_Ready(&GraphType) < 0) {
        return NULL;
    }
    PyObject* algorithmTuple = getAlgorithmTuple();
    if (NULL == algorithmTuple) {
        Py_DECREF(m);
        return NULL;
    }
    if (PyModule_AddObject(m, "ALGORITHMS", algorithmTuple) < 0) {
        Py_DECREF(m);
        Py_DECREF(algorithmTuple);
        return NULL;
    }

    Py_INCREF(&GraphType);
    if (PyModule_AddObject(m, "Graph", (PyObject *) &GraphType) < 0) {
        Py_DECREF(&GraphType);
        Py_DECREF(m);
        Py_DECREF(algorithmTuple);
        return NULL;
    }
    return m;
}
