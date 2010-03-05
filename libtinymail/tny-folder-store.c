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
 * TnyFolderStore:
 *
 * A store with folders
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#ifdef DBC
#include <string.h>
#endif

#include <tny-folder-store.h>
#include <tny-folder-store-observer.h>
#include <tny-list.h>


/**
 * tny_folder_store_add_observer:
 * @self: a #TnyFolder
 * @observer: a #TnyFolderStoreObserver
 *
 * Add @observer to the list of event observers. These observers will get notified
 * about folder creations, deletions and subscription changes. A rename will be
 * notified as a deletion followed by a creation. Folder creations, deletions,
 * renames and subscription state changes can happen spontaneous too.
 *
 * A weak reference is added to @observer. When @observer finalises will @self
 * automatically update itself by unregistering @observer. It's recommended to
 * use tny_folder_remove_observer() yourself to unregister @observer as observer
 * of @self.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_store_add_observer (TnyFolderStore *self, TnyFolderStoreObserver *observer)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER_STORE (self));
	g_assert (observer);
	g_assert (TNY_IS_FOLDER_STORE_OBSERVER (observer));
	g_assert (TNY_FOLDER_STORE_GET_IFACE (self)->add_observer!= NULL);
#endif

	TNY_FOLDER_STORE_GET_IFACE (self)->add_observer(self, observer);

#ifdef DBC /* ensure */
	/* TNY TODO: Check whether it's really added */
#endif

	return;
}


/**
 * tny_folder_store_remove_observer:
 * @self: a #TnyFolderStore
 * @observer: a #TnyFolderStoreObserver
 *
 * Remove @observer from the list of event observers of @self.
 *
 * The weak reference added by tny_folder_store_add_observer() to @observer, is
 * removed.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_store_remove_observer (TnyFolderStore *self, TnyFolderStoreObserver *observer)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER_STORE (self));
	g_assert (observer);
	g_assert (TNY_IS_FOLDER_STORE_OBSERVER (observer));
	g_assert (TNY_FOLDER_STORE_GET_IFACE (self)->remove_observer!= NULL);
#endif

	TNY_FOLDER_STORE_GET_IFACE (self)->remove_observer(self, observer);

#ifdef DBC /* ensure */
	/* TNY TODO: Check whether it's really removed */
#endif

	return;

}

/**
 * tny_folder_store_remove_folder:
 * @self: a #TnyFolderStore
 * @folder: a #TnyFolder to remove
 * @err: (null-ok): a #GError or NULL
 *
 * Removes a @folder from the folder store @self. You are responsible for 
 * unreferencing the @folder instance yourself. This method will not do this
 * for you, leaving the @folder instance in an unusable state.
 *
 * All the #TnyFolderObservers and #TnyFolderStoreObservers of @folder, will
 * automatically be unsubscribed.
 * 
 * This method will always recursively delete all child folders of @self. While
 * deleting folders, the folders will also always get unsubscribed (for example
 * in case of IMAP, which means that the folder wont be in LSUB either anymore).
 *
 * Observers of @self and of any the child folders of @self will receive delete
 * events in deletion order (childs first, parents after that). Types like the
 * #TnyGtkFolderStoreListModel know about this and act on folder deletions by
 * automatically updating themselves.
 *
 * Example:
 * <informalexample><programlisting>
 * static void
 * my_remove_a_folder (TnyFolderStore *store, TnyFolder *remfol, GError **err)
 * {
 *     tny_folder_store_remove_folder (store, remfol, err);
 *     g_object_unref (remfol);
 * }
 * </programlisting></informalexample>
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_store_remove_folder (TnyFolderStore *self, TnyFolder *folder, GError **err)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER_STORE (self));
	g_assert (folder);
	g_assert (TNY_IS_FOLDER (folder));
	g_assert (TNY_FOLDER_STORE_GET_IFACE (self)->remove_folder!= NULL);
#endif

	TNY_FOLDER_STORE_GET_IFACE (self)->remove_folder(self, folder, err);

#ifdef DBC /* ensure */
	/* Checking this is something for a unit test */
#endif

	return;
}

/**
 * tny_folder_store_create_folder:
 * @self: a #TnyFolderStore 
 * @name: The folder name to create
 * @err: (null-ok): a #GError or NULL
 *
 * Creates a new folder in @self. If not NULL, the value returned is the newly 
 * created folder instance and must be unreferenced after use.
 *
 * Example:
 * <informalexample><programlisting>
 * TnyFolderStore *store = ...
 * TnyFolder *createfol;
 * createfol = tny_folder_store_create_folder (store, "Test", NULL);
 * if (createfol) 
 *      g_object_unref (createfol);
 * </programlisting></informalexample>
 * 
 * Deprecated: 1.0: Use tny_folder_store_create_folder_async in stead
 * returns: (null-ok) (caller-owns): the folder that was created or NULL
 * since: 1.0
 * audience: application-developer
 **/
