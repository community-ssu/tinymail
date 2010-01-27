/* libtinymail-camel - The Tiny Mail base library for Maemo
 * Copyright (C) 2006-2007 Philip Van Hoof <pvanhoof@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with self library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <glib.h>
#include <glib-object.h>
#include <tny-maemo-conic-device.h>
#include <conicevent.h>
#include <coniciap.h>
#include <conicconnection.h>
#include <conicconnectionevent.h>
#include <string.h>
#include <tny-error.h>
#include <gtk/gtkmessagedialog.h>

static gboolean on_dummy_connection_check (gpointer user_data);
static gboolean tny_maemo_conic_device_is_online (TnyDevice *self);
static gboolean tny_maemo_conic_device_is_forced (TnyDevice *self);

typedef struct {
	TnyMaemoConicDevice *self;
	gchar* iap_id;
	gpointer user_data;
	TnyMaemoConicDeviceConnectCallback callback;
} ConnectInfo;

/* #include "coniciap-private.h"
 * This is not installed, so we predeclare the struct instead. Of course, this 
 * is a hack and could break if the private API changes. It would be better for 
 * libconic to support scratchbox. */

struct _ConIcIap 
{
	GObject parent_instance;
	gboolean dispose_has_run;
	gchar *id;
	gchar *name;
	gchar *bearer;
};

#define MAEMO_CONIC_DUMMY_IAP_ID_FILENAME "maemo_conic_dummy_id"
#define MAEMO_CONIC_DUMMY_IAP_ID_NONE "none"

static GObjectClass *parent_class = NULL;


static gboolean
dnsmasq_has_resolv (void)
{
	/* This is because silly Conic does not have a blocking API that tells
	 * us immediately when the device is online. */

	if (!g_file_test ("/var/run/resolv.conf", G_FILE_TEST_EXISTS))
		if (!g_file_test ("/tmp/resolv.conf.wlan0", G_FILE_TEST_EXISTS))
			if (!g_file_test ("/tmp/resolv.conf.ppp0", G_FILE_TEST_EXISTS))
				return FALSE;

	return TRUE;
}

typedef struct {
	ConIcConnection *cnx;
	gboolean is_online;
	gchar *iap;
	gboolean forced; /* Whether the is_online value is forced rather than real. */
	ConnectInfo *connect_slot;
	/* When non-NULL, we are waiting for the success or failure signal. */
	gint dummy_env_check_timeout;
} TnyMaemoConicDevicePriv;

#define TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_MAEMO_CONIC_DEVICE, TnyMaemoConicDevicePriv))

typedef struct {
	GObject *self;
	gboolean status;
} EmitStatusInfo;

static gboolean
conic_emit_status_idle (gpointer user_data)
{
	EmitStatusInfo *info = (EmitStatusInfo *) user_data;

	/* We lock the gdk thread because tinymail wants implementations to do
	 * this before emitting signals from within a g_idle_add_full callback.
	 * See http://www.tinymail.org/trac/tinymail/wiki/HowTnyLockable */

	gdk_threads_enter ();
	g_signal_emit (info->self, tny_device_signals [TNY_DEVICE_CONNECTION_CHANGED],
		0, info->status);
	gdk_threads_leave ();

	return FALSE;
}

static void
conic_emit_status_destroy (gpointer user_data)
{
	EmitStatusInfo *info = (EmitStatusInfo *) user_data;
	g_object_unref (info->self);
	g_slice_free (EmitStatusInfo, info);
	return;
}

static void 
conic_emit_status (TnyDevice *self, gboolean status)
{
	/* Emit it in an idle handler: */
	EmitStatusInfo *info = g_slice_new (EmitStatusInfo);
	guint time = 1000;
	info->self = g_object_ref (self);
	info->status = status;

	if (!dnsmasq_has_resolv())
		time = 5000;

	g_timeout_add_full (G_PRIORITY_DEFAULT, time, conic_emit_status_idle,
		info, conic_emit_status_destroy);

	return;
}

