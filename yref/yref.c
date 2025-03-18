#include <Python.h>
#include "entries.h"
#include "refer.h"
#include "find_links.h"

static PyObject* refer(PyObject* self, PyObject* args) {
    PyObject *list;

    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &list)) {
        return NULL;
    }


    struct RefBuffer* data = malloc(sizeof(struct RefBuffer));
    if (!data) {
        PyErr_SetString(PyExc_MemoryError, "Memory allocation failed in refer");
        return NULL;
    }


    data->count = 0;
    data->state = 0;

    Py_ssize_t size = PyList_Size(list);
    PyObject *result = NULL;  // We'll store the result here
    for (Py_ssize_t i = 0; i < size; i++) {
        PyObject *item = PyList_GetItem(list, i);

        bool is_source = PyLong_AsLong(PyList_GetItem(item, 0)) != 0;
        char *name  = (char *)PyUnicode_AsUTF8(PyList_GetItem(item, 1));
        char *val   = (char *)PyUnicode_AsUTF8(PyList_GetItem(item, 2));
        int lang    = (int)PyLong_AsLong(PyList_GetItem(item, 3));
        char *alias = (char *)PyUnicode_AsUTF8(PyList_GetItem(item, 4));

        struct RefLink* link = _setup_link(
            data,
            alias,
            name,
            lang,
            val,
            is_source
        );

        interpret(data, link);

        if (is_source) {
          result = PyUnicode_FromString(link->render);
          break;  // We found the link, so we can stop the loop
        }
    }

    // Cleanup memory before returning
    for (int i = 0; i < data->count; i++) {
        free(data->links[i]);
    }
    free(data);

    if (result == NULL) {
        Py_RETURN_NONE;
    }

    return result;
}


static PyMethodDef DictMethods[] = {
    {"refer" , refer      , METH_VARARGS, "Process documents with references and interprets values"},
    {"find"  , find_links , METH_VARARGS, "Finds refenreces in string"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef yrefmodule = {
    PyModuleDef_HEAD_INIT,
    "yref",
    "A C extension for yref utilities",
    -1,
    DictMethods
};

PyMODINIT_FUNC PyInit_yref(void) {
    PyObject *module = PyModule_Create(&yrefmodule);

    if (!module) {
      return NULL;
    }

    PyObject *types_dict = PyDict_New();

    if (!types_dict) {
       Py_DECREF(module);
       return NULL;
    }

    PyDict_SetItemString(types_dict, "yaml" , PyLong_FromLong(TYPE_YAML));
    PyDict_SetItemString(types_dict, "json" , PyLong_FromLong(TYPE_JSON));
    PyDict_SetItemString(types_dict, "css"  , PyLong_FromLong(TYPE_CSS));

    PyModule_AddObject(module, "types", types_dict);

    return module;
}