TnyFolder*
tny_folder_store_create_folder (TnyFolderStore *self, const gchar *name, GError **err)
{
	TnyFolder *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER_STORE (self));
	g_assert (name);
	g_assert (strlen (name) > 0);
	g_assert (TNY_FOLDER_STORE_GET_IFACE (self)->create_folder!= NULL);
#endif

	retval = TNY_FOLDER_STORE_GET_IFACE (self)->create_folder(self, name, err);

#ifdef DBC /* ensure */
	if (retval)
		g_assert (TNY_IS_FOLDER (retval));
#endif

	return retval;
}



/** 
 * TnyCreateFolderCallback:
 * @self: a #TnyFolderStore that caused the callback
 * @cancelled: if the operation got cancelled
 * @new_folder: (null-ok): a newly created #TnyFolder or NULL
 * @err: (null-ok): if an error occurred
 * @user_data: (null-ok):  user data that was passed to the callbacks
 *
 * A folder store callback for when a folder creation was requested. If allocated,
 * you must cleanup @user_data at the end of your implementation of the callback.
 * All other fields in the parameters of the callback are read-only.
 *
 * When cancelled, @cancelled will be TRUE while @err might nonetheless be NULL.
 * If @err is not NULL, an error occurred that you should handle gracefully.
 * The @new_folder folder parameter will only not be NULL if the creation of the
 * folder in @self was considered successful.
 *
 * since: 1.0
 * audience: application-developer
 **/

/**
 * tny_folder_store_create_folder_async:
 * @self: a #TnyFolderStore 
 * @name: The folder name to create
 * @callback: (null-ok): a #TnyCreateFolderCallback or NULL
 * @status_callback: (null-ok): a #TnyStatusCallback or NULL
 * @user_data: (null-ok): user data that will be passed to the callbacks
 *
 * Creates a new folder in @self, asynchronously.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_store_create_folder_async (TnyFolderStore *self, const gchar *name, TnyCreateFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER_STORE (self));
	g_assert (name);
	g_assert (strlen (name) > 0);
	g_assert (TNY_FOLDER_STORE_GET_IFACE (self)->create_folder_async!= NULL);
#endif

	TNY_FOLDER_STORE_GET_IFACE (self)->create_folder_async(self, name, callback, status_callback, user_data);

#ifdef DBC /* ensure */
#endif

	return;
}


/**
 * tny_folder_store_get_folders:
 * @self: a #TnyFolderStore
 * @list: a #TnyList to to which the folders will be prepended
 * @query: (null-ok): a #TnyFolderStoreQuery or NULL
 * @refresh: synchronize with the service first
 * @err: (null-ok): a #GError or NULL
 *
 * Get a list of child folders from @self. You can use @query to limit the list 
 * of folders with only folders that match a query or NULL if you don't want
 * to limit the list at all.
 * 
 * Example:
 * <informalexample><programlisting>
 * TnyFolderStore *store = ...
 * TnyIterator *iter; TnyFolderStoreQuery *query = ...
 * TnyList *folders = tny_simple_list_new ();
 * tny_folder_store_get_folders (store, folders, query, TRUE, NULL);
 * iter = tny_list_create_iterator (folders);
 * while (!tny_iterator_is_done (iter))
 * {
 *     TnyFolder *folder = TNY_FOLDER (tny_iterator_get_current (iter));
 *     g_print ("%s\n", tny_folder_get_name (folder));
 *     g_object_unref (folder);
 *     tny_iterator_next (iter);
 * }
 * g_object_unref (iter);
 * g_object_unref (folders);
 * </programlisting></informalexample>
 *
 * Deprecated: 1.0: Use tny_folder_store_get_folders_async in stead
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_store_get_folders (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, gboolean refresh, GError **err)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER_STORE (self));
	g_assert (list);
	g_assert (TNY_IS_LIST (list));
	if (query)
		g_assert (TNY_IS_FOLDER_STORE_QUERY (query));
	g_assert (TNY_FOLDER_STORE_GET_IFACE (self)->get_folders!= NULL);
#endif

	TNY_FOLDER_STORE_GET_IFACE (self)->get_folders(self, list, query, refresh, err);

#ifdef DBC /* ensure */
#endif

	return;
}



