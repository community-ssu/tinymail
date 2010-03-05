/* libtinymailui-gtk - The Tiny Mail UI library for Gtk+
 * Copyright (C) 2006-2008 Philip Van Hoof <pvanhoof@gnome.org>
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
 * TnyGtkFolderListStore:
 *
 * A #GtkTreeModel for #TnyFolderStore instances that'll become #TnyFolderStoreObserver
 * and #TnyFolderObserver (in case the #TnyFolderStore instance is a #TnyFolder
 * too) of each added #TnyFolderStore and each of its children en grandchildren
 * (recursively).
 *
 * It's based on the implementation of #TnyGtkFolderListTreeModel, but it stores
 * folders and accounts in a #GtkListStore, using a plain representation.
 *
 * It will detect changes in the instance's tree structure this way and it will
 * adapt itself to a new situation automatically. It also contains columns that
 * contain certain popular numbers (like the unread and the total counts of
 * #TnyFolder instances).
 *
 * Note that a #TnyGtkFolderListStore is a #TnyList too. You can use the
 * #TnyList API on instances of this type too.
 *
 * Note that you must make sure that you unreference #TnyFolderStore instances
 * that you get out of the instance column of this type using the #GtkTreeModel
 * API gtk_tree_model_get().
 *
 * free-function: g_object_unref
 **/

#include <config.h>
#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-folder.h>
#include <tny-folder-store.h>
#include <tny-simple-list.h>
#include <tny-merge-folder.h>

#include <tny-store-account.h>
#include <tny-folder-store-change.h>
#include <tny-folder-store-observer.h>
#include <tny-folder-change.h>
#include <tny-folder-observer.h>

#include <tny-gtk-folder-list-store.h>

#include "tny-gtk-folder-list-store-iterator-priv.h"

#define DEFAULT_PATH_SEPARATOR " "

static GObjectClass *parent_class = NULL;

enum {
	ACTIVITY_CHANGED_SIGNAL,
	LAST_SIGNAL
};

guint tny_gtk_folder_list_store_signals [LAST_SIGNAL];

typedef void (*listaddfunc) (GtkListStore *list_store, GtkTreeIter *iter);

static void tny_gtk_folder_list_store_on_constatus_changed (TnyAccount *account, 
							    TnyConnectionStatus status, 
							    TnyGtkFolderListStore *self);

static void constatus_do_refresh (TnyAccount *account, 
				  TnyConnectionStatus status, 
				  TnyGtkFolderListStore *self,
				  gboolean force_poke_status);

static gboolean delayed_refresh_timeout_handler (TnyGtkFolderListStore *self);

typedef struct {
	TnyGtkFolderListStore *self;
	gboolean do_poke_status;
} _RefreshInfo;

static void
update_delayed_refresh (TnyGtkFolderListStore *self)
{
	if (self->flags & TNY_GTK_FOLDER_LIST_STORE_FLAG_DELAYED_REFRESH) {
		if (self->delayed_refresh_timeout_id > 0) {
			g_source_remove (self->delayed_refresh_timeout_id);
		}
		self->delayed_refresh_timeout_id = g_timeout_add_seconds_full (G_PRIORITY_DEFAULT,
									       2,
									       (GSourceFunc) delayed_refresh_timeout_handler,
									       g_object_ref (self),
									       g_object_unref);
	}
}

static gboolean
delayed_refresh_timeout_handler (TnyGtkFolderListStore *self)
{
	GList *node;
	self->delayed_refresh_timeout_id = 0;

	self->flags &= (~TNY_GTK_FOLDER_LIST_STORE_FLAG_DELAYED_REFRESH);
	self->flags &= (~TNY_GTK_FOLDER_LIST_STORE_FLAG_NO_REFRESH);

	g_mutex_lock (self->iterator_lock);
	for (node = self->first; node != NULL; node = g_list_next (node)) {
		if (TNY_IS_ACCOUNT (node->data)) {
			constatus_do_refresh (
				TNY_ACCOUNT (node->data),
				tny_account_get_connection_status (TNY_ACCOUNT (node->data)),
				self,
				TRUE);
		}
	}
	g_mutex_unlock (self->iterator_lock);

	return FALSE;
}


static void 
add_folder_observer_weak (TnyGtkFolderListStore *self, TnyFolder *folder)
{
	if (TNY_IS_FOLDER (folder)) {
		tny_folder_add_observer (folder, TNY_FOLDER_OBSERVER (self));
		self->fol_obs = g_list_prepend (self->fol_obs, folder);
	}
}

static void 
add_folder_store_observer_weak (TnyGtkFolderListStore *self, TnyFolderStore *store)
{
	if (TNY_IS_FOLDER_STORE (store)) {
		tny_folder_store_add_observer (store, TNY_FOLDER_STORE_OBSERVER (self));
		self->store_obs = g_list_prepend (self->store_obs, store);
	}
}

static void 
remove_folder_observer_weak (TnyGtkFolderListStore *self, TnyFolder *folder, gboolean final)
{
	if (TNY_IS_FOLDER (folder)) {
		if (!final)
			self->fol_obs = g_list_remove (self->fol_obs, folder);
		tny_folder_remove_observer (folder, (TnyFolderObserver *) self);
	}
}

static void
remove_folder_store_observer_weak (TnyGtkFolderListStore *self, TnyFolderStore *store, gboolean final)
{
	if (TNY_IS_FOLDER_STORE (store)) {
		if (!final)
			self->store_obs = g_list_remove (self->store_obs, store);
		tny_folder_store_remove_observer (store, (TnyFolderStoreObserver *) self);
	}
}

static gchar *
get_parent_full_name (TnyFolderStore *store, gchar *path_separator)
{
	TnyList *folders;
	TnyIterator *iter;
	TnyFolderStore *current;
	gchar *name = NULL;

	folders = tny_simple_list_new ();
	current = g_object_ref (store);

	while (current && !TNY_IS_ACCOUNT (current)) {
		TnyFolderStore *to_unref;
		to_unref = current;
		tny_list_prepend (folders, (GObject *) current);
		current = tny_folder_get_folder_store (TNY_FOLDER (current));
		g_object_unref (to_unref);
	}

	g_object_unref (current);

	iter = tny_list_create_iterator (folders);
	while (!tny_iterator_is_done (iter)) {
		current = (TnyFolderStore *) tny_iterator_get_current (iter);
		if (current) {
			gchar *tmp = NULL;
			if (G_LIKELY (!name)) {
				tmp = g_strdup (tny_folder_get_name (TNY_FOLDER (current)));
			} else {
				tmp = g_strconcat (name, path_separator,
						   tny_folder_get_name (TNY_FOLDER (current)),
						   NULL);
			}
			g_free (name);
			name = tmp;

			g_object_unref (current);
		}
		tny_iterator_next (iter);
	}
	g_object_unref (iter);
	g_object_unref (folders);

	return name;
}

