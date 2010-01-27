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
 * TnyMergeFolder:
 *
 * A #TnyFolder that merges other folders together 
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <string.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include <tny-merge-folder.h>
#include <tny-error.h>
#include <tny-simple-list.h>
#include <tny-folder-observer.h>
#include <tny-noop-lockable.h>

static GObjectClass *parent_class = NULL;

typedef struct _TnyMergeFolderPriv TnyMergeFolderPriv;

struct _TnyMergeFolderPriv
{
	gchar *id, *name;
	TnyList *mothers;
	GStaticRecMutex *lock;
	TnyFolderType folder_type;
	GList *obs;
	TnyLockable *ui_locker;
};

#define TNY_MERGE_FOLDER_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_MERGE_FOLDER, TnyMergeFolderPriv))



static void
notify_folder_observers_about (TnyFolder *self, TnyFolderChange *change)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	GList *list, *list_iter;

	g_static_rec_mutex_lock (priv->lock);
	if (!priv->obs) {
		g_static_rec_mutex_unlock (priv->lock);
		return;
	}
	list = g_list_copy (priv->obs);
	list_iter = list;
	g_static_rec_mutex_unlock (priv->lock);


	while (list_iter)
	{
		TnyFolderObserver *observer = TNY_FOLDER_OBSERVER (list_iter->data);

		/* We don't need to hold the ui-lock here, because this is called
		 * from the update function, which is guaranteed to happen in the
		 * UI context already (the TnyFolder layer takes care of this) */

		tny_folder_observer_update (observer, change);
		list_iter = g_list_next (list_iter);
	}

	g_list_free (list);

	return;
}



static void
tny_merge_folder_remove_msg (TnyFolder *self, TnyHeader *header, GError **err)
{
	TnyFolder *fol = tny_header_get_folder (header);

	tny_folder_remove_msg (fol, header, err);
	g_object_unref (fol);

	return;
}



typedef struct 
{
	TnyFolder *self;
	TnyList *headers;
	TnyFolderCallback callback;
	TnyStatusCallback status_callback;
	gpointer user_data;
	gboolean cancelled, refresh;
	guint depth;
	GError *err;
} RemMsgsInfo;


static void
remove_msgs_async_destroyer (gpointer thr_user_data)
{
	RemMsgsInfo *info = thr_user_data;

	/* thread reference */
	g_object_unref (info->self);
	g_object_unref (info->headers);

	if (info->err)
		g_error_free (info->err);

	g_slice_free (RemMsgsInfo, thr_user_data);

	return;
}

static gboolean
remove_msgs_async_callback (gpointer thr_user_data)
{
	RemMsgsInfo *info = thr_user_data;
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (info->self);

	if (info->callback) {
		tny_lockable_lock (priv->ui_locker);
		info->callback (info->self, info->cancelled, info->err, info->user_data);
		tny_lockable_unlock (priv->ui_locker);
	}

	return FALSE;
}


static gpointer 
remove_msgs_async_thread (gpointer thr_user_data)
{
	RemMsgsInfo *info = thr_user_data;
	TnyFolder *self = info->self;
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	TnyIterator *iter;
	GError *err = NULL;
	TnyList *copy;

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	info->cancelled = FALSE;

	iter = tny_list_create_iterator (copy);
	while (!tny_iterator_is_done (iter))
	{
		TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));

		tny_folder_remove_msgs (cur, info->headers, &err);

		/* TODO: Handler err */

		/* TODO: Handle progress status callbacks ( info->status_callback )
		 * you might have to start using refresh_async for that (in a 
		 * serialized way, else you'd launch a buch of concurrent threads
		 * and ain't going to be nice, perhaps). */

		g_object_unref (cur);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (copy);

	info->err = NULL;

	if (info->callback)
	{
		if (info->depth > 0)
		{
			g_idle_add_full (G_PRIORITY_HIGH, 
				remove_msgs_async_callback, 
				info, remove_msgs_async_destroyer);
		} else {
			remove_msgs_async_callback (info);
			remove_msgs_async_destroyer (info);
		}
	} else { /* Thread reference */
		remove_msgs_async_destroyer (info);
	}

	g_thread_exit (NULL);

	return NULL;
}


static void
tny_merge_folder_remove_msgs_async (TnyFolder *self, TnyList *headers, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	RemMsgsInfo *info;
	GThread *thread;

	info = g_slice_new0 (RemMsgsInfo);
	info->err = NULL;
	info->self = self;
	info->headers = headers;
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;
	info->depth = g_main_depth ();

	/* thread reference */
	g_object_ref (self);
	g_object_ref (headers);

	thread = g_thread_create (remove_msgs_async_thread, info, FALSE, NULL);

	return;
}


static void
tny_merge_folder_remove_msgs (TnyFolder *self, TnyList *headers, GError **err)
{
	TnyIterator *iter = tny_list_create_iterator (headers);
	while (!tny_iterator_is_done (iter)) {
		TnyHeader *cur = (TnyHeader *) tny_iterator_get_current (iter);
		tny_merge_folder_remove_msg (self, cur, err);
		/* TODO: check for err */
		tny_iterator_next (iter);
	}
	g_object_unref (iter);
}

static void
tny_merge_folder_add_msg_async (TnyFolder *self, TnyMsg *msg, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	g_warning ("tny_merge_folder_add_msg_async not implemented: "
		   "add it to the mother folder instead\n");

	if (callback) {
		GError *err = NULL;
		g_set_error (&err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_UNSUPPORTED,
			"tny_merge_folder_add_msg not implemented: add it to the mother "
			"folder instead. This problem indicates a bug in the software.");
		callback (self, TRUE, err, user_data);
		if (err)
			g_error_free (err);
	}
}

static void
tny_merge_folder_add_msg (TnyFolder *self, TnyMsg *msg, GError **err)
{
	g_warning ("tny_merge_folder_add_msg not implemented: "
		   "add it to the mother folder instead\n");

	g_set_error (err, TNY_ERROR_DOMAIN,
		TNY_SERVICE_ERROR_UNSUPPORTED,
		"tny_merge_folder_add_msg not implemented: add it to the mother "
		"folder instead. This problem indicates a bug in the software.");
}

