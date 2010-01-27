#ifndef TNY_GNOME_KEYRING_PASSWORD_GETTER_H
#define TNY_GNOME_KEYRING_PASSWORD_GETTER_H

/* libtinymail-gnome-desktop - The Tiny Mail base library for Gnome
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

#include <gtk/gtk.h>
#include <glib-object.h>
#include <tny-shared.h>
#include <tny-password-getter.h>
#include <libgnomeui/gnome-password-dialog.h>
#include <gnome-keyring.h>

G_BEGIN_DECLS

#define TNY_TYPE_GNOME_KEYRING_PASSWORD_GETTER             (tny_gnome_keyring_password_getter_get_type ())
#define TNY_GNOME_KEYRING_PASSWORD_GETTER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_GNOME_KEYRING_PASSWORD_GETTER, TnyGnomeKeyringPasswordGetter))
#define TNY_GNOME_KEYRING_PASSWORD_GETTER_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_GNOME_KEYRING_PASSWORD_GETTER, TnyGnomeKeyringPasswordGetterClass))
#define TNY_IS_GNOME_KEYRING_PASSWORD_GETTER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_GNOME_KEYRING_PASSWORD_GETTER))
#define TNY_IS_GNOME_KEYRING_PASSWORD_GETTER_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_GNOME_KEYRING_PASSWORD_GETTER))
#define TNY_GNOME_KEYRING_PASSWORD_GETTER_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_GNOME_KEYRING_PASSWORD_GETTER, TnyGnomeKeyringPasswordGetterClass))

typedef struct _TnyGnomeKeyringPasswordGetter TnyGnomeKeyringPasswordGetter;
typedef struct _TnyGnomeKeyringPasswordGetterClass TnyGnomeKeyringPasswordGetterClass;

struct _TnyGnomeKeyringPasswordGetter
{
	GObject parent;
};

struct _TnyGnomeKeyringPasswordGetterClass
{
	GObjectClass parent_class;
};

GType tny_gnome_keyring_password_getter_get_type (void);
TnyPasswordGetter* tny_gnome_keyring_password_getter_new (void);


G_END_DECLS

#endif