static void
recurse_folders_async_cb (TnyFolderStore *store,
			  gboolean cancelled,
			  TnyList *folders,
			  GError *err,
			  gpointer user_data)
{
	TnyIterator *iter;
	TnyGtkFolderListStore *self;
	gchar *parent_name;
	gboolean do_poke_status;
	_RefreshInfo *info;

	info = (_RefreshInfo *) user_data;
	self = info->self;
	do_poke_status = info->do_poke_status;
	g_slice_free (_RefreshInfo, info);

	if (cancelled || err || self->flags & TNY_GTK_FOLDER_LIST_STORE_FLAG_DISPOSED) {
		g_warning ("%s Error getting the folders", __FUNCTION__);
		if (self->progress_count > 0) {
			self->progress_count--;
			if (self->progress_count == 0)
				g_signal_emit (self, tny_gtk_folder_list_store_signals[ACTIVITY_CHANGED_SIGNAL], 0, FALSE);
		}
		g_object_unref (self);
		return;
	}

	/* TODO add error checking and reporting here */
	iter = tny_list_create_iterator (folders);

	parent_name = get_parent_full_name (store, self->path_separator);

	while (!tny_iterator_is_done (iter))
	{
		GtkTreeModel *mmodel = (GtkTreeModel *) self;
		GtkListStore *model = GTK_LIST_STORE (self);
		GObject *instance = G_OBJECT (tny_iterator_get_current (iter));
		GtkTreeIter tree_iter;
		TnyFolder *folder = NULL;
		TnyFolderStore *folder_store = NULL;
		GtkTreeIter miter;
		gboolean found = FALSE;

		if (instance && (TNY_IS_FOLDER (instance) || TNY_IS_MERGE_FOLDER (instance)))
			folder = TNY_FOLDER (instance);

		if (instance && (TNY_IS_FOLDER_STORE (instance)))
			folder_store = TNY_FOLDER_STORE (instance);

		if (gtk_tree_model_get_iter_first (mmodel, &miter))
		  do
		  {
			GObject *citem = NULL;
			TnyIterator *niter = NULL;

			gtk_tree_model_get (mmodel, &miter,
				TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN,
				&citem, -1);

			if (citem == instance)
			{
				found = TRUE;
				if (citem)
					g_object_unref (citem);
				break;
			}

			if (citem)
				g_object_unref (citem);

		  } while (gtk_tree_model_iter_next (mmodel, &miter));

		/* It was not found, so let's start adding it to the model! */

		if (!found)
		{
			gtk_list_store_append (model, &tree_iter);

			/* Making self both a folder-store as a folder observer
			 * of this folder. This way we'll get notified when 
			 * both a removal and a creation happens. Also when a 
			 * rename happens: that's a removal and a creation. */

			if (folder)
			{
				gchar *name;

				/* This adds a reference count to folder too. When it gets removed, that
				   reference count is decreased automatically by the gtktreestore infra-
				   structure. */

				/* Note that it could be a TnyMergeFolder */
				if (TNY_IS_FOLDER_STORE (folder))
					add_folder_store_observer_weak (self, (TnyFolderStore *) folder);
				add_folder_observer_weak (self, folder);

				if (self->flags & TNY_GTK_FOLDER_LIST_STORE_FLAG_SHOW_PATH) {
					if ((parent_name == NULL) || *parent_name == '\0') {
						name = g_strdup (tny_folder_get_name (folder));
					} else {
						name = g_strconcat (parent_name,
								    self->path_separator,
								    tny_folder_get_name (folder), 
								    NULL);
					}
				} else {
					name = g_strdup (tny_folder_get_name (folder));
				}

				gtk_list_store_set  (model, &tree_iter,
					TNY_GTK_FOLDER_LIST_STORE_NAME_COLUMN, 
					name,
					TNY_GTK_FOLDER_LIST_STORE_UNREAD_COLUMN, 
					tny_folder_get_unread_count (TNY_FOLDER (folder)),
					TNY_GTK_FOLDER_LIST_STORE_ALL_COLUMN, 
					tny_folder_get_all_count (TNY_FOLDER (folder)),
					TNY_GTK_FOLDER_LIST_STORE_TYPE_COLUMN,
					tny_folder_get_folder_type (TNY_FOLDER (folder)),
					TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN,
					folder, -1);

				g_free (name);
			}

			/* it's a store by itself, so keep on recursing */
			if (folder_store) {
				TnyList *folders = tny_simple_list_new ();
				_RefreshInfo *new_info = g_slice_new (_RefreshInfo);

				add_folder_store_observer_weak (self, folder_store);
				self->progress_count++;
				update_delayed_refresh (self);
				new_info->self = g_object_ref (self);
				new_info->do_poke_status = do_poke_status;
				tny_folder_store_get_folders_async (folder_store,
								    folders, NULL, 
								    FALSE,
								    recurse_folders_async_cb,
								    NULL, new_info);
				g_object_unref (folders);
			}

			/* We're a folder, we'll request a status, since we've
			 * set self to be a folder observers of folder, we'll 
			 * receive the status update. This makes it possible to
			 * instantly get the unread and total counts, of course.
			 * 
			 * Note that the initial value is read from the cache in
			 * case the account is set offline, in TnyCamelFolder. 
			 * In case the account is online a STAT for POP or a 
			 * STATUS for IMAP is asked during the poke-status impl.
			 *
			 * This means that no priv->folder must be loaded, no
			 * memory peak will happen, few data must be transmitted
			 * in case we're online. Which is perfect! */

			if (folder && 
			    (!(self->flags & TNY_GTK_FOLDER_LIST_STORE_FLAG_DELAYED_REFRESH)||
			     do_poke_status))
				tny_folder_poke_status (TNY_FOLDER (folder));


		} else {
			if (folder && do_poke_status)
				tny_folder_poke_status (TNY_FOLDER (folder));
			if (folder_store) {
				_RefreshInfo *new_info = g_slice_new (_RefreshInfo);
				/* We still keep recursing already fetch folders, to know if there are new child
				   folders */
				TnyList *folders = tny_simple_list_new ();

				self->progress_count++;
				update_delayed_refresh (self);
				new_info->self = g_object_ref (self);
				new_info->do_poke_status = do_poke_status;
				tny_folder_store_get_folders_async (folder_store,
								    folders, NULL, 
								    FALSE,
								    recurse_folders_async_cb,
								    NULL, new_info);
				g_object_unref (folders);
			}
		}

		g_object_unref (instance);

		tny_iterator_next (iter);
	}
	g_object_unref (iter);

	g_free (parent_name);

	/* Remove objects present in the model that are not in the
	   list returned by the folder store */
	GtkTreeIter model_iter;
	GtkTreeModel *mmodel = (GtkTreeModel *) self;
	if (gtk_tree_model_get_iter_first (mmodel, &model_iter)) {
		do {
			GObject *citem = NULL;
			gboolean found;
			gboolean process;

			found = FALSE;
			process = FALSE;

			gtk_tree_model_get (mmodel, &model_iter,
					    TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN,
					    &citem, -1);

			/* Only process the model item if its parent
			   is the store passed as parametter */
			if (TNY_IS_FOLDER (citem) && !TNY_IS_MERGE_FOLDER (citem)) {
				TnyFolderStore *f_store = tny_folder_get_folder_store (TNY_FOLDER (citem));
				if (f_store) {
					if (f_store == store)
						process = TRUE;
					g_object_unref (f_store);
				}
			}

			/* The list of folders is not supposed to be large so this should be really fast */
			if (process) {
				TnyIterator *niter = NULL;

				niter = tny_list_create_iterator (folders);
				while (!found && !tny_iterator_is_done (niter)) {
					TnyFolderStore *store = (TnyFolderStore *) tny_iterator_get_current (niter);
					if (store) {
						if ((GObject *) store == citem)
							found = TRUE;
						g_object_unref (store);
					}
					tny_iterator_next (niter);
				}
				g_object_unref (niter);

				/* Remove from model if it's not present in the list returned by provider */
				if (!found)
					gtk_list_store_remove ((GtkListStore *) mmodel, &model_iter);
			}

			if (citem)
				g_object_unref (citem);

			if (!process || found)
				gtk_tree_model_iter_next (mmodel, &model_iter);

		} while (gtk_list_store_iter_is_valid ((GtkListStore*) mmodel, &model_iter));
	}

	if (self->progress_count > 0) {
		self->progress_count--;
		if (self->progress_count == 0)
			g_signal_emit (self, tny_gtk_folder_list_store_signals[ACTIVITY_CHANGED_SIGNAL], 0, FALSE);
	}

	g_object_unref (self);
}


