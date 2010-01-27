/* libtinymail - The Tiny Mail base library
 * Copyright (C) 2006-2007 Philip Van Hoof <pvanhoof@gnome.org>
 * Copyright (C) 2008 Rob Taylor <rob.taylor@codethink.co.uk>
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
 * TnyMergeFolderStore:
 *
 * Merges together two or more folder stores.
 * All write operations are performed against the base folder store.
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#ifdef DBC
#include <string.h>
#endif

#include <tny-folder-store.h>
#include <tny-folder-store-observer.h>
#include <tny-merge-folder-store.h>
#include <tny-list.h>
#include <tny-simple-list.h>
#include <tny-error.h>
#include "observer-mixin-priv.h"

static GObjectClass *parent_class = NULL;

#define TNY_MERGE_FOLDER_STORE_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_MERGE_FOLDER_STORE, TnyMergeFolderStorePriv))

typedef struct _TnyMergeFolderStorePriv TnyMergeFolderStorePriv;
struct _TnyMergeFolderStorePriv
{
	GStaticRecMutex *lock;
	TnyList *stores;
	ObserverMixin observers;
	GHashTable *known_folders;
	TnyLockable *ui_lock;
};


static void
known_folder_del (gpointer user_data, GObject *folder)
{
	TnyMergeFolderStorePriv *priv = TNY_MERGE_FOLDER_STORE_GET_PRIVATE (user_data);
	g_hash_table_remove (priv->known_folders, folder);
}


static gboolean
known_folder_remover (GObject *folder,
                      gpointer value,
                      TnyMergeFolderStore *self)
{
	g_object_weak_unref (folder, known_folder_del, self);
	return TRUE;
}


static void
tny_merge_folder_store_observer_update(TnyFolderStoreObserver *self, TnyFolderStoreChange *change)
{
	TnyMergeFolderStorePriv *priv = TNY_MERGE_FOLDER_STORE_GET_PRIVATE (self);
	TnyFolderStoreChangeChanged changed = tny_folder_store_change_get_changed (change);

	g_static_rec_mutex_lock (priv->lock);

	if (changed & TNY_FOLDER_STORE_CHANGE_CHANGED_CREATED_FOLDERS) {
		TnyList *added = tny_simple_list_new();
		TnyIterator *iter;
	
		tny_folder_store_change_get_created_folders (change, added);
		iter = tny_list_create_iterator (added);
		while (!tny_iterator_is_done (iter)) {
			TnyFolder *folder = TNY_FOLDER(tny_iterator_get_current (iter));
			if (!g_hash_table_lookup_extended (priv->known_folders, folder, NULL, NULL)) {
				g_hash_table_insert(priv->known_folders, folder, NULL);
				g_object_weak_ref (G_OBJECT(folder), known_folder_del, self);
			}
			g_object_unref (folder);
			tny_iterator_next (iter);
		}
		g_object_unref (iter);
		g_object_unref (added);
	}
	if (changed & TNY_FOLDER_STORE_CHANGE_CHANGED_REMOVED_FOLDERS) {
		TnyList *removed = tny_simple_list_new ();
		TnyIterator *iter;

		tny_folder_store_change_get_removed_folders (change, removed);
		iter = tny_list_create_iterator (removed);
		while (!tny_iterator_is_done (iter)) {
			TnyFolder *folder = TNY_FOLDER(tny_iterator_get_current (iter));
			g_hash_table_remove(priv->known_folders, folder);
			g_object_weak_unref (G_OBJECT(folder), known_folder_del, self);
			g_object_unref (folder);
			tny_iterator_next (iter);
		}

		g_object_unref (iter);
		g_object_unref (removed);
	}

	g_static_rec_mutex_unlock (priv->lock);

	_observer_mixin_notify_observers_about (&(priv->observers), (ObserverUpdateMethod) tny_folder_store_observer_update, change, priv->ui_lock);
}

static void
tny_merge_folder_store_add_observer_default (TnyFolderStore *self, TnyFolderStoreObserver *observer)
{
	TnyMergeFolderStorePriv *priv = TNY_MERGE_FOLDER_STORE_GET_PRIVATE (self);

	_observer_mixin_add_observer (self, &(priv->observers), observer);
}

static void
tny_merge_folder_store_remove_observer_default (TnyFolderStore *self, TnyFolderStoreObserver *observer)
{
	TnyMergeFolderStorePriv *priv = TNY_MERGE_FOLDER_STORE_GET_PRIVATE (self);

	_observer_mixin_remove_observer (self, &(priv->observers), observer);
}

static void
tny_merge_folder_store_remove_folder_default (TnyFolderStore *self, TnyFolder *folder, GError **err)
{
	g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_UNSUPPORTED,
			"You can't use the tny_folder_store_remove_folder API "
			"on merge folders. This problem indicates a bug in the "
			"software.");

	return;

	/* TODO: derive TnyMergeWritableFolderStore which does this:
	 * if folder in base_store
		remove_folder (base_store, folder, err)
	   else
	      err = not_supported
		*/
}

