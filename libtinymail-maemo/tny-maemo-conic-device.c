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
#include <gdk/gdk.h> /* For GDK_THREAD_ENTER/LEAVE */

static void stop_loop (TnyMaemoConicDevice *self);

static gboolean tny_maemo_conic_device_is_online (TnyDevice *self);
static gboolean tny_maemo_conic_device_is_forced (TnyDevice *self);

static GObjectClass *parent_class = NULL;

typedef struct {
	TnyMaemoConicDevice *self;
	gchar* iap_id;
	gpointer user_data;
	TnyMaemoConicDeviceConnectCallback callback;
} ConnectInfo;

typedef struct {
	ConIcConnection *cnx;
	gboolean is_online;
	gchar *iap;
	gboolean forced; /* Whether the is_online value is forced rather than real. */
	GList *connect_slots;
	GStaticMutex connect_slots_lock;
	/* When non-NULL, we are waiting for the success or failure signal. */
	GMainLoop *loop;
	gint signal1;
	guint emit_status_id;
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
	EmitStatusInfo *info;

	g_return_val_if_fail (user_data, FALSE);
	
	info  = (EmitStatusInfo *) user_data;

	g_debug ("%s: destroying %p (idle)", __FUNCTION__, user_data);

	/* We lock the gdk thread because tinymail wants implementations to do
	 * this before emitting signals from within a g_idle_add_full callback.
	 * See http://www.tinymail.org/trac/tinymail/wiki/HowTnyLockable */
	gdk_threads_enter ();
	g_signal_emit (info->self, tny_device_signals [TNY_DEVICE_CONNECTION_CHANGED],
		0, info->status);
	gdk_threads_leave ();

	g_debug ("%s: emitted tny-device-connection-changed signal", __FUNCTION__);

	return FALSE;
}

static void
conic_emit_status_destroy (gpointer user_data)
{
	EmitStatusInfo *info;

	g_return_if_fail (user_data);

	info = (EmitStatusInfo *) user_data;
	
	g_debug ("%s: destroying status info (%p)", __FUNCTION__, user_data);

	if (G_IS_OBJECT(info->self)) {
		g_object_unref (info->self);
		g_slice_free (EmitStatusInfo, info);
		g_debug ("%s: destroyed %p", __FUNCTION__, user_data);
	} else
		g_warning ("%s: BUG: not a valid info", __FUNCTION__);
}

static void
conic_emit_status (TnyDevice *self, gboolean status)
{
	EmitStatusInfo *info;
	guint time = 2500;
	TnyMaemoConicDevicePriv *priv;

	g_return_if_fail (TNY_IS_DEVICE(self));

	priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self);

	/* Emit it in an idle handler: */
	info = g_slice_new (EmitStatusInfo);
	info->self = g_object_ref (self);
	info->status = status;

	if (priv->emit_status_id) {
		g_debug ("%s: removing the previous emission", __FUNCTION__);
		g_source_remove (priv->emit_status_id);
		priv->emit_status_id = 0;
	}

	g_debug ("%s: emitting status (%p, %s) in %d ms",
		   __FUNCTION__, info, status ? "true" : "false", time);

	priv->emit_status_id = g_timeout_add_full (G_PRIORITY_DEFAULT, time,
						   conic_emit_status_idle,
						   info, conic_emit_status_destroy);
}

static void
tny_maemo_conic_device_reset (TnyDevice *device)
{
	TnyMaemoConicDevice *self;
	TnyMaemoConicDevicePriv *priv;

	g_return_if_fail (TNY_IS_DEVICE(device));

	self = TNY_MAEMO_CONIC_DEVICE (device);
	priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self);

	priv->forced = FALSE;
	if (priv->iap) {
		g_free (priv->iap);
		priv->iap = NULL;
	}

	/* The only way to get the connection status is by issuing a
	   connection request with the AUTOMATICALLY_TRIGGERED
	   flag. Yes it's really sad, but there is no other way to do
	   that */
	tny_maemo_conic_device_connect_async (device, priv->iap, FALSE, NULL, NULL);
}