static const gchar*
get_root_name (TnyFolderStore *folder_store)
{
	const gchar *root_name;
	if (TNY_IS_ACCOUNT (folder_store))
		root_name = tny_account_get_name (TNY_ACCOUNT (folder_store));
	else
		root_name = _("Folder bag");
	return root_name;
}


static void
get_folders_cb (TnyFolderStore *fstore, gboolean cancelled, TnyList *list, GError *err, gpointer user_data)
{
	_RefreshInfo *info = (_RefreshInfo *) user_data;
	TnyGtkFolderListStore *self = (TnyGtkFolderListStore *) info->self;
	gboolean do_poke_status = info->do_poke_status;
	GtkTreeModel *model = GTK_TREE_MODEL (self);
	GtkTreeIter iter;
	GtkTreeIter name_iter;
	gboolean found = FALSE;

	g_slice_free (_RefreshInfo, info);

	g_object_unref (list);

	/* Note that the very first time, this code will pull all folder info.
	 * Note that when it runs, you'll see LIST commands happen on the IMAP
	 * socket. Especially the first time. You'll also see STATUS commands 
	 * happen. This is, indeed, normal behaviour when this component is used.*/

	/* But first, let's find-back that damn account so that we have the
	 * actual iter behind it in the model. */

	if ((!(self->flags & TNY_GTK_FOLDER_LIST_STORE_FLAG_DISPOSED)) && gtk_tree_model_get_iter_first (model, &iter))
	  do 
	  {
		GObject *citem;

		gtk_tree_model_get (model, &iter, 
			TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN, 
			&citem, -1);

		if (citem == (GObject *) fstore)
		{
			name_iter = iter;
			found = TRUE;
			if (citem)
				g_object_unref (citem);
			break;
		}

		g_object_unref (citem);

	  } while (gtk_tree_model_iter_next (model, &iter));

	/* We found it, so we'll now just recursively start asking for all its
	 * folders and subfolders. The recurse_folders_sync can indeed cope with
	 * folders that already exist (it wont add them a second time). */

	if (found) {
		_RefreshInfo *new_info = g_slice_new (_RefreshInfo);
		TnyList *folders = tny_simple_list_new ();
		self->progress_count++;
		update_delayed_refresh (self);
		if (self->progress_count == 1) {
			g_signal_emit (self, tny_gtk_folder_list_store_signals[ACTIVITY_CHANGED_SIGNAL], 0, TRUE);
		}
		update_delayed_refresh (self);
		new_info->self = g_object_ref (self);
		new_info->do_poke_status = do_poke_status;
		tny_folder_store_get_folders_async (fstore,
						    folders, NULL, 
						    FALSE,
						    recurse_folders_async_cb,
						    NULL, new_info);
		g_object_unref (folders);
	}
	if (self->progress_count > 0) {
		self->progress_count --;

		if (self->progress_count == 0)
			g_signal_emit (self, tny_gtk_folder_list_store_signals[ACTIVITY_CHANGED_SIGNAL], 0, FALSE);
	}

	g_object_unref (self);

	return;
}

static void 
tny_gtk_folder_list_store_on_constatus_changed (TnyAccount *account, TnyConnectionStatus status, TnyGtkFolderListStore *self)
{
	constatus_do_refresh (account, status, self, FALSE);
}
static void 
constatus_do_refresh (TnyAccount *account, TnyConnectionStatus status, TnyGtkFolderListStore *self, gboolean do_poke_status)
{
	TnyList *list = NULL;
	_RefreshInfo *info = NULL;

	if (!self || !TNY_IS_GTK_FOLDER_LIST_STORE (self))
		return;

	if (self->flags & TNY_GTK_FOLDER_LIST_STORE_FLAG_DISPOSED)
		return;

	/* This callback handler deals with connection status changes. In case
	 * we got connected, we can expect that, at least sometimes, new folders
	 * might have arrived. We'll need to scan for those, so we'll recursively
	 * start asking the account about its folders. */
	
	if (status == TNY_CONNECTION_STATUS_RECONNECTING || status == TNY_CONNECTION_STATUS_DISCONNECTED)
		return;

	list = tny_simple_list_new ();
	self->progress_count++;
	if (self->progress_count == 1) {
		g_signal_emit (self, tny_gtk_folder_list_store_signals[ACTIVITY_CHANGED_SIGNAL], 0, TRUE);
	}

	info = g_slice_new (_RefreshInfo);
	info->self = g_object_ref (self);
	info->do_poke_status = do_poke_status;

	tny_folder_store_get_folders_async (TNY_FOLDER_STORE (account),
					    list, self->query,
					    !(self->flags & TNY_GTK_FOLDER_LIST_STORE_FLAG_NO_REFRESH),  
					    get_folders_cb, NULL, info);

	return;
}



static void 
tny_gtk_folder_list_store_on_changed (TnyAccount *account, TnyGtkFolderListStore *self)
{
	GtkTreeModel *model = GTK_TREE_MODEL (self);
	GtkTreeIter iter;
	GtkTreeIter name_iter;
	gboolean found = FALSE;

	if (gtk_tree_model_get_iter_first (model, &iter))
	  do 
	  {
		GObject *citem = NULL;

		gtk_tree_model_get (model, &iter, 
			TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN, 
			&citem, -1);

		if (citem == (GObject *) account)
		{
			name_iter = iter;
			found = TRUE;
			if (citem)
				g_object_unref (citem);
			break;
		}

		if (citem)
			g_object_unref (citem);

	  } while (gtk_tree_model_iter_next (model, &iter));

	if (found) {
		gtk_list_store_set (GTK_LIST_STORE (model), &name_iter,
				    TNY_GTK_FOLDER_LIST_STORE_NAME_COLUMN, 
				    get_root_name (TNY_FOLDER_STORE (account)), -1);
	}
}

typedef struct
{
	GObject *instance;
	guint handler_id;
} SignalSlot;

typedef struct
{
	TnyGtkFolderListStore *self;
	TnyAccount *account;
} AccNotYetReadyInfo;

static void 
notify_signal_slots (gpointer user_data, GObject *instance)
{
	TnyGtkFolderListStore *self = (TnyGtkFolderListStore *) user_data;
	int i=0;

	for (i=0; i < self->signals->len; i++) {
		SignalSlot *slot = (SignalSlot *) self->signals->pdata[i];
		if (slot->instance == instance)
			slot->instance = NULL;
	}
}

static gboolean
account_was_not_yet_ready_idle (gpointer user_data)
{
	AccNotYetReadyInfo *info = (AccNotYetReadyInfo *) user_data;
	gboolean repeat = TRUE;

	if (tny_account_is_ready (info->account))
	{
		SignalSlot *slot;

		slot = g_slice_new (SignalSlot);
		slot->instance = (GObject *) info->account;
		slot->handler_id = g_signal_connect (info->account, "connection-status-changed",
			G_CALLBACK (tny_gtk_folder_list_store_on_constatus_changed), info->self);
		g_object_weak_ref (G_OBJECT (info->account), notify_signal_slots, info->self);
		g_ptr_array_add (info->self->signals, slot);

		slot = g_slice_new (SignalSlot);
		slot->instance = (GObject *) info->account;
		slot->handler_id = g_signal_connect (info->account, "changed",
			G_CALLBACK (tny_gtk_folder_list_store_on_changed), info->self);
		g_object_weak_ref (G_OBJECT (info->account), notify_signal_slots, info->self);
		g_ptr_array_add (info->self->signals, slot);

		tny_gtk_folder_list_store_on_constatus_changed (info->account, 
			tny_account_get_connection_status (info->account), info->self);

		tny_gtk_folder_list_store_on_changed (info->account, 
			info->self);

		repeat = FALSE;
	}

	return repeat;
}