/** 
 * TnyGetFoldersCallback:
 * @self: a #TnyFolderStore that caused the callback
 * @cancelled: if the operation got cancelled
 * @list: (null-ok): a #TnyList with fetched #TnyFolder instances or NULL
 * @err: (null-ok): if an error occurred
 * @user_data: (null-ok):  user data that was passed to the callbacks
 *
 * A folder store callback for when a list of folders was requested. If allocated,
 * you must cleanup @user_data at the end of your implementation of the callback.
 * All other fields in the parameters of the callback are read-only.
 *
 * When cancelled, @cancelled will be TRUE while @err might nonetheless be NULL.
 * If @err is not NULL, an error occurred that you should handle gracefully.
 * The @list parameter might be NULL in case of cancellation or error. If not
 * NULL it contains the list of folders in @self that got fetched during the
 * request.
 *
 * since: 1.0
 * audience: application-developer
 **/

/**
 * tny_folder_store_get_folders_async:
 * @self: a #TnyFolderStore
 * @list: a #TnyList to to which the folders will be prepended
 * @query: (null-ok): A #TnyFolderStoreQuery object
 * @refresh: synchronize with the service first
 * @callback: (null-ok): a #TnyGetFoldersCallback or NULL
 * @status_callback: (null-ok): a #TnyStatusCallback or NULL
 * @user_data: (null-ok): user data that will be passed to the callbacks
 *
 * Get a list of child folders from the folder store @self and call back when 
 * finished. You can use @query to limit the list of folders with only folders 
 * that match a query or NULL if you don't want to limit the list at all.
 *
 * Example:
 * <informalexample><programlisting>
 * static void 
 * callback (TnyFolderStore *self, gboolean cancelled, TnyList *list, GError **err, gpointer user_data)
 * {
 *     TnyIterator *iter = tny_list_create_iterator (list);
 *     while (!tny_iterator_is_done (iter))
 *     {
 *         TnyFolderStore *folder = tny_iterator_get_current (iter);
 *         TnyList *folders = tny_simple_list_new ();
 *         g_print ("%s\n", tny_folder_get_name (TNY_FOLDER (folder)));
 *         tny_folder_store_get_folders_async (folder,
 *             folders, NULL, true, callback, NULL, NULL);
 *         g_object_unref (folder);
 *         tny_iterator_next (iter);
 *     }
 *     g_object_unref (iter);
 * } 
 * static void
 * get_all_folders (TnyStoreAccount *account)
 * {
 *     TnyList *folders;
 *     folders = tny_simple_list_new ();
 *     tny_folder_store_get_folders_async (TNY_FOLDER_STORE (account),
 *         folders, NULL, TRUE, callback, NULL, NULL);
 *     g_object_unref (folders);
 * }
 * </programlisting></informalexample>
 *
 * This is a cancelable operation which means that if another cancelable 
 * operation executes, this operation will be aborted. Being aborted means that
 * the callback will still be called, but with cancelled=TRUE.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_store_get_folders_async (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, gboolean refresh, TnyGetFoldersCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER_STORE (self));
	g_assert (list);
	g_assert (TNY_IS_LIST (list));
	if (query)
		g_assert (TNY_IS_FOLDER_STORE_QUERY (query));
	g_assert (TNY_FOLDER_STORE_GET_IFACE (self)->get_folders_async!= NULL);
#endif

	TNY_FOLDER_STORE_GET_IFACE (self)->get_folders_async(self, list, query, refresh, callback, status_callback, user_data);

#ifdef DBC /* ensure */
#endif

	return;
}


/**
 * tny_folder_store_refresh_async:
 * @self: a #TnyFolderStore
 * @callback: (null-ok): a #TnyFolderStoreCallback or NULL
 * @status_callback: (null-ok): a #TnyStatusCallback or NULL
 * @user_data: (null-ok): user data that will be passed to the callbacks
 *
 * Refresh @self and callback when finished. 
 *
 * While the refresh takes place, it's possible that event observers of @self
 * will get notified of new folders.
 *
 * since: 1.0
 * audience: application-developer
 **/
void tny_folder_store_refresh_async (TnyFolderStore *self, TnyFolderStoreCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER_STORE (self));
	g_assert (TNY_FOLDER_STORE_GET_IFACE (self)->refresh_async != NULL);
#endif

	TNY_FOLDER_STORE_GET_IFACE (self)->refresh_async(self, callback, status_callback, user_data);

#ifdef DBC /* ensure */
#endif

	return;
}

static void
tny_folder_store_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_folder_store_register_type (gpointer notused)
{
	GType type = 0;
	
	static const GTypeInfo info = 
		{
			sizeof (TnyFolderStoreIface),
			tny_folder_store_base_init,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL,    /* instance_init */
			NULL
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyFolderStore", &info, 0);
	return GSIZE_TO_POINTER (type);
}

GType
tny_folder_store_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_folder_store_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
