/* libtinymail-camel - The Tiny Mail base library for Camel
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

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <tny-gnome-device.h>

/*
#ifdef GNOME
#undef GNOME
#endif
*/

#ifdef GNOME
#include <libnm_glib.h>
#endif

static GObjectClass *parent_class = NULL;

#include "tny-gnome-device-priv.h"

static void tny_gnome_device_on_online (TnyDevice *self);
static void tny_gnome_device_on_offline (TnyDevice *self);
static gboolean tny_gnome_device_is_online (TnyDevice *self);
static gboolean tny_gnome_device_is_forced (TnyDevice *self);

static gboolean
emit_status (TnyDevice *self)
{
	if (tny_gnome_device_is_online (self))
		tny_gnome_device_on_online (self);
	else
		tny_gnome_device_on_offline (self);

	return FALSE;
}


#ifdef GNOME
static void 
nm_callback (libnm_glib_ctx *nm_ctx, gpointer user_data)
{
	TnyDevice *self = (TnyDevice *)user_data;

	emit_status (self);

	return;
}
#endif

static void 
tny_gnome_device_reset (TnyDevice *self)
{
	TnyGnomeDevicePriv *priv = TNY_GNOME_DEVICE_GET_PRIVATE (self);

	const gboolean status_before = tny_gnome_device_is_online (self);

	priv->fset = FALSE;
	priv->forced = FALSE;

	/* Signal if it changed: */
	if (status_before != tny_gnome_device_is_online (self))
		g_idle_add_full (G_PRIORITY_DEFAULT, 
			(GSourceFunc) emit_status, 
			g_object_ref (self), 
			(GDestroyNotify) g_object_unref);

	return;
}

static void 
tny_gnome_device_force_online (TnyDevice *self)
{
	TnyGnomeDevicePriv *priv = TNY_GNOME_DEVICE_GET_PRIVATE (self);

	const gboolean already_online = tny_gnome_device_is_online (self);

	priv->fset = TRUE;
	priv->forced = TRUE;

	/* Signal if it changed: */
	if (!already_online)
		g_idle_add_full (G_PRIORITY_DEFAULT, 
			(GSourceFunc) emit_status, 
			g_object_ref (self), 
			(GDestroyNotify) g_object_unref);

	return;
}


static void
tny_gnome_device_force_offline (TnyDevice *self)
{
	TnyGnomeDevicePriv *priv = TNY_GNOME_DEVICE_GET_PRIVATE (self);

	const gboolean already_offline = !tny_gnome_device_is_online (self);

	priv->fset = TRUE;
	priv->forced = FALSE;

	/* Signal if it changed: */
	if (!already_offline)
		g_idle_add_full (G_PRIORITY_DEFAULT, 
			(GSourceFunc) emit_status, 
			g_object_ref (self), 
			(GDestroyNotify) g_object_unref);

	return;
}

static void
tny_gnome_device_on_online (TnyDevice *self)
{
	TnyGnomeDevicePriv *priv = TNY_GNOME_DEVICE_GET_PRIVATE (self);

	gdk_threads_enter ();
	if (!priv->current_state) {
		priv->current_state = TRUE;
		g_signal_emit (self, tny_device_signals [TNY_DEVICE_CONNECTION_CHANGED], 0, TRUE);
	}
	gdk_threads_leave ();

	return;
}

static void
tny_gnome_device_on_offline (TnyDevice *self)
{
	TnyGnomeDevicePriv *priv = TNY_GNOME_DEVICE_GET_PRIVATE (self);

	gdk_threads_enter ();
	if (priv->current_state) {
		priv->current_state = FALSE;
		g_signal_emit (self, tny_device_signals [TNY_DEVICE_CONNECTION_CHANGED], 0, FALSE);
	}
	gdk_threads_leave ();

	return;
}