static void 
account_was_not_yet_ready_destroy (gpointer user_data)
{
	AccNotYetReadyInfo *info = (AccNotYetReadyInfo *) user_data;

	g_object_unref (info->account);
	g_object_unref (info->self);

	g_slice_free (AccNotYetReadyInfo, user_data);
}


static void
tny_gtk_folder_list_store_add_i (TnyGtkFolderListStore *self, TnyFolderStore *folder_store, listaddfunc func, const gchar *root_name)
{
	GtkListStore *model = GTK_LIST_STORE (self);
	TnyList *folders = tny_simple_list_new ();
	GtkTreeIter name_iter;
	_RefreshInfo *info = NULL;

	func (model, &name_iter);

	gtk_list_store_set (model, &name_iter,
			    TNY_GTK_FOLDER_LIST_STORE_NAME_COLUMN, root_name,
			    TNY_GTK_FOLDER_LIST_STORE_UNREAD_COLUMN, 0,
			    TNY_GTK_FOLDER_LIST_STORE_ALL_COLUMN, 0,
			    TNY_GTK_FOLDER_LIST_STORE_TYPE_COLUMN, TNY_FOLDER_TYPE_ROOT,
			    TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN, folder_store, 
			    -1);

	/* In case we added a store account, it's possible that the account 
	 * will have "the account just got connected" events happening. Accounts
	 * that just got connected might have new folders for us to know about.
	 * That's why we'll handle connection changes. */

	if (TNY_IS_STORE_ACCOUNT (folder_store)) {
		if (!tny_account_is_ready (TNY_ACCOUNT (folder_store))) 
		{
			AccNotYetReadyInfo *info = g_slice_new (AccNotYetReadyInfo);

			info->account = TNY_ACCOUNT (g_object_ref (folder_store));
			info->self = TNY_GTK_FOLDER_LIST_STORE (g_object_ref (self));

			g_timeout_add_full (G_PRIORITY_HIGH, 100, 
				account_was_not_yet_ready_idle, 
				info, account_was_not_yet_ready_destroy);

		} else {
			SignalSlot *slot;

			slot = g_slice_new (SignalSlot);

			slot->instance = (GObject *)folder_store; 
			slot->handler_id = g_signal_connect (folder_store, "connection-status-changed",
				G_CALLBACK (tny_gtk_folder_list_store_on_constatus_changed), self);
			g_object_weak_ref (G_OBJECT (folder_store), notify_signal_slots, self);
			g_ptr_array_add (self->signals, slot);

			slot = g_slice_new (SignalSlot);
			slot->instance = (GObject *)folder_store; 
			slot->handler_id = g_signal_connect (folder_store, "changed",
				G_CALLBACK (tny_gtk_folder_list_store_on_changed), self);
			g_object_weak_ref (G_OBJECT (folder_store), notify_signal_slots, self);
			g_ptr_array_add (self->signals, slot);
		}
	}

	/* Being online at startup is being punished by this: it'll have to
	 * wait for the connection operation to finish before this queued item
	 * will get its turn. TNY TODO: figure out how we can avoid this and
	 * already return the offline folders (if we had those from a session
	 * before this one) */

	self->progress_count++;
	if (self->progress_count == 1) {
		g_signal_emit (self, tny_gtk_folder_list_store_signals[ACTIVITY_CHANGED_SIGNAL], 0, TRUE);
	}

	update_delayed_refresh (self);

	info = g_slice_new (_RefreshInfo);
	info->self = g_object_ref (self);
	info->do_poke_status = FALSE;
	tny_folder_store_get_folders_async (TNY_FOLDER_STORE (folder_store), 
					    folders, self->query, 
					    !(self->flags & TNY_GTK_FOLDER_LIST_STORE_FLAG_NO_REFRESH),  
					    get_folders_cb, NULL, info);

	/* Add an observer for the root folder store, so that we can observe 
	 * the actual account too. */

	add_folder_store_observer_weak (self, folder_store);

	/* g_object_unref (G_OBJECT (folders)); */

	return;
}


/**
 * tny_gtk_folder_list_store_new:
 * @query: the #TnyFolderStoreQuery that will be used to retrieve the child folders of each #TnyFolderStore
 *
 * Create a new #GtkTreeModel for showing #TnyFolderStore instances, with default flags
 * 
 * returns: (caller-owns): a new #GtkTreeModel for #TnyFolderStore instances
 * since: 1.0
 * audience: application-developer
 **/
GtkTreeModel*
tny_gtk_folder_list_store_new (TnyFolderStoreQuery *query)
{
	return tny_gtk_folder_list_store_new_with_flags (query, 0);
}

/**
 * tny_gtk_folder_list_store_new:
 * @query: the #TnyFolderStoreQuery that will be used to retrieve the child folders of each #TnyFolderStore
 * @flags: #TnyGtkFolderListStoreFlags for setting the store
 *
 * Create a new #GtkTreeModel for showing #TnyFolderStore instances
 * 
 * returns: (caller-owns): a new #GtkTreeModel for #TnyFolderStore instances
 * since: 1.0
 * audience: application-developer
 **/
GtkTreeModel*
tny_gtk_folder_list_store_new_with_flags (TnyFolderStoreQuery *query,
						TnyGtkFolderListStoreFlags flags)
{
	TnyGtkFolderListStore *self = g_object_new (TNY_TYPE_GTK_FOLDER_LIST_STORE, NULL);

	if (query) 
		self->query = g_object_ref (query);

	/* DELAYED_REFRESH implies NO_REFRESH */
	if (flags & TNY_GTK_FOLDER_LIST_STORE_FLAG_DELAYED_REFRESH)
		flags |= TNY_GTK_FOLDER_LIST_STORE_FLAG_NO_REFRESH;

	self->flags = flags;

	return GTK_TREE_MODEL (self);
}



static void
tny_gtk_folder_list_store_dispose (GObject *object)
{
	TnyGtkFolderListStore *me = (TnyGtkFolderListStore*) object;

	me->flags |= TNY_GTK_FOLDER_LIST_STORE_FLAG_DISPOSED;

	(*parent_class->dispose) (object);
}

static void
tny_gtk_folder_list_store_finalize (GObject *object)
{
	TnyGtkFolderListStore *me = (TnyGtkFolderListStore*) object;
	int i = 0;

	if (me->delayed_refresh_timeout_id > 0) {
		g_source_remove (me->delayed_refresh_timeout_id);
		me->delayed_refresh_timeout_id = 0;
	}

	for (i = 0; i < me->signals->len; i++) {
		SignalSlot *slot = (SignalSlot *) me->signals->pdata [i];
		if (slot->instance) {
			g_signal_handler_disconnect (slot->instance, slot->handler_id);
			g_object_weak_unref (G_OBJECT (slot->instance), notify_signal_slots, object);
		}
		g_slice_free (SignalSlot, slot);
	}

	g_ptr_array_free (me->signals, TRUE);
	me->signals = NULL;

	if (me->fol_obs)
		g_list_free (me->fol_obs);
	me->fol_obs = NULL;

	if (me->store_obs)
		g_list_free (me->store_obs);
	me->store_obs = NULL;

	g_mutex_lock (me->iterator_lock);
	if (me->first) {
		if (me->first_needs_unref)
			g_list_foreach (me->first, (GFunc)g_object_unref, NULL);
		me->first_needs_unref = FALSE;
		g_list_free (me->first); 
	}
	me->first = NULL;
	g_mutex_unlock (me->iterator_lock);


	g_mutex_free (me->iterator_lock);
	me->iterator_lock = NULL;

	if (me->query)
		g_object_unref (me->query);

	(*parent_class->finalize) (object);
}

