#include <pygobject.h>
 
void pytinymail_register_classes (PyObject *d); 
void pytinymail_add_constants(PyObject *module, const gchar *strip_prefix);
extern PyMethodDef pytinymail_functions[];

DL_EXPORT(void)
init_tinymail(void)
{
    PyObject *m, *d;
 
    init_pygobject ();
 
    m = Py_InitModule ("_tinymail", pytinymail_functions);
    d = PyModule_GetDict (m);
 
    pytinymail_register_classes (d);
    pytinymail_add_constants (m, "TNY_");
       
    if (PyErr_Occurred ()) {
        Py_FatalError ("can't initialise module tinymail");
    }
}