static void
handle_connect (TnyMaemoConicDevice *self, int con_err, int con_state)
{
	TnyMaemoConicDevicePriv *priv;
	GList *copy, *iter;

	g_return_if_fail (TNY_IS_MAEMO_CONIC_DEVICE (self));

	priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self);

	if (!priv->connect_slots)
		return;

	/* Use a copy in order not to lock too much */
	g_static_mutex_lock (&priv->connect_slots_lock);
	copy = priv->connect_slots;
	priv->connect_slots = NULL;
	g_static_mutex_unlock (&priv->connect_slots_lock);

	iter = copy;
	while (iter) {
		GError *err = NULL;
		gboolean canceled = FALSE;
		ConnectInfo *info = (ConnectInfo *) iter->data;

		switch (con_err) {
			case CON_IC_CONNECTION_ERROR_NONE:
				break;
			case CON_IC_CONNECTION_ERROR_INVALID_IAP:
				g_set_error (&err, TNY_ERROR_DOMAIN, TNY_SERVICE_ERROR_UNKNOWN,
					"IAP is invalid");
				break;
			case CON_IC_CONNECTION_ERROR_CONNECTION_FAILED:
				g_set_error (&err, TNY_ERROR_DOMAIN, TNY_SERVICE_ERROR_UNKNOWN,
					"Connection failed");
				break;
			case CON_IC_CONNECTION_ERROR_USER_CANCELED:
			default:
				canceled = TRUE;
				break;
		}

		if (info->callback) {
			/* We lock the gdk thread because tinymail wants implementations to do
			 * this before invoking callbacks from within a g_idle_add_full callback.
			 * See http://www.tinymail.org/trac/tinymail/wiki/HowTnyLockable */

			gdk_threads_enter ();
			info->callback (info->self, info->iap_id, canceled, err, info->user_data);
			gdk_threads_leave ();
		}

		/* Frees */
		if (err)
			g_error_free (err);
		if (info->self)
			g_object_unref (info->self);
		g_free (info->iap_id);
		info->iap_id = NULL;
		g_slice_free (ConnectInfo, info);

		/* Go to next */
		iter = g_list_next (iter);
	}

	/* Free the list */
	g_list_free (copy);
}

typedef struct {
	TnyMaemoConicDevice *self;
	int con_err;
	int con_state;
} HandleConnInfo;

static gboolean 
handle_con_idle (gpointer data)
{
	HandleConnInfo *info;

	g_return_val_if_fail (data, FALSE);
	
	info = (HandleConnInfo *) data;
	handle_connect (info->self, info->con_err, info->con_state);

	return FALSE;
}

static void 
handle_con_idle_destroy (gpointer data) 
{
	HandleConnInfo *info;
	
	g_return_if_fail (data);
	
	info = (HandleConnInfo *) data;

	if (G_IS_OBJECT(info->self)) {
		g_object_unref (info->self);
		g_slice_free (HandleConnInfo, data);
	} else
		g_warning ("%s: BUG: data seems b0rked", __FUNCTION__);
}