static void
tny_gtk_folder_list_store_class_init (TnyGtkFolderListStoreClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_gtk_folder_list_store_finalize;
	object_class->dispose = tny_gtk_folder_list_store_dispose;

	tny_gtk_folder_list_store_signals [ACTIVITY_CHANGED_SIGNAL] =
		g_signal_new ("activity_changed",
			      TNY_TYPE_GTK_FOLDER_LIST_STORE,
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (TnyGtkFolderListStoreClass, activity_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

	return;
}

static void
tny_gtk_folder_list_store_instance_init (GTypeInstance *instance, gpointer g_class)
{
	GtkListStore *store = (GtkListStore*) instance;
	TnyGtkFolderListStore *me = (TnyGtkFolderListStore*) instance;
	static GType types[] = { G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INT, G_TYPE_OBJECT };

	me->signals = g_ptr_array_new ();
	me->fol_obs = NULL;
	me->store_obs = NULL;
	me->first = NULL;
	me->iterator_lock = g_mutex_new ();
	me->first_needs_unref = FALSE;

	me->flags = 0;
	me->path_separator = g_strdup (DEFAULT_PATH_SEPARATOR);

	me->progress_count = 0;
	me->delayed_refresh_timeout_id = 0;

	gtk_list_store_set_column_types (store, 
		TNY_GTK_FOLDER_LIST_STORE_N_COLUMNS, types);

	return;
}



static TnyIterator*
tny_gtk_folder_list_store_create_iterator (TnyList *self)
{
	TnyGtkFolderListStore *me = (TnyGtkFolderListStore*)self;

	/* Return a new iterator */

	return TNY_ITERATOR (_tny_gtk_folder_list_store_iterator_new (me));
}


/**
 * tny_gtk_folder_list_store_prepend:
 * @self: a #TnyGtkFolderListStore
 * @item: a #TnyFolderStore to add
 * @root_name: The node's root name 
 *
 * Prepends an item to the model
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_gtk_folder_list_store_prepend (TnyGtkFolderListStore *self, TnyFolderStore* item, const gchar *root_name)
{
	TnyGtkFolderListStore *me = (TnyGtkFolderListStore*)self;

	g_mutex_lock (me->iterator_lock);

	/* Prepend something to the list */
	me->first = g_list_prepend (me->first, item);

	tny_gtk_folder_list_store_add_i (me, TNY_FOLDER_STORE (item), 
		gtk_list_store_prepend, root_name);

	g_mutex_unlock (me->iterator_lock);
}


static void
tny_gtk_folder_list_store_prepend_i (TnyList *self, GObject* item)
{
	TnyGtkFolderListStore *me = (TnyGtkFolderListStore*)self;

	g_mutex_lock (me->iterator_lock);

	/* Prepend something to the list */
	me->first = g_list_prepend (me->first, item);

	tny_gtk_folder_list_store_add_i (me, TNY_FOLDER_STORE (item), 
		gtk_list_store_prepend, get_root_name (TNY_FOLDER_STORE (item)));

	g_mutex_unlock (me->iterator_lock);
}

/**
 * tny_gtk_folder_list_store_append:
 * @self: a #TnyGtkFolderListStore
 * @item: a #TnyFolderStore to add
 * @root_name: The node's root name 
 *
 * Appends an item to the model
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_gtk_folder_list_store_append (TnyGtkFolderListStore *self, TnyFolderStore* item, const gchar *root_name)
{
	TnyGtkFolderListStore *me = (TnyGtkFolderListStore*)self;

	g_mutex_lock (me->iterator_lock);

	/* Append something to the list */
	me->first = g_list_append (me->first, item);

	tny_gtk_folder_list_store_add_i (me, TNY_FOLDER_STORE (item), 
		gtk_list_store_append, root_name);

	g_mutex_unlock (me->iterator_lock);
}

static void
tny_gtk_folder_list_store_append_i (TnyList *self, GObject* item)
{
	TnyGtkFolderListStore *me = (TnyGtkFolderListStore*)self;

	g_mutex_lock (me->iterator_lock);

	/* Append something to the list */
	me->first = g_list_append (me->first, item);

	tny_gtk_folder_list_store_add_i (me, TNY_FOLDER_STORE (item), 
		gtk_list_store_append, get_root_name (TNY_FOLDER_STORE (item)));

	g_mutex_unlock (me->iterator_lock);
}

static guint
tny_gtk_folder_list_store_get_length (TnyList *self)
{
	TnyGtkFolderListStore *me = (TnyGtkFolderListStore*)self;
	guint retval = 0;

	g_mutex_lock (me->iterator_lock);

	retval = me->first?g_list_length (me->first):0;

	g_mutex_unlock (me->iterator_lock);

	return retval;
}

static void
tny_gtk_folder_list_store_remove (TnyList *self, GObject* item)
{
	TnyGtkFolderListStore *me = (TnyGtkFolderListStore*)self;
	GtkTreeModel *model = GTK_TREE_MODEL (me);
	GtkTreeIter iter;

	g_return_if_fail (G_IS_OBJECT (item));
	g_return_if_fail (G_IS_OBJECT (me));

	/* Remove something from the list */

	g_mutex_lock (me->iterator_lock);

	me->first = g_list_remove (me->first, (gconstpointer)item);

	if (gtk_tree_model_get_iter_first (model, &iter)) {
		gboolean more_items = TRUE;

		do {
			GObject *citem = NULL;
			gboolean deleted;

			deleted = FALSE;
			gtk_tree_model_get (model, &iter, 
					    TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN, 
					    &citem, -1);

			if (TNY_IS_ACCOUNT (citem) || TNY_IS_MERGE_FOLDER (citem)) {
				if (citem == item) {
					more_items = gtk_list_store_remove (GTK_LIST_STORE (me), &iter);
					deleted = TRUE;
				}
			} else if (TNY_IS_FOLDER (citem)) {
				TnyAccount *account = tny_folder_get_account (TNY_FOLDER (citem));
				if (account) {
					if ((GObject *) account == item) {
						more_items = gtk_list_store_remove (GTK_LIST_STORE (me), &iter);
						deleted = TRUE;
						if (TNY_GTK_FOLDER_LIST_STORE (self)->progress_count > 0)
							tny_account_cancel (TNY_ACCOUNT (account));
					}
					g_object_unref (account);
				}
			}
			if (citem)
				g_object_unref (citem);

			/* If the item was deleted then the iter was
			   moved to the next row */
			if (!deleted)
				more_items = gtk_tree_model_iter_next (model, &iter);
		} while (more_items);
	}


	g_mutex_unlock (me->iterator_lock);
}


static TnyList*
tny_gtk_folder_list_store_copy_the_list (TnyList *self)
{
	TnyGtkFolderListStore *me = (TnyGtkFolderListStore*)self;
	TnyGtkFolderListStore *copy = g_object_new (TNY_TYPE_GTK_FOLDER_LIST_STORE, NULL);
	GList *list_copy = NULL;

	/* This only copies the TnyList pieces. The result is not a correct or good
	   TnyHeaderListModel. But it will be a correct TnyList instance. It's the 
	   only thing the user of this method expects (that is the contract of it).

	   The new list will point to the same instances, of course. It's only a 
	   copy of the list-nodes of course. */

	g_mutex_lock (me->iterator_lock);
	list_copy = g_list_copy (me->first);
	g_list_foreach (list_copy, (GFunc)g_object_ref, NULL);
	copy->first_needs_unref = TRUE;
	copy->first = list_copy;
	g_mutex_unlock (me->iterator_lock);

	return TNY_LIST (copy);
}

