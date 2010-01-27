#include <pygobject.h>
 
void pyplatform_register_classes (PyObject *d); 
extern PyMethodDef pyplatform_functions[];
 
DL_EXPORT(void)
initplatform(void)
{
    PyObject *m, *d;
 
    init_pygobject ();
 
    m = Py_InitModule ("platform", pyplatform_functions);
    d = PyModule_GetDict (m);
 
    pyplatform_register_classes (d);
 
    if (PyErr_Occurred ()) {
        PyErr_Print();
    }
}