static void
on_connection_event (ConIcConnection *cnx, ConIcConnectionEvent *event, gpointer user_data)
{
	TnyMaemoConicDevice *device;
	TnyMaemoConicDevicePriv *priv;
	gboolean is_online = FALSE;
	gboolean emit = FALSE;
	HandleConnInfo *iinfo;
	int con_err, con_state;
	const gchar *event_iap_id;

	/* we don't need cnx in this function */
	g_return_if_fail (user_data && TNY_IS_MAEMO_CONIC_DEVICE(user_data));
	g_return_if_fail (event && CON_IC_IS_CONNECTION_EVENT(event));

	device = TNY_MAEMO_CONIC_DEVICE (user_data);
	priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (device);

	/* Don't emit nor make any changes in case of forced state */

	if (priv->forced)
		return;

	con_err = con_ic_connection_event_get_error (event);
	con_state = con_ic_connection_event_get_status (event);
	event_iap_id = con_ic_event_get_iap_id ((ConIcEvent *) event);

	switch (con_err) {
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
			g_return_if_reached ();
	}

	switch (con_state) {
		case CON_IC_STATUS_CONNECTED:
			if (priv->is_online) {
				/* Emit if the IAP have changed. Means a reconnection */
				if (priv->iap && event_iap_id && !strcmp (priv->iap, event_iap_id))
					emit = FALSE;
				else
					emit = TRUE;
				g_free (priv->iap);
				priv->iap = NULL;
			} else {
				emit = TRUE;
			}
			priv->iap = g_strdup (event_iap_id);
			is_online = TRUE;
			/* Stop blocking tny_maemo_conic_device_connect(), if we are: */
			stop_loop (device);
			break;

		case CON_IC_STATUS_DISCONNECTED:
			if (priv->is_online) {
				/* If the IAP is the same then the current connection
				   was disconnected */
				if (priv->iap && event_iap_id && !strcmp (priv->iap, event_iap_id)) {
					g_free (priv->iap);
					priv->iap = NULL;
					is_online = FALSE;
					emit = TRUE;
				} else {
					/* Means that the current connection was not
					   disconnected but a previous one. For example
					   when switching between networks */
					is_online = TRUE;
					emit = FALSE;
				}
			} else {
				/* Disconnection when we are already disconnected */
				if (priv->iap) {
					g_free (priv->iap);
					priv->iap =  NULL;
				}
				is_online = FALSE;
				emit = FALSE;
			}
			/* if iap is "" then connection attempt has been canceled */

			if ((con_err == CON_IC_CONNECTION_ERROR_NONE) &&
			    (!event_iap_id || (strlen (event_iap_id)== 0)))
				con_err = CON_IC_CONNECTION_ERROR_USER_CANCELED;

			/* Stop blocking tny_maemo_conic_device_connect(), if we are: */
			stop_loop (device);
			break;

		case CON_IC_STATUS_DISCONNECTING:
			break;
		default:
			g_return_if_reached (); 
	}

	priv->is_online = is_online;

	if (priv->connect_slots) {

		iinfo = g_slice_new (HandleConnInfo);
		iinfo->self = (TnyMaemoConicDevice *) g_object_ref (device);
		iinfo->con_err = con_err;
		iinfo->con_state = con_state;

		g_idle_add_full (G_PRIORITY_HIGH, handle_con_idle, iinfo, 
				 handle_con_idle_destroy);
	}

	if (emit) {
		g_debug ("%s: emiting is_online (%s)",
			 __FUNCTION__, is_online ? "true" : "false");
		conic_emit_status (TNY_DEVICE (device), is_online);
	}
}


