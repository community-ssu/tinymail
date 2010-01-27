#include <pygobject.h>
 
void pyui_register_classes (PyObject *d); 
extern PyMethodDef pyui_functions[];

DL_EXPORT(void)
initui(void)
{
    PyObject *m, *d;
 
    init_pygobject ();
 
    m = Py_InitModule ("ui", pyui_functions);
    d = PyModule_GetDict (m);
 
    pyui_register_classes (d);
 
    if (PyErr_Occurred ()) {
        Py_FatalError ("can't initialise module ui");
    }
}
