
#include <gtk/gtk.h>

#include <stdlib.h>

#include "check_libtinymail.h"

int
main (void)
{
     int n;
     SRunner *sr;

     gtk_init (NULL, NULL);	/* Why doesn't this require gtk CFLAGS&LIBS? */
     g_thread_init (NULL);

     sr = srunner_create (NULL);
     srunner_add_suite (sr, (Suite *) create_tny_account_store_suite ());
     srunner_add_suite (sr, (Suite *) create_tny_account_suite ());
     srunner_add_suite (sr, (Suite *) create_tny_device_suite ());
     srunner_add_suite (sr, (Suite *) create_tny_folder_store_query_suite ());
     srunner_add_suite (sr, (Suite *) create_tny_folder_store_suite ());
     srunner_add_suite (sr, (Suite *) create_tny_folder_suite ());
     srunner_add_suite (sr, (Suite *) create_tny_header_suite ());
     srunner_add_suite (sr, (Suite *) create_tny_list_suite ());
     srunner_add_suite (sr, (Suite *) create_tny_mime_part_suite ());
     srunner_add_suite (sr, (Suite *) create_tny_msg_suite ());
     srunner_add_suite (sr, (Suite *) create_tny_stream_suite ());

     srunner_run_all (sr, CK_VERBOSE);
     n = srunner_ntests_failed (sr);
     srunner_free (sr);
     return (n == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
