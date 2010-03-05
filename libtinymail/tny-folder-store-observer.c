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
 * TnyFolderStoreObserver:
 *
 * A event observer for a #TnyFolderStore
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <tny-folder-store-observer.h>

/**
 * tny_folder_store_observer_update:
 * @self: a #TnyFolderStoreObserver
 * @change: a #TnyFolderStoreChange
 *
 * Observer's update method, @change is the delta of changes between the last 
 * and the current state. It contains for example the deleted and created
 * folders in the folder store of @self.
 *
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
void 
tny_folder_store_observer_update (TnyFolderStoreObserver *self, TnyFolderStoreChange *change)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER_STORE_OBSERVER (self));
	g_assert (change);
	g_assert (TNY_IS_FOLDER_STORE_CHANGE (change));
	g_assert (TNY_FOLDER_STORE_OBSERVER_GET_IFACE (self)->update!= NULL);
#endif

	TNY_FOLDER_STORE_OBSERVER_GET_IFACE (self)->update(self, change);

#ifdef DBC /* ensure */
#endif

	return;
}


static void
tny_folder_store_observer_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_folder_store_observer_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyFolderStoreObserverIface),
			tny_folder_store_observer_base_init,   /* base_init */
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
				       "TnyFolderStoreObserver", &info, 0);
	return GSIZE_TO_POINTER (type);
}

GType
tny_folder_store_observer_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_folder_store_observer_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