static void
tny_merge_folder_sync (TnyFolder *self, gboolean expunge, GError **err)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	TnyIterator *iter;
	TnyList *copy;

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	iter = tny_list_create_iterator (copy);
	while (!tny_iterator_is_done (iter))
	{
		GError *new_err = NULL;
		TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));

		tny_folder_sync (cur, expunge, &new_err);
		g_object_unref (cur);

		if (new_err != NULL)
		{
			g_propagate_error (err, new_err);
			break;
		}

		tny_iterator_next (iter);
	}
	g_object_unref (iter);
	g_object_unref (copy);


	return;
}





typedef struct 
{
	TnyFolder *self;
	TnyFolderCallback callback;
	TnyStatusCallback status_callback;
	gpointer user_data;
	gboolean cancelled, expunge;
	guint depth;
	GError *err;
} SyncFolderInfo;


static void
sync_async_destroyer (gpointer thr_user_data)
{
	SyncFolderInfo *info = thr_user_data;
	TnyFolder *self = info->self;

	/* thread reference */
	g_object_unref (self);
	if (info->err)
		g_error_free (info->err);

	g_slice_free (SyncFolderInfo, thr_user_data);

	return;
}

static gboolean
sync_async_callback (gpointer thr_user_data)
{
	SyncFolderInfo *info = thr_user_data;
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (info->self);

	if (info->callback) {
		tny_lockable_lock (priv->ui_locker);
		info->callback (info->self, info->cancelled, info->err, info->user_data);
		tny_lockable_unlock (priv->ui_locker);
	}

	return FALSE;
}


static gpointer 
sync_async_thread (gpointer thr_user_data)
{
	SyncFolderInfo *info = thr_user_data;
	TnyFolder *self = info->self;
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	TnyIterator *iter;
	GError *err = NULL;
	TnyList *copy;

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	info->cancelled = FALSE;

	iter = tny_list_create_iterator (copy);
	while (!tny_iterator_is_done (iter))
	{
		TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));

		tny_folder_sync_async (cur, info->expunge, NULL, NULL, NULL);

		/* TODO: Handler err */

		/* TODO: Handle progress status callbacks ( info->status_callback )
		 * you might have to start using refresh_async for that (in a 
		 * serialized way, else you'd launch a buch of concurrent threads
		 * and ain't going to be nice, perhaps). */

		g_object_unref (cur);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (copy);

	info->err = NULL;

	if (info->callback)
	{
		if (info->depth > 0)
		{
			g_idle_add_full (G_PRIORITY_HIGH, 
				sync_async_callback, 
				info, sync_async_destroyer);
		} else {
			sync_async_callback (info);
			sync_async_destroyer (info);
		}
	} else { /* Thread reference */
		sync_async_destroyer (info);
	}

	g_thread_exit (NULL);

	return NULL;
}


static void
tny_merge_folder_sync_async (TnyFolder *self, gboolean expunge, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	SyncFolderInfo *info;
	GThread *thread;

	info = g_slice_new0 (SyncFolderInfo);
	info->err = NULL;
	info->self = self;
	info->expunge = expunge;
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;
	info->depth = g_main_depth ();

	/* thread reference */
	g_object_ref (self);

	thread = g_thread_create (sync_async_thread, info, FALSE, NULL);

	return;
}

static TnyMsgRemoveStrategy*
tny_merge_folder_get_msg_remove_strategy (TnyFolder *self)
{

	g_warning ("tny_merge_folder_get_msg_remove_strategy not implemented: "
		   "add it to the mother folder instead\n");

	return NULL;
}

static void
tny_merge_folder_set_msg_remove_strategy (TnyFolder *self, TnyMsgRemoveStrategy *st)
{

	g_warning ("tny_merge_folder_set_msg_remove_strategy not implemented: "
		   "add it to the mother folder instead\n");

	return;
}

static TnyMsgReceiveStrategy*
tny_merge_folder_get_msg_receive_strategy (TnyFolder *self)
{

	g_warning ("tny_merge_folder_get_msg_receive_strategy not implemented: "
		   "add it to the mother folder instead\n");

	return NULL;
}

static void
tny_merge_folder_set_msg_receive_strategy (TnyFolder *self, TnyMsgReceiveStrategy *st)
{

	g_warning ("tny_merge_folder_set_msg_receive_strategy not implemented: "
		   "add it to the mother folder instead\n");

	return;

}

static TnyMsg*
tny_merge_folder_get_msg (TnyFolder *self, TnyHeader *header, GError **err)
{
	TnyFolder *fol = tny_header_get_folder (header);
	TnyMsg *retval = tny_folder_get_msg (fol, header, err);

	g_object_unref (fol);

	return retval;
}

static TnyMsg*
tny_merge_folder_find_msg (TnyFolder *self, const gchar *url_string, GError **err)
{

	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	TnyIterator *iter;
	TnyMsg *retval = NULL;
	TnyList *copy;

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	iter = tny_list_create_iterator (copy);
	while (!tny_iterator_is_done (iter) && !retval)
	{
		TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));
		retval = tny_folder_find_msg (cur, url_string, NULL);
		g_object_unref (cur);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (copy);


	if (!retval)
		g_set_error (err, TNY_ERROR_DOMAIN,
				TNY_SERVICE_ERROR_NO_SUCH_MESSAGE,
				"Message not found");

	return retval;
}

/* get_msg & get_msg_async */
typedef struct 
{
	TnyFolder *self;
	TnyMsg *msg;
	TnyHeader *header;
	GError *err;
	gpointer user_data;
	guint depth;
	TnyGetMsgCallback callback;
	TnyStatusCallback status_callback;
} GetMsgInfo;

static void
get_msg_async_destroyer (gpointer thr_user_data)
{
	GetMsgInfo *info = (GetMsgInfo *) thr_user_data;

	/* thread reference */
	g_object_unref (info->self);

	if (info->msg)
		g_object_unref (info->msg);

	if (info->err)
		g_error_free (info->err);

	g_slice_free (GetMsgInfo, info);
}

