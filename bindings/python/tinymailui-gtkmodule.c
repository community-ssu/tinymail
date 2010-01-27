#include <pygobject.h>
#include <pygtk/pygtk.h>

void pyuigtk_register_classes (PyObject *d); 
void pyuigtk_add_constants(PyObject *module, const gchar *strip_prefix);
extern PyMethodDef pyuigtk_functions[];

DL_EXPORT(void)
inituigtk(void)
{
    PyObject *m, *d;
 
    init_pygobject ();
    init_pygtk();
    
    m = Py_InitModule ("uigtk", pyuigtk_functions);
    d = PyModule_GetDict (m);
 
    pyuigtk_register_classes (d);
    pyuigtk_add_constants (m, "TNY_");

    if (PyErr_Occurred ()) {
        PyErr_Print();
        Py_FatalError ("can't initialise module uigtk");
    }
}