static gboolean
tny_gnome_device_is_online (TnyDevice *self)
{
	TnyGnomeDevicePriv *priv = TNY_GNOME_DEVICE_GET_PRIVATE (self);
	gboolean retval = priv->forced;

#ifdef GNOME
	if (!priv->fset && !priv->invnm)
	{
		libnm_glib_state state = libnm_glib_get_network_state (priv->nm_ctx);
		
		switch (state)
		{
			case LIBNM_ACTIVE_NETWORK_CONNECTION:
			retval = TRUE;
			break;

			case LIBNM_NO_DBUS:
			case LIBNM_NO_NETWORKMANAGER:
			case LIBNM_INVALID_CONTEXT:
			g_print (_("Invalid network manager installation. Going to assume Offline status\n"));
			priv->invnm = TRUE;

			libnm_glib_unregister_callback (priv->nm_ctx, priv->callback_id);
			libnm_glib_shutdown (priv->nm_ctx);
			priv->nm_ctx = NULL;

			case LIBNM_NO_NETWORK_CONNECTION:
			default:
			retval = FALSE;
			break;
		}
	}
#endif

	return retval;
}

static gboolean
tny_gnome_device_is_forced (TnyDevice *self)
{
	TnyGnomeDevicePriv *priv = TNY_GNOME_DEVICE_GET_PRIVATE (self);

	return priv->fset;
}


/* #define IMMEDIATE_ONLINE_TEST */

static void
tny_gnome_device_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyGnomeDevice *self = (TnyGnomeDevice *)instance;
	TnyGnomeDevicePriv *priv = TNY_GNOME_DEVICE_GET_PRIVATE (self);

	priv->fset = FALSE;
	priv->forced = FALSE;

#ifdef IMMEDIATE_ONLINE_TEST
	priv->fset = TRUE;
	priv->forced = TRUE;
	priv->current_state = TRUE;
#else
	priv->current_state = FALSE;
#endif

#ifdef GNOME
	priv->invnm = FALSE;
	priv->nm_ctx = libnm_glib_init ();
#ifndef IMMEDIATE_ONLINE_TEST
	priv->current_state = tny_gnome_device_is_online (TNY_DEVICE (self));
	if (priv->nm_ctx)
		priv->callback_id = libnm_glib_register_callback 
			(priv->nm_ctx, nm_callback, self, NULL);
#endif
#endif

	return;
}



/**
 * tny_gnome_device_new:
 *
 * Create a #TnyDevice for GNOME desktops. If available, it uses NetworkManager
 * to know about the network status and changes of your computer.
 * 
 * Return value: A new #TnyDevice instance
 **/
TnyDevice*
tny_gnome_device_new (void)
{
	TnyGnomeDevice *self = g_object_new (TNY_TYPE_GNOME_DEVICE, NULL);

	return TNY_DEVICE (self);
}


static void
tny_gnome_device_finalize (GObject *object)
{
	TnyGnomeDevice *self = (TnyGnomeDevice *)object;
	TnyGnomeDevicePriv *priv = TNY_GNOME_DEVICE_GET_PRIVATE (self);

#ifdef GNOME
	if (!priv->invnm) {
		libnm_glib_unregister_callback (priv->nm_ctx, priv->callback_id);
		libnm_glib_shutdown (priv->nm_ctx);
	}
#endif

	(*parent_class->finalize) (object);

	return;
}


static void
tny_device_init (gpointer g, gpointer iface_data)
{
	TnyDeviceIface *klass = (TnyDeviceIface *)g;

	klass->is_online= tny_gnome_device_is_online;
	klass->is_forced= tny_gnome_device_is_forced;
	klass->reset= tny_gnome_device_reset;
	klass->force_offline= tny_gnome_device_force_offline;
	klass->force_online= tny_gnome_device_force_online;

	return;
}



static void 
tny_gnome_device_class_init (TnyGnomeDeviceClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_gnome_device_finalize;

	g_type_class_add_private (object_class, sizeof (TnyGnomeDevicePriv));

	return;
}

static gpointer
tny_gnome_device_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyGnomeDeviceClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_gnome_device_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyGnomeDevice),
			0,      /* n_preallocs */
			tny_gnome_device_instance_init    /* instance_init */
		};
	
	static const GInterfaceInfo tny_device_info = 
		{
			(GInterfaceInitFunc) tny_device_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyGnomeDevice",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_DEVICE, 
				     &tny_device_info);
	

	return GUINT_TO_POINTER (type);
}

GType 
tny_gnome_device_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gnome_device_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
