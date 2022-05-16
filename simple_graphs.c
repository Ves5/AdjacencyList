//#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

typedef struct ListItemStruct {
    unsigned short value;
    struct ListItemStruct* nextItem;
} ListItem;

typedef struct {
    PyObject_HEAD
    unsigned short vertices;
    ListItem* neigh[16];
} AdjacencyList;

static PyObject* AdjacencyList__new__(PyTypeObject* type, PyObject* args, PyObject* kdws){
    AdjacencyList* self;
    self = (AdjacencyList*)type->tp_alloc(type, 0);
    return (PyObject*) self;
};

static void AdjacencyList_dealloc(AdjacencyList* self){
    for(int i = 0; i < 16; i++){
        while(self->neigh[i]){
            ListItem* toRm = self->neigh[i];
            self->neigh[i] = toRm->nextItem;
            free(toRm);
        }
    }
    Py_TYPE(self)->tp_free((PyObject*) self);
}

int insertEdge(AdjacencyList* self, int v, int u){
    // insert into sorted list
    // first edge for vertex v -> head empty or with higher vertex number
    if (self->neigh[v] == NULL || self->neigh[v]->value > u){
        ListItem* newEdge = (ListItem*) malloc(sizeof(ListItem*));
        if (!newEdge) return -1;
        newEdge->value = u;
        newEdge->nextItem = self->neigh[v];
        self->neigh[v] = newEdge;
    } else {
        ListItem* current = self->neigh[v];
        // break when there isn't a next vertex or next value not bigger than vertex u number
        while(current->nextItem != NULL && current->nextItem->value <= u) {
            current = current->nextItem;
        }
        // ignoring if the pairing already exists
        if (u != current->value){
            ListItem* newEdge = (ListItem*) malloc(sizeof(ListItem*));
            if (!newEdge) return -1;
            newEdge->value = u;
            newEdge->nextItem = current->nextItem;
            current->nextItem = newEdge;
        }
    }
    // the same for vertex u
    if (self->neigh[u] == NULL || self->neigh[u]->value > v){
        ListItem* newEdge = (ListItem*) malloc(sizeof(ListItem*));
        if (!newEdge) return -1;
        newEdge->value = v;
        newEdge->nextItem = self->neigh[u];
        self->neigh[u] = newEdge;
    } else {
        ListItem* current = self->neigh[u];
        // break when there isn't a next vertex or next value not bigger than vertex v number
        while(current->nextItem != NULL && current->nextItem->value <= v) {
            current = current->nextItem;
        }
        // ignoring if the pairing already exists
        if (v != current->value){
            ListItem* newEdge = (ListItem*) malloc(sizeof(ListItem*));
            if (!newEdge) return -1;
            newEdge->value = v;
            newEdge->nextItem = current->nextItem;
            current->nextItem = newEdge;
        }
    }
    return 0;
}

static int AdjacencyList__init__(AdjacencyList* self, PyObject* args, PyObject* kwds){
    char* text = "?";
    if(!PyArg_ParseTuple(args, "|s", &text)){
        return NULL;
    }
    int length = (int) text[0] - 63; // 63 = "?"
    
    self->vertices = 0;
    unsigned short number = 0x8000;
    // starting setting vertices from leftmost bit
    for (int i = 0; i < length; i++){
        self->vertices = self->vertices | number; // setting vertex
        number = (number >> 1); // shifting number to the right
    }

    for (int i = 0; i < 16; i++){
        self->neigh[i] = NULL;
    }
    int k = 0;
    int i = 1;
    int c;
    for (int v = 1; v < length; v++){
        for (int u = 0; u < v; u++){
            if (k == 0){
                c = (int) text[i] - 63;
                i++;
                k = 6;
            }
            k--;
            if ((c & (1 << k)) != 0){
                insertEdge(self, v, u);
            }
        }
    }

    return 0;
}

static PyMemberDef AdjacencyList_Members[] = {
    {"vertices", T_USHORT, offsetof(AdjacencyList, vertices), 0, "Vertices in the graph"},
    {"neigh", T_UINT, offsetof(AdjacencyList, neigh), 0, "Neighbours of vertices"},
    {NULL}
};

static PyObject* number_of_vertices(AdjacencyList* self){
    int count = 0;
    unsigned short number = 0x8000;
    for (int i = 0; i < 16; i++){
        if(self->vertices & number) count++;
        number = number >> 1;
    }
    return Py_BuildValue("i", count);
}