static void 
tny_gtk_folder_list_store_foreach_in_the_list (TnyList *self, GFunc func, gpointer user_data)
{
	TnyGtkFolderListStore *me = (TnyGtkFolderListStore*)self;

	/* Foreach item in the list (without using a slower iterator) */

	g_mutex_lock (me->iterator_lock);
	g_list_foreach (me->first, func, user_data);
	g_mutex_unlock (me->iterator_lock);

	return;
}

typedef struct _FindParentHelperInfo {
	GtkTreeIter *iter;
	TnyFolder *folder;
	TnyAccount *account;
	gboolean found;
} FindParentHelperInfo;

static gboolean
find_parent_helper (GtkTreeModel *model,
		    GtkTreePath *path,
		    GtkTreeIter *iter,
		    gpointer userdata)
{
	TnyFolderStore *folder_store;
	FindParentHelperInfo *helper_info = (FindParentHelperInfo *) userdata;
	TnyList *children;
	TnyIterator *iterator;

	gtk_tree_model_get (model, iter, 
			    TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN, 
			    &folder_store, -1);

	/* Only search on same account */
	if (TNY_IS_ACCOUNT (folder_store) && ((TnyAccount *)folder_store == helper_info->account)) {
		g_object_unref (folder_store);
		return FALSE;
	}

	if (TNY_IS_FOLDER (folder_store)) {
		TnyAccount *account = NULL;
		account = tny_folder_get_account (TNY_FOLDER (folder_store));
		g_object_unref (account);
		if (account == helper_info->account) {
			g_object_unref (folder_store);
			return FALSE;
		}
	}

	children = TNY_LIST (tny_simple_list_new ());
	tny_folder_store_get_folders (folder_store, children, NULL, FALSE, NULL);
	iterator = tny_list_create_iterator (children);

	while (!tny_iterator_is_done (iterator)) {
		TnyFolderStore *child;

		child = (TnyFolderStore *) tny_iterator_get_current (iterator);
		g_object_unref (child);
		if (child == (TnyFolderStore *) helper_info->folder) {
			helper_info->found = TRUE;
			*helper_info->iter = *iter;
			break;
		}
		tny_iterator_next (iterator);
	}
	g_object_unref (iterator);
	g_object_unref (children);
	g_object_unref (folder_store);

	return helper_info->found;
	
}

static gboolean
find_parent (GtkTreeModel *model, TnyFolder *folder, GtkTreeIter *iter)
{
	FindParentHelperInfo *helper_info;
	gboolean result;

	helper_info = g_slice_new0 (FindParentHelperInfo);

	helper_info->folder = folder;
	helper_info->iter = iter;
	helper_info->account = tny_folder_get_account (folder);

	gtk_tree_model_foreach (model, find_parent_helper, helper_info);

	g_object_unref (helper_info->account);
	result = helper_info->found;
	g_slice_free (FindParentHelperInfo, helper_info);

	return result;
}

typedef struct _FindHelperInfo {
	GtkTreeIter *iter;
	TnyFolder *folder;
	gboolean found;
} FindHelperInfo;

static gboolean
find_node_helper (GtkTreeModel *model,
		  GtkTreePath *path,
		  GtkTreeIter *iter,
		  gpointer userdata)
{
	TnyFolderStore *folder_store;
	FindHelperInfo *helper_info = (FindHelperInfo *) userdata;

	gtk_tree_model_get (model, iter, 
			    TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN, 
			    &folder_store, -1);

	if ((TnyFolderStore *) helper_info->folder == folder_store) {
		helper_info->found = TRUE;
		*helper_info->iter = *iter;
	}
	g_object_unref (folder_store);

	return helper_info->found;
	
}

static gboolean
find_node (GtkTreeModel *model, TnyFolder *folder, GtkTreeIter *iter)
{
	FindHelperInfo *helper_info;
	gboolean result;

	helper_info = g_slice_new0 (FindHelperInfo);

	helper_info->folder = folder;
	helper_info->iter = iter;

	gtk_tree_model_foreach (model, find_node_helper, helper_info);

	result = helper_info->found;
	g_slice_free (FindHelperInfo, helper_info);

	return result;
}

static void
update_children_names (GtkTreeModel *model, TnyFolder *folder, const gchar *name)
{
	TnyList *children;
	TnyIterator *iterator;
	TnyGtkFolderListStore *self = TNY_GTK_FOLDER_LIST_STORE (model);

	children = TNY_LIST (tny_simple_list_new ());
	iterator = tny_list_create_iterator (children);

	while (!tny_iterator_is_done (iterator)) {
		GtkTreeIter iter;
		TnyFolder *child;

		child = TNY_FOLDER (tny_iterator_get_current (iterator));
		if (find_node (model, child, &iter)) {
			gchar *new_name;
			new_name = g_strconcat (name, self->path_separator,
						tny_folder_get_name (TNY_FOLDER (child)),
						NULL);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    TNY_GTK_FOLDER_LIST_STORE_NAME_COLUMN, 
					    new_name,
					    -1);
			update_children_names (model, child, new_name);
			g_free (new_name);
		}

		g_object_unref (folder);
		tny_iterator_next (iterator);
	}

	g_object_unref (iterator);
	g_object_unref (children);
}

static void
update_folder_name (GtkTreeModel *model, TnyFolder *folder, GtkTreeIter *iter, gboolean update_children)
{
	GtkTreeIter parent_iter;
	gchar *name = NULL;
	TnyGtkFolderListStore *self = TNY_GTK_FOLDER_LIST_STORE (model);

	if (self->flags & TNY_GTK_FOLDER_LIST_STORE_FLAG_SHOW_PATH) {
		if (find_parent (model, folder, &parent_iter)) {
			gchar *parent_name;
			gtk_tree_model_get (model, &parent_iter, 
					    TNY_GTK_FOLDER_LIST_STORE_NAME_COLUMN, 
					    &parent_name, -1);
			if (parent_name && parent_name[0] == '\0')
				name = g_strconcat (parent_name, self->path_separator,
						    tny_folder_get_name (TNY_FOLDER (folder)),
						    NULL);
			g_free (parent_name);
		}
	}

	if (name == NULL)
		name = g_strdup (tny_folder_get_name (TNY_FOLDER (folder)));
	
	gtk_list_store_set (GTK_LIST_STORE (model), iter,
			    TNY_GTK_FOLDER_LIST_STORE_NAME_COLUMN, 
			    name,
			    -1);
	if (update_children && 
	    (self->flags & TNY_GTK_FOLDER_LIST_STORE_FLAG_SHOW_PATH)) {
		update_children_names (model, folder, name);
	}
	g_free (name);
}

static void
update_names (TnyGtkFolderListStore *self)
{
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self), &iter))
		return;

	do {
		TnyFolderStore *store;
		gtk_tree_model_get (GTK_TREE_MODEL (self), &iter, 
				    TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN, 
				    &store, -1);

		if (TNY_IS_FOLDER (store)) {
			update_folder_name (GTK_TREE_MODEL (self),
					    TNY_FOLDER (store),
					    &iter,
					    FALSE /*don't update children*/);
		}
		g_object_unref (store);
		
	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (self), &iter));
}


