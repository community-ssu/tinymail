/* Build with
 *  gcc test-conic-initial-signal.c `pkg-config gtk+-2.0 libosso conic --cflags --libs`
 *
 * To test it when started from the menu (which was suspected to not work),
 * move the executable to /usr/bin/test-conic-initial-signal and make it executable by all.
 * and put test-conic-initial-signal.desktop in /usr/share/applications/hildon/.
 * It will appear in the Extras sub-menu.
 */

#include <gtk/gtk.h>

#include <conicevent.h>
#include <coniciap.h>
#include <conicconnection.h>
#include <conicconnectionevent.h>

#include <libosso.h>

#include <stdio.h>

static ConIcConnection *cnx = NULL;
static gboolean is_online = FALSE;

static GtkWidget *label = NULL, *label2 = NULL;


static gboolean on_emitted (gpointer user_data)
{
	gboolean onli = (gboolean) user_data;
	static gboolean first=TRUE;
	
	if (first)
		gtk_label_set_text (GTK_LABEL (label2), onli?"1st online onemi":"1st offline onemi");
	else
		gtk_label_set_text (GTK_LABEL (label2), onli?"2e online onemi":"2e offline onemi");
	
	first=FALSE;
	
	return FALSE;
}

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
		is_online = TRUE;

	g_idle_add (on_emitted, (gpointer)is_online);
	
		break;
	case CON_IC_STATUS_DISCONNECTED:
		printf ("DEBUG: %s: Disconnected.\n", __FUNCTION__);
		is_online = FALSE;


	g_idle_add (on_emitted, (gpointer)is_online);
	
		break;
	case CON_IC_STATUS_DISCONNECTING:
		printf ("DEBUG: %s: new status: DISCONNECTING.\n", __FUNCTION__);

		break;
	default:
		printf ("DEBUG: %s: Unexpected status.\n", __FUNCTION__);
		break;
	}

}

void use_conic()
{
	printf ("%s: Creating ConIcConnection.\n", __FUNCTION__);

	cnx = con_ic_connection_new ();
	if (!cnx) {
		g_warning ("con_ic_connection_new failed.");
	}

	const int connection = g_signal_connect (cnx, "connection-event",
		G_CALLBACK(on_connection_event), NULL);

	/* This might be necessary to make the connection object 
	 * actually emit the signal, though the documentation says 
	 * that they should be sent even when this is not set, 
	 * when we explicitly try to connect. 
	 */
	g_object_set (cnx, "automatic-connection-events", TRUE, NULL);

	printf ("%s: Attempting automatic connect. Waiting for signal.\n", __FUNCTION__);
	
	if (!con_ic_connection_connect (cnx, CON_IC_CONNECT_FLAG_AUTOMATICALLY_TRIGGERED))
		g_warning ("could not send connect dbus message");

	printf ("%s: After automatic connect. Waiting for signal.\n", __FUNCTION__);
}

void on_button_clicked (gpointer user_data)
{
	use_conic();
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

	GtkWidget *box = gtk_vbox_new (FALSE, 6);
	gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (box));
	gtk_widget_show (box);

	GtkWidget *button = gtk_button_new_with_label ("Test libconic");
	gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (button), FALSE, FALSE, 0);
	gtk_widget_show (button);

	g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (on_button_clicked), NULL);

 	use_conic();
	
	label = gtk_label_new (is_online?"initial online":"initial offline");
	gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (label), FALSE, FALSE, 0);
	gtk_widget_show (label);

	label2 = gtk_label_new ("before");
	gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (label2), FALSE, FALSE, 0);
	gtk_widget_show (label2);

 	g_signal_connect (G_OBJECT (window), "delete_event",
		G_CALLBACK (on_window_delete_event), NULL);
	g_signal_connect (G_OBJECT (window), "destroy",
		G_CALLBACK (on_window_destroy), NULL);
	gtk_widget_show  (window);   


	gtk_main ();
    
	return 0;
}
