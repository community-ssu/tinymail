/* Build with
 *  gcc test-device-conic.c `pkg-config gtk+-2.0 libosso libtinymail-maemo-1.0 conic --cflags --libs`
 *
 */

#include <tny-maemo-conic-device.h>
#include <gtk/gtk.h>
#include <libosso.h>

#include <stdio.h>


TnyMaemoConicDevice* device = NULL;

void on_button_clicked (gpointer user_data)
{
	g_return_if_fail (device);

	printf ("%s: Attempting to connect...\n", __FUNCTION__);
	tny_maemo_conic_device_connect (device, NULL);
	printf ("%s: ...Attempting finished.\n", __FUNCTION__);
}

static gboolean on_window_delete_event( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{
	return FALSE;
}

static void on_window_destroy( GtkWidget *widget,
                     gpointer   data )
{
	g_object_unref (device);
	printf ("%s: Destroying device (causes disconnect).\n", __FUNCTION__);
	gtk_main_quit ();
}

int main(int argc, char *argv[])
{
	gtk_init (&argc, &argv);

	osso_context_t * osso_context = osso_initialize(
	    "test_hello", "0.0.1", TRUE, NULL);
	       
	/* Check that initialization was ok */
	if (osso_context == NULL)
	{
		printf("osso_initialize() failed.\n");
	    return OSSO_ERROR;
	}
    
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *button = gtk_button_new_with_label ("Attempt connection.");
	gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (button));
	gtk_widget_show (button);

	g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (on_button_clicked), NULL);


 	g_signal_connect (G_OBJECT (window), "delete_event",
		G_CALLBACK (on_window_delete_event), NULL);
	g_signal_connect (G_OBJECT (window), "destroy",
		G_CALLBACK (on_window_destroy), NULL);
	gtk_widget_show  (window);


	device = TNY_MAEMO_CONIC_DEVICE (tny_maemo_conic_device_new ());

	gtk_main ();
    
	return 0;
}