static TnyFolder*
tny_merge_folder_store_create_folder_default (TnyFolderStore *self, const gchar *name, GError **err)
{
	g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_UNSUPPORTED,
			"You can't use the tny_folder_store_create_folder API "
			"on merge folders. This problem indicates a bug in the "
			"software.");

	return NULL;


	/* TODO: derive TnyMergeWritableFolderStore which does this:
	 * create_folder (base_store, name, err); */
}

typedef struct
{
	TnyFolderStore *self;
	gchar *name;
	TnyCreateFolderCallback callback;
	gpointer user_data;
} CreateFolderInfo;

static gboolean
create_folder_async_callback (gpointer user_data)
{
	CreateFolderInfo *info = user_data;
	TnyMergeFolderStorePriv *priv = TNY_MERGE_FOLDER_STORE_GET_PRIVATE (info->self);
	if (info->callback) {
		GError *err;
		err = g_error_new (TNY_ERROR_DOMAIN,
				   TNY_SERVICE_ERROR_UNSUPPORTED,
				   "You can't use the tny_folder_store_create_folder "
				   "API on send queues. This problem indicates a "
				   "bug in the software.");


		if (priv->ui_lock)
			tny_lockable_lock (priv->ui_lock);
		info->callback (info->self, FALSE, NULL, err, info->user_data);
		if (priv->ui_lock)
			tny_lockable_unlock (priv->ui_lock);
		g_error_free (err);
	}
	g_slice_free (CreateFolderInfo, info);

	return FALSE;
}

static void
tny_merge_folder_store_create_folder_async_default (TnyFolderStore *self, const gchar *name, TnyCreateFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	CreateFolderInfo *info;

	/* Idle info for the callbacks */
	info = g_slice_new (CreateFolderInfo);
	info->self = self;
	info->name = g_strdup (name);
	info->callback = callback;
	info->user_data = user_data;

	g_object_ref (info->self);

	g_idle_add (create_folder_async_callback, info);

	/* TODO: derive TnyMergeWritableFolderStore which does this:
	 * create_folder_async (base_store, name, err); */
}

static void
tny_merge_folder_store_get_folders_default (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, gboolean refresh, GError **err)
{
	TnyMergeFolderStorePriv *priv = TNY_MERGE_FOLDER_STORE_GET_PRIVATE (self);

	g_static_rec_mutex_lock (priv->lock);

	TnyIterator *iter = tny_list_create_iterator (priv->stores);
	while (!tny_iterator_is_done(iter)) {
		TnyFolderStore *store = TNY_FOLDER_STORE(tny_iterator_get_current (iter));
		tny_folder_store_get_folders (store, list, query, refresh, err);
		g_object_unref (store);

		if (err)
			break;

		tny_iterator_next (iter);
	}
	g_object_unref (iter);
	g_static_rec_mutex_unlock (priv->lock);
}


typedef struct _GetFoldersInfo
{
	TnyFolderStore *self;
	TnyList *waiting_items;
	TnyList *list;
	TnyGetFoldersCallback callback;
	TnyStatusCallback status_callback;
	gpointer user_data;
	gboolean cancelled;
	GError *error;
} GetFoldersInfo;

static void
get_folders_cb (TnyFolderStore *store,
                     gboolean cancelled,
                     TnyList *list,
                     GError *err,
                     gpointer user_data)
{
	GetFoldersInfo *info = user_data;
	tny_list_remove (info->waiting_items, G_OBJECT(store));
	if (cancelled)
		info->cancelled = TRUE;

	/*we'll only end up reporting the first error that occured, but I can't think of a better option */
	if (err && info->error == NULL)
		info->error = g_error_copy (err);

	if (tny_list_get_length (info->waiting_items) == 0) {

		info->callback (info->self, info->cancelled, info->list,
				info->error, info->user_data);
		g_object_unref (info->waiting_items);
		g_object_unref (info->list);
		g_object_unref (info->self);

		if (info->error)
			g_error_free (info->error);

		g_slice_free (GetFoldersInfo, info);
	}
}

