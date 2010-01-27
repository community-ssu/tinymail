
#include <gtk/gtk.h>

#include <stdlib.h>

#include "check_libtinymailui.h"

#include <gtk/gtk.h>

int
main (void)
{
     int n;
     SRunner *sr;

     gtk_init (NULL, NULL);	/* Why doesn't this require gtk CFLAGS&LIBS? */
     g_thread_init (NULL);

     sr = srunner_create ((Suite *) create_tny_platform_factory_suite ());

     srunner_run_all (sr, CK_VERBOSE);
     n = srunner_ntests_failed (sr);
     srunner_free (sr);
     return (n == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
