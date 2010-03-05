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
 * TnyLockable:
 *
 * A type for locking and unlocking things
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <tny-lockable.h>

/**
 * tny_lockable_lock:
 * @self: a #TnyLockable
 * 
 * Lock @self
 *
 * since: 1.0
 * audience: platform-developer
 **/
void 
tny_lockable_lock (TnyLockable *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_LOCKABLE (self));
	g_assert (TNY_LOCKABLE_GET_IFACE (self)->lock!= NULL);
#endif

	TNY_LOCKABLE_GET_IFACE (self)->lock(self);
	return;
}

/**
 * tny_lockable_unlock:
 * @self: a #TnyLockable
 * 
 * Unlock @self
 *
 * since: 1.0
 * audience: platform-developer
 **/
void 
tny_lockable_unlock (TnyLockable *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_LOCKABLE (self));
	g_assert (TNY_LOCKABLE_GET_IFACE (self)->unlock!= NULL);
#endif

	TNY_LOCKABLE_GET_IFACE (self)->unlock(self);
	return;
}



static void
tny_lockable_base_init (gpointer g_class)
{
	return;
}

static gpointer
tny_lockable_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyLockableIface),
			tny_lockable_base_init,   /* base_init */
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
				       "TnyLockable", &info, 0);
	return GSIZE_TO_POINTER (type);
}

GType
tny_lockable_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_lockable_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