static void
get_folders_status_cb (GObject *self,
                            TnyStatus *status,
                            gpointer user_data)
{
	GetFoldersInfo *info = user_data;
	TnyMergeFolderStorePriv *priv = TNY_MERGE_FOLDER_STORE_GET_PRIVATE (info->self);
	guint num = tny_list_get_length (priv->stores);

	if (num)
		status->of_total = status->of_total * num;

	info->status_callback (G_OBJECT(info->self), status, info->user_data);
}


static void
tny_merge_folder_store_get_folders_async_default (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, gboolean refresh, TnyGetFoldersCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TnyMergeFolderStorePriv *priv = TNY_MERGE_FOLDER_STORE_GET_PRIVATE (self);

	GetFoldersInfo *info = g_slice_new0 (GetFoldersInfo);

	info->self = g_object_ref (self);
	info->list = g_object_ref (list);
	info->waiting_items = tny_list_copy (priv->stores);
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;

	g_static_rec_mutex_lock (priv->lock);

	TnyIterator *iter = tny_list_create_iterator (priv->stores);
	while (!tny_iterator_is_done(iter)) {
		TnyFolderStore *store = TNY_FOLDER_STORE (tny_iterator_get_current (iter));
		tny_folder_store_get_folders_async (store, list, query, refresh, get_folders_cb, get_folders_status_cb, info);
		g_object_unref (store);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);
	g_static_rec_mutex_unlock (priv->lock);

}



/* refresh_async:
 * call refresh on each store
 * same thing for status_callback as above
 * in callback., just count up and call user's callback when its all done.
 */

typedef struct _RefreshAsyncInfo
{
	TnyFolderStore *self;
	TnyList *waiting_items;
	TnyFolderStoreCallback callback;
	TnyStatusCallback status_callback;
	gpointer user_data;
	gboolean cancelled;
	GError *error;
} RefreshAsyncInfo;

static void
refresh_async_cb (TnyFolderStore *store,
                       gboolean cancelled,
                       GError *err,
                       gpointer user_data)
{
	RefreshAsyncInfo *info = user_data;
	tny_list_remove (info->waiting_items, G_OBJECT(store));
	if (cancelled)
		info->cancelled = TRUE;

	/*we'll only end up reporting the first error that occured, but I can't think of a better option */
	if (err && info->error == NULL)
		info->error = g_error_copy (err);

	if (tny_list_get_length (info->waiting_items) == 0) {
		if (info->callback)
			info->callback (info->self, info->cancelled, info->error, info->user_data);
		g_object_unref (info->waiting_items);
		g_object_unref (info->self);

		if (info->error)
			g_error_free (info->error);

		g_slice_free (RefreshAsyncInfo, info);
	}
}

static void
refresh_async_status_cb (GObject *self,
                              TnyStatus *status,
                              gpointer user_data)
{
	RefreshAsyncInfo *info = user_data;
	TnyMergeFolderStorePriv *priv = TNY_MERGE_FOLDER_STORE_GET_PRIVATE (info->self);
	guint num = tny_list_get_length (priv->stores);

	if (num)
		status->of_total = status->of_total * num;

	info->status_callback (G_OBJECT(info->self), status, info->user_data);
}

static void
tny_merge_folder_store_refresh_async_default (TnyFolderStore *self, TnyFolderStoreCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TnyMergeFolderStorePriv *priv = TNY_MERGE_FOLDER_STORE_GET_PRIVATE (self);

	RefreshAsyncInfo *info = g_slice_new0 (RefreshAsyncInfo);

	info->self = g_object_ref (self);
	info->waiting_items = tny_list_copy (priv->stores);
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;

	g_static_rec_mutex_lock (priv->lock);

	TnyIterator *iter = tny_list_create_iterator (priv->stores);
	while (!tny_iterator_is_done(iter)) {
		TnyFolderStore *store = TNY_FOLDER_STORE (tny_iterator_get_current (iter));
		tny_folder_store_refresh_async (store, refresh_async_cb, refresh_async_status_cb, info);
		g_object_unref (store);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);
	g_static_rec_mutex_unlock (priv->lock);

}

/**
 * tny_merge_folder_store_add_store:
 * @self: a #TnyMergeFolderStore
 * @store: a #TnyFoldeStore to add to the TnyMergeFolderStore
 *
 * Adds #store as a source for this TnyMergeFolderStore
 */
void
tny_merge_folder_store_add_store (TnyMergeFolderStore *self, TnyFolderStore *store)
{
	TNY_MERGE_FOLDER_STORE_GET_CLASS (self)->add_store (self, store);
}

