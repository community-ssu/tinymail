/* libtinymail - The Tiny Mail base library
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

/**
 * TnyDevice:
 * 
 * A device, with online and offline state
 *
 * free-function: g_object_unref
 */

#include <config.h>

#include <tny-device.h>

guint tny_device_signals [TNY_DEVICE_LAST_SIGNAL];

/**
 * tny_device_reset:
 * @self: a #TnyDevice
 * 
 * Reset the status of @self, unforce the status.
 *
 * This reverses the effects of tny_device_force_online and
 * tny_device_force_offline. Future changes of connection status will
 * cause the connection_changed signal to be emitted, and tny_device_is_online
 * will return a correct value.
 *
 * The connection_changed signal will be emitted if to return a different value
 * than before, for instance if the network connection has actually become
 * available or unavailable while the status was forced. For example in case
 * the forced state was offline, and after reset the actual state is online. Or
 * in case the forced state was online, and after reset the actual state is 
 * offline.
 *
 * since: 1.0
 * audience: platform-developer, type-implementer
 **/
void 
tny_device_reset (TnyDevice *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_DEVICE (self));
	g_assert (TNY_DEVICE_GET_IFACE (self)->reset!= NULL);
#endif

	TNY_DEVICE_GET_IFACE (self)->reset(self);

#ifdef DBC /* ensure */
#endif

	return;
}

/**
 * tny_device_force_online:
 * @self: a #TnyDevice
 * 
 * Force online status, so that tny_device_is_online() returns TRUE, regardless 
 * of whether there is an actual network connection. The connection_changed 
 * signal will be emitted if the returned value of tny_device_is_online()
 * changed by this function.
 *
 * This can be used on platforms that cannot detect whether a network connection
 * exists. This will usually not attempt to make a real network connection. 
 *
 * See also tny_device_force_offline() and tny_device_reset().
 *
 * since: 1.0
 * audience: platform-developer, type-implementer
 **/
void 
tny_device_force_online (TnyDevice *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_DEVICE (self));
	g_assert (TNY_DEVICE_GET_IFACE (self)->force_online!= NULL);
#endif

	TNY_DEVICE_GET_IFACE (self)->force_online(self);

#ifdef DBC /* ensure */
	g_assert (tny_device_is_online (self) == TRUE);
#endif

	return;
}

/**
 * tny_device_force_offline:
 * @self: a #TnyDevice
 * 
 * Force offline status, so that tny_device_is_online() returns FALSE, regardless 
 * of whether there is an actual network connection.  The connection_changed 
 * signal will be emitted if the returned value of tny_device_is_online()
 * changed by this function.
 *
 * This can be used to mark a device as offline if the connection is unusable 
 * due to some specific error, such as a failure to access a server or to use
 * a particular port, or if the user specifically chose "offline mode". It can
 * also be used on platforms that cannot detect whether a network connection
 * exists. This will usually not attempt to disconnect a real network connection.
 *
 * Example:
 * <informalexample><programlisting>
 * TnyDevice *device = ...
 * tny_device_force_offline (device);
 * if (tny_device_is_online (device))
 *      g_print ("Something is wrong\n");
 * tny_device_reset (device);
 * </programlisting></informalexample>
 *
 * See also tny_device_force_online() and tny_device_reset().
 *
 * since: 1.0
 * audience: platform-developer, type-implementer
 **/
void
tny_device_force_offline (TnyDevice *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_DEVICE (self));
	g_assert (TNY_DEVICE_GET_IFACE (self)->force_offline!= NULL);
#endif

	TNY_DEVICE_GET_IFACE (self)->force_offline(self);

#ifdef DBC /* ensure */
	g_assert (tny_device_is_online (self) == FALSE);
#endif

	return;
}