static gboolean 
updater (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data1)
{
	TnyFolderType type = TNY_FOLDER_TYPE_UNKNOWN;
	TnyFolderChange *change = user_data1;
	TnyFolder *changed_folder = tny_folder_change_get_folder (change);
	TnyFolderChangeChanged changed = tny_folder_change_get_changed (change);

	/* This updater will get the folder out of the model, compare with 
	 * the changed_folder pointer, and if there's a match it will update
	 * the values of the model with the values from the folder. */

/*
	For your debugging advertures:

	print ((TnyCamelFolderPriv *) g_type_instance_get_private \
		(tny_iterator_get_current (tny_list_create_iterator \
		(((TnyMergeFolderPriv *) g_type_instance_get_private \
		(folder, tny_merge_folder_get_type()))->mothers)), \
		tny_camel_folder_get_type()))->folder->summary->messages->len

	print * (CamelLocalFolder *) ((TnyCamelFolderPriv *) \
		g_type_instance_get_private (tny_iterator_get_current \
		(tny_list_create_iterator (((TnyMergeFolderPriv *) \
		g_type_instance_get_private (folder, \
		tny_merge_folder_get_type()))->mothers)), \
		tny_camel_folder_get_type()))->folder

*/


	gtk_tree_model_get (model, iter, 
		TNY_GTK_FOLDER_LIST_STORE_TYPE_COLUMN, 
		&type, -1);

	if (type != TNY_FOLDER_TYPE_ROOT) 
	{
		TnyFolder *folder;
		gint unread, total;

		gtk_tree_model_get (model, iter, 
			TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN, 
			&folder, -1);

		if (changed & TNY_FOLDER_CHANGE_CHANGED_ALL_COUNT)
			total = tny_folder_change_get_new_all_count (change);
		else
			total = tny_folder_get_all_count (folder);

		if (changed & TNY_FOLDER_CHANGE_CHANGED_UNREAD_COUNT)
			unread = tny_folder_change_get_new_unread_count (change);
		else
			unread = tny_folder_get_unread_count (folder);

		if (folder == changed_folder)
		{
			/* TNY TODO: This is not enough: Subfolders will be incorrect because the
			   the full_name of the subfolders will still be the old full_name!*/

			gtk_list_store_set (GTK_LIST_STORE (model), iter,
				TNY_GTK_FOLDER_LIST_STORE_UNREAD_COLUMN, 
				unread,
				TNY_GTK_FOLDER_LIST_STORE_ALL_COLUMN, 
				total, -1);

			if (changed & TNY_FOLDER_CHANGE_CHANGED_FOLDER_RENAME) {
				update_folder_name (model, folder, iter, TRUE /*update children*/);
			}
		}

		g_object_unref (folder);
	}

	g_object_unref (changed_folder);

	return FALSE;
}

static gboolean
is_folder_ancestor (GObject *parent, GObject *item)
{
	gboolean retval = FALSE;
	TnyFolderStore *parent_store;

	if (!TNY_IS_FOLDER (item) || 
	    !TNY_IS_FOLDER (parent) || 
	    TNY_IS_MERGE_FOLDER (item))
		return FALSE;

	parent_store = tny_folder_get_folder_store (TNY_FOLDER (item));
	while (TNY_IS_FOLDER (parent_store) && !retval) {
		if (parent_store == (TnyFolderStore *) parent) {
			retval = TRUE;
		} else {
			GObject *old = (GObject *) parent_store;
			parent_store = tny_folder_get_folder_store (TNY_FOLDER (parent_store));
			g_object_unref (old);
		}
	}
	g_object_unref (parent_store);

	return retval;
}

static void
deleter (GtkTreeModel *model, TnyFolder *folder)
{
	TnyGtkFolderListStore *me = (TnyGtkFolderListStore*) model;
	GtkTreeIter iter;

	/* The deleter will compare all folders in the model with the deleted 
	 * folder @folder, and if there's a match it will delete the folder's
	 * row from the model. */
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		gboolean more_items = TRUE;

		do {
			GObject *citem = NULL;
			gboolean deleted;

			deleted = FALSE;
			gtk_tree_model_get (model, &iter,
					    TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN,
					    &citem, -1);

			if (TNY_IS_FOLDER (citem)) {
				/* We need to remove both the folder and its children */
				if ((citem == (GObject *) folder) ||
				    is_folder_ancestor ((GObject *) folder, citem)) {

					remove_folder_observer_weak (me, TNY_FOLDER (citem), FALSE);
					remove_folder_store_observer_weak (me, TNY_FOLDER_STORE (citem), FALSE);

					more_items = gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
					deleted = TRUE;
				}
			}

			if (citem)
				g_object_unref (citem);

			/* If the item was deleted then the iter was
			   moved to the next row */
			if (!deleted)
				more_items = gtk_tree_model_iter_next (model, &iter);
		} while (more_items);
	}
}

static gboolean
creater (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *in_iter, gpointer user_data)
{
	TnyGtkFolderListStore *self = (TnyGtkFolderListStore*) model;
	TnyFolderStore *fol = NULL;
	gboolean found = FALSE;
	const gchar *mid = NULL, *sid = NULL;
	TnyFolderStoreChange *change = (TnyFolderStoreChange *) user_data;
	TnyFolderStore *parent_store;
	GtkTreeIter iter;

	/* This creater will get the store out of the model, compare with 
	 * the change's store pointer, and if there's a match it will add the
	 * the created folders to the model at that location. */

	if (!gtk_tree_model_get_iter (model, &iter, path)) {
		g_warning ("Internal state of the TnyGtkFolderListStore is corrupted\n");
		return FALSE;
	}

	parent_store = tny_folder_store_change_get_folder_store (change);
	gtk_tree_model_get (model, &iter, 
		TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN, 
		&fol, -1);

	if (fol == parent_store)
		found = TRUE;

	if (!found) {
		TnyAccount *maccount = NULL, *saccount = NULL;
		const gchar *ma_id = NULL, *sa_id = NULL;

		if (fol && TNY_IS_FOLDER (fol)) {
			mid = tny_folder_get_id (TNY_FOLDER (fol));
			if (!TNY_IS_MERGE_FOLDER (fol))
				maccount = tny_folder_get_account (TNY_FOLDER (fol));
			if (maccount)
				ma_id = tny_account_get_id (maccount);
		}
		if (parent_store && TNY_IS_FOLDER (parent_store)) {
			sid = tny_folder_get_id (TNY_FOLDER (parent_store));
			if (!TNY_IS_MERGE_FOLDER (parent_store))
				saccount = tny_folder_get_account (TNY_FOLDER (parent_store));
			if (saccount)
				sa_id = tny_account_get_id (saccount);
		}
		if (sid && mid && !strcmp (sid, mid)) {
			if (ma_id && sa_id) {
				if (!strcmp (ma_id, sa_id))
					found = TRUE;
			} else {
				found = TRUE;
			}
		}
		if (maccount)
			g_object_unref (maccount);
		if (saccount)
			g_object_unref (saccount);
	}

	if (found) 
	{
		TnyList *created = tny_simple_list_new ();
		TnyIterator *miter;
		gchar *parent_name;

		tny_folder_store_change_get_created_folders (change, created);
		miter = tny_list_create_iterator (created);

		/* We assume parent name is already the expected one in full path style */
		gtk_tree_model_get (model, in_iter,
				    TNY_GTK_FOLDER_LIST_STORE_NAME_COLUMN, &parent_name, 
				    -1);

		while (!tny_iterator_is_done (miter))
		{
			GtkTreeIter newiter;
			TnyFolder *folder = TNY_FOLDER (tny_iterator_get_current (miter));
			gchar *finalname;

			add_folder_observer_weak (self, folder);
			add_folder_store_observer_weak (self, TNY_FOLDER_STORE (folder));

			/* This adds a reference count to folder_store too. When it gets 
			   removed, that reference count is decreased automatically by 
			   the gtktreestore infrastructure. */

			if (TNY_GTK_FOLDER_LIST_STORE (self)->flags &
			    TNY_GTK_FOLDER_LIST_STORE_FLAG_SHOW_PATH) {
				if (TNY_IS_FOLDER (fol) && parent_name && *parent_name != '\0')
					finalname = g_strconcat (parent_name, self->path_separator,
								 tny_folder_get_name (TNY_FOLDER (folder)), NULL);
				else
					finalname = g_strdup (tny_folder_get_name (TNY_FOLDER (folder)));
			} else {
				finalname = g_strdup (tny_folder_get_name (TNY_FOLDER (folder)));
			}

			gtk_list_store_prepend (GTK_LIST_STORE (model), &newiter);

			gtk_list_store_set (GTK_LIST_STORE (model), &newiter,
				TNY_GTK_FOLDER_LIST_STORE_NAME_COLUMN, 
				finalname,
				TNY_GTK_FOLDER_LIST_STORE_UNREAD_COLUMN, 
				tny_folder_get_unread_count (TNY_FOLDER (folder)),
				TNY_GTK_FOLDER_LIST_STORE_ALL_COLUMN, 
				tny_folder_get_all_count (TNY_FOLDER (folder)),
				TNY_GTK_FOLDER_LIST_STORE_TYPE_COLUMN,
				tny_folder_get_folder_type (TNY_FOLDER (folder)),
				TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN,
				folder, -1);
			g_free (finalname);

			g_object_unref (folder);
			tny_iterator_next (miter);
		}
		g_free (parent_name);
		g_object_unref (miter);
		g_object_unref (created);
	}

	if (fol)
		g_object_unref (fol);

	if (parent_store)
		g_object_unref (parent_store);

	return found;
}