static gboolean
get_msg_async_callback (gpointer thr_user_data)
{
	GetMsgInfo *info = (GetMsgInfo *) thr_user_data;
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (info->self);

	if (info->callback) { 
		/* TNY TODO: the cancelled field */
		tny_lockable_lock (priv->ui_locker);
		info->callback (info->self, FALSE, info->msg, info->err, info->user_data);
		tny_lockable_unlock (priv->ui_locker);
	}

	return FALSE;
}


static gpointer 
get_msg_async_thread (gpointer thr_user_data)
{
	GetMsgInfo *info = (GetMsgInfo *) thr_user_data;

	info->msg = tny_folder_get_msg (info->self, info->header, &info->err);

	if (info->err != NULL)
	{
		if (info->msg && G_IS_OBJECT (info->msg))
			g_object_unref (info->msg);
		info->msg = NULL;
	}

	g_object_unref (info->header);

	if (info->callback)
	{
		if (info->depth > 0)
		{
			g_idle_add_full (G_PRIORITY_HIGH, 
				get_msg_async_callback, 
				info, get_msg_async_destroyer);
		} else {
			get_msg_async_callback (info);
			get_msg_async_destroyer (info);
		}
	} else { /* thread reference */
		get_msg_async_destroyer (info);
	}

	g_thread_exit (NULL);

	return NULL;

}

static void
tny_merge_folder_get_msg_async (TnyFolder *self, TnyHeader *header, TnyGetMsgCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	GetMsgInfo *info;
	GThread *thread;

	info = g_slice_new0 (GetMsgInfo);
	info->self = self;
	info->header = header;
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;
	info->depth = g_main_depth ();
	info->err = NULL;

	/* thread reference */
	g_object_ref (info->self);
	g_object_ref (info->header);

	thread = g_thread_create (get_msg_async_thread, info, FALSE, NULL);

	return;
}


typedef struct 
{
	TnyFolder *self;
	TnyMsg *msg;
	gchar *url_string;
	GError *err;
	gpointer user_data;
	guint depth;
	TnyGetMsgCallback callback;
	TnyStatusCallback status_callback;
} FindMsgInfo;

static void
find_msg_async_destroyer (gpointer thr_user_data)
{
	FindMsgInfo *info = (FindMsgInfo *) thr_user_data;

	/* thread reference */
	g_object_unref (info->self);

	if (info->msg)
		g_object_unref (info->msg);

	if (info->url_string)
		g_free (info->url_string);

	if (info->err)
		g_error_free (info->err);

	g_slice_free (FindMsgInfo, info);
}

static gboolean
find_msg_async_callback (gpointer thr_user_data)
{
	FindMsgInfo *info = (FindMsgInfo *) thr_user_data;
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (info->self);

	if (info->callback) { 
		/* TNY TODO: the cancelled field */
		tny_lockable_lock (priv->ui_locker);
		info->callback (info->self, FALSE, info->msg, info->err, info->user_data);
		tny_lockable_unlock (priv->ui_locker);
	}

	return FALSE;
}

static gpointer 
find_msg_async_thread (gpointer thr_user_data)
{
	FindMsgInfo *info = (FindMsgInfo *) thr_user_data;

	info->msg = tny_folder_find_msg (info->self, info->url_string, &info->err);

	if (info->err != NULL)
	{
		if (info->msg && G_IS_OBJECT (info->msg))
			g_object_unref (info->msg);
		info->msg = NULL;
	}

	if (info->callback)
	{
		if (info->depth > 0)
		{
			g_idle_add_full (G_PRIORITY_HIGH, 
				find_msg_async_callback, 
				info, find_msg_async_destroyer);
		} else {
			find_msg_async_callback (info);
			find_msg_async_destroyer (info);
		}
	} else { /* thread reference */
		find_msg_async_destroyer (info);
	}

	g_thread_exit (NULL);

	return NULL;

}

static void
tny_merge_folder_find_msg_async (TnyFolder *self, const gchar *url_string, TnyGetMsgCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	FindMsgInfo *info;
	GThread *thread;

	info = g_slice_new0 (FindMsgInfo);
	info->self = self;
	info->url_string = g_strdup (url_string);
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;
	info->depth = g_main_depth ();
	info->err = NULL;

	/* thread reference */
	g_object_ref (info->self);

	thread = g_thread_create (find_msg_async_thread, info, FALSE, NULL);

	return;
}



typedef struct 
{
	TnyFolder *self;
	TnyList *headers;
	TnyGetHeadersCallback callback;
	TnyStatusCallback status_callback;
	gpointer user_data;
	gboolean cancelled, refresh;
	guint depth;
	GError *err;
} GetHeadersFolderInfo;


static void
get_headers_async_destroyer (gpointer thr_user_data)
{
	GetHeadersFolderInfo *info = thr_user_data;

	/* thread reference */
	g_object_unref (info->self);
	g_object_unref (info->headers);

	if (info->err)
		g_error_free (info->err);

	g_slice_free (GetHeadersFolderInfo, thr_user_data);

	return;
}

static gboolean
get_headers_async_callback (gpointer thr_user_data)
{
	GetHeadersFolderInfo *info = thr_user_data;
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (info->self);

	if (info->callback) {
		tny_lockable_lock (priv->ui_locker);
		info->callback (info->self, info->cancelled, info->headers, info->err, info->user_data);
		tny_lockable_unlock (priv->ui_locker);
	}

	return FALSE;
}


static gpointer 
get_headers_async_thread (gpointer thr_user_data)
{
	GetHeadersFolderInfo *info = thr_user_data;
	TnyFolder *self = info->self;
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	TnyIterator *iter;
	GError *err = NULL;
	TnyList *copy;

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	info->cancelled = FALSE;

	iter = tny_list_create_iterator (copy);
	while (!tny_iterator_is_done (iter))
	{
		TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));

		tny_folder_get_headers (cur, info->headers, info->refresh, &err);

		/* TODO: Handler err */

		/* TODO: Handle progress status callbacks ( info->status_callback )
		 * you might have to start using refresh_async for that (in a 
		 * serialized way, else you'd launch a buch of concurrent threads
		 * and ain't going to be nice, perhaps). */

		g_object_unref (cur);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (copy);

	info->err = NULL;

	if (info->callback)
	{
		if (info->depth > 0)
		{
			g_idle_add_full (G_PRIORITY_HIGH, 
				get_headers_async_callback, 
				info, get_headers_async_destroyer);
		} else {
			get_headers_async_callback (info);
			get_headers_async_destroyer (info);
		}
	} else { /* Thread reference */
		get_headers_async_destroyer (info);
	}

	g_thread_exit (NULL);

	return NULL;
}