/**
 * tny_device_is_online:
 * @self: a #TnyDevice
 * 
 * Request the current state of @self. In case of forced online, this function
 * returns TRUE. In case of forced offline, this function returns FALSE. In case
 * of online, this function returns TRUE. In case of offline, this function
 * returns FALSE. 
 *
 * Example:
 * <informalexample><programlisting>
 * TnyDevice *device = ...
 * tny_device_force_online (device);
 * if (!tny_device_is_online (device))
 *      g_print ("Something is wrong\n");
 * tny_device_reset (device);
 * </programlisting></informalexample>
 * 
 * returns: TRUE if online or forced online, FALSE if offline or forced offline
 * since: 1.0
 * audience: platform-developer, type-implementer
 **/
gboolean 
tny_device_is_online (TnyDevice *self)
{
	gboolean retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_DEVICE (self));
	g_assert (TNY_DEVICE_GET_IFACE (self)->is_online!= NULL);
#endif

	retval = TNY_DEVICE_GET_IFACE (self)->is_online(self);

#ifdef DBC /* ensure */
#endif

	return retval;
}

static void
tny_device_base_init (gpointer g_class)
{
	static gboolean tny_device_initialized = FALSE;

	if (!tny_device_initialized) 
	{
/**
 * TnyDevice::connection-changed
 * @self: the object on which the signal is emitted
 * @arg1: Whether or not the device is now online
 * @user_data: (null-ok): user data set when the signal handler was connected.
 *
 * Emitted when the connection status of a device changes.
 * This signal will not be emitted in response to actual connection changes 
 * while the status is forced with tny_device_force_online() or 
 * tny_device_force_offline().
 *
 * since: 1.0
 * audience: platform-developer, type-implementer
 **/
		tny_device_signals[TNY_DEVICE_CONNECTION_CHANGED] =
		   g_signal_new ("connection_changed",
			TNY_TYPE_DEVICE,
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (TnyDeviceIface, connection_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__BOOLEAN, G_TYPE_NONE, 1,
				G_TYPE_BOOLEAN);

		tny_device_initialized = TRUE;
	}
}

static gpointer
tny_device_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyDeviceIface),
			tny_device_base_init,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL,   /* instance_init */
			NULL
		};
	
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyDevice", &info, 0);
	
	g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);

	return GSIZE_TO_POINTER (type);
}

GType
tny_device_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	g_once (&once, tny_device_register_type, NULL);

	return GPOINTER_TO_SIZE (once.retval);
}


static gpointer
tny_device_signal_register_type (gpointer notused)
{
  GType etype = 0;
  static const GEnumValue values[] = {
    { TNY_DEVICE_CONNECTION_CHANGED, "TNY_DEVICE_CONNECTION_CHANGED", "connection-changed" },
    { TNY_DEVICE_LAST_SIGNAL, "TNY_DEVICE_LAST_SIGNAL", "last-signal" },
    { 0, NULL, NULL }
  };
  etype = g_enum_register_static ("TnyDeviceSignal", values);
  return GSIZE_TO_POINTER (etype);
}

/**
 * tny_device_signal_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType
tny_device_signal_get_type (void)
{
  static GOnce once = G_ONCE_INIT;
  g_once (&once, tny_device_signal_register_type, NULL);
  return GPOINTER_TO_SIZE (once.retval);
}

/**
 * tny_device_is_forced:
 * @self: a #TnyDevice
 *
 * Request the current state of @self. In case of forced online or
 * forced offline this function returns TRUE. Otherwise this function
 * returns FALSE
 *
 * Example:
 * <informalexample><programlisting>
 * TnyDevice *device = ...
 * tny_device_force_online (device);
 * if (tny_device_is_forced (device))
 *      tny_device_reset (device);
 * else
 *      g_print ("Something went wrong\n");
 * </programlisting></informalexample>
 *
 * returns: TRUE if forced online/offline, FALSE otherwise
 * since: 1.0
 * audience: platform-developer, type-implementer
 **/
gboolean
tny_device_is_forced (TnyDevice *self)
{
	gboolean retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_DEVICE (self));
	g_assert (TNY_DEVICE_GET_IFACE (self)->is_forced!= NULL);
#endif

	retval = TNY_DEVICE_GET_IFACE (self)->is_forced(self);

#ifdef DBC /* ensure */
#endif

	return retval;
}
