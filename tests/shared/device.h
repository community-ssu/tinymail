#ifndef DEVICE_H
#define DEVICE_H

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

G_BEGIN_DECLS

#define TNY_TYPE_TEST_DEVICE             (tny_test_device_get_type ())
#define TNY_TEST_DEVICE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_TEST_DEVICE, TnyTestDevice))
#define TNY_TEST_DEVICE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_TEST_DEVICE, TnyTestDeviceClass))
#define TNY_IS_TEST_DEVICE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_TEST_DEVICE))
#define TNY_IS_TEST_DEVICE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_TEST_DEVICE))
#define TNY_TEST_DEVICE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_TEST_DEVICE, TnyTestDeviceClass))

/* This is an abstract type */

typedef struct _TnyTestDevice TnyTestDevice;
typedef struct _TnyTestDeviceClass TnyTestDeviceClass;

struct _TnyTestDevice
{
	GObject parent;
};

struct _TnyTestDeviceClass 
{
	GObjectClass parent;
};

GType tny_test_device_get_type (void);
TnyDevice* tny_test_device_new (void);

G_END_DECLS

#endif