static void
tny_merge_folder_get_headers_async (TnyFolder *self, TnyList *headers, gboolean refresh, TnyGetHeadersCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	GetHeadersFolderInfo *info;
	GThread *thread;

	info = g_slice_new0 (GetHeadersFolderInfo);
	info->err = NULL;
	info->self = self;
	info->headers = headers;
	info->refresh = refresh;
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;
	info->depth = g_main_depth ();

	/* thread reference */
	g_object_ref (self);
	g_object_ref (headers);

	thread = g_thread_create (get_headers_async_thread, info, FALSE, NULL);

	return;
}

static void
tny_merge_folder_get_headers (TnyFolder *self, TnyList *headers, gboolean refresh, GError **err)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	TnyIterator *iter;
	TnyList *copy;

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	iter = tny_list_create_iterator (copy);

	while (!tny_iterator_is_done (iter))
	{
		GError *new_err = NULL;
		TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));
		tny_folder_get_headers (cur, headers, refresh, &new_err);
		g_object_unref (cur);

		if (new_err != NULL)
		{
			g_propagate_error (err, new_err);
			break;
		}

		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (copy);

}

static const gchar*
tny_merge_folder_get_name (TnyFolder *self)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);

	return priv->name;
}

static const gchar*
tny_merge_folder_get_id (TnyFolder *self)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);

	if (!priv->id)
	{
		TnyIterator *iter;
		GString *ids = g_string_new ("");
		gboolean first = TRUE;
		TnyList *copy;

		g_static_rec_mutex_lock (priv->lock);
		copy = tny_list_copy (priv->mothers);
		g_static_rec_mutex_unlock (priv->lock);

		iter = tny_list_create_iterator (copy);

		while (!tny_iterator_is_done (iter))
		{
			TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));
			TnyAccount *account = tny_folder_get_account (cur);
			if (!first)
				g_string_append_c (ids, '&');
			if (TNY_IS_ACCOUNT (account)) {
				g_string_append (ids, tny_account_get_id (account));
				g_string_append_c (ids, '+');
			}
			g_string_append (ids, tny_folder_get_id (cur));
			g_object_unref (cur);
			if (account)
				g_object_unref (account);
			first = FALSE;
			tny_iterator_next (iter);
		}

		priv->id = ids->str;
		g_string_free (ids, FALSE);

		g_object_unref (iter);
		g_object_unref (copy);

	}

	/* The get_id_func() DBC contract does not allow this to be NULL or "": */
	if ( (priv->id == NULL) || (strlen (priv->id) == 0)) {
		priv->id = g_strdup ("unknown_mergefolder");
	}

	return priv->id;
}

static TnyAccount*
tny_merge_folder_get_account (TnyFolder *self)
{
	g_warning ("tny_merge_folder_get_account not implemented."
		   "Use the mother folders for this functionatily\n");

	return NULL;
}


/**
 * tny_merge_folder_set_folder_type
 * @self: a #TnyMergeFolder
 * @folder_type: the new folder
 * 
 * Set the folder type of the #TnyMergeFolder. The default is TNY_FOLDER_TYPE_MERGE
 * but you can change this to any folder type. It will not affect anything except
 * that tny_folder_get_folder_type() will return the new type.
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_merge_folder_set_folder_type (TnyMergeFolder* self, TnyFolderType folder_type)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	g_return_if_fail (priv != NULL);
	priv->folder_type = folder_type;
}

static TnyFolderType
tny_merge_folder_get_folder_type (TnyFolder *self)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	g_return_val_if_fail (priv != NULL, TNY_FOLDER_TYPE_MERGE);
	return priv->folder_type;
}

static guint
tny_merge_folder_get_all_count (TnyFolder *self)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	TnyIterator *iter;
	guint total = 0;
	TnyList *copy;

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	iter = tny_list_create_iterator (copy);

	while (!tny_iterator_is_done (iter))
	{
		TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));
		total += tny_folder_get_all_count (cur);
		g_object_unref (cur);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (copy);


	return total;
}

static guint
tny_merge_folder_get_unread_count (TnyFolder *self)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	TnyIterator *iter;
	guint total = 0;
	TnyList *copy;

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	iter = tny_list_create_iterator (copy);

	while (!tny_iterator_is_done (iter))
	{
		TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));
		total += tny_folder_get_unread_count (cur);
		g_object_unref (cur);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (copy);

	return total;
}


static guint
tny_merge_folder_get_local_size (TnyFolder *self)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	TnyIterator *iter;
	guint total = 0;
	TnyList *copy;

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	iter = tny_list_create_iterator (copy);

	while (!tny_iterator_is_done (iter))
	{
		TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));
		total += tny_folder_get_local_size (cur);
		g_object_unref (cur);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (copy);

	return total;
}

static gboolean
tny_merge_folder_is_subscribed (TnyFolder *self)
{
	return TRUE;
}

/* refresh & refresh_async */
static void
tny_merge_folder_refresh (TnyFolder *self, GError **err)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	TnyIterator *iter;
	TnyList *copy;

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	iter = tny_list_create_iterator (copy);
	while (!tny_iterator_is_done (iter))
	{
		GError *new_err = NULL;
		TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));
		tny_folder_refresh (cur, &new_err);
		g_object_unref (cur);

		if (new_err != NULL)
		{
			g_propagate_error (err, new_err);
			break;
		}

		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (copy);

	return;
}


typedef struct 
{
	TnyFolder *self;
	TnyFolderCallback callback;
	TnyStatusCallback status_callback;
	gpointer user_data;
	gboolean cancelled;
	guint depth;
	GError *err;
} RefreshFolderInfo;