/**
 * tny_maemo_conic_device_connect_async:
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
	TnyMaemoConicDevicePriv *priv = NULL;
	gboolean request_failed = FALSE;
	ConnectInfo *info;
	GError *err = NULL;
	ConIcConnectFlags flags;

	g_return_if_fail (self && TNY_IS_MAEMO_CONIC_DEVICE(self));

	priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self);

	info = g_slice_new (ConnectInfo);
	info->self = (TnyMaemoConicDevice *) g_object_ref (self);
	info->callback = callback;
	info->user_data = user_data;
	info->iap_id = iap_id ? g_strdup (iap_id) : NULL; /* iap_id can be NULL */

	g_static_mutex_lock (&priv->connect_slots_lock);
	priv->connect_slots = g_list_append (priv->connect_slots, info);
	g_static_mutex_unlock (&priv->connect_slots_lock);

	/* Set the flags */
	if (user_requested)
		flags = CON_IC_CONNECT_FLAG_NONE;
	else
		flags = CON_IC_CONNECT_FLAG_AUTOMATICALLY_TRIGGERED;

	if (iap_id) {
		if (!con_ic_connection_connect_by_id (priv->cnx, iap_id, flags)) {
			g_set_error (&err, TNY_ERROR_DOMAIN, TNY_SERVICE_ERROR_UNKNOWN,
				"Could not send connect_by_id dbus message");
			request_failed = TRUE;
		}
	} else {
		if (!con_ic_connection_connect (priv->cnx, flags)) {
			g_set_error (&err, TNY_ERROR_DOMAIN, TNY_SERVICE_ERROR_UNKNOWN,
				"Could not send connect dbus message");
			request_failed = TRUE;
		}
	}

	if (request_failed) {
		/* Remove info from connect slots */
		g_static_mutex_lock (&priv->connect_slots_lock);
		priv->connect_slots = g_list_remove (priv->connect_slots, info);
		g_static_mutex_unlock (&priv->connect_slots_lock);

		/* Callback */
		if (info->callback)
			info->callback (info->self, iap_id, FALSE, err, info->user_data);

		/* Frees */
		g_object_unref (info->self);
		g_free (info->iap_id);
		g_slice_free (ConnectInfo, info);
	}
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
	TnyMaemoConicDevicePriv *priv = NULL;

	g_return_val_if_fail (self && TNY_IS_MAEMO_CONIC_DEVICE(self), FALSE);

	priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self);
	g_return_val_if_fail (priv->cnx, FALSE);

	if (iap_id) {
		if (!con_ic_connection_disconnect_by_id (priv->cnx, iap_id)) {
			g_warning ("%s: disconnect_by_id failed (%s)", 
				   __FUNCTION__, iap_id);
			return FALSE;
		}
	} else {
		if (!con_ic_connection_disconnect (priv->cnx))
			g_warning ("Could not send disconnect dbus message");
		return FALSE;
	}

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
	TnyMaemoConicDevicePriv *priv;
	g_return_val_if_fail (TNY_IS_MAEMO_CONIC_DEVICE(self), NULL);
	g_return_val_if_fail (iap_id, NULL);

	priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self);
	g_return_val_if_fail (priv->cnx, NULL);

	/* Note that it is very unusual to return a reference from a get_() 
	 * function, but we must do so because that mistake has already been 
	 * made in con_ic_connection_get_iap (). If we just unref immediately 
	 * then libconic might destroy the object. */

	return con_ic_connection_get_iap (priv->cnx, iap_id);
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
	TnyMaemoConicDevicePriv *priv;
	
	g_return_val_if_fail (TNY_IS_MAEMO_CONIC_DEVICE(self), NULL);

	priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self);
	g_return_val_if_fail (priv->cnx, NULL);

	return con_ic_connection_get_all_iaps (priv->cnx);
}

static void
unref_gobject (GObject *obj)
{
	if (G_IS_OBJECT(obj))
		g_object_unref(obj);
	else
		g_warning ("%s: not a valid GObject (%p)",
			   __FUNCTION__, obj);
}


/**
 * tny_maemo_conic_device_free_iap_list:
 * @self: a #TnyDevice object
 * @cnx_list: a list of IAP objects
 *
 * free a  list of IAP objects retrieved from tny_maemo_conic_device_get_iap_list
 **/
void
tny_maemo_conic_device_free_iap_list (TnyMaemoConicDevice *self, GSList* cnx_list)
{
	g_slist_foreach (cnx_list, (GFunc)unref_gobject, NULL);
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

	/* Signal if it changed: */
	if (!already_online) {
		g_debug ("%s: emiting connection-changed signal",
			 __FUNCTION__);
		g_signal_emit (device, tny_device_signals [TNY_DEVICE_CONNECTION_CHANGED], 0, TRUE);
	}
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

	/* Signal if it changed: */
	if (!already_offline)
		conic_emit_status (device, FALSE);
}

static gboolean
tny_maemo_conic_device_is_online (TnyDevice *self)
{
	g_return_val_if_fail (TNY_IS_DEVICE(self), FALSE);

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

	priv->forced       = FALSE;
	priv->iap          = NULL;
	priv->is_online    = FALSE;
	priv->connect_slots = NULL;
	priv->loop         = NULL;
	priv->cnx = con_ic_connection_new ();
	priv->emit_status_id = 0;
	g_static_mutex_init (&priv->connect_slots_lock);

	if (!priv->cnx) {
		g_warning ("%s: con_ic_connection_new failed.", __FUNCTION__);
		return;
	}

	/* This might be necessary to make the connection object actually emit
	 * the signal, though the documentation says that they should be sent
	 * even when this is not set, when we explicitly try to connect. The
	 * signal still does not seem to be emitted. */
	g_object_set (priv->cnx, "automatic-connection-events", TRUE, NULL);

	priv->signal1 = (gint) g_signal_connect (priv->cnx, "connection-event",
						 G_CALLBACK(on_connection_event), self);
}