static void
tny_merge_folder_store_add_store_default (TnyMergeFolderStore *self, TnyFolderStore *store)
{
	TnyMergeFolderStorePriv *priv = TNY_MERGE_FOLDER_STORE_GET_PRIVATE (self);

	g_static_rec_mutex_lock (priv->lock);
	tny_folder_store_add_observer (store, TNY_FOLDER_STORE_OBSERVER (self));
	tny_list_prepend (priv->stores, G_OBJECT(store));
	g_static_rec_mutex_unlock (priv->lock);
}

/**
 * tny_merge_folder_store_remove_store:
 * @self: a #TnyMergeFolderStore
 * @store: a #TnyFoldeStore to remove from the TnyMergeFolderStore
 *
 * Removes #store as a source for this TnyMergeFolderStore
 */
void
tny_merge_folder_store_remove_store (TnyMergeFolderStore *self, TnyFolderStore *store)
{
	TNY_MERGE_FOLDER_STORE_GET_CLASS (self)->remove_store (self, store);
}

static void
tny_merge_folder_store_remove_store_default (TnyMergeFolderStore *self, TnyFolderStore *store)
{
	TnyMergeFolderStorePriv *priv = TNY_MERGE_FOLDER_STORE_GET_PRIVATE (self);

	g_static_rec_mutex_lock (priv->lock);
	tny_folder_store_remove_observer (store, TNY_FOLDER_STORE_OBSERVER (self));
	tny_list_remove (priv->stores, G_OBJECT(store));
	g_static_rec_mutex_unlock (priv->lock);
}


/**
 * tny_merge_folder_store_new:
 * @ui_locker: a #TnyLockable for locking your ui
 *
 * Creates a a new TnyMergeFolderStore instance that can merge multiple
 * #TnyFolderStore instances together read only. Upon construction it
 * instantly sets the ui locker. For Gtk+ you should use a #TnyGtkLockable here.
 * #ui_locker maybe NULL if your toolkit is unthreaded.
 *
 * returns: (caller-owns): a new #TnyMergeFolderStore instance
 * since: 1.0
 * audience: application-developer
 **/
TnyMergeFolderStore*
tny_merge_folder_store_new (TnyLockable *ui_locker)
{
	TnyMergeFolderStore *self = g_object_new (TNY_TYPE_MERGE_FOLDER_STORE, NULL);
	TnyMergeFolderStorePriv *priv = TNY_MERGE_FOLDER_STORE_GET_PRIVATE (self);
	priv->ui_lock = ui_locker;
	return self;
}


static void
tny_merge_folder_store_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyMergeFolderStore *self = (TnyMergeFolderStore *)instance;
	TnyMergeFolderStorePriv *priv = TNY_MERGE_FOLDER_STORE_GET_PRIVATE (self);

	priv->stores = tny_simple_list_new ();

	_observer_mixin_init (&(priv->observers));

	priv->known_folders = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);

	priv->lock = g_new0 (GStaticRecMutex, 1);
	g_static_rec_mutex_init (priv->lock);
}

static void
tny_merge_folder_store_dispose (GObject *object)
{
	TnyMergeFolderStore *self = (TnyMergeFolderStore *)object;
	TnyMergeFolderStorePriv *priv = TNY_MERGE_FOLDER_STORE_GET_PRIVATE (self);

	_observer_mixin_destroy (self, &(priv->observers));

	g_static_rec_mutex_lock (priv->lock);

	if (priv->known_folders) {
		g_hash_table_foreach_remove (priv->known_folders, (GHRFunc) known_folder_remover, self);
		g_hash_table_unref(priv->known_folders);
		priv->known_folders = NULL;
	}

	g_object_unref (priv->stores);
	priv->stores = NULL;

	g_static_rec_mutex_unlock (priv->lock);
}

static void
tny_merge_folder_store_finalize (GObject *object)
{
	TnyMergeFolderStore *self = (TnyMergeFolderStore *) object;
	TnyMergeFolderStorePriv *priv = TNY_MERGE_FOLDER_STORE_GET_PRIVATE (self);

	g_free (priv->lock);
	priv->lock = NULL;

	(*parent_class->finalize) (object);
}