static PyObject* vertices(AdjacencyList* self){
    PyObject* set = PySet_New(NULL);
    unsigned short number = 0x8000;
    for (int i = 0; i < 16; i++){
        if((self->vertices & number)){
            PyObject* py_int = Py_BuildValue("i", i);
            PySet_Add(set, py_int);
            Py_DECREF(py_int);
        }
        number = number >> 1;
    }
    return set;
}

static PyObject* vertex_degree(AdjacencyList* self, PyObject* vertex){
    int count = 0;
    int v;

    if(!PyArg_ParseTuple(vertex, "i", &v)) return NULL;
    
    ListItem* current = self->neigh[v];
    while(current){
        count++;
        current = current->nextItem;
    }

    return Py_BuildValue("i", count);
}

static PyObject* vertex_neighbors(AdjacencyList* self, PyObject* vertex){
    int v;
    if (!PyArg_ParseTuple(vertex, "i", &v)) return NULL;

    PyObject* set = PySet_New(NULL);
    ListItem* next = self->neigh[v];
    while(next){
        PyObject* py_int = Py_BuildValue("i", next->value);
        PySet_Add(set, py_int);
        Py_DECREF(py_int);
        next = next->nextItem;
    }

    return set;
}

static PyObject* add_vertex(AdjacencyList* self, PyObject* vertex){
    int v;
    if (!PyArg_ParseTuple(vertex, "i", &v)) return NULL;

    unsigned short number = 0x8000 >> v;
    self->vertices |= number;

    Py_RETURN_NONE;
}

static PyObject* delete_vertex(AdjacencyList* self, PyObject* vertex){
    int v;
    if (!PyArg_ParseTuple(vertex, "i", &v)) return NULL;

    unsigned short number = ~(0x8000 >> v);
    self->vertices &= number; // xor would toggle for already unset bit
    ListItem* next = self->neigh[v];
    if(next){ // ignore if already null
        self->neigh[v] = NULL;
        while(next){
            ListItem* nextItem = next->nextItem;
            free(next);
            next = nextItem;
        }
    }
    for(int i = 0; i < 16; i++){
        if(i != v){
            ListItem* current = self->neigh[i];
            if(current && current->value == v){
                self->neigh[i] = current->nextItem;
                free(current);
            } else {
                while(current) {
                    if (current->nextItem) {
                        if(current->nextItem->value == v){
                            ListItem* del = current->nextItem;
                            current->nextItem = del->nextItem;
                            free(del);
                        }
                    }
                    current = current->nextItem;
                }
            }
        }
    }

    Py_RETURN_NONE;
}

static PyObject* number_of_edges(AdjacencyList* self){
    int count = 0;
    for (int i = 0; i < 16; i++){
        ListItem* current = self->neigh[i];
        while(current){
            count++;
            current = current->nextItem;
        }
    }

    return Py_BuildValue("i", count/2);
}

static PyObject* edges(AdjacencyList* self){
    PyObject* set = PySet_New(NULL);
    for(int i = 0; i < 16; i++){
        ListItem* current = self->neigh[i];
        while(current){
            PyObject* py_tuple;
            if (i < current->value)
                py_tuple = Py_BuildValue("(ii)", i, current->value);
            else
                py_tuple = Py_BuildValue("(ii)", current->value, i);
            
            PySet_Add(set, py_tuple);
            Py_DECREF(py_tuple);

            current = current->nextItem;
        }
    }

    return set;
}

static PyObject* is_edge(AdjacencyList* self, PyObject* uv){
    int u,v;
    if (!PyArg_ParseTuple(uv, "ii", &u, &v)) return NULL;

    ListItem* current = self->neigh[u];
    while(current){
        if(current->value == v){
            Py_RETURN_TRUE;
        }
        current = current->nextItem;
    }

    Py_RETURN_FALSE;
}

static PyObject* add_edge(AdjacencyList* self, PyObject* uv){
    int u,v;
    if (!PyArg_ParseTuple(uv, "ii", &u, &v)) return NULL;

    if(u == v) Py_RETURN_NONE;

    unsigned short number = 0x8000;
    // at least one of them isn't a vertice
    if(((number >> u) & self->vertices) == 0 || ((number >> v) & self->vertices) == 0) return NULL;

    int res = insertEdge(self, u, v);
    if (res == -1) return NULL;

    Py_RETURN_NONE;
}