static void 
tny_maemo_conic_device_reset (TnyDevice *device)
{
	TnyMaemoConicDevice *self;
	TnyMaemoConicDevicePriv *priv;
	gboolean status_before = FALSE;

	g_return_if_fail (TNY_IS_DEVICE(device));

	self = TNY_MAEMO_CONIC_DEVICE (device);
	priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self);

	priv->forced = FALSE;

	/* The only way to get the connection status is by issuing a
	   connection request with the AUTOMATICALLY_TRIGGERED
	   flag. Yes it's really sad, but there is no other way to do
	   that */
	tny_maemo_conic_device_connect_async ((TnyMaemoConicDevice *) device, NULL, FALSE, NULL, NULL);
}


static gchar*
get_dummy_filename ()
{
	gchar *filename = g_build_filename (
		g_get_home_dir (), 
		MAEMO_CONIC_DUMMY_IAP_ID_FILENAME,
		NULL);
	return filename;
}


static void 
dummy_con_ic_connection_connect_by_id_async_cb (GtkWidget *dialog,  
						gint response, 
						gpointer user_data)
{
	ConnectInfo *info = (ConnectInfo *) user_data;
	GError *error = NULL;
	gboolean canceled = FALSE;

	if (response == GTK_RESPONSE_OK) {
		/* Make a connection, by setting a name in our dummy text file,
		 * which will be read later: */
		gchar *filename = get_dummy_filename ();

		g_file_set_contents (filename, "debug id0", -1, &error);
		if(error) {
			g_warning("%s: error from g_file_set_contents(): %s\n", 
				__FUNCTION__, error->message);
		}
		g_free (filename);
	} else {
		canceled = TRUE;
		on_dummy_connection_check (info->self);
	}

	/* No need to gdk_threads_enter/leave here. We are in Gtk+'s context
	 * already (being a signal handler for a Gtk+ component) */

	if (info->callback)
		info->callback (info->self, info->iap_id, canceled, error, info->user_data);

	if (error)
		g_error_free (error);
	error = NULL;

	if (dialog)
		gtk_widget_destroy (dialog);

	g_free (info->iap_id);
	g_object_unref (info->self);
	g_slice_free (ConnectInfo, info);

	return;
}

static void 
dummy_con_ic_connection_connect_by_id_async (TnyMaemoConicDevice *self, 
					     const gchar* iap_id, 
					     gboolean user_requested,
					     TnyMaemoConicDeviceConnectCallback callback, 
					     gpointer user_data)
{
	ConnectInfo *info = g_slice_new0 (ConnectInfo);
	GtkDialog *dialog;

	info->self = (TnyMaemoConicDevice *) g_object_ref (self);
	info->callback = callback;
	info->iap_id = g_strdup (iap_id);
	info->user_data = user_data;

	/* Show a dialog, because libconic would show a dialog here,
	 * and give the user a chance to refuse a new connection, because libconic would allow that too.
	 * This allows us to see roughly similar behaviour in scratchbox as on the device. */

	if (user_requested) {
		dialog = GTK_DIALOG (gtk_message_dialog_new( NULL, GTK_DIALOG_MODAL,
							     GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL,
							     "TnyMaemoConicDevice fake scratchbox implementation:\nThe application requested a connection. Make a fake connection?"));

		g_signal_connect (dialog, "response",
				  G_CALLBACK (dummy_con_ic_connection_connect_by_id_async_cb),
				  info);

		gtk_widget_show (GTK_WIDGET (dialog));
	} else {
		dummy_con_ic_connection_connect_by_id_async_cb (NULL, GTK_RESPONSE_CANCEL, info);
	}
}


/**
 * tny_maemo_conic_device_connect:
 * @self: a #TnyDevice object
 * @iap_id: the id of the Internet Access Point (IAP), or NULL for 'any;
 * @user_requested: whether or not the connection was automatically requested or by an user action
 * @callback: a #TnyMaemoConicDeviceConnectCallback
 * @user_data: user data for @callback
 * 
 * Try to connect to a specific IAP, or to any if @iap_id == NULL
 * this calls con_ic_connection_connect(_by_id).
 * This may show a dialog to allow the user to select a connection, or 
 * may otherwise take a significant amount of time. 
 **/
void
tny_maemo_conic_device_connect_async (TnyMaemoConicDevice *self, 
				      const gchar* iap_id, 
				      gboolean user_requested,
				      TnyMaemoConicDeviceConnectCallback callback, 
				      gpointer user_data)
{
	dummy_con_ic_connection_connect_by_id_async (self, iap_id, user_requested, callback, user_data);
	return;
}