static void
refresh_async_destroyer (gpointer thr_user_data)
{
	RefreshFolderInfo *info = thr_user_data;
	TnyFolder *self = info->self;

	/* thread reference */
	g_object_unref (self);
	if (info->err)
		g_error_free (info->err);

	g_slice_free (RefreshFolderInfo, thr_user_data);

	return;
}

static gboolean
refresh_async_callback (gpointer thr_user_data)
{
	RefreshFolderInfo *info = thr_user_data;
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (info->self);

	if (info->callback) {
		tny_lockable_lock (priv->ui_locker);
		info->callback (info->self, info->cancelled, info->err, info->user_data);
		tny_lockable_unlock (priv->ui_locker);
	}

	/* TNY TODO: trigger this change notification

	if (info->oldlen != priv->cached_length || info->oldurlen != priv->unread_length)
	{
		TnyFolderChange *change = tny_folder_change_new (self);
		if (info->oldlen != priv->cached_length)
			tny_folder_change_set_new_all_count (change, priv->cached_length);
		if (info->oldurlen != priv->unread_length)
			tny_folder_change_set_new_unread_count (change, priv->unread_length);
		notify_folder_observers_about (self, change);
		g_object_unref (change);
	} */

	return FALSE;
}


static gpointer 
refresh_async_thread (gpointer thr_user_data)
{
	RefreshFolderInfo *info = thr_user_data;
	TnyFolder *self = info->self;
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	TnyIterator *iter;
	GError *err = NULL;
	TnyList *copy;

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	info->cancelled = FALSE;

	iter = tny_list_create_iterator (copy);
	while (!tny_iterator_is_done (iter))
	{
		TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));

		tny_folder_refresh (cur, &err);

		/* TODO: Handler err */

		/* TODO: Handle progress status callbacks ( info->status_callback )
		 * you might have to start using refresh_async for that (in a 
		 * serialized way, else you'd launch a buch of concurrent threads
		 * and ain't going to be nice, perhaps). */

		g_object_unref (cur);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (copy);

	info->err = NULL;


	if (info->callback)
	{
		if (info->depth > 0)
		{
			g_idle_add_full (G_PRIORITY_HIGH, 
				refresh_async_callback, 
				info, refresh_async_destroyer);
		} else {
			refresh_async_callback (info);
			refresh_async_destroyer (info);
		}
	} else {
		refresh_async_destroyer (info);
	}

	g_thread_exit (NULL);

	return NULL;
}


static void
tny_merge_folder_refresh_async (TnyFolder *self, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	RefreshFolderInfo *info;
	GThread *thread;

	info = g_slice_new0 (RefreshFolderInfo);
	info->err = NULL;
	info->self = self;
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;
	info->depth = g_main_depth ();

	/* thread reference */
	g_object_ref (self);

	thread = g_thread_create (refresh_async_thread, info, FALSE, NULL);

	return;
}

/* transfer_msgs & transfer_msgs_async */
static void
tny_merge_folder_transfer_msgs (TnyFolder *self, TnyList *header_list, TnyFolder *folder_dst, gboolean delete_originals, GError **err)
{
	TnyIterator *iter;

	iter = tny_list_create_iterator (header_list);

	while (!tny_iterator_is_done (iter))
	{
		TnyHeader *current = TNY_HEADER (tny_iterator_get_current (iter));
		TnyFolder *folder = tny_header_get_folder (current);
		GError *new_err = NULL;
		TnyList *nlist = tny_simple_list_new ();
		tny_list_prepend (nlist, G_OBJECT (current));

		tny_folder_transfer_msgs (folder, nlist, folder_dst, delete_originals, &new_err);

		g_object_unref (nlist);
		g_object_unref (folder);
		g_object_unref (current);

		if (new_err != NULL)
		{
			g_propagate_error (err, new_err);
			break;
		}

		tny_iterator_next (iter);
	}

	g_object_unref (iter);

	return;
}

typedef struct
{
	TnyFolder *self;
	TnyList *header_list;
	TnyFolder *folder_dst;
	gboolean delete_originals;
	TnyTransferMsgsCallback callback;
	gpointer user_data;
	gint depth;
	GError *err;
} TransferMsgsInfo;


static void
transfer_msgs_async_destroyer (gpointer thr_user_data)
{
	TransferMsgsInfo *info = thr_user_data;

	/* thread reference */
	g_object_unref (info->self);
	g_object_unref (info->folder_dst);
	g_object_unref (info->header_list);

	if (info->err)
		g_error_free (info->err);

	g_slice_free (TransferMsgsInfo, thr_user_data);

	return;
}

static gboolean
transfer_msgs_async_callback (gpointer thr_user_data)
{
	TransferMsgsInfo *info = thr_user_data;
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (info->self);

	if (info->callback) {
		/* TNY TODO: the cancelled field */
		tny_lockable_lock (priv->ui_locker);
		info->callback (info->self, FALSE, info->err, info->user_data);
		tny_lockable_unlock (priv->ui_locker);
	}

	return FALSE;
}


static gpointer 
transfer_msgs_async_thread (gpointer thr_user_data)
{
	TransferMsgsInfo *info = thr_user_data;

	tny_merge_folder_transfer_msgs (info->self, info->header_list, info->folder_dst, info->delete_originals, &info->err);

	if (info->callback)
	{
		if (info->depth > 0)
		{
			g_idle_add_full (G_PRIORITY_HIGH, 
				transfer_msgs_async_callback, 
				info, transfer_msgs_async_destroyer);
		} else {
			transfer_msgs_async_callback (info);
			transfer_msgs_async_destroyer (info);
		}
	} else  { /* Thread reference */
		transfer_msgs_async_destroyer (info);
	}

	g_thread_exit (NULL);

	return NULL;
}

