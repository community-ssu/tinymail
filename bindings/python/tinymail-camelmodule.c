#include <pygobject.h>
 
void pycamel_register_classes (PyObject *d); 
/* void pycamel_add_constants(PyObject *module, const gchar *strip_prefix); */
extern PyMethodDef pycamel_functions[];

DL_EXPORT(void)
initcamel(void)
{
    PyObject *m, *d;
 
    init_pygobject ();
 
    m = Py_InitModule ("camel", pycamel_functions);
    d = PyModule_GetDict (m);
 
    pycamel_register_classes (d);
/*    pycamel_add_constants (m, "TNY_"); */
       
    if (PyErr_Occurred ()) {
	PyErr_Print();
    }
}
