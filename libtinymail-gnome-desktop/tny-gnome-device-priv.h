#ifndef TNY_GNOME_DEVICE_PRIV_H
#define TNY_GNOME_DEVICE_PRIV_H

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

typedef struct _TnyGnomeDevicePriv TnyGnomeDevicePriv;

struct _TnyGnomeDevicePriv
{
	gboolean forced, fset, current_state;

#ifdef GNOME
	gboolean invnm;
	guint callback_id;
	libnm_glib_ctx *nm_ctx;
#endif

};


#define TNY_GNOME_DEVICE_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_GNOME_DEVICE, TnyGnomeDevicePriv))

#endif