static void
tny_merge_folder_transfer_msgs_async (TnyFolder *self, TnyList *header_list, TnyFolder *folder_dst, gboolean delete_originals, TnyTransferMsgsCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TransferMsgsInfo *info;
	GThread *thread;

	info = g_slice_new0 (TransferMsgsInfo);
	info->err = NULL;
	info->self = self;
	info->callback = callback;
	info->header_list = header_list;
	info->user_data = user_data;
	info->depth = g_main_depth ();
	info->delete_originals = delete_originals;
	info->folder_dst = folder_dst;
	info->err = NULL;

	/* thread reference */
	g_object_ref (self);
	g_object_ref (folder_dst);
	g_object_ref (header_list);

	thread = g_thread_create (transfer_msgs_async_thread, info, FALSE, NULL);

	return;
}


/* copy */
static TnyFolder*
tny_merge_folder_copy (TnyFolder *self, TnyFolderStore *into, const gchar *new_name, gboolean del, GError **err)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	TnyIterator *iter;
	TnyFolder *nfol = NULL;
	TnyList *copy;

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	iter = tny_list_create_iterator (copy);
	while (!tny_iterator_is_done (iter))
	{
		TnyFolder *folder = TNY_FOLDER (tny_iterator_get_current (iter));

		if (!nfol)
		{
			GError *new_err = NULL;
			nfol = tny_folder_copy (folder, into, new_name, del, &new_err);

			if (new_err != NULL)
			{
				g_object_unref (folder);
				g_propagate_error (err, new_err);
				break;
			}

		} else {
			TnyList *nlist = tny_simple_list_new ();
			GError *new_err = NULL;

			tny_folder_get_headers (folder, nlist, FALSE, &new_err);
			if (new_err == NULL)
				tny_folder_transfer_msgs (folder, nlist, nfol, del, &new_err);
			g_object_unref (nlist);

			if (new_err != NULL)
			{
				g_object_unref (folder);
				g_propagate_error (err, new_err);
				break;
			}

		}


		g_object_unref (folder);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (copy);

	return nfol;
}

static void
tny_merge_folder_poke_status (TnyFolder *self)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	TnyIterator *iter;
	TnyList *copy;

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	iter = tny_list_create_iterator (copy);
	while (!tny_iterator_is_done (iter))
	{
		TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));
		tny_folder_poke_status (cur);
		g_object_unref (cur);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (copy);

	return;
}


static void 
notify_observer_del (gpointer user_data, GObject *observer)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (user_data);
	g_static_rec_mutex_lock (priv->lock);
	priv->obs = g_list_remove (priv->obs, observer);
	g_static_rec_mutex_unlock (priv->lock);
}

static void
tny_merge_folder_add_observer (TnyFolder *self, TnyFolderObserver *observer)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);

	g_static_rec_mutex_lock (priv->lock);
	if (!g_list_find (priv->obs, observer)) {
		priv->obs = g_list_prepend (priv->obs, observer);
		g_object_weak_ref (G_OBJECT (observer), notify_observer_del, self);
	}
	g_static_rec_mutex_unlock (priv->lock);

	return;
}

static void
tny_merge_folder_remove_observer (TnyFolder *self, TnyFolderObserver *observer)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	GList *found = NULL;

	g_assert (TNY_IS_FOLDER_OBSERVER (observer));

	g_static_rec_mutex_lock (priv->lock);

	if (!priv->obs) {
		g_static_rec_mutex_unlock (priv->lock);
		return;
	}

	found = g_list_find (priv->obs, observer);
	if (found) {
		priv->obs = g_list_remove_link (priv->obs, found);
		g_object_weak_unref (found->data, notify_observer_del, self);
		g_list_free (found);
	}

	g_static_rec_mutex_unlock (priv->lock);

	return;
}

static TnyFolderStore*
tny_merge_folder_get_folder_store (TnyFolder *self)
{
	g_warning ("tny_merge_folder_get_folder_store not reliable. "
		   "Please don't use this functionality\n");

	return TNY_FOLDER_STORE (g_object_ref (self));
}

static TnyFolderStats*
tny_merge_folder_get_stats (TnyFolder *self)
{
	TnyFolderStats *retval = tny_folder_stats_new (self);
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	TnyIterator *iter;
	gint total_size = 0;
	TnyList *copy;

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	iter = tny_list_create_iterator (copy);
	while (!tny_iterator_is_done (iter))
	{
		TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));
		TnyFolderStats *cstats = tny_folder_get_stats (cur);
		total_size += tny_folder_stats_get_local_size (cstats);
		g_object_unref (cstats);
		g_object_unref (cur);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (copy);

	/* TNY TODO: update unread, all_count and local_size here ! */

	tny_folder_stats_set_local_size (retval, total_size);

	return retval;
}

static gchar*
tny_merge_folder_get_url_string (TnyFolder *self)
{
	return g_strdup_printf ("merge://%s", tny_folder_get_id (self));
}

static TnyFolderCaps
tny_merge_folder_get_caps (TnyFolder *self)
{
	/* All should be off: since @self isn't yet observing its mothers 
	  push e-mail ain't working yet, and self ain't writable either: the
	  app developer is expected to use the mother folders for writing 
	  operations! */

	return 0;
}