/**
 * tny_maemo_conic_device_new:
 *
 * Return value: A new #TnyDevice instance
 **/
TnyDevice*
tny_maemo_conic_device_new (void)
{
	return TNY_DEVICE(g_object_new (TNY_TYPE_MAEMO_CONIC_DEVICE, NULL));
}

static void
tny_maemo_conic_device_finalize (GObject *obj)
{
	TnyMaemoConicDevicePriv *priv;

	g_return_if_fail (TNY_IS_MAEMO_CONIC_DEVICE(obj));

	g_debug ("%s: shutting the device down...", __FUNCTION__);

	priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (obj);

	if (priv->emit_status_id) {
		g_source_remove (priv->emit_status_id);
		priv->emit_status_id = 0;
	}

	if (g_signal_handler_is_connected (priv->cnx, priv->signal1))
		g_signal_handler_disconnect (priv->cnx, priv->signal1);

	if (priv->cnx && CON_IC_IS_CONNECTION(priv->cnx)) {
		tny_maemo_conic_device_disconnect (TNY_MAEMO_CONIC_DEVICE(obj),
						   priv->iap);
		g_object_unref (priv->cnx);
		priv->cnx = NULL;
	} else
		g_warning ("%s: BUG: priv->cnx is not a valid connection",
			   __FUNCTION__);

	g_free (priv->iap);
	priv->iap = NULL;
	g_static_mutex_free (&priv->connect_slots_lock);

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
static void 
stop_loop (TnyMaemoConicDevice *self)
{
	TnyMaemoConicDevicePriv *priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self);
	if (priv->loop)
		g_main_loop_quit (priv->loop);
	return;
}


/**
 * tny_maemo_conic_device_connect:
 * @self: a #TnyDevice object
 * @iap_id: the id of the Internet Access Point (IAP), or NULL for 'any;
 * 
 * Try to connect to a specific IAP, or to any if @iap_id == NULL
 * this calls con_ic_connection_connect(_by_id).
 * This may show a dialog to allow the user to select a connection, or 
 * may otherwise take a significant amount of time. This function blocks until 
 * the connection has either succeeded or failed.
 * 
 * Returns TRUE if a connection was made, FALSE otherwise.
 **/
gboolean
tny_maemo_conic_device_connect (TnyMaemoConicDevice *self, 
				const gchar* iap_id,
				gboolean user_requested)
{
	TnyMaemoConicDevicePriv *priv = NULL;
	gboolean request_failed = FALSE;
	ConIcConnectFlags flags;

	g_warning ("%s: you should tny_maemo_conic_device_connect_async",
		   __FUNCTION__);

	g_return_val_if_fail (TNY_IS_DEVICE(self), FALSE);
	priv = TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE (self);

	g_return_val_if_fail (priv->cnx, FALSE);
	priv->loop = g_main_loop_new(NULL, FALSE /* not running immediately. */);

	/* Set the flags */
	if (user_requested)
		flags = CON_IC_CONNECT_FLAG_NONE;
	else
		flags = CON_IC_CONNECT_FLAG_AUTOMATICALLY_TRIGGERED;

	if (iap_id) {
		if (!con_ic_connection_connect_by_id (priv->cnx, iap_id, flags)) {
			g_warning ("could not send connect_by_id dbus message");
			request_failed = TRUE;
		}
	} else {
		if (!con_ic_connection_connect (priv->cnx, flags)) {
			g_warning ("could not send connect dbus message");
			request_failed = TRUE;
		}
	}

	if (request_failed) {
		g_main_loop_unref (priv->loop);
		priv->loop = NULL;
	}
	
	/* Wait for the CON_IC_STATUS_CONNECTED (succeeded) or 
	 * CON_IC_STATUS_DISCONNECTED event: */
	 
	/* This is based on code found in gtk_dialog_run(): */
	GDK_THREADS_LEAVE();
	/* Run until g_main_loop_quit() is called by our signal handler. */
	if (priv->loop)
		g_main_loop_run (priv->loop);
	GDK_THREADS_ENTER();

	if (priv->loop) {
		g_main_loop_unref (priv->loop);
		priv->loop = NULL;
	}

	return priv->is_online;
}
