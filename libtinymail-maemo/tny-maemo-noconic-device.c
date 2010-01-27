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
#include <string.h>
#include <tny-error.h>

static gboolean tny_maemo_conic_device_is_online (TnyDevice *self);
static gboolean tny_maemo_conic_device_is_forced (TnyDevice *self);

static GObjectClass *parent_class = NULL;

typedef struct {
} TnyMaemoConicDevicePriv;


#define TNY_MAEMO_CONIC_DEVICE_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_MAEMO_CONIC_DEVICE, TnyMaemoConicDevicePriv))


static void 
tny_maemo_conic_device_reset (TnyDevice *device)
{
	return;
}



void 
tny_maemo_conic_device_connect_async (TnyMaemoConicDevice *self, 
				      const gchar* iap_id, 
				      gboolean user_requested,
				      TnyMaemoConicDeviceConnectCallback callback, 
				      gpointer user_data)
{
	return;
}

gboolean
tny_maemo_conic_device_disconnect (TnyMaemoConicDevice *self, const gchar* iap_id)
{
	return TRUE;
}


const gchar*
tny_maemo_conic_device_get_current_iap_id (TnyMaemoConicDevice *self)
{
	return "dummy-connection";
}


ConIcIap*
tny_maemo_conic_device_get_iap (TnyMaemoConicDevice *self, const gchar *iap_id)
{
	return NULL;
}

GSList*
tny_maemo_conic_device_get_iap_list (TnyMaemoConicDevice *self)
{
	return NULL;
}


void
tny_maemo_conic_device_free_iap_list (TnyMaemoConicDevice *self, GSList* cnx_list)
{
	return;
}


static void 
tny_maemo_conic_device_force_online (TnyDevice *device)
{
	return;
}


static void
tny_maemo_conic_device_force_offline (TnyDevice *device)
{
	return;
}

static gboolean
tny_maemo_conic_device_is_online (TnyDevice *self)
{
	return TRUE;
}

static gboolean
tny_maemo_conic_device_is_forced (TnyDevice *self)
{
	return FALSE;
}

static void
tny_maemo_conic_device_instance_init (GTypeInstance *instance, gpointer g_class)
{
	return;
}


TnyDevice*
tny_maemo_conic_device_new (void)
{
	TnyMaemoConicDevice *self = g_object_new (TNY_TYPE_MAEMO_CONIC_DEVICE, NULL);

	g_warning ("%s: using fake TnyMaemoConicDevice which assumes we're always online",
		   __FUNCTION__);
	
	return TNY_DEVICE (self);
}

static void
tny_maemo_conic_device_finalize (GObject *obj)
{
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

gboolean
tny_maemo_conic_device_connect (TnyMaemoConicDevice *self, 
				const gchar* iap_id,
				gboolean user_requested)
{
	return TRUE; /* this is a dummy, we're always online */	
}
