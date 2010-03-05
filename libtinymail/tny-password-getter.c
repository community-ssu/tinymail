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
 * TnyPasswordGetter:
 *
 * Gets a password
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#ifdef DBC
#include <string.h>
#endif

#include <tny-password-getter.h>

/**
 * tny_password_getter_get_password:
 * @self: a #TnyPasswordGetter
 * @aid: a unique string identifying the requested password
 * @prompt: (null-ok): A human-readable password question
 * @cancel: (out): by reference whether or not the user cancelled
 * 
 * Get the password of @self identified by @aid. If you set the by reference
 * boolean @cancel to TRUE, the caller (who requested the password) will see 
 * this as a negative answer (For example when the user didn't know the password,
 * and therefore pressed a cancel button).
 *
 * The @aid string can be used for so called password stores. It will contain 
 * a unique string. Possible values of this string are "acap.server.com" or the
 * result of a tny_account_get_id(), or a combination of things.
 *
 * returns: (null-ok) (caller-owns): the password
 * since: 1.0
 * audience: application-developer, type-implementer, platform-developer
 **/
const gchar * 
tny_password_getter_get_password (TnyPasswordGetter *self, const gchar *aid, const gchar *prompt, gboolean *cancel)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_PASSWORD_GETTER (self));
	g_assert (TNY_PASSWORD_GETTER_GET_IFACE (self)->get_password!= NULL);
#endif

	return TNY_PASSWORD_GETTER_GET_IFACE (self)->get_password(self, aid, prompt, cancel);
}

/**
 * tny_password_getter_forget_password:
 * @self: a #TnyPasswordGetter object
 * @aid: a unique string identifying the requested password
 * 
 * Forget the password in @self identified by @aid. This usually indicates that
 * the password was wrong. A subsequent call to tny_password_getter_get_password()
 * should not result in returning the same password anymore.
 *
 * since: 1.0
 * audience: application-developer, type-implementer, platform-developer
 **/
void 
tny_password_getter_forget_password (TnyPasswordGetter *self, const gchar *aid)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_PASSWORD_GETTER (self));
	g_assert (TNY_PASSWORD_GETTER_GET_IFACE (self)->forget_password!= NULL);
#endif

	TNY_PASSWORD_GETTER_GET_IFACE (self)->forget_password(self, aid);

	return;
}



static void
tny_password_getter_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_password_getter_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyPasswordGetterIface),
			tny_password_getter_base_init,   /* base_init */
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
				       "TnyPasswordGetter", &info, 0);

	return GSIZE_TO_POINTER (type);
}

GType
tny_password_getter_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_password_getter_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
