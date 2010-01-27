#ifndef TNY_PASSWORD_GETTER_H
#define TNY_PASSWORD_GETTER_H

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

#include <glib.h>
#include <glib-object.h>

#include <tny-shared.h>
#include <tny-iterator.h>

G_BEGIN_DECLS

#define TNY_TYPE_PASSWORD_GETTER             (tny_password_getter_get_type ())
#define TNY_PASSWORD_GETTER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_PASSWORD_GETTER, TnyPasswordGetter))
#define TNY_IS_PASSWORD_GETTER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_PASSWORD_GETTER))
#define TNY_PASSWORD_GETTER_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_PASSWORD_GETTER, TnyPasswordGetterIface))

#ifndef TNY_SHARED_H
typedef struct _TnyPasswordGetter TnyPasswordGetter;
typedef struct _TnyPasswordGetterIface TnyPasswordGetterIface;
#endif

struct _TnyPasswordGetterIface
{
	GTypeInterface parent;

	const gchar* (*get_password) (TnyPasswordGetter *self, const gchar *aid, const gchar *prompt, gboolean *cancel);
	void (*forget_password) (TnyPasswordGetter *self, const gchar *aid);

};

GType tny_password_getter_get_type (void);


const gchar* tny_password_getter_get_password (TnyPasswordGetter *self, const gchar *aid, const gchar *prompt, gboolean *cancel);
void tny_password_getter_forget_password (TnyPasswordGetter *self, const gchar *aid);

G_END_DECLS

#endif