static void
tny_merge_folder_store_class_init (TnyMergeFolderStoreClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	object_class->finalize = tny_merge_folder_store_finalize;
	object_class->dispose = tny_merge_folder_store_dispose;

	class->get_folders_async= tny_merge_folder_store_get_folders_async_default;
	class->get_folders= tny_merge_folder_store_get_folders_default;
	class->create_folder= tny_merge_folder_store_create_folder_default;
	class->create_folder_async= tny_merge_folder_store_create_folder_async_default;
	class->remove_folder= tny_merge_folder_store_remove_folder_default;
	class->add_store_observer= tny_merge_folder_store_add_observer_default;
	class->remove_store_observer= tny_merge_folder_store_remove_observer_default;
	class->refresh_async = tny_merge_folder_store_refresh_async_default;

	class->add_store = tny_merge_folder_store_add_store_default;
	class->remove_store = tny_merge_folder_store_remove_store_default;

	g_type_class_add_private (object_class, sizeof (TnyMergeFolderStorePriv));

	return;
}

static void
tny_merge_folder_store_remove_folder (TnyFolderStore *self, TnyFolder *folder, GError **err)
{
	TNY_MERGE_FOLDER_STORE_GET_CLASS (self)->remove_folder(self, folder, err);
}


static TnyFolder*
tny_merge_folder_store_create_folder (TnyFolderStore *self, const gchar *name, GError **err)
{
	return TNY_MERGE_FOLDER_STORE_GET_CLASS (self)->create_folder(self, name, err);
}

static void
tny_merge_folder_store_create_folder_async (TnyFolderStore *self, const gchar *name, TnyCreateFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TNY_MERGE_FOLDER_STORE_GET_CLASS (self)->create_folder_async(self, name, callback, status_callback, user_data);
}

static void
tny_merge_folder_store_get_folders (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, gboolean refresh, GError **err)
{
	TNY_MERGE_FOLDER_STORE_GET_CLASS (self)->get_folders(self, list, query, refresh, err);
}

static void
tny_merge_folder_store_get_folders_async (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, gboolean refresh, TnyGetFoldersCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TNY_MERGE_FOLDER_STORE_GET_CLASS (self)->get_folders_async(self, list, query, refresh, callback, status_callback, user_data);
}

static void
tny_merge_folder_store_add_observer (TnyFolderStore *self, TnyFolderStoreObserver *observer)
{
	TNY_MERGE_FOLDER_STORE_GET_CLASS (self)->add_store_observer(self, observer);
}

static void
tny_merge_folder_store_remove_observer (TnyFolderStore *self, TnyFolderStoreObserver *observer)
{
	TNY_MERGE_FOLDER_STORE_GET_CLASS (self)->remove_store_observer(self, observer);
}


static void
tny_merge_folder_store_refresh_async (TnyFolderStore *self, TnyFolderStoreCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TNY_MERGE_FOLDER_STORE_GET_CLASS (self)->refresh_async(self, callback, status_callback, user_data);
}

static void
tny_folder_store_init (gpointer g, gpointer iface_data)
{
	TnyFolderStoreIface *klass = (TnyFolderStoreIface *)g;

	klass->remove_folder= tny_merge_folder_store_remove_folder;
	klass->create_folder= tny_merge_folder_store_create_folder;
	klass->create_folder_async= tny_merge_folder_store_create_folder_async;
	klass->get_folders= tny_merge_folder_store_get_folders;
	klass->get_folders_async= tny_merge_folder_store_get_folders_async;
	klass->add_observer= tny_merge_folder_store_add_observer;
	klass->remove_observer= tny_merge_folder_store_remove_observer;
	klass->refresh_async = tny_merge_folder_store_refresh_async;
}

static void
tny_folder_store_observer_init (gpointer g, gpointer iface_data)
{
	TnyFolderStoreObserverIface *klass = (TnyFolderStoreObserverIface *)g;

	klass->update = tny_merge_folder_store_observer_update;
}


static gpointer
tny_merge_folder_store_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info =
		{
			sizeof (TnyMergeFolderStoreClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_merge_folder_store_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyMergeFolderStore),
			0,      /* n_preallocs */
			tny_merge_folder_store_instance_init    /* instance_init */
		};

	static const GInterfaceInfo tny_folder_store_info =
		{
			(GInterfaceInitFunc) tny_folder_store_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};

	static const GInterfaceInfo tny_folder_store_observer_info =
		{
			(GInterfaceInitFunc) tny_folder_store_observer_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};


	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyMergeFolderStore",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_FOLDER_STORE,
				     &tny_folder_store_info);

	g_type_add_interface_static (type, TNY_TYPE_FOLDER_STORE_OBSERVER,
				     &tny_folder_store_observer_info);

	return GUINT_TO_POINTER (type);
}

GType
tny_merge_folder_store_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_merge_folder_store_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
