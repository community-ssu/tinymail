/* libtinymail- The Tiny Mail base library
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
 * TnyNoopLockable:
 *
 * A lockable that does nothing
 *
 * free-functions: g_object_unref 
 **/

#include <config.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <tny-noop-lockable.h>

static GObjectClass *parent_class = NULL;

static void
tny_noop_lockable_lock (TnyLockable *self)
{
	return;
}

static void
tny_noop_lockable_unlock (TnyLockable *self)
{
	return;
}

static void
tny_noop_lockable_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
tny_lockable_init (TnyLockableIface *klass)
{
	klass->lock= tny_noop_lockable_lock;
	klass->unlock= tny_noop_lockable_unlock;
}

static void
tny_noop_lockable_class_init (TnyNoopLockableClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	object_class->finalize = tny_noop_lockable_finalize;
}

static void
tny_noop_lockable_instance_init (GTypeInstance *instance, gpointer g_class)
{
	return;
}

/**
 * tny_noop_lockable_new:
 *
 * Create a lockable that does nothing
 *
 * returns: (caller-owns): a #TnyLockable instance that does nothing
 * since: 1.0
 * audience: application-developer
 **/
TnyLockable*
tny_noop_lockable_new (void)
{
	TnyNoopLockable *self = g_object_new (TNY_TYPE_NOOP_LOCKABLE, NULL);
	return TNY_LOCKABLE (self);
}


static gpointer
tny_noop_lockable_register_type (gpointer notused)
{
	GType type = 0;
	static const GTypeInfo info = 
		{
			sizeof (TnyNoopLockableClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_noop_lockable_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyNoopLockable),
			0,      /* n_preallocs */
			tny_noop_lockable_instance_init,    /* instance_init */
			NULL
		};

	static const GInterfaceInfo tny_lockable_info = 
		{
			(GInterfaceInitFunc) tny_lockable_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyNoopLockable",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_LOCKABLE,
				     &tny_lockable_info);

	return GSIZE_TO_POINTER (type);
}

GType
tny_noop_lockable_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_noop_lockable_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