static PyObject* delete_edge(AdjacencyList* self, PyObject* uv){
    int u,v;
    if (!PyArg_ParseTuple(uv, "ii", &u, &v)) return NULL;

    if (u == v) Py_RETURN_NONE;

    unsigned short number = 0x8000;
    // at least one of them isn't a vertice
    if(((number >> u) & self->vertices) == 0 || ((number >> v) & self->vertices) == 0) return NULL;

    ListItem* current = self->neigh[u];
    if(current->value == v){
        current = current->nextItem;
        free(self->neigh[u]);
        self->neigh[u] = current;
    } else{
        while(current){
            if(current->nextItem){
                if(current->nextItem->value == v){
                    ListItem* del = current->nextItem;
                    current->nextItem = del->nextItem;
                    free(del);
                }
            }
            current = current->nextItem;
        }
    }

    current = self->neigh[v];
    if(current->value == u){
        current = current->nextItem;
        free(self->neigh[v]);
        self->neigh[v] = current;
    } else{
        while(current){
            if(current->nextItem){
                if(current->nextItem->value == u){
                    ListItem* del = current->nextItem;
                    current->nextItem = del->nextItem;
                    free(del);
                }
            }
            current = current->nextItem;
        }
    }

    Py_RETURN_NONE;
}

static PyObject* create_star(PyTypeObject* type, PyObject* nv){
    int n;
    if (!PyArg_ParseTuple(nv, "i", &n)) return NULL;

    AdjacencyList* object;
    object = (AdjacencyList*)type->tp_alloc(type, 0);
    object->vertices = 0;
    for (int i = 0; i < 16; i++){
        object->neigh[i] = NULL;
    }
    // set vertices
    unsigned short number = 0x8000;
    for(int i = 0; i < n; i++){
        object->vertices = (object->vertices | number);
        number = number >> 1;
    }
    // add edges
    for(int i = 1; i < n; i++){
        insertEdge(object, 0, i);
    }

    return (PyObject*) object;
} // requires more flags in methods struct (because it's @staticmethod)

static PyMethodDef AdjacencyList_Methods[] = {
    {"number_of_vertices", (PyCFunction) number_of_vertices, METH_NOARGS, "Returns number of vertices"},
    {"vertices", (PyCFunction) vertices, METH_NOARGS, "Returns set of graph's vertices"},
    {"vertex_degree", (PyCFunction) vertex_degree, METH_VARARGS, "Returns degree of given vertex"},
    {"vertex_neighbors", (PyCFunction) vertex_neighbors, METH_VARARGS, "Returns neighbors of given vertex"},
    {"add_vertex", (PyCFunction) add_vertex, METH_VARARGS, "Adds vertex"},
    {"delete_vertex", (PyCFunction) delete_vertex, METH_VARARGS, "Removes vertex and all edges to it"},
    {"number_of_edges", (PyCFunction) number_of_edges, METH_NOARGS, "Returns number of edges"},
    {"edges", (PyCFunction) edges, METH_NOARGS, "Returns set of graph's edges"},
    {"is_edge", (PyCFunction) is_edge, METH_VARARGS, "Checks if an edge exists"},
    {"add_edge", (PyCFunction) add_edge, METH_VARARGS, "Adds edge to the graph"},
    {"delete_edge", (PyCFunction) delete_edge, METH_VARARGS, "Removes edge from graph"},
    {"create_star", (PyCFunction) create_star, METH_CLASS|METH_VARARGS, "Creates star graph"},
    {NULL, NULL, 0, NULL}
};

static PyTypeObject AdjacencyListType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "adjacencylist.AdjacencyList",
    .tp_basicsize = sizeof(AdjacencyList),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = PyDoc_STR("Simple graph stored as adjacency list"),
    .tp_new = AdjacencyList__new__,
    .tp_init = (initproc) AdjacencyList__init__,
    .tp_dealloc = (destructor) AdjacencyList_dealloc,
    .tp_members = AdjacencyList_Members,
    .tp_methods = AdjacencyList_Methods,
};


static PyModuleDef AdjacencyList_Module = {
    PyModuleDef_HEAD_INIT,
    "adjacencylist",
    "Module for simple graph functionality",
    -1,
};

PyMODINIT_FUNC PyInit_simple_graphs(void){
    PyObject* al;
    if (PyType_Ready(&AdjacencyListType) < 0) return NULL;

    al = PyModule_Create(&AdjacencyList_Module);
    if (al == NULL) return NULL;

    Py_INCREF(&AdjacencyListType);
    if(PyModule_AddObject(al, "AdjacencyList", (PyObject*) &AdjacencyListType) < 0) {
        Py_DECREF(&AdjacencyListType);
        Py_DECREF(al);
    }
    return al;
}