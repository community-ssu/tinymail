/* Build with
 *  gcc test-conic.c `pkg-config gtk+-2.0 conic --cflags --libs`
 */

#include <gtk/gtk.h>

#include <conicevent.h>
#include <coniciap.h>
#include <conicconnection.h>
#include <conicconnectionevent.h>

#include <libosso.h>

#include <stdio.h>

static ConIcConnection *cnx = NULL;

static void
on_connection_event (ConIcConnection *cnx, ConIcConnectionEvent *event, gpointer user_data)
{
	printf ("%s\n", __FUNCTION__);

	/* Handle errors: */
	switch (con_ic_connection_event_get_error(event)) {
	case CON_IC_CONNECTION_ERROR_NONE:
		break;
	case CON_IC_CONNECTION_ERROR_INVALID_IAP:
		g_warning ("conic: IAP is invalid");
		break;
	case CON_IC_CONNECTION_ERROR_CONNECTION_FAILED:
		g_warning ("conic: connection failed");
		break;
	case CON_IC_CONNECTION_ERROR_USER_CANCELED:
		g_warning ("conic: user cancelled");
		break;
	default:
		g_warning ("conic: unexpected error");
		break;
	}

	/* Handle status changes: */
	switch (con_ic_connection_event_get_status(event)) {
	case CON_IC_STATUS_CONNECTED:
		printf ("DEBUG: %s: Connected.\n", __FUNCTION__);
		break;
	case CON_IC_STATUS_DISCONNECTED:
		printf ("DEBUG: %s: Disconnected.\n", __FUNCTION__);
		break;
	case CON_IC_STATUS_DISCONNECTING:
		printf ("DEBUG: %s: new status: DISCONNECTING.\n", __FUNCTION__);

		break;
	default:
		printf ("DEBUG: %s: Unexpected status.\n", __FUNCTION__);
		break;
	}

}

bool connect_and_wait()
{
	printf ("%s: Creating ConIcConnection.\n", __FUNCTION__);
	cnx = con_ic_connection_new ();
	if (!cnx) {
		g_warning ("con_ic_connection_new failed.");
	}
	g_signal_connect (cnx, "connection-event",
			  G_CALLBACK(on_connection_event), NULL);

	printf ("%s: Attempting to connect. Waiting for signal.\n", __FUNCTION__);
	
	if (!con_ic_connection_connect (cnx, CON_IC_CONNECT_FLAG_NONE)) {
		g_warning ("con_ic_connection_connect() failed.");
	}
}

void on_button_clicked (gpointer user_data)
{
	printf ("%s: Calling connect_and_wait().\n", __FUNCTION__);
	gboolean result = connect_and_wait();
	printf ("%s: connect_and_wait() returned %d.\n", __FUNCTION__, result);
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
   

	
	

	/* This might be necessary to make the connection object 
	 * actually emit the signal, though the documentation says 
	 * that they should be sent even when this is not set, 
	 * when we explicitly try to connect. 
	 * The signal still does not seem to be emitted.
	 */
	g_object_set (cnx, "automatic-connection-events", TRUE, NULL);


 	g_signal_connect (G_OBJECT (window), "delete_event",
		G_CALLBACK (on_window_delete_event), NULL);
	g_signal_connect (G_OBJECT (window), "destroy",
		G_CALLBACK (on_window_destroy), NULL);
	gtk_widget_show  (window);    
	gtk_main ();
    
	return 0;
}