void 
tny_gtk_folder_list_store_set_path_separator (TnyGtkFolderListStore *self, 
					      const gchar *separator)
{
	g_return_if_fail (TNY_IS_GTK_FOLDER_LIST_STORE (self));

	if (separator == NULL)
		separator = "";

	g_free (self->path_separator);

	self->path_separator = g_strdup (separator);

	if (self->flags & TNY_GTK_FOLDER_LIST_STORE_FLAG_SHOW_PATH)
		update_names (self);
}

const gchar *
tny_gtk_folder_list_store_get_path_separator (TnyGtkFolderListStore *self)
{
	g_return_val_if_fail (TNY_IS_GTK_FOLDER_LIST_STORE (self), NULL);

	return self->path_separator;
}


gboolean 
tny_gtk_folder_list_store_get_activity (TnyGtkFolderListStore *self)
{
	g_return_val_if_fail (TNY_IS_GTK_FOLDER_LIST_STORE (self), FALSE);

	return (self->progress_count > 0);
}


static void
tny_gtk_folder_list_store_folder_obsr_update (TnyFolderObserver *self, TnyFolderChange *change)
{
	TnyFolderChangeChanged changed = tny_folder_change_get_changed (change);
	GtkTreeModel *model = (GtkTreeModel *) self;

	if (changed & TNY_FOLDER_CHANGE_CHANGED_FOLDER_RENAME ||
		changed & TNY_FOLDER_CHANGE_CHANGED_ALL_COUNT || 
		changed & TNY_FOLDER_CHANGE_CHANGED_UNREAD_COUNT)
	{
		gtk_tree_model_foreach (model, updater, change);
	}

	return;
}

static void 
delete_these_folders (GtkTreeModel *model, TnyList *list)
{
		TnyIterator *miter;
		miter = tny_list_create_iterator (list);
		while (!tny_iterator_is_done (miter))
		{
			TnyFolder *folder = TNY_FOLDER (tny_iterator_get_current (miter));
			deleter (model, folder);
			g_object_unref (folder);
			tny_iterator_next (miter);
		}
		g_object_unref (miter);
}

static void
tny_gtk_folder_list_store_store_obsr_update (TnyFolderStoreObserver *self, TnyFolderStoreChange *change)
{
	TnyFolderStoreChangeChanged changed = tny_folder_store_change_get_changed (change);
	GtkTreeModel *model = GTK_TREE_MODEL (self);

	if (changed & TNY_FOLDER_STORE_CHANGE_CHANGED_CREATED_FOLDERS) {
		/* TnyList *created = tny_simple_list_new ();
		 * tny_folder_store_change_get_created_folders (change, created);
		 * delete_these_folders (model, created);
		 * g_object_unref (created); */
		gtk_tree_model_foreach (model, creater, change);
	}

	if (changed & TNY_FOLDER_STORE_CHANGE_CHANGED_REMOVED_FOLDERS)
	{
		TnyList *removed = tny_simple_list_new ();
		tny_folder_store_change_get_removed_folders (change, removed);
		delete_these_folders (model, removed);
		g_object_unref (removed);
	}

	return;
}

static void
tny_folder_store_observer_init (TnyFolderStoreObserverIface *klass)
{
	klass->update= tny_gtk_folder_list_store_store_obsr_update;
}

static void
tny_folder_observer_init (TnyFolderObserverIface *klass)
{
	klass->update= tny_gtk_folder_list_store_folder_obsr_update;
}

static void
tny_list_init (TnyListIface *klass)
{
	klass->get_length= tny_gtk_folder_list_store_get_length;
	klass->prepend= tny_gtk_folder_list_store_prepend_i;
	klass->append= tny_gtk_folder_list_store_append_i;
	klass->remove= tny_gtk_folder_list_store_remove;
	klass->create_iterator= tny_gtk_folder_list_store_create_iterator;
	klass->copy= tny_gtk_folder_list_store_copy_the_list;
	klass->foreach= tny_gtk_folder_list_store_foreach_in_the_list;

	return;
}

static gpointer
tny_gtk_folder_list_store_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyGtkFolderListStoreClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_gtk_folder_list_store_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyGtkFolderListStore),
		  0,      /* n_preallocs */
		  tny_gtk_folder_list_store_instance_init    /* instance_init */
		};

	static const GInterfaceInfo tny_list_info = {
		(GInterfaceInitFunc) tny_list_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo tny_folder_store_observer_info = {
		(GInterfaceInitFunc) tny_folder_store_observer_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo tny_folder_observer_info = {
		(GInterfaceInitFunc) tny_folder_observer_init,
		NULL,
		NULL
	};

	type = g_type_register_static (GTK_TYPE_LIST_STORE, "TnyGtkFolderListStore",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_LIST,
				     &tny_list_info);
	g_type_add_interface_static (type, TNY_TYPE_FOLDER_STORE_OBSERVER,
				     &tny_folder_store_observer_info);
	g_type_add_interface_static (type, TNY_TYPE_FOLDER_OBSERVER,
				     &tny_folder_observer_info);

	return GSIZE_TO_POINTER (type);
}

/**
 * tny_gtk_folder_list_store_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType
tny_gtk_folder_list_store_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gtk_folder_list_store_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}

static gpointer
tny_gtk_folder_list_store_column_register_type (gpointer notused)
{
  static GType etype = 0;
  static const GEnumValue values[] = {
      { TNY_GTK_FOLDER_LIST_STORE_NAME_COLUMN, "TNY_GTK_FOLDER_LIST_STORE_NAME_COLUMN", "name" },
      { TNY_GTK_FOLDER_LIST_STORE_UNREAD_COLUMN, "TNY_GTK_FOLDER_LIST_STORE_UNREAD_COLUMN", "unread" },
      { TNY_GTK_FOLDER_LIST_STORE_ALL_COLUMN, "TNY_GTK_FOLDER_LIST_STORE_ALL_COLUMN", "all" },
      { TNY_GTK_FOLDER_LIST_STORE_TYPE_COLUMN, "TNY_GTK_FOLDER_LIST_STORE_TYPE_COLUMN", "type" },
      { TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN, "TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN", "instance" },
      { TNY_GTK_FOLDER_LIST_STORE_N_COLUMNS, "TNY_GTK_FOLDER_LIST_STORE_N_COLUMNS", "n" },
      { 0, NULL, NULL }
  };
  etype = g_enum_register_static ("TnyGtkFolderListStoreColumn", values);
  return GSIZE_TO_POINTER (etype);
}

/**
 * tny_gtk_folder_list_store_column_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType
tny_gtk_folder_list_store_column_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gtk_folder_list_store_column_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