/**
 * tny_maemo_conic_device_disconnect:
 * @self: a #TnyDevice object
 * @iap_id: the id of the Internet Access Point (IAP), or NULL for 'any';
 * 
 * try to disconnect from a specific IAP, or to any if @iap_id == NULL
 * this calls con_ic_connection_disconnect(_by_id)
 * 
 * Returns TRUE if sending the command worked, FALSE otherwise
 **/
gboolean
tny_maemo_conic_device_disconnect (TnyMaemoConicDevice *self, const gchar* iap_id)
{
	/* don't try to disconnect if we're in dummy mode, as we're not "really"
	 * connected in that case either
	 */

	return TRUE;
}


static gboolean
on_dummy_connection_check (gpointer user_data)
{
	TnyMaemoConicDevice *self = NULL;
	TnyMaemoConicDevicePriv *priv = NULL;
	gchar *filename = NULL;
	gchar *contents = NULL;
	GError* error = NULL;
	gboolean test = FALSE;
	static gboolean first_time = TRUE;
		
	self = TNY_MAEMO_CONIC_DEVICE (user_data);

	priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self);

	if (priv->forced)
		return TRUE;

	/* Check whether the enviroment variable has changed, 
	 * so we can fake a connection change: */
	filename = get_dummy_filename ();
	
	test = g_file_get_contents (filename, &contents, NULL, &error);

	if(error) {
		/* g_debug("%s: error from g_file_get_contents(): %s\n", __FUNCTION__, error->message); */
		g_error_free (error);
		error = NULL;
	}
	
	if (!test || !contents) {
		/* g_debug ("DEBUG1: %s: priv->iap = %s\n", priv->iap); */
		/* Default to the first debug connection: */
		contents = g_strdup ("debug id0");
	}
	
	if (contents)
		g_strstrip(contents);

	if ((priv->iap == NULL) || (strcmp (contents, priv->iap) != 0)) {
		if (priv->iap) {
			g_free (priv->iap);
			priv->iap = NULL;
		}
			
		/* We store even the special "none" text, so we can detect changes. */
		priv->iap = g_strdup (contents);
		
		if (strcmp (priv->iap, MAEMO_CONIC_DUMMY_IAP_ID_NONE) == 0) {
			priv->is_online = FALSE;
		} else {
			priv->is_online = TRUE;
		}

		/* Do not need to emit it the first time because it's
		   called from the instance init */
		if (!first_time)		
			conic_emit_status (TNY_DEVICE (self), priv->is_online);
		else
			first_time = FALSE;
	}
	
	g_free (contents);
	g_free (filename);
	
	return TRUE;
}

/**
 * tny_maemo_conic_device_get_current_iap_id:
 * @self: a #TnyDevice object
 * 
 * retrieve the iap-id of the connection that is currently active; a precondition is
 * that we _are_ connected.
 * 
 * Returns: the iap-id for the current connection, or NULL in case of error
 **/
const gchar*
tny_maemo_conic_device_get_current_iap_id (TnyMaemoConicDevice *self)
{
	TnyMaemoConicDevicePriv *priv = NULL;

	g_return_val_if_fail (TNY_IS_MAEMO_CONIC_DEVICE(self), NULL);
	g_return_val_if_fail (tny_maemo_conic_device_is_online(TNY_DEVICE(self)), NULL);

	priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self);

	on_dummy_connection_check (self);

	/* Handle the special "none" text: */
	if (priv->iap && (strcmp (priv->iap, MAEMO_CONIC_DUMMY_IAP_ID_NONE) == 0))
		return NULL;

	return priv->iap;
}



/**
 * tny_maemo_conic_device_get_iap:
 * @self: a #TnyDevice object
 * @iap_id: the id of the IAP to get
 * 
 * get the IAP object (#ConIcIap) for the given iap-id. The returned GObject must be
 * freed with g_object_unref after use. Refer to the ConIc documentation for details about
 * the #ConICIap.
 
 * 
 * Returns: ConIcIap object or NULL in case of error
 **/