static void 
tny_merge_folder_update (TnyFolderObserver *self, TnyFolderChange *change)
{
	/* Create a new folder change for the merge folder to propagate.
	 * The new folder change has the merge folder instead of the
	 * underlaying folder as changed folder (if someone is interested
	 * in the actual underlaying folder she should rather observe that
	 * particular folder). We also do not propagate folder renames
	 * because these do not rename the merge folder. */

	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	TnyFolderChange* new_change = tny_folder_change_new (TNY_FOLDER (self));
	TnyList *list;
	TnyIterator *iter;
	gint total = 0, unread = 0;
	TnyList *copy;
	gboolean check_duplicates;

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	iter = tny_list_create_iterator (copy);
	while (!tny_iterator_is_done (iter))
	{
		TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));
		total += tny_folder_get_all_count (cur);
		unread += tny_folder_get_unread_count (cur);
		g_object_unref (cur);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (copy);

	check_duplicates = tny_folder_change_get_check_duplicates (change);
	tny_folder_change_set_check_duplicates (new_change, check_duplicates);
	tny_folder_change_set_new_all_count (new_change, total);
	tny_folder_change_set_new_unread_count (new_change, unread);

	if (tny_folder_change_get_changed (change) & TNY_FOLDER_CHANGE_CHANGED_ADDED_HEADERS)
	{
		list = tny_simple_list_new ();
		tny_folder_change_get_added_headers (change, list);
		iter = tny_list_create_iterator (list);
		while (!tny_iterator_is_done (iter))
		{
			TnyHeader *header = TNY_HEADER (tny_iterator_get_current (iter));
			tny_folder_change_add_added_header (new_change, header);
			g_object_unref (header);
			tny_iterator_next (iter);
		}
		g_object_unref (iter);
		g_object_unref (list);
	}

	if(tny_folder_change_get_changed (change) & TNY_FOLDER_CHANGE_CHANGED_EXPUNGED_HEADERS)
	{
		list = tny_simple_list_new ();
		tny_folder_change_get_expunged_headers (change, list);
		iter = tny_list_create_iterator (list);
		while (!tny_iterator_is_done (iter))
		{
			TnyHeader *header = TNY_HEADER (tny_iterator_get_current (iter));
			tny_folder_change_add_expunged_header (new_change, header);
			g_object_unref (header);
			tny_iterator_next (iter);
		}
		g_object_unref (iter);
		g_object_unref (list);
	}

	if(tny_folder_change_get_changed (change) & TNY_FOLDER_CHANGE_CHANGED_MSG_RECEIVED)
	{
		TnyMsg *msg = tny_folder_change_get_received_msg (change);
		tny_folder_change_set_received_msg (new_change, msg);
		g_object_unref (msg);
	}

	notify_folder_observers_about (TNY_FOLDER (self), new_change);
	g_object_unref (new_change);
}

/**
 * tny_merge_folder_add_folder:
 * @self: a #TnyMergeFolder
 * @folder: a #TnyFolder 
 *
 * Add @folder to the list of folders that are merged by @self.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_merge_folder_add_folder (TnyMergeFolder *self, TnyFolder *folder)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);

	/* TODO: register @self as observer of @folder and proxy the
	 * events through to observers of @self. */

	g_static_rec_mutex_lock (priv->lock);

	tny_list_prepend (priv->mothers, G_OBJECT (folder));
	tny_folder_add_observer (folder, TNY_FOLDER_OBSERVER (self));

	g_static_rec_mutex_unlock (priv->lock);

	return;
}

/**
 * tny_merge_folder_remove_folder:
 * @self: a #TnyMergeFolder
 * @folder: a #TnyFolder 
 *
 * Removes @folder from the list of folders that are merged by
 * @self.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_merge_folder_remove_folder (TnyMergeFolder *self, TnyFolder *folder)
{
	TnyMergeFolderPriv *priv;

	g_return_if_fail (TNY_IS_MERGE_FOLDER (self));
	g_return_if_fail (TNY_IS_FOLDER (folder));

	priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);

	g_static_rec_mutex_lock (priv->lock);

	tny_folder_remove_observer (folder, TNY_FOLDER_OBSERVER (self));
	tny_list_remove (priv->mothers, G_OBJECT (folder));

	g_static_rec_mutex_unlock (priv->lock);

	return;
}

/**
 * tny_merge_folder_get_folders:
 * @self: a #TnyMergeFolder
 * @list: a #TnyList to which the folders will be prepended
 *
 * Get the folders that are merged in the merge folder @self.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_merge_folder_get_folders (TnyMergeFolder *self, TnyList *list)
{
	TnyMergeFolderPriv *priv;
	TnyIterator *iter;
	TnyList *copy;

	g_return_if_fail (TNY_IS_MERGE_FOLDER (self));
	g_return_if_fail (TNY_IS_LIST (list));

	priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	iter = tny_list_create_iterator (copy);
	while (!tny_iterator_is_done (iter)) {
		GObject *folder;

		folder = tny_iterator_get_current (iter);
		tny_list_append (list, folder);
		g_object_unref (folder);

		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (copy);

}

/**
 * tny_merge_folder_new:
 * @folder_name: the name of the merged folder
 *
 * Creates a a new TnyMergeFolder instance that can merge multiple #TnyFolder 
 * instances together (partly read only, though).
 *
 * Important consideration: if you use this type within a Gtk+ application,
 * you probably want to use tny_merge_folder_new_with_ui_locker in stead. If
 * your UI toolkid isn't thread safe (most aren't), but so-called thread aware
 * because it has a UI context lock (like Gtk+ has), then you must create this
 * type with a TnyLockable that aquires and releases this lock correctly.
 * For Gtk+ there's a default implementation called #TnyGtkLockable available
 * in libtinymailui-gtk. Please use this, else you can have threading related
 * problems in your user interface software.
 *
 * returns: (caller-owns): a new #TnyMergeFolder instance
 * since: 1.0
 * audience: application-developer
 **/
TnyFolder*
tny_merge_folder_new (const gchar *folder_name)
{
	TnyMergeFolder *self = g_object_new (TNY_TYPE_MERGE_FOLDER, NULL);
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);

	priv->name = g_strdup (folder_name);

	return TNY_FOLDER (self);
}


/**
 * tny_merge_folder_new_with_ui_locker:
 * @folder_name: the name of the merged folder
 * @ui_locker: a #TnyLockable for locking your ui
 *
 * Creates a a new TnyMergeFolder instance that can merge multiple #TnyFolder 
 * instances together (partly read only, though). Upon construction it 
 * instantly sets the ui locker. For Gtk+ you should use a #TnyGtkLockable here.
 *
 * returns: (caller-owns): a new #TnyMergeFolder instance
 * since: 1.0
 * audience: application-developer
 **/
TnyFolder*
tny_merge_folder_new_with_ui_locker (const gchar *folder_name, TnyLockable *ui_locker)
{
	TnyMergeFolder *self = TNY_MERGE_FOLDER (tny_merge_folder_new (folder_name));
	tny_merge_folder_set_ui_locker (self, ui_locker);
	return TNY_FOLDER (self);
}

