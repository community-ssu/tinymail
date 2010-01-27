#ifndef TNY_MAEMO_CONIC_DEVICE_H
#define TNY_MAEMO_CONIC_DEVICE_H

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

#include <glib.h>
#include <glib-object.h>

#include <tny-device.h>

/* this TnyMaemoConicDevice comes with three different implementations;
 * one is chosen at configure time:
 * 1) the real TnyMaemoConicDevice (tny-maemo-conic-device.c for use on real N800/N810)
 * 2) a dummy  TnyMaemoConicDevice (tny-maemo-conic-dummy-device.c, for use in Scratchbox)
 * 3) another dummy TnyMaemoConicDevice (tny-maemo-noconic-device.c. for use if
 *    libconic is not available at all (such as in Ubuntu Embedded)
 */

#ifdef LIBTINYMAIL_MAEMO_WITHOUT_CONIC
typedef struct {
} ConIcIap;
#else
#include <coniciap.h>
#endif /*LIBTINYMAIL_MAEMO_WITHOUT_CONIC*/

G_BEGIN_DECLS

#define TNY_TYPE_MAEMO_CONIC_DEVICE             (tny_maemo_conic_device_get_type ())
#define TNY_MAEMO_CONIC_DEVICE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_MAEMO_CONIC_DEVICE, TnyMaemoConicDevice))
#define TNY_MAEMO_CONIC_DEVICE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_MAEMO_CONIC_DEVICE, TnyMaemoConicDeviceClass))
#define TNY_IS_MAEMO_CONIC_DEVICE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_MAEMO_CONIC_DEVICE))
#define TNY_IS_MAEMO_CONIC_DEVICE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_MAEMO_CONIC_DEVICE))
#define TNY_MAEMO_CONIC_DEVICE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_MAEMO_CONIC_DEVICE, TnyMaemoConicDeviceClass))

/* This is an abstract type */

typedef struct _TnyMaemoConicDevice      TnyMaemoConicDevice;
typedef struct _TnyMaemoConicDeviceClass TnyMaemoConicDeviceClass;

struct _TnyMaemoConicDevice {
	GObject parent;
};

struct _TnyMaemoConicDeviceClass {
	GObjectClass parent;
};

typedef void (*TnyMaemoConicDeviceConnectCallback) (TnyMaemoConicDevice *self, const gchar* iap_id,
						    gboolean canceled, GError *err, gpointer user_data);


GType tny_maemo_conic_device_get_type (void);
TnyDevice* tny_maemo_conic_device_new (void);

gboolean tny_maemo_conic_device_connect (TnyMaemoConicDevice *self, const gchar* iap_id, gboolean user_requested);
void tny_maemo_conic_device_connect_async (TnyMaemoConicDevice *self, const gchar* iap_id, gboolean user_requested, TnyMaemoConicDeviceConnectCallback callback, gpointer user_data);
gboolean tny_maemo_conic_device_disconnect (TnyMaemoConicDevice *self, const gchar* iap_id);
const gchar* tny_maemo_conic_device_get_current_iap_id (TnyMaemoConicDevice *self);
ConIcIap* tny_maemo_conic_device_get_iap (TnyMaemoConicDevice *self, const gchar *iap_id);
GSList* tny_maemo_conic_device_get_iap_list (TnyMaemoConicDevice *self);
void tny_maemo_conic_device_free_iap_list (TnyMaemoConicDevice *self, GSList* cnx_list);

G_END_DECLS

#endif