ConIcIap*
tny_maemo_conic_device_get_iap (TnyMaemoConicDevice *self, const gchar *iap_id)
{
	ConIcIap *iap = NULL;

	g_return_val_if_fail (TNY_IS_MAEMO_CONIC_DEVICE(self), NULL);
	g_return_val_if_fail (iap_id, NULL);

	/* Note that we have re-declared the private struct so that we 
	 * can do this, which is very bad and fragile: */

	iap = g_object_new (CON_IC_TYPE_IAP, NULL);
	iap->id = g_strdup(iap_id);
	iap->name = g_strdup_printf("%s name", iap->id);

	return iap;
}


/**
 * tny_maemo_conic_device_get_iap_list:
 * @self: a #TnyDevice object
 * 
 * get a list of all IAP objects (#ConIcIap) that are available; it should be freed
 * with #tny_maemo_conic_device_free_iap_list. This function uses
 * con_ic_connection_get_all_iaps
 *  
 * Returns: the list or NULL in case of error
 **/
GSList*
tny_maemo_conic_device_get_iap_list (TnyMaemoConicDevice *self)
{
	GSList* result = NULL;
	int i = 0;
	
	g_return_val_if_fail (TNY_IS_MAEMO_CONIC_DEVICE(self), NULL);

	/* libconic does not return a list of connections when running in 
	 * scratchbox, though it might do this in future when "ethernet support" 
	 * is implemented. So we return a fake list so we can exercise 
	 * functionality that uses connections: */

	for (i = 0; i < 10; ++i) 
	{
		/* Note that we have re-declared the private struct so that we 
		 * can do this, which is very bad and fragile: */

		ConIcIap *iap = g_object_new (CON_IC_TYPE_IAP, NULL);
		iap->id = g_strdup_printf("debug id%d", i);
		iap->name = g_strdup_printf("%s name", iap->id);

		result = g_slist_append (result, iap);
	}

	return result;
}


/**
 * tny_maemo_conic_device_free_iap_list:
 * @self: a #TnyDevice object
 *  
 * free a  list of IAP objects retrieved from tny_maemo_conic_device_get_iap_list
 *  
 **/
void
tny_maemo_conic_device_free_iap_list (TnyMaemoConicDevice *self, GSList* cnx_list)
{
	GSList *cur = cnx_list;
	while (cur) {
		g_object_unref (G_OBJECT(cur->data));
		cur = g_slist_next (cur);
	}
	g_slist_free (cnx_list);
}



static void 
tny_maemo_conic_device_force_online (TnyDevice *device)
{
	TnyMaemoConicDevice *self;
	TnyMaemoConicDevicePriv *priv;
	gboolean already_online = FALSE;
	g_return_if_fail (TNY_IS_DEVICE(device));
	self = TNY_MAEMO_CONIC_DEVICE (device);
	priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self);

	already_online = tny_maemo_conic_device_is_online (device);

	priv->forced = TRUE;
	priv->is_online = TRUE;

	if (priv->iap) {
		g_free (priv->iap);
		priv->iap = NULL;
	}

	/* Signal if it changed: */
	if (!already_online)
		g_signal_emit (device, tny_device_signals [TNY_DEVICE_CONNECTION_CHANGED], 0, TRUE);

	return;
}


static void
tny_maemo_conic_device_force_offline (TnyDevice *device)
{
	TnyMaemoConicDevice *self;
	TnyMaemoConicDevicePriv *priv;
	gboolean already_offline = FALSE;
	
	g_return_if_fail (TNY_IS_DEVICE(device));
	self = TNY_MAEMO_CONIC_DEVICE (device);
	priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self);

	already_offline = !tny_maemo_conic_device_is_online (device);
	priv->forced = TRUE;
	priv->is_online = FALSE;

	if (priv->iap) {
		g_free (priv->iap);
		priv->iap = NULL;
	}

	/* Signal if it changed: */
	if (!already_offline)
		conic_emit_status (device, FALSE);

	return;
}

static gboolean
tny_maemo_conic_device_is_online (TnyDevice *self)
{
	g_return_val_if_fail (TNY_IS_DEVICE(self), FALSE);

	on_dummy_connection_check (self);

	return TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self)->is_online;
}

static gboolean
tny_maemo_conic_device_is_forced (TnyDevice *self)
{
	TnyMaemoConicDevicePriv *priv;

	g_return_val_if_fail (TNY_IS_DEVICE(self), FALSE);

	priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self);

	return priv->forced;
}

