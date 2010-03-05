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
 * TnyFolderObserver:
 *
 * A event observer for a #TnyFolder
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <tny-folder-observer.h>


/**
 * tny_folder_observer_update:
 * @self: a #TnyFolderObserver
 * @change: a #TnyFolderChange
 *
 * Observer's update method. The @change is the delta of changes between the last 
 * and the current state. It contains for example the added and removed headers
 * and the new all- and unread count of the #TnyFolder. 
 *
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
void 
tny_folder_observer_update (TnyFolderObserver *self, TnyFolderChange *change)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER_OBSERVER (self));
	g_assert (change);
	g_assert (TNY_IS_FOLDER_CHANGE (change));
	g_assert (TNY_FOLDER_OBSERVER_GET_IFACE (self)->update!= NULL);
#endif

	TNY_FOLDER_OBSERVER_GET_IFACE (self)->update(self, change);

#ifdef DBC /* ensure */
#endif

	return;
}


static void
tny_folder_observer_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_folder_observer_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyFolderObserverIface),
			tny_folder_observer_base_init,   /* base_init */
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
				       "TnyFolderObserver", &info, 0);
	return GSIZE_TO_POINTER (type);
}

GType
tny_folder_observer_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_folder_observer_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