/**
 * tny_merge_folder_set_ui_locker:
 * @self: a #TnyMergeFolder
 * @ui_locker: a #TnyLockable locker for locking your ui
 *
 * Sets the ui locker. For Gtk+ you should use a #TnyGtkLockable here.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_merge_folder_set_ui_locker (TnyMergeFolder *self, TnyLockable *ui_locker)
{
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	if (priv->ui_locker)
		g_object_unref (priv->ui_locker);
	priv->ui_locker = TNY_LOCKABLE (g_object_ref (ui_locker));
	return;
}


static void
tny_merge_folder_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyMergeFolder *self = (TnyMergeFolder *) instance;
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);

	priv->id = NULL;
	priv->mothers = tny_simple_list_new ();
	priv->lock = g_new0 (GStaticRecMutex, 1);
	g_static_rec_mutex_init (priv->lock);
	priv->obs = NULL;
	priv->folder_type = TNY_FOLDER_TYPE_MERGE;
	priv->ui_locker = tny_noop_lockable_new ();

	return;
}

static void
tny_merge_folder_finalize (GObject *object)
{
	TnyMergeFolder *self = (TnyMergeFolder *) object;
	TnyMergeFolderPriv *priv = TNY_MERGE_FOLDER_GET_PRIVATE (self);
	TnyIterator *iter;
	TnyList *copy;

	g_static_rec_mutex_lock (priv->lock);
	copy = tny_list_copy (priv->mothers);
	g_static_rec_mutex_unlock (priv->lock);

	iter = tny_list_create_iterator (copy);
	while (!tny_iterator_is_done (iter))
	{
		TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));
		tny_folder_remove_observer (cur, TNY_FOLDER_OBSERVER (self));
		g_object_unref (cur);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (copy);


	if (priv->obs) {
		GList *copy = priv->obs;
		while (copy) {
			g_object_weak_unref ((GObject *) copy->data, notify_observer_del, self);
			copy = g_list_next (copy);
		}
		g_list_free (priv->obs);
		priv->obs = NULL;
	}

	g_object_unref (priv->mothers);
	g_object_unref (priv->ui_locker);

	if (priv->id)
		g_free (priv->id);

	if (priv->name)
		g_free (priv->name);

	/* g_static_rec_mutex_free (priv->lock); */
	g_free (priv->lock);
	priv->lock = NULL;

	parent_class->finalize (object);
}

static void
tny_folder_observer_init (TnyFolderObserverIface *klass)
{
	klass->update= tny_merge_folder_update;
}

static void
tny_folder_init (TnyFolderIface *klass)
{
	klass->remove_msg= tny_merge_folder_remove_msg;
	klass->add_msg= tny_merge_folder_add_msg;
	klass->add_msg_async= tny_merge_folder_add_msg_async;
	klass->sync= tny_merge_folder_sync;
	klass->get_msg_remove_strategy= tny_merge_folder_get_msg_remove_strategy;
	klass->set_msg_remove_strategy= tny_merge_folder_set_msg_remove_strategy;
	klass->get_msg_receive_strategy= tny_merge_folder_get_msg_receive_strategy;
	klass->set_msg_receive_strategy= tny_merge_folder_set_msg_receive_strategy;
	klass->get_msg= tny_merge_folder_get_msg;
	klass->find_msg= tny_merge_folder_find_msg;
	klass->get_msg_async= tny_merge_folder_get_msg_async;
	klass->find_msg_async= tny_merge_folder_find_msg_async;
	klass->get_headers= tny_merge_folder_get_headers;
	klass->get_headers_async= tny_merge_folder_get_headers_async;
	klass->get_name= tny_merge_folder_get_name;
	klass->get_id= tny_merge_folder_get_id;
	klass->get_account= tny_merge_folder_get_account;
	klass->get_folder_type= tny_merge_folder_get_folder_type;
	klass->get_all_count= tny_merge_folder_get_all_count;
	klass->get_unread_count= tny_merge_folder_get_unread_count;
	klass->get_local_size= tny_merge_folder_get_local_size;
	klass->is_subscribed= tny_merge_folder_is_subscribed;
	klass->sync_async= tny_merge_folder_sync_async;
	klass->refresh_async= tny_merge_folder_refresh_async;
	klass->refresh= tny_merge_folder_refresh;
	klass->transfer_msgs= tny_merge_folder_transfer_msgs;
	klass->transfer_msgs_async= tny_merge_folder_transfer_msgs_async;
	klass->copy= tny_merge_folder_copy;
	klass->poke_status= tny_merge_folder_poke_status;
	klass->add_observer= tny_merge_folder_add_observer;
	klass->remove_observer= tny_merge_folder_remove_observer;
	klass->get_folder_store= tny_merge_folder_get_folder_store;
	klass->get_stats= tny_merge_folder_get_stats;
	klass->get_url_string= tny_merge_folder_get_url_string;
	klass->get_caps= tny_merge_folder_get_caps;
	klass->remove_msgs= tny_merge_folder_remove_msgs;
	klass->remove_msgs_async= tny_merge_folder_remove_msgs_async;
}

static void
tny_merge_folder_class_init (TnyMergeFolderClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	object_class->finalize = tny_merge_folder_finalize;

	g_type_class_add_private (object_class, sizeof (TnyMergeFolderPriv));

	return;
}


static gpointer
tny_merge_folder_register_type (gpointer notused)
{
	GType type = 0;
	static const GTypeInfo info = 
		{
			sizeof (TnyMergeFolderClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_merge_folder_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyMergeFolder),
			0,      /* n_preallocs */
			tny_merge_folder_instance_init,    /* instance_init */
			NULL
		};
	
	
	static const GInterfaceInfo tny_folder_info = 
		{
			(GInterfaceInitFunc) tny_folder_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	static const GInterfaceInfo tny_folder_observer_info = 
		{
			(GInterfaceInitFunc) tny_folder_observer_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyMergeFolder",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_FOLDER,
				     &tny_folder_info);
	
	g_type_add_interface_static (type, TNY_TYPE_FOLDER_OBSERVER,
				     &tny_folder_observer_info);
	return GUINT_TO_POINTER (type);
}

GType
tny_merge_folder_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_merge_folder_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