static void
tny_maemo_conic_device_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyMaemoConicDevice *self = (TnyMaemoConicDevice *)instance;
	TnyMaemoConicDevicePriv *priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self);

	/* We should not have a real is_online, based on what libconic has told us: */
	priv->forced = FALSE;
	priv->iap = NULL;
	priv->is_online = dnsmasq_has_resolv ();

	/* Check if we're online right now */
	on_dummy_connection_check (self);

	/* Allow debuggers to fake a connection change by setting an environment 
	 * variable, which we check ever 1 second. This should match one of the 
	 * fake iap IDs that we created in tny_maemo_conic_device_get_iap_list().*/
	priv->dummy_env_check_timeout =
		g_timeout_add (1000, on_dummy_connection_check, self);

	return;
}


/**
 * tny_maemo_conic_device_new:
 *
 * Return value: A new #TnyDevice instance
 **/
TnyDevice*
tny_maemo_conic_device_new (void)
{
	TnyMaemoConicDevice *self = g_object_new (TNY_TYPE_MAEMO_CONIC_DEVICE, NULL);
	return TNY_DEVICE (self);
}

static void
tny_maemo_conic_device_finalize (GObject *obj)
{
	TnyMaemoConicDevicePriv *priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (obj);

	if (priv->dummy_env_check_timeout) {
		g_source_remove (priv->dummy_env_check_timeout);
		priv->dummy_env_check_timeout = 0;
	}

	if (priv->iap) {
		g_free (priv->iap);
		priv->iap = NULL;
	}
	

	(*parent_class->finalize) (obj);
}


static void
tny_device_init (gpointer g, gpointer iface_data)
{
	TnyDeviceIface *klass = (TnyDeviceIface *)g;

	klass->is_online     = tny_maemo_conic_device_is_online;
	klass->is_forced     = tny_maemo_conic_device_is_forced;
	klass->reset         = tny_maemo_conic_device_reset;
	klass->force_offline = tny_maemo_conic_device_force_offline;
	klass->force_online  = tny_maemo_conic_device_force_online;
}



static void 
tny_maemo_conic_device_class_init (TnyMaemoConicDeviceClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_maemo_conic_device_finalize;

	g_type_class_add_private (object_class, sizeof (TnyMaemoConicDevicePriv));
}

static gpointer 
tny_maemo_conic_device_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyMaemoConicDeviceClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_maemo_conic_device_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyMaemoConicDevice),
			0,      /* n_preallocs */
			tny_maemo_conic_device_instance_init    /* instance_init */
		};
	
	static const GInterfaceInfo tny_device_info = 
		{
			(GInterfaceInitFunc) tny_device_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyMaemoConicDevice",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_DEVICE, 
				     &tny_device_info);

	return GUINT_TO_POINTER (type);
}

GType 
tny_maemo_conic_device_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_maemo_conic_device_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}

static gboolean 
dummy_con_ic_connection_connect_by_id (TnyMaemoConicDevice *self, const gchar* iap_id)
{
	int response = 0;

	/* Show a dialog, because libconic would show a dialog here,
	 * and give the user a chance to refuse a new connection, because libconic would allow that too.
	 * This allows us to see roughly similar behaviour in scratchbox as on the device. */
	GtkDialog *dialog = GTK_DIALOG (gtk_message_dialog_new( NULL, GTK_DIALOG_MODAL,
			GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, 
			"TnyMaemoConicDevice fake scratchbox implementation:\nThe application requested a connection. Make a fake connection?"));

	response = gtk_dialog_run (dialog);
	gtk_widget_hide (GTK_WIDGET (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
	
	if (response == GTK_RESPONSE_OK) {
		GError* error = NULL;
		/* Make a connection, by setting a name in our dummy text file,
		 * which will be read later: */
		gchar *filename = get_dummy_filename ();

		g_file_set_contents (filename, "debug id0", -1, &error);
		if(error) {
			g_warning("%s: error from g_file_set_contents(): %s\n", __FUNCTION__, error->message);
			g_error_free (error);
			error = NULL;
		}

		g_free (filename);

		return TRUE;
	}
	else
		return FALSE;
}

gboolean
tny_maemo_conic_device_connect (TnyMaemoConicDevice *self, const gchar* iap_id, gboolean user_requested)
{
	return dummy_con_ic_connection_connect_by_id (self, iap_id);
}
