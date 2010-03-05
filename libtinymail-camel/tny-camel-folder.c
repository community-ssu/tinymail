/* libtinymail-camel - The Tiny Mail base library for Camel
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

#include <config.h>

#include <glib/gi18n-lib.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>


#include <tny-status.h>
#include <tny-folder-store.h>
#include <tny-folder.h>
#include <tny-folder-stats.h>
#include <tny-camel-folder.h>
#include <tny-msg.h>
#include <tny-header.h>
#include <tny-camel-msg.h>
#include <tny-store-account.h>
#include <tny-camel-store-account.h>
#include <tny-list.h>
#include <tny-error.h>
#include <tny-folder-change.h>
#include <tny-folder-observer.h>
#include <tny-folder-store-change.h>
#include <tny-folder-store-observer.h>
#include <tny-simple-list.h>
#include <tny-merge-folder.h>
#include <tny-connection-policy.h>

#include <tny-camel-imap-folder.h>
#include <tny-camel-pop-folder.h>


#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API

#include "tny-camel-queue-priv.h"

#include <camel/camel-folder.h>
#include <camel/camel.h>
#include <camel/camel-session.h>
#include <camel/camel-store.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <tny-camel-msg-remove-strategy.h>
#include <tny-camel-full-msg-receive-strategy.h>
#include <tny-camel-partial-msg-receive-strategy.h>
#include <tny-session-camel.h>

#include "tny-camel-account-priv.h"
#include "tny-camel-store-account-priv.h"
#include "tny-camel-folder-priv.h"
#include "tny-camel-header-priv.h"
#include "tny-camel-msg-priv.h"
#include "tny-camel-common-priv.h"
#include "tny-session-camel-priv.h"
#include "tny-camel-msg-header-priv.h"

#include <tny-camel-shared.h>

#include <camel/camel-folder-summary.h>

static GObjectClass *parent_class = NULL;


static void tny_camel_folder_transfer_msgs_shared (TnyFolder *self, TnyList *headers, TnyFolder *folder_dst, gboolean delete_originals, TnyList *new_headers, GError **err);
static gboolean load_folder_no_lock (TnyCamelFolderPriv *priv);
static void folder_changed (CamelFolder *camel_folder, CamelFolderChangeInfo *info, gpointer user_data);



typedef struct { 
	GObject *self;
	GObject *change; 
	TnySessionCamel *session;
	CamelFolderSummary *summary;
} NotFolObInIdleInfo;

static void 
do_notify_in_idle_destroy (gpointer user_data)
{
	NotFolObInIdleInfo *info = (NotFolObInIdleInfo *) user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);
	gboolean remove_reference = FALSE;

	if (TNY_IS_FOLDER_CHANGE (info->change)) {
		TnyFolderChangeChanged changed = tny_folder_change_get_changed  ((TnyFolderChange *) info->change);

		/* Check if we have to remove the extra reference to
		   the summary if the change contains headers */
		if (changed & TNY_FOLDER_CHANGE_CHANGED_ADDED_HEADERS ||
		    changed & TNY_FOLDER_CHANGE_CHANGED_EXPUNGED_HEADERS) {
			remove_reference = TRUE;
		}
	}
	g_object_unref (info->change);

	/* Remove the reference *after* deleting the
	   TnyFolderChange. Otherwise the unref of the TnyCamelHeaders
	   could cause a crash as there is no summary */
	if (remove_reference)
		if (info->summary)
			camel_object_unref (info->summary);

	_tny_camel_folder_unreason (priv);
	g_object_unref (info->self);
	camel_object_unref (info->session);

	g_slice_free (NotFolObInIdleInfo, info);
}


static void 
do_notify_in_idle_destroy_for_acc (gpointer user_data)
{
	NotFolObInIdleInfo *info = (NotFolObInIdleInfo *) user_data;

	g_object_unref (info->change);
	g_object_unref (info->self);
	camel_object_unref (info->session);

	g_slice_free (NotFolObInIdleInfo, info);
}

static void
notify_folder_store_observers_about (TnyFolderStore *self, TnyFolderStoreChange *change, TnySessionCamel *session)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	GList *list, *list_iter;

	g_static_rec_mutex_lock (priv->obs_lock);
	if (!priv->sobs) {
		g_static_rec_mutex_unlock (priv->obs_lock);
		return;
	}
	list = g_list_copy (priv->sobs);
	list_iter = list;
	g_static_rec_mutex_unlock (priv->obs_lock);

	while (list_iter)
	{
		TnyFolderStoreObserver *observer = TNY_FOLDER_STORE_OBSERVER (list_iter->data);
		tny_lockable_lock (session->priv->ui_lock);
		tny_folder_store_observer_update (observer, change);
		tny_lockable_unlock (session->priv->ui_lock);
		list_iter = g_list_next (list_iter);
	}

	g_list_free (list);

	return;
}

static gboolean 
notify_folder_store_observers_about_idle (gpointer user_data)
{
	NotFolObInIdleInfo *info = (NotFolObInIdleInfo *) user_data;
	notify_folder_store_observers_about (TNY_FOLDER_STORE (info->self), 
		TNY_FOLDER_STORE_CHANGE (info->change), info->session);
	return FALSE;
}

static void
notify_folder_store_observers_about_in_idle (TnyFolderStore *self, TnyFolderStoreChange *change, TnySessionCamel *session)
{
	NotFolObInIdleInfo *info;
	TnyCamelFolderPriv *priv;

	/* This could happen as the session argument is sometimes
	   obtained from TNY_FOLDER_PRIV_GET_SESSION that could return
	   NULL */
	if (!session) {
		g_warning ("%s: session destroyed before notifying observers. Notification will be lost", __FUNCTION__);
		return;
	}

	info = g_slice_new (NotFolObInIdleInfo);
	priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	_tny_camel_folder_reason (priv);
	info->self = g_object_ref (self);
	info->change = g_object_ref (change);
	info->session = session;
	camel_object_ref (info->session);
	info->summary = NULL;

	g_idle_add_full (G_PRIORITY_HIGH, notify_folder_store_observers_about_idle,
		info, do_notify_in_idle_destroy);
}


static void
notify_folder_observers_about (TnyFolder *self, TnyFolderChange *change, TnySessionCamel *session)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	GList *list, *list_iter;

	g_static_rec_mutex_lock (priv->obs_lock);
	if (!priv->obs) {
		g_static_rec_mutex_unlock (priv->obs_lock);
		return;
	}
	list = g_list_copy (priv->obs);
	list_iter = list;
	g_static_rec_mutex_unlock (priv->obs_lock);

	while (list_iter)
	{
		TnyFolderObserver *observer = TNY_FOLDER_OBSERVER (list_iter->data);
		tny_lockable_lock (session->priv->ui_lock);
		tny_folder_observer_update (observer, change);
		tny_lockable_unlock (session->priv->ui_lock);
		list_iter = g_list_next (list_iter);
	}

	g_list_free (list);

	return;
}


static gboolean 
notify_folder_observers_about_idle (gpointer user_data)
{
	NotFolObInIdleInfo *info = (NotFolObInIdleInfo *) user_data;
	notify_folder_observers_about (TNY_FOLDER (info->self), 
		TNY_FOLDER_CHANGE (info->change), info->session);

	return FALSE;
}

static void
notify_folder_observers_about_in_idle (TnyFolder *self, TnyFolderChange *change, TnySessionCamel *session)
{
	NotFolObInIdleInfo *info;
	TnyCamelFolderPriv *priv;
	TnyFolderChangeChanged changed;

	/* This could happen as the session argument is sometimes
	   obtained from TNY_FOLDER_PRIV_GET_SESSION that could return
	   NULL */
	if (!session) {
		g_warning ("%s: session destroyed before notifying observers. Notification will be lost", __FUNCTION__);
		return;
	}

	info = g_slice_new (NotFolObInIdleInfo);
	priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	_tny_camel_folder_reason (priv);
	info->self = g_object_ref (self);
	info->change = g_object_ref (change);
	info->session = session;
	camel_object_ref (info->session);
	info->summary = NULL;

	if (TNY_IS_FOLDER_CHANGE (change)) {
		/* Increase the summary references, we have to do it, because
		   TnyFolderChange contains TnyCamelHeader instances and those
		   instances hold a pointer to the summary, and the summary
		   could be freed (if the folder is unloaded for example),
		   before the idle handler is run */
		changed = tny_folder_change_get_changed  (change);
		if (changed & TNY_FOLDER_CHANGE_CHANGED_ADDED_HEADERS ||
		    changed & TNY_FOLDER_CHANGE_CHANGED_EXPUNGED_HEADERS) {
			if (priv->folder) {
				info->summary = priv->folder->summary;
				camel_object_ref (info->summary);
			}
		}
	}

	g_idle_add_full (G_PRIORITY_HIGH, notify_folder_observers_about_idle,
		info, do_notify_in_idle_destroy);
}


static void
notify_folder_store_observers_about_for_store_acc (TnyFolderStore *self, TnyFolderStoreChange *change, TnySessionCamel *session)
{
	TnyCamelStoreAccountPriv *priv = TNY_CAMEL_STORE_ACCOUNT_GET_PRIVATE (self);
	GList *list = NULL, *list_iter;

	g_static_rec_mutex_lock (priv->obs_lock);
	if (!priv->sobs) {
		g_static_rec_mutex_unlock (priv->obs_lock);
		return;
	}
	list = g_list_copy (priv->sobs);
	list_iter = list;
	g_static_rec_mutex_unlock (priv->obs_lock);

	while (list_iter)
	{
		TnyFolderStoreObserver *observer = TNY_FOLDER_STORE_OBSERVER (list_iter->data);
		tny_lockable_lock (session->priv->ui_lock);
		tny_folder_store_observer_update (observer, change);
		tny_lockable_unlock (session->priv->ui_lock);
		list_iter = g_list_next (list_iter);
	}

	g_list_free (list);

	return;
}

static gboolean 
notify_folder_store_observers_about_for_store_acc_idle (gpointer user_data)
{
	NotFolObInIdleInfo *info = (NotFolObInIdleInfo *) user_data;
	notify_folder_store_observers_about_for_store_acc (TNY_FOLDER_STORE (info->self), 
		TNY_FOLDER_STORE_CHANGE (info->change), info->session);
	return FALSE;
}

static void
notify_folder_store_observers_about_for_store_acc_in_idle (TnyFolderStore *self, TnyFolderStoreChange *change, TnySessionCamel *session)
{
	NotFolObInIdleInfo *info = g_slice_new (NotFolObInIdleInfo);

	info->self = g_object_ref (self);
	info->change = g_object_ref (change);
	info->session = session;
	camel_object_ref (info->session);
	info->summary = NULL;

	g_idle_add_full (G_PRIORITY_HIGH, notify_folder_store_observers_about_for_store_acc_idle,
		info, do_notify_in_idle_destroy_for_acc);
}

static void 
reset_local_size (TnyCamelFolderPriv *priv)
{
	if (priv->folder)
		priv->local_size = camel_folder_get_local_size (priv->folder);
	else if (priv->store)
		priv->local_size = camel_store_get_local_size (priv->store, priv->folder_name);
}


static void 
update_iter_counts (TnyCamelFolderPriv *priv)
{
	if (priv->iter)
	{
		priv->iter->unread = priv->unread_length;
		priv->iter->total = priv->cached_length;
	}
}

void 
_tny_camel_folder_check_unread_count (TnyCamelFolder *self)
{
	TnyFolderChange *change = tny_folder_change_new (TNY_FOLDER (self));
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	priv->cached_length = camel_folder_get_message_count (priv->folder);
	priv->unread_length = camel_folder_get_unread_message_count (priv->folder);
	update_iter_counts (priv);
	tny_folder_change_set_new_unread_count (change, priv->unread_length);
	tny_folder_change_set_new_all_count (change, priv->cached_length);
	notify_folder_observers_about_in_idle (TNY_FOLDER (self), change, 
		TNY_FOLDER_PRIV_GET_SESSION (priv));
	g_object_unref (change);
}

static void 
folder_tracking_changed (CamelFolder *camel_folder, CamelFolderChangeInfo *info, gpointer user_data)
{
	TnyCamelFolder *self = user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	/* Ignore this callback if the folder is already loaded as
	   this notification will be received by folder_changed as
	   well. Checks for folder_changed_id and loaded are harmless
	   and unlikely needed */
	if (!g_static_rec_mutex_trylock (priv->folder_lock))
		return;

	if (priv->folder && priv->folder_changed_id && priv->loaded) {
		g_static_rec_mutex_unlock (priv->folder_lock);
		return;
	}

	_tny_camel_folder_reason (priv);
	if (!load_folder_no_lock (priv)) {
		_tny_camel_folder_unreason (priv);
		g_static_rec_mutex_unlock (priv->folder_lock);
		return;
	}

	folder_changed (camel_folder, info, priv);
	_tny_camel_folder_unreason (priv);
	g_static_rec_mutex_unlock (priv->folder_lock);

	return;
}

static void 
folder_changed (CamelFolder *camel_folder, CamelFolderChangeInfo *info, gpointer user_data)
{
	TnyCamelFolderPriv *priv = (TnyCamelFolderPriv *) user_data;
	TnyFolder *self = priv->self;
	TnyFolderChange *change = NULL;
	CamelFolderSummary *summary;
	gboolean old = priv->dont_fkill;
	gint i = 0; gboolean urcnted = FALSE;

	if (!priv->handle_changes)
		return;

	if (g_static_rec_mutex_trylock (priv->folder_lock))
	{
		if (!priv->folder) 
		{
			g_static_rec_mutex_unlock (priv->folder_lock);
			return;
		}

		summary = priv->folder->summary;

		if (!change && info->uid_changed != NULL && info->uid_changed->len > 0) {
			/* Commented because it seems to be the source of weird hangs */
			priv->cached_length = (guint) camel_folder_get_message_count (priv->folder);
			priv->unread_length = (guint) camel_folder_get_unread_message_count (priv->folder);
			urcnted = TRUE;
			change = tny_folder_change_new (TNY_FOLDER (self));
		}

		if (info->uid_added && info->uid_added->len > 0) 
		{
			if (!urcnted) {
				priv->cached_length = (guint) camel_folder_get_message_count (priv->folder);
				priv->unread_length = (guint) camel_folder_get_unread_message_count (priv->folder);
				urcnted = TRUE;
			}

			for (i = 0; i< info->uid_added->len; i++)
			{
				const char *uid = info->uid_added->pdata[i];

				CamelMessageInfo *minfo = camel_folder_summary_uid (summary, uid);
				if (minfo)
				{
					TnyHeader *hdr = _tny_camel_header_new ();

					if (info->push_email_event) 
						priv->cached_length++;

					if (!change)
						change = tny_folder_change_new (TNY_FOLDER (self));

					/* This adds a reason to live to self */
					_tny_camel_header_set_folder (TNY_CAMEL_HEADER (hdr), 
						TNY_CAMEL_FOLDER (self), priv);
					/* hdr will take care of the freeup*/
					_tny_camel_header_set_as_memory (TNY_CAMEL_HEADER (hdr), minfo);
					tny_folder_change_add_added_header (change, hdr);
					g_object_unref (hdr);
				}
			}
		}

		if (info->uid_removed && info->uid_removed->len > 0) 
		{
			if (!urcnted) {
				priv->cached_length = (guint) camel_folder_get_message_count (priv->folder);
				priv->unread_length = (guint) camel_folder_get_unread_message_count (priv->folder);
				urcnted = TRUE;
			}

			for (i = 0; i< info->uid_removed->len; i++)
			{
				const char *uid = info->uid_removed->pdata[i];

				CamelMessageInfo *minfo = camel_message_info_new_uid (NULL, uid);

				if (minfo) {
					TnyHeader *hdr = _tny_camel_header_new ();
					if (!change)
						change = tny_folder_change_new (self);
					/* This adds a reason to live to self */
					_tny_camel_header_set_folder (TNY_CAMEL_HEADER (hdr), 
						TNY_CAMEL_FOLDER (self), priv);
					/* hdr will take care of the freeup */
					_tny_camel_header_set_as_memory (TNY_CAMEL_HEADER (hdr), minfo);
					tny_folder_change_add_expunged_header (change, hdr);
					g_object_unref (hdr);
				}
			}
		}

		update_iter_counts (priv);

		g_static_rec_mutex_unlock (priv->folder_lock);
	} else
		g_warning ("Tinymail Oeps: Failed to lock during a notification\n");


	if (change)
	{
		tny_folder_change_set_new_unread_count (change, priv->unread_length);
		tny_folder_change_set_new_all_count (change, priv->cached_length);
		priv->dont_fkill = TRUE;
		notify_folder_observers_about_in_idle (TNY_FOLDER (self), change,
			TNY_FOLDER_PRIV_GET_SESSION (priv));
		g_object_unref (change);
		priv->dont_fkill = old;
	}

	return;
}


static void
unload_folder_no_lock (TnyCamelFolderPriv *priv, gboolean destroy)
{
	if (priv->dont_fkill)
		return;

	if (priv->folder && !CAMEL_IS_FOLDER (priv->folder))
	{
		if (CAMEL_IS_OBJECT (priv->folder))
		{
			g_critical ("Killing invalid CamelObject (should be a Camelfolder)\n");
			while (((CamelObject*)priv->folder)->ref_count >= 1)
				camel_object_unref (CAMEL_OBJECT (priv->folder));
		} else
			g_critical ("Corrupted CamelFolder instance at (I can't camel_recover from this state, therefore I will leak)\n");
	}

	if (G_LIKELY (priv->folder) && CAMEL_IS_FOLDER (priv->folder))
	{
		if (priv->folder_changed_id != 0)
			camel_object_remove_event (priv->folder, priv->folder_changed_id);

		camel_folder_set_push_email (priv->folder, FALSE);

		/* printf ("UNLOAD (%s): %d\n",
				priv->folder_name?priv->folder_name:"NUL",
				(((CamelObject*)priv->folder)->ref_count));  */

		camel_object_unref (CAMEL_OBJECT (priv->folder));
		priv->folder = NULL;
	}

	priv->folder = NULL;
	priv->loaded = FALSE;

	return;
}

static void 
unload_folder (TnyCamelFolderPriv *priv, gboolean destroy)
{
	g_static_rec_mutex_lock (priv->folder_lock);
	unload_folder_no_lock (priv, destroy);
	g_static_rec_mutex_unlock (priv->folder_lock);
}

static void
determine_push_email (TnyCamelFolderPriv *priv)
{
	g_static_rec_mutex_lock (priv->folder_lock);
	if (!priv->folder || (((CamelObject *)priv->folder)->ref_count <= 0) || !CAMEL_IS_FOLDER (priv->folder))
	{
		g_static_rec_mutex_unlock (priv->folder_lock);
		return;
	}

	if (priv->obs && g_list_length (priv->obs) > 0) {
		camel_folder_set_push_email (priv->folder, TRUE);
		priv->push = TRUE;
	} else {
		camel_folder_set_push_email (priv->folder, FALSE);
		priv->push = FALSE;
	}
	g_static_rec_mutex_unlock (priv->folder_lock);
	return;
}

static void 
do_try_on_success (CamelStore *store, TnyCamelFolderPriv *priv, CamelException *ex)
{
	if (priv->folder && !camel_exception_is_set (ex) && CAMEL_IS_FOLDER (priv->folder)) 
	{

		if (priv->folder->folder_flags & CAMEL_FOLDER_IS_READONLY)
			priv->caps &= ~TNY_FOLDER_CAPS_WRITABLE;
		else
			priv->caps |= TNY_FOLDER_CAPS_WRITABLE;

		if (priv->folder->folder_flags & CAMEL_FOLDER_HAS_PUSHEMAIL_CAPABILITY)
			priv->caps |= TNY_FOLDER_CAPS_PUSHEMAIL;
		else
			priv->caps &= ~TNY_FOLDER_CAPS_PUSHEMAIL;

		if (priv->folder_name) 
		{
			if (!priv->iter || !priv->iter->name || strcmp (priv->iter->full_name, priv->folder_name) != 0)
			{
				guint32 flags = CAMEL_STORE_FOLDER_INFO_FAST | CAMEL_STORE_FOLDER_INFO_NO_VIRTUAL |
					CAMEL_STORE_FOLDER_INFO_RECURSIVE;

				if (priv->iter && !priv->iter_parented)
					camel_folder_info_free  (priv->iter);

				priv->iter = camel_store_get_folder_info (store, priv->folder_name, flags, ex);
				priv->iter_parented = TRUE;
			}
		}
	}
}

static gboolean
load_folder_no_lock (TnyCamelFolderPriv *priv)
{
	if (priv->folder && !CAMEL_IS_FOLDER (priv->folder))
		unload_folder_no_lock (priv, FALSE);

	if (!priv->folder && !priv->loaded && priv->folder_name)
	{
		guint newlen = 0;
		CamelStore *store = priv->store;

		if (store == NULL) {
			camel_exception_set (&priv->load_ex, CAMEL_EXCEPTION_SYSTEM, 
				"No store loaded yet");
			return FALSE;
		}

		priv->load_ex.id = 0;
		priv->load_ex.desc = NULL;

		priv->folder = camel_store_get_folder 
			(store, priv->folder_name, 0, &priv->load_ex);

		if (!priv->folder || camel_exception_is_set (&priv->load_ex) || !CAMEL_IS_FOLDER (priv->folder))
		{
			/* TNY TODO: Leak it? (this is "gash" anyway) */
			priv->folder = camel_store_get_folder (store, priv->folder_name, 0, &priv->load_ex);

			if (!priv->folder || camel_exception_is_set (&priv->load_ex) || !CAMEL_IS_FOLDER (priv->folder))
			{
				/* g_critical ("Can't load folder: %s", camel_exception_get_description (&priv->load_ex)); */

				priv->folder = NULL;
				priv->loaded = FALSE;

				return FALSE;
			} else {
				do_try_on_success (store, priv, &priv->load_ex);
			}
		} else 
			do_try_on_success (store, priv, &priv->load_ex);

		determine_push_email (priv);

		if (store->flags & CAMEL_STORE_SUBSCRIPTIONS)
			priv->subscribed = 
				camel_store_folder_subscribed (store,
					camel_folder_get_full_name (priv->folder));
		else 
			priv->subscribed = TRUE;

		newlen = camel_folder_get_message_count (priv->folder);

		priv->folder_changed_id = camel_object_hook_event (priv->folder, 
			"folder_changed", (CamelObjectEventHookFunc)folder_changed, 
			priv);

		priv->has_summary_cap = camel_folder_has_summary_capability (priv->folder);

		if (G_LIKELY (priv->folder) && G_LIKELY (priv->has_summary_cap))
			priv->unread_length = (guint)
				camel_folder_get_unread_message_count (priv->folder);

		priv->loaded = TRUE;
	}
	
	return TRUE;
}

gboolean 
_tny_camel_folder_load_folder_no_lock (TnyCamelFolderPriv *priv)
{
	return load_folder_no_lock (priv);
}


static gboolean
load_folder (TnyCamelFolderPriv *priv)
{
	gboolean retval;

	g_static_rec_mutex_lock (priv->folder_lock);
	retval = load_folder_no_lock (priv);
	g_static_rec_mutex_unlock (priv->folder_lock);

	return retval;
}

void 
_tny_camel_folder_uncache_attachments (TnyCamelFolder *self, const gchar *uid)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	g_static_rec_mutex_lock (priv->folder_lock);

	if (!priv->folder || !priv->loaded || !CAMEL_IS_FOLDER (priv->folder))
		if (!load_folder_no_lock (priv))
		{
			g_static_rec_mutex_unlock (priv->folder_lock);
			return;
		}

	camel_folder_delete_attachments (priv->folder, uid);

	g_static_rec_mutex_unlock (priv->folder_lock);
}

void 
_tny_camel_folder_rewrite_cache (TnyCamelFolder *self, const gchar *uid, CamelMimeMessage *msg)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	g_static_rec_mutex_lock (priv->folder_lock);

	if (!priv->folder || !priv->loaded || !CAMEL_IS_FOLDER (priv->folder))
		if (!load_folder_no_lock (priv))
		{
			g_static_rec_mutex_unlock (priv->folder_lock);
			return;
		}

	camel_folder_rewrite_cache (priv->folder, uid, msg);

	g_static_rec_mutex_unlock (priv->folder_lock);
}

gboolean
_tny_camel_folder_get_allow_external_images (TnyCamelFolder *self, const gchar *uid)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	gboolean retval;

	g_static_rec_mutex_lock (priv->folder_lock);

	if (!priv->folder || !priv->loaded || !CAMEL_IS_FOLDER (priv->folder))
		if (!load_folder_no_lock (priv))
		{
			g_static_rec_mutex_unlock (priv->folder_lock);
			return FALSE;
		}

	retval = camel_folder_get_allow_external_images (priv->folder, uid);

	g_static_rec_mutex_unlock (priv->folder_lock);
	return retval;
}

void
_tny_camel_folder_set_allow_external_images (TnyCamelFolder *self, const gchar *uid, gboolean allow)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	gboolean retval;

	g_static_rec_mutex_lock (priv->folder_lock);

	if (!priv->folder || !priv->loaded || !CAMEL_IS_FOLDER (priv->folder))
		if (!load_folder_no_lock (priv))
		{
			g_static_rec_mutex_unlock (priv->folder_lock);
			return;
		}

	camel_folder_set_allow_external_images (priv->folder, uid, allow);

	g_static_rec_mutex_unlock (priv->folder_lock);
}

static gboolean
tny_camel_folder_add_msg_shared (TnyFolder *self, TnyMsg *msg, TnyFolderChange *change, GError **err)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	CamelMimeMessage *message;
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;
	gboolean haderr = FALSE;

	if (!load_folder_no_lock (priv)) {
		_tny_camel_exception_to_tny_error (&priv->load_ex, err);
		camel_exception_clear (&priv->load_ex);
		return FALSE;
	}

	message = _tny_camel_msg_get_camel_mime_message (TNY_CAMEL_MSG (msg));
	if (message && CAMEL_IS_MIME_MESSAGE (message)) 
	{
		gboolean first = TRUE;
		GPtrArray *dst_orig_uids = NULL;
		gint a = 0, len = 0, nlen = 0;
		CamelException ex2 = CAMEL_EXCEPTION_INITIALISER;

		len = priv->folder->summary->messages->len;
		dst_orig_uids = g_ptr_array_sized_new (len);

		for (a = 0; a < len; a++) 
		{
			CamelMessageInfo *om = 
				camel_folder_summary_index (priv->folder->summary, a);
			if (om && om->uid)
				g_ptr_array_add (dst_orig_uids, g_strdup (om->uid));
			if (om)
				camel_message_info_free (om);
		}

		camel_folder_append_message (priv->folder, message, NULL, NULL, &ex);
		priv->unread_length = camel_folder_get_unread_message_count (priv->folder);
		priv->cached_length = camel_folder_get_message_count (priv->folder);
		camel_folder_refresh_info (priv->folder, &ex2);
		nlen = priv->folder->summary->messages->len;

		for (a = 0; a < nlen; a++) 
		{
			CamelMessageInfo *om = 
				camel_folder_summary_index (priv->folder->summary, a);

			if (om && om->uid) 
			{
				gint b = 0;
				gboolean found = FALSE;

				for (b = 0; b < dst_orig_uids->len; b++) {
					/* Finding the needle ... */
					if (!strcmp (dst_orig_uids->pdata[b], om->uid)) {
						found = TRUE;
						break;
					}
				}

				if (!found) { 
					/* Jeej! a new one! */
					TnyHeader *hdr = _tny_camel_header_new ();
					if (first && msg) 
					{
						TnyHeader *header = NULL;
						_tny_camel_msg_set_folder (TNY_CAMEL_MSG (msg), self);
						header = tny_msg_get_header (msg);
						((TnyCamelMsgHeader *)header)->old_uid = g_strdup (om->uid);
						g_object_unref (header);
						first = FALSE;
					}

					/* This adds a reason to live for folder_dst */
					_tny_camel_header_set_folder (TNY_CAMEL_HEADER (hdr), 
						TNY_CAMEL_FOLDER (self), priv);
					/* hdr will take care of the freeup */
					_tny_camel_header_set_as_memory (TNY_CAMEL_HEADER (hdr), om);
					if (change)
						tny_folder_change_add_added_header (change, hdr);
					g_object_unref (hdr);
				} else /* Not-new message, freeup */
					camel_message_info_free (om);
			} else if (om) /* arg? */
				camel_message_info_free (om);
		}

		if (dst_orig_uids) {
			g_ptr_array_foreach (dst_orig_uids, (GFunc) g_free, NULL);
			g_ptr_array_free (dst_orig_uids, TRUE);
		}

	} else {
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_MIME_ERROR_MALFORMED,
			_("Malformed message"));
		haderr = TRUE;
	}

	if (camel_exception_is_set (&ex)) {
		_tny_camel_exception_to_tny_error (&ex, err);
		camel_exception_clear (&ex);
		haderr = TRUE;
	}

	return !haderr;
}

typedef struct {
	TnyCamelQueueable parent;

	TnyFolder *self;
	TnyFolderCallback callback;
	TnyStatusCallback status_callback;
	gpointer user_data;
	TnyMsg *adding_msg;
	gboolean cancelled;
	TnyIdleStopper* stopper;
	GError *err;
	TnySessionCamel *session;
	TnyFolderChange *change;

} AddMsgFolderInfo;


static void
tny_camel_folder_add_msg_async_destroyer (gpointer thr_user_data)
{
	AddMsgFolderInfo *info = thr_user_data;
	TnyFolder *self = info->self;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	if (info->adding_msg)
		g_object_unref (info->adding_msg);

	/* thread reference */
	_tny_camel_folder_unreason (priv);
	g_object_unref (self);

	if (info->err)
		g_error_free (info->err);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	/**/
	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_add_msg_async_callback (gpointer thr_user_data)
{
	AddMsgFolderInfo *info = thr_user_data;
	TnyFolder *self = info->self;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyFolderChange *change = info->change;

	/* Dot not add a ADDED_HEADERS change, because its already added */
	/* by 'folder_change' event. */
	if (change)
	{
		tny_folder_change_set_new_all_count (change, priv->cached_length);
		tny_folder_change_set_new_unread_count (change, priv->unread_length);
		notify_folder_observers_about (self, change, info->session);
		g_object_unref (change);
	}

	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, info->cancelled, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}

	tny_idle_stopper_stop (info->stopper);

	return FALSE;
}


static void
tny_camel_folder_add_msg_async_status (struct _CamelOperation *op, const char *what, int sofar, int oftotal, void *thr_user_data)
{
	AddMsgFolderInfo *oinfo = thr_user_data;
	TnyProgressInfo *info = NULL;

	info = tny_progress_info_new (G_OBJECT (oinfo->self), oinfo->status_callback, 
		TNY_FOLDER_STATUS, TNY_FOLDER_STATUS_CODE_XFER_MSGS, what, sofar, 
		oftotal, oinfo->stopper, oinfo->session->priv->ui_lock, oinfo->user_data);

	g_idle_add_full (TNY_PRIORITY_LOWER_THAN_GTK_REDRAWS,
		tny_progress_info_idle_func, info,
		tny_progress_info_destroy);

	return;
}



static gpointer 
tny_camel_folder_add_msg_async_thread (gpointer thr_user_data)
{
	AddMsgFolderInfo *info = thr_user_data;
	TnyFolder *self = info->self;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	if (!priv->account) {
		info->cancelled = TRUE;
		return NULL;
	}

	g_static_rec_mutex_lock (priv->folder_lock);

	_tny_camel_account_start_camel_operation (TNY_CAMEL_ACCOUNT (priv->account), 
		tny_camel_folder_add_msg_async_status, info, 
		"Adding message");

	info->change = tny_folder_change_new (info->self);
	tny_folder_change_set_check_duplicates (info->change, TRUE);

	tny_camel_folder_add_msg_shared (info->self, info->adding_msg, 
		info->change, &info->err);

	info->cancelled = FALSE;
	if (info->err != NULL) {
		if (camel_strstrcase (info->err->message, "cancel") != NULL)
			info->cancelled = TRUE;
	}

	priv->cached_length = camel_folder_get_message_count (priv->folder);
	priv->unread_length = (guint)camel_folder_get_unread_message_count (priv->folder);
	update_iter_counts (priv);
	reset_local_size (priv);

	_tny_camel_account_stop_camel_operation (TNY_CAMEL_ACCOUNT (priv->account));

	g_static_rec_mutex_unlock (priv->folder_lock);


	return NULL;
}

static void
tny_camel_folder_add_msg_async_cancelled_destroyer (gpointer thr_user_data)
{
	AddMsgFolderInfo *info = thr_user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);

	camel_object_unref (info->session);

	if (info->adding_msg)
		g_object_unref (info->adding_msg);

	/* thread reference */
	_tny_camel_folder_unreason (priv);
	g_object_unref (info->self);

	if (info->err)
		g_error_free (info->err);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	return;
}

static gboolean
tny_camel_folder_add_msg_async_cancelled_callback (gpointer thr_user_data)
{
	AddMsgFolderInfo *info = thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, TRUE, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}


static void 
tny_camel_folder_add_msg_async (TnyFolder *self, TnyMsg *msg, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->add_msg_async(self, msg, callback, status_callback, user_data);
	return;
}

static void 
tny_camel_folder_add_msg_async_default (TnyFolder *self, TnyMsg *msg, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{

	AddMsgFolderInfo *info;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	/* Idle info for the callbacks */
	info = g_slice_new (AddMsgFolderInfo);
	info->session = TNY_FOLDER_PRIV_GET_SESSION (priv);
	camel_object_ref (info->session);
	info->self = self;
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;
	info->adding_msg = msg;
	info->err = NULL;

	info->stopper = tny_idle_stopper_new();

	g_object_ref (info->adding_msg);

	/* thread reference */
	g_object_ref (info->self);
	_tny_camel_folder_reason (priv);

	_tny_camel_queue_launch (TNY_FOLDER_PRIV_GET_QUEUE (priv), 
		tny_camel_folder_add_msg_async_thread, 
		tny_camel_folder_add_msg_async_callback,
		tny_camel_folder_add_msg_async_destroyer, 
		tny_camel_folder_add_msg_async_cancelled_callback,
		tny_camel_folder_add_msg_async_cancelled_destroyer, 
		&info->cancelled,
		info, sizeof (AddMsgFolderInfo), __FUNCTION__);

	return;
}


static void 
tny_camel_folder_add_msg (TnyFolder *self, TnyMsg *msg, GError **err)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->add_msg(self, msg, err);
	return;
}

static void 
tny_camel_folder_add_msg_default (TnyFolder *self, TnyMsg *msg, GError **err)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyFolderChange *change = NULL;

	if (!TNY_IS_CAMEL_MSG (msg)) {
		g_critical ("You must not use a non-TnyCamelMsg implementation "
			"of TnyMsg with TnyCamelFolder types. This indicates a "
			"problem in the software (unsupported operation)\n");
		g_assert (TNY_IS_CAMEL_MSG (msg));
	}

	if (!_tny_session_check_operation (TNY_FOLDER_PRIV_GET_SESSION(priv), 
			priv->account, err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_ADD_MSG))
		return;

	if (!priv->account) {
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_ADD_MSG,
			"Folder not ready for adding messages");
		return;
	}

	g_static_rec_mutex_lock (priv->folder_lock);

	if (!priv->folder || !priv->loaded || !CAMEL_IS_FOLDER (priv->folder))
		if (!load_folder_no_lock (priv))
		{
			_tny_camel_exception_to_tny_error (&priv->load_ex, err);
			camel_exception_clear (&priv->load_ex);
			_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
			g_static_rec_mutex_unlock (priv->folder_lock);
			return;
		}

	change = tny_folder_change_new (self);
	tny_folder_change_set_check_duplicates (change, TRUE);

	if (tny_camel_folder_add_msg_shared (self, msg, change, err))
	{
		/* TNY Question: should change contain the new header? */
		tny_folder_change_set_new_all_count (change, priv->cached_length);
		tny_folder_change_set_new_unread_count (change, priv->unread_length);
		reset_local_size (priv);
		notify_folder_observers_about_in_idle (self, change,
			TNY_FOLDER_PRIV_GET_SESSION (priv));
	}

	g_object_unref (change);

	g_static_rec_mutex_unlock (priv->folder_lock);

	_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));

	return;
}


static void 
tny_camel_folder_remove_msg (TnyFolder *self, TnyHeader *header, GError **err)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->remove_msg(self, header, err);
	return;
}

static void 
tny_camel_folder_remove_msg_default (TnyFolder *self, TnyHeader *header, GError **err)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyFolderChange *change = NULL;

	g_assert (TNY_IS_HEADER (header));

	if (!_tny_session_check_operation (TNY_FOLDER_PRIV_GET_SESSION(priv), 
			priv->account, err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_REMOVE_MSG))
		return;

	if (!priv->account) {
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_REMOVE_MSG,
			_("Folder not ready for removing"));
		return;
	}

	if (!priv->remove_strat) {
		_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
		return;
	}

	g_static_rec_mutex_lock (priv->folder_lock);

	if (!priv->folder || !priv->loaded || !CAMEL_IS_FOLDER (priv->folder))
		if (!load_folder_no_lock (priv))
		{
			_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
			g_static_rec_mutex_unlock (priv->folder_lock);
			return;
		}

	tny_msg_remove_strategy_perform_remove (priv->remove_strat, self, header, err);

	/* Notify about unread count */
	_tny_camel_folder_check_unread_count (TNY_CAMEL_FOLDER (self));

	/* Reset local size info */
	reset_local_size (priv);

	/* Notify header has been removed */
	change = tny_folder_change_new (self);
	tny_folder_change_set_check_duplicates (change, TRUE);
	tny_folder_change_add_expunged_header (change, header);
	notify_folder_observers_about_in_idle (self, change,
		TNY_FOLDER_PRIV_GET_SESSION (priv));
	g_object_unref (change);

	g_static_rec_mutex_unlock (priv->folder_lock);

	_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));

	return;
}




typedef struct 
{
	TnyCamelQueueable parent;

	GError *err;
	TnyFolder *self;
	TnyList *headers;
	TnyFolderCallback callback;
	gpointer user_data;
	TnySessionCamel *session;
	gboolean cancelled;

} RemMsgsInfo;


static void
tny_camel_folder_remove_msgs_async_destroyer (gpointer thr_user_data)
{
	RemMsgsInfo *info = thr_user_data;

	/* thread reference */
	g_object_unref (info->self);
	g_object_unref (info->headers);

	if (info->err)
		g_error_free (info->err);

	/**/

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_remove_msgs_async_callback (gpointer thr_user_data)
{
	RemMsgsInfo *info = thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, info->cancelled, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}

static gpointer 
tny_camel_folder_remove_msgs_async_thread (gpointer thr_user_data)
{
	RemMsgsInfo *info = (RemMsgsInfo*) thr_user_data;
	info->err = NULL;
	tny_folder_remove_msgs (info->self, info->headers, &info->err);
	info->cancelled = FALSE;

	if (info->err != NULL) {
		if (camel_strstrcase (info->err->message, "cancel") != NULL)
			info->cancelled = TRUE;
	}

	return NULL;
}

static void
tny_camel_folder_remove_msgs_async_cancelled_destroyer (gpointer thr_user_data)
{
	RemMsgsInfo *info = thr_user_data;
	if (info->err)
		g_error_free (info->err);
	g_object_unref (info->self);
	g_object_unref (info->headers);

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_remove_msgs_async_cancelled_callback (gpointer thr_user_data)
{
	RemMsgsInfo *info = thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, TRUE, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}

static void 
tny_camel_folder_remove_msgs_async (TnyFolder *self, TnyList *headers, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->remove_msgs_async(self, headers, callback, status_callback, user_data);
	return;
}


static void 
tny_camel_folder_remove_msgs_async_default (TnyFolder *self, TnyList *headers, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	RemMsgsInfo *info;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	/* Idle info for the callbacks */
	info = g_slice_new (RemMsgsInfo);
	info->session = TNY_FOLDER_PRIV_GET_SESSION (priv);
	camel_object_ref (info->session);

	info->self = self;
	info->headers = headers;
	info->callback = callback;
	info->user_data = user_data;
	info->err = NULL;
	info->cancelled = FALSE;

	/* thread reference */
	g_object_ref (info->self);
	g_object_ref (info->headers);

	_tny_camel_queue_launch (TNY_FOLDER_PRIV_GET_QUEUE (priv), 
		tny_camel_folder_remove_msgs_async_thread, 
		tny_camel_folder_remove_msgs_async_callback,
		tny_camel_folder_remove_msgs_async_destroyer, 
		tny_camel_folder_remove_msgs_async_cancelled_callback,
		tny_camel_folder_remove_msgs_async_cancelled_destroyer, 
		&info->cancelled,
		info, sizeof (RemMsgsInfo), __FUNCTION__);

	return;
}



static void 
tny_camel_folder_remove_msgs (TnyFolder *self, TnyList *headers, GError **err)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->remove_msgs(self, headers, err);
	return;
}

static void 
tny_camel_folder_remove_msgs_default (TnyFolder *self, TnyList *headers, GError **err)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyFolderChange *change = NULL;
	TnyIterator *iter = NULL;
	TnyHeader *header = NULL;

	g_assert (TNY_IS_LIST (headers));

	if (!_tny_session_check_operation (TNY_FOLDER_PRIV_GET_SESSION(priv), 
			priv->account, err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_REMOVE_MSG))
		return;

	if (!priv->account) {
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_REMOVE_MSG,
			_("Folder not ready for removing"));
		return;
	}

	if (!priv->remove_strat) {
		_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
		return;
	}

	g_static_rec_mutex_lock (priv->folder_lock);

	if (!priv->folder || !priv->loaded || !CAMEL_IS_FOLDER (priv->folder))
		if (!load_folder_no_lock (priv))
		{
			_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
			g_static_rec_mutex_unlock (priv->folder_lock);
			return;
		}

	change = tny_folder_change_new (self);

	tny_folder_change_set_check_duplicates (change, TRUE);
	iter = tny_list_create_iterator (headers);
	while (!tny_iterator_is_done (iter)) {
		TnyFolder *folder;

		header = TNY_HEADER(tny_iterator_get_current (iter));
		folder = tny_header_get_folder (header);
		if (folder == self) {
			/* Performs remove */
			tny_msg_remove_strategy_perform_remove (priv->remove_strat, self, header, err);
			/* Add expunged headers to change event */
			tny_folder_change_add_expunged_header (change, header);
		}
		g_object_unref (header);
		g_object_unref (folder);
		tny_iterator_next (iter);
	}

	/* Notify about unread count */
	_tny_camel_folder_check_unread_count (TNY_CAMEL_FOLDER (self));
	/* Reset local size info */
	reset_local_size (priv);

	/* Notify header has been removed */
	notify_folder_observers_about_in_idle (self, change,
		TNY_FOLDER_PRIV_GET_SESSION (priv));

	/* Free */
	g_object_unref (change);
	g_object_unref (iter);

	g_static_rec_mutex_unlock (priv->folder_lock);

	_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));

	return;
}


CamelFolder*
_tny_camel_folder_get_camel_folder (TnyCamelFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	CamelFolder *retval;

	if (!priv->folder || !priv->loaded || !CAMEL_IS_FOLDER (priv->folder))
		if (!load_folder_no_lock (priv))
			return NULL;

	retval = priv->folder;

	return retval;
}


static gboolean
tny_camel_folder_is_subscribed (TnyFolder *self)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->is_subscribed(self);
}

static gboolean
tny_camel_folder_is_subscribed_default (TnyFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	gboolean retval;

	g_static_rec_mutex_lock (priv->folder_lock);

	if (!priv->folder || !priv->loaded || !CAMEL_IS_FOLDER (priv->folder))
	{
		CamelStore *store;
		CamelFolder *cfolder;

		if (!load_folder_no_lock (priv)) 
		{
			g_static_rec_mutex_unlock (priv->folder_lock);
			return FALSE;
		}

		store = priv->store;
		cfolder = _tny_camel_folder_get_camel_folder (TNY_CAMEL_FOLDER (self));
		priv->subscribed = camel_store_folder_subscribed (store, 
					camel_folder_get_full_name (cfolder));
	}

	retval = priv->subscribed;
	g_static_rec_mutex_unlock (priv->folder_lock);

	return retval;
}

void
_tny_camel_folder_set_subscribed (TnyCamelFolder *self, gboolean subscribed)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	priv->subscribed = subscribed;
	return;
}

static guint
tny_camel_folder_get_local_size (TnyFolder *self)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->get_local_size(self);
}

static guint
tny_camel_folder_get_local_size_default (TnyFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	return priv->local_size;
}

static guint
tny_camel_folder_get_unread_count (TnyFolder *self)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->get_unread_count(self);
}

static guint
tny_camel_folder_get_unread_count_default (TnyFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	if (priv->unread_length == 0 && !priv->unread_read) {
		priv->unread_read = TRUE;
		g_static_rec_mutex_lock (priv->folder_lock);
		if (priv->folder) {
			priv->unread_length = camel_folder_get_unread_message_count (priv->folder);
			update_iter_counts (priv);
		}
		g_static_rec_mutex_unlock (priv->folder_lock);
	}

	return priv->unread_length;
}

void
_tny_camel_folder_set_unread_count (TnyCamelFolder *self, guint len)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	priv->unread_length = len;
	return;
}

void
_tny_camel_folder_set_all_count (TnyCamelFolder *self, guint len)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	priv->cached_length = len;
	return;
}


void
_tny_camel_folder_set_local_size (TnyCamelFolder *self, guint len)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	priv->local_size = len;
	return;
}

static guint
tny_camel_folder_get_all_count (TnyFolder *self)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->get_all_count(self);
}

static guint
tny_camel_folder_get_all_count_default (TnyFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	return priv->cached_length;
}


static TnyAccount*  
tny_camel_folder_get_account (TnyFolder *self)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->get_account(self);
}

static TnyAccount*  
tny_camel_folder_get_account_default (TnyFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	return priv->account?TNY_ACCOUNT (g_object_ref (priv->account)):NULL;
}

#ifdef ACCOUNT_WEAK_REF
static void 
notify_account_del (gpointer user_data, GObject *parent)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (user_data);
	priv->account = NULL;
}
#endif

void
_tny_camel_folder_set_account (TnyCamelFolder *self, TnyAccount *account)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	g_assert (TNY_IS_CAMEL_ACCOUNT (account));

#ifdef ACCOUNT_WEAK_REF
	if (priv->account)
		g_object_weak_unref (G_OBJECT (priv->account), notify_account_del, self);
	g_object_weak_ref (G_OBJECT (account), notify_account_del, self);
	priv->account = account;
#else
	if (priv->account)
		g_object_unref (priv->account);
	priv->account = TNY_ACCOUNT (g_object_ref (account));
#endif

	if (priv->store) {
		camel_object_unref (priv->store);
	}
	priv->store = (CamelStore*) _tny_camel_account_get_service (TNY_CAMEL_ACCOUNT (priv->account));
	if (priv->store) {
		camel_object_ref (priv->store);
	}

	return;
}




static void 
tny_camel_folder_sync (TnyFolder *self, gboolean expunge, GError **err)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->sync(self, expunge, err);
	return;
}

static void 
tny_camel_folder_sync_default (TnyFolder *self, gboolean expunge, GError **err)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;

	if (!_tny_session_check_operation (TNY_FOLDER_PRIV_GET_SESSION(priv), 
			priv->account, err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_SYNC))
		return;

	g_static_rec_mutex_lock (priv->folder_lock);

	if (!priv->folder || !priv->loaded || !CAMEL_IS_FOLDER (priv->folder))
		if (!load_folder_no_lock (priv))
		{
			_tny_camel_exception_to_tny_error (&priv->load_ex, err);
			camel_exception_clear (&priv->load_ex);
			g_static_rec_mutex_unlock (priv->folder_lock);
			_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
			return;
		}

	camel_folder_sync (priv->folder, expunge, &ex);
	_tny_camel_folder_reason (priv);
	_tny_camel_folder_check_unread_count (TNY_CAMEL_FOLDER (self));
	reset_local_size (priv);
	_tny_camel_folder_unreason (priv);

	g_static_rec_mutex_unlock (priv->folder_lock);

	if (camel_exception_is_set (&ex)) {
		_tny_camel_exception_to_tny_error (&ex, err);
		camel_exception_clear (&ex);
	}

	_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));

	return;
}




typedef struct 
{
	TnyCamelQueueable parent;

	TnyFolder *self;
	TnyFolderCallback callback;
	TnyStatusCallback status_callback;
	gpointer user_data;
	gboolean cancelled, expunge;
	TnyIdleStopper* stopper;
	GError *err;
	TnySessionCamel *session;

} SyncFolderInfo;


static void
tny_camel_folder_sync_async_destroyer (gpointer thr_user_data)
{
	SyncFolderInfo *info = thr_user_data;
	TnyFolder *self = info->self;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	/* thread reference */
	_tny_camel_folder_unreason (priv);
	g_object_unref (self);

	if (info->err)
		g_error_free (info->err);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	/**/

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_sync_async_callback (gpointer thr_user_data)
{
	SyncFolderInfo *info = thr_user_data;
	TnyFolder *self = info->self;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyFolderChange *change = tny_folder_change_new (self);

	tny_folder_change_set_new_all_count (change, priv->cached_length);
	tny_folder_change_set_new_unread_count (change, priv->unread_length);
	notify_folder_observers_about (self, change, info->session);
	g_object_unref (change);

	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, info->cancelled, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}

	tny_idle_stopper_stop (info->stopper);

	return FALSE;
}


static void
tny_camel_folder_sync_async_status (struct _CamelOperation *op, const char *what, int sofar, int oftotal, void *thr_user_data)
{
	SyncFolderInfo *oinfo = thr_user_data;
	TnyProgressInfo *info = NULL;

	info = tny_progress_info_new (G_OBJECT (oinfo->self), oinfo->status_callback, 
		TNY_FOLDER_STATUS, TNY_FOLDER_STATUS_CODE_SYNC, what, sofar, 
		oftotal, oinfo->stopper, oinfo->session->priv->ui_lock, oinfo->user_data);

	g_idle_add_full (TNY_PRIORITY_LOWER_THAN_GTK_REDRAWS,
		tny_progress_info_idle_func, info,
		tny_progress_info_destroy);

	return;
}


static gpointer 
tny_camel_folder_sync_async_thread (gpointer thr_user_data)
{
	SyncFolderInfo *info = thr_user_data;
	TnyFolder *self = info->self;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyCamelAccountPriv *apriv = NULL;
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;

	if (!priv->account) {
		g_set_error (&info->err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_SYNC,
			"Folder not ready for synchronization");
		info->cancelled = TRUE;
		return NULL;
	}

	apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (priv->account);

	g_static_rec_mutex_lock (priv->folder_lock);

	info->cancelled = FALSE;

	_tny_camel_account_start_camel_operation (TNY_CAMEL_ACCOUNT (priv->account), 
		tny_camel_folder_sync_async_status, info, 
		"Synchronizing folder");

	if (load_folder_no_lock (priv))
	{
		priv->want_changes = FALSE;
		camel_folder_sync (priv->folder, info->expunge, &ex);
		priv->want_changes = TRUE;

		if (apriv)
			info->cancelled = camel_operation_cancel_check (apriv->cancel);
		else
			info->cancelled = FALSE;

		priv->cached_length = camel_folder_get_message_count (priv->folder);
		priv->unread_length = (guint)camel_folder_get_unread_message_count (priv->folder);
		update_iter_counts (priv);
		reset_local_size (priv);

		info->err = NULL;
		if (camel_exception_is_set (&ex)) {
			_tny_camel_exception_to_tny_error (&ex, &info->err);
			camel_exception_clear (&ex);
		}
	} else {
		_tny_camel_exception_to_tny_error (&priv->load_ex, &info->err);
		camel_exception_clear (&priv->load_ex);
	}

	_tny_camel_account_stop_camel_operation (TNY_CAMEL_ACCOUNT (priv->account));

	g_static_rec_mutex_unlock (priv->folder_lock);

	return NULL;
}

static void
tny_camel_folder_sync_async_cancelled_destroyer (gpointer thr_user_data)
{
	SyncFolderInfo *info = thr_user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);

	/* thread reference */
	_tny_camel_folder_unreason (priv);
	g_object_unref (info->self);

	if (info->err)
		g_error_free (info->err);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	/**/

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_sync_async_cancelled_callback (gpointer thr_user_data)
{
	SyncFolderInfo *info = thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, TRUE, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}

void 
tny_camel_folder_sync_async_default (TnyFolder *self, gboolean expunge, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	SyncFolderInfo *info;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	/* Idle info for the callbacks */
	info = g_slice_new (SyncFolderInfo);
	info->session = TNY_FOLDER_PRIV_GET_SESSION (priv);
	camel_object_ref (info->session);
	info->self = self;
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;
	info->expunge = expunge;
	info->err = NULL;
	info->cancelled = FALSE;
	info->stopper = tny_idle_stopper_new();

	/* thread reference */
	g_object_ref (info->self);
	_tny_camel_folder_reason (priv);

	_tny_camel_queue_launch_wflags (TNY_FOLDER_PRIV_GET_QUEUE (priv), 
		tny_camel_folder_sync_async_thread, 
		tny_camel_folder_sync_async_callback,
		tny_camel_folder_sync_async_destroyer,
		tny_camel_folder_sync_async_cancelled_callback,
		tny_camel_folder_sync_async_cancelled_destroyer, 
		&info->cancelled,
		info, sizeof (SyncFolderInfo), 
		TNY_CAMEL_QUEUE_AUTO_CANCELLABLE_ITEM|
		TNY_CAMEL_QUEUE_CANCELLABLE_ITEM|
		TNY_CAMEL_QUEUE_SYNC_ITEM, 
		__FUNCTION__);
}

void 
tny_camel_folder_sync_async (TnyFolder *self, gboolean expunge, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->sync_async(self, expunge, callback, status_callback, user_data);
	return;
}

typedef struct 
{
	TnyCamelQueueable parent;

	TnyFolder *self;
	TnyFolderCallback callback;
	TnyStatusCallback status_callback;
	gpointer user_data;
	gboolean cancelled;
	TnyIdleStopper* stopper;
	GError *err;
	TnySessionCamel *session;

} RefreshFolderInfo;


/** This is the GDestroyNotify callback provided to g_idle_add_full()
 * for tny_camel_folder_refresh_async_callback().*/

static void
tny_camel_folder_refresh_async_destroyer (gpointer thr_user_data)
{
	RefreshFolderInfo *info = thr_user_data;
	TnyFolder *self = info->self;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	/* thread reference */
	_tny_camel_folder_unreason (priv);
	g_object_unref (self);

	if (info->err)
		g_error_free (info->err);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	/**/

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_refresh_async_callback (gpointer thr_user_data)
{
	RefreshFolderInfo *info = thr_user_data;
	TnyFolder *self = info->self;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyFolderChange *change = tny_folder_change_new (self);
	TnyConnectionPolicy *constrat;

	tny_folder_change_set_new_all_count (change, priv->cached_length);
	tny_folder_change_set_new_unread_count (change, priv->unread_length);
	notify_folder_observers_about (self, change, info->session);
	g_object_unref (change);

	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, info->cancelled, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}

	if (priv->account) {
		constrat = tny_account_get_connection_policy (priv->account);
		tny_connection_policy_set_current (constrat, priv->account, self);
		g_object_unref (constrat);
	}

	tny_idle_stopper_stop (info->stopper);

	return FALSE;
}


static void
tny_camel_folder_refresh_async_status (struct _CamelOperation *op, const char *what, int sofar, int oftotal, void *thr_user_data)
{
	RefreshFolderInfo *oinfo = thr_user_data;
	TnyProgressInfo *info = NULL;

	info = tny_progress_info_new (G_OBJECT (oinfo->self), oinfo->status_callback, 
		TNY_FOLDER_STATUS, TNY_FOLDER_STATUS_CODE_REFRESH, what, sofar, 
		oftotal, oinfo->stopper, oinfo->session->priv->ui_lock, 
		oinfo->user_data);

	g_idle_add_full (TNY_PRIORITY_LOWER_THAN_GTK_REDRAWS,
		tny_progress_info_idle_func, info,
		tny_progress_info_destroy);

	return;
}


static gpointer 
tny_camel_folder_refresh_async_thread (gpointer thr_user_data)
{
	RefreshFolderInfo *info = thr_user_data;
	TnyFolder *self = info->self;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyCamelAccountPriv *apriv = NULL;
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;

	if (!priv->account) {
		info->cancelled = TRUE;
		return NULL;
	}

	apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (priv->account);

	g_static_rec_mutex_lock (priv->folder_lock);

	info->cancelled = FALSE;

	_tny_camel_account_start_camel_operation (TNY_CAMEL_ACCOUNT (priv->account), 
		tny_camel_folder_refresh_async_status, info, 
		"Fetching summary information for new messages in folder");

	if (load_folder_no_lock (priv))
	{
		priv->want_changes = FALSE;
		camel_folder_refresh_info (priv->folder, &ex);
		priv->want_changes = TRUE;

		info->cancelled = camel_operation_cancel_check (apriv->cancel);

		priv->cached_length = camel_folder_get_message_count (priv->folder);
		priv->unread_length = (guint)camel_folder_get_unread_message_count (priv->folder);
		update_iter_counts (priv);

	} else
		camel_exception_setv (&ex, CAMEL_EXCEPTION_SYSTEM, 
			"Can't load folder %s\n", 
			priv->folder_name?priv->folder_name:"(null)");

	reset_local_size (priv);

	_tny_camel_account_stop_camel_operation (TNY_CAMEL_ACCOUNT (priv->account));

	info->err = NULL;
	if (camel_exception_is_set (&ex)) {
		_tny_camel_exception_to_tny_error (&ex, &info->err);
		camel_exception_clear (&ex);
	}

	g_static_rec_mutex_unlock (priv->folder_lock);

	return NULL;
}

static void
tny_camel_folder_refresh_async (TnyFolder *self, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->refresh_async(self, callback, status_callback, user_data);
	return;
}

static void
tny_camel_folder_refresh_async_cancelled_destroyer (gpointer thr_user_data)
{
	RefreshFolderInfo *info = thr_user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);

	/* thread reference */
	_tny_camel_folder_unreason (priv);
	g_object_unref (info->self);
	if (info->err)
		g_error_free (info->err);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	/**/

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_refresh_async_cancelled_callback (gpointer thr_user_data)
{
	RefreshFolderInfo *info = thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, TRUE, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}


/**
 * tny_camel_folder_refresh_async_default:
 *
 * This is non-public API documentation
 *
 * It's actually very simple: just store all the interesting info in a struct 
 * launch a thread and keep that struct-instance around. In the callbacks,
 * which you stored as function pointers, camel_recover that info and pass it to the
 * user of the _async method.
 *
 * Important is to add and remove references. You don't want the reference to
 * become zero while doing stuff on the instance in the background, don't you?
 **/

static void
tny_camel_folder_refresh_async_default (TnyFolder *self, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	RefreshFolderInfo *info;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	/* Idle info for the callbacks */
	info = g_slice_new (RefreshFolderInfo);
	info->session = TNY_FOLDER_PRIV_GET_SESSION (priv);
	camel_object_ref (info->session);
	info->self = self;
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;
	info->err = NULL;
	info->cancelled = FALSE;
	info->stopper = tny_idle_stopper_new();

	/* thread reference */
	g_object_ref (self);
	_tny_camel_folder_reason (priv);

	_tny_camel_queue_cancel_remove_items (TNY_FOLDER_PRIV_GET_QUEUE (priv),
		TNY_CAMEL_QUEUE_AUTO_CANCELLABLE_ITEM|
		TNY_CAMEL_QUEUE_REFRESH_ITEM);

	_tny_camel_queue_launch_wflags (TNY_FOLDER_PRIV_GET_QUEUE (priv), 
		tny_camel_folder_refresh_async_thread, 
		tny_camel_folder_refresh_async_callback,
		tny_camel_folder_refresh_async_destroyer, 
		tny_camel_folder_refresh_async_cancelled_callback,
		tny_camel_folder_refresh_async_cancelled_destroyer, 
		&info->cancelled,
		info, sizeof (RefreshFolderInfo), 
		TNY_CAMEL_QUEUE_PRIORITY_ITEM|TNY_CAMEL_QUEUE_AUTO_CANCELLABLE_ITEM|
			TNY_CAMEL_QUEUE_CANCELLABLE_ITEM|TNY_CAMEL_QUEUE_REFRESH_ITEM, 
		__FUNCTION__);

	return;
}

static void 
tny_camel_folder_refresh (TnyFolder *self, GError **err)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->refresh(self, err);
	return;
}

static void 
tny_camel_folder_refresh_default (TnyFolder *self, GError **err)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;
	guint oldlen, oldurlen;
	TnyFolderChange *change = NULL;
	TnyConnectionPolicy *constrat;

	if (!_tny_session_check_operation (TNY_FOLDER_PRIV_GET_SESSION(priv), 
			priv->account, err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_REFRESH))
		return;

	if (!priv->account) {
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_REFRESH,
			_("Folder not ready for refresh"));
		return;
	}

	g_static_rec_mutex_lock (priv->folder_lock);

	if (!load_folder_no_lock (priv))
	{
		g_static_rec_mutex_unlock (priv->folder_lock);
		_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
		return;
	}

	/* We reason the folder to make sure it does not lose all the references
	 * and uncache, causing an interlock */
	_tny_camel_folder_reason (priv);

	oldlen = priv->cached_length;
	oldurlen = priv->unread_length;

	priv->want_changes = FALSE;
	camel_folder_refresh_info (priv->folder, &ex);
	priv->want_changes = TRUE;

	priv->cached_length = camel_folder_get_message_count (priv->folder);    
	if (G_LIKELY (priv->folder) && CAMEL_IS_FOLDER (priv->folder) && G_LIKELY (priv->has_summary_cap)) 
		priv->unread_length = (guint) camel_folder_get_unread_message_count (priv->folder);
	update_iter_counts (priv);

	if (camel_exception_is_set (&ex)) {
		_tny_camel_exception_to_tny_error (&ex, err);
		camel_exception_clear (&ex);
	}

	_tny_camel_folder_unreason (priv);

	g_static_rec_mutex_unlock (priv->folder_lock);

	reset_local_size (priv);

	_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));

	change = tny_folder_change_new (self);
	tny_folder_change_set_new_all_count (change, priv->cached_length);
	tny_folder_change_set_new_unread_count (change, priv->unread_length);
	notify_folder_observers_about_in_idle (self, change,
		TNY_FOLDER_PRIV_GET_SESSION (priv));
	g_object_unref (change);

	if (priv->account) {
		constrat = tny_account_get_connection_policy (priv->account);
		tny_connection_policy_set_current (constrat, priv->account, self);
		g_object_unref (constrat);
	}

	return;
}



typedef struct 
{ 	/* This is a speedup trick */
	TnyFolder *self;
	TnyCamelFolderPriv *priv;
	TnyList *headers;
} FldAndPriv;

static void
add_message_with_uid (gpointer data, gpointer user_data)
{
	TnyHeader *header = NULL;
	FldAndPriv *ptr = user_data;
	CamelMessageInfo *mi = (CamelMessageInfo *) data;

	/* Unpack speedup trick */
	TnyFolder *self = ptr->self;
	TnyCamelFolderPriv *priv = ptr->priv;
	TnyList *headers = ptr->headers;

	/* TODO: Proxy instantiation (happens a lot, could use a pool) */

	header = _tny_camel_header_new ();
	_tny_camel_header_set_folder ((TnyCamelHeader *) header, (TnyCamelFolder *) self, priv);
	_tny_camel_header_set_camel_message_info ((TnyCamelHeader *) header, mi, FALSE);
	tny_list_prepend (headers, (GObject*) header);
	g_object_unref (header);

	return;
}



typedef struct 
{
	TnyCamelQueueable parent;

	GError *err;
	TnyFolder *self;
	TnyList *headers;
	gboolean refresh;
	TnyGetHeadersCallback callback;
	TnyStatusCallback status_callback;
	gpointer user_data;
	TnySessionCamel *session;
	gboolean cancelled;
	TnyIdleStopper *stopper;

} GetHeadersInfo;


static void
tny_camel_folder_get_headers_async_destroyer (gpointer thr_user_data)
{
	GetHeadersInfo *info = thr_user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);

	/* thread reference */
	_tny_camel_folder_unreason (priv);
	g_object_unref (info->self);
	g_object_unref (info->headers);

	if (info->err)
		g_error_free (info->err);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	/**/

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_get_headers_async_callback (gpointer thr_user_data)
{
	GetHeadersInfo *info = thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, info->cancelled, info->headers, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	tny_idle_stopper_stop (info->stopper);

	return FALSE;
}

static void
tny_camel_folder_get_headers_async_status (struct _CamelOperation *op, const char *what, int sofar, int oftotal, void *thr_user_data)
{
	GetHeadersInfo *oinfo = thr_user_data;
	TnyProgressInfo *info = NULL;

	info = tny_progress_info_new (G_OBJECT (oinfo->self), oinfo->status_callback, 
				      TNY_FOLDER_STATUS, TNY_FOLDER_STATUS_CODE_GET_MSG, what, sofar, 
				      oftotal, oinfo->stopper, oinfo->session->priv->ui_lock, oinfo->user_data);
	
	g_idle_add_full (TNY_PRIORITY_LOWER_THAN_GTK_REDRAWS, 
			 tny_progress_info_idle_func, info, 
			 tny_progress_info_destroy);
	
	return;
}


static gpointer 
tny_camel_folder_get_headers_async_thread (gpointer thr_user_data)
{
	GetHeadersInfo *info = (GetHeadersInfo*) thr_user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);

	info->err = NULL;
	info->cancelled = FALSE;

	g_static_rec_mutex_lock (priv->folder_lock);

	_tny_camel_account_start_camel_operation (TNY_CAMEL_ACCOUNT (priv->account),
						  tny_camel_folder_get_headers_async_status, 
						  info, "Getting headers");

	tny_folder_get_headers (info->self, info->headers, info->refresh, &info->err);

	_tny_camel_account_stop_camel_operation (TNY_CAMEL_ACCOUNT (priv->account));

	if (info->err != NULL) {
		if (camel_strstrcase (info->err->message, "cancel") != NULL)
			info->cancelled = TRUE;
	}

	g_static_rec_mutex_unlock (priv->folder_lock);

	return NULL;
}

static void
tny_camel_folder_get_headers_async_cancelled_destroyer (gpointer thr_user_data)
{
	GetHeadersInfo *info = thr_user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);

	if (info->err)
		g_error_free (info->err);
	_tny_camel_folder_unreason (priv);
	g_object_unref (info->self);
	g_object_unref (info->headers);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_get_headers_async_cancelled_callback (gpointer thr_user_data)
{
	GetHeadersInfo *info = thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, TRUE, info->headers, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}

static void 
tny_camel_folder_get_headers_async (TnyFolder *self, TnyList *headers, gboolean refresh, TnyGetHeadersCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->get_headers_async(self, headers, refresh, callback, status_callback, user_data);
	return;
}


static void 
tny_camel_folder_get_headers_async_default (TnyFolder *self, TnyList *headers, gboolean refresh, TnyGetHeadersCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	GetHeadersInfo *info;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	
	/* Idle info for the callbacks */
	info = g_slice_new (GetHeadersInfo);
	info->session = TNY_FOLDER_PRIV_GET_SESSION (priv);
	camel_object_ref (info->session);
	info->self = self;
	info->headers = headers;
	info->refresh = refresh;
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;
	info->err = NULL;
	info->cancelled = FALSE;
	info->stopper = tny_idle_stopper_new();

	/* thread reference */
	g_object_ref (info->self);
	g_object_ref (info->headers);

	_tny_camel_folder_reason (priv);
	_tny_camel_queue_cancel_remove_items (TNY_FOLDER_PRIV_GET_QUEUE (priv), 
		TNY_CAMEL_QUEUE_GET_HEADERS_ITEM|TNY_CAMEL_QUEUE_SYNC_ITEM|
		TNY_CAMEL_QUEUE_REFRESH_ITEM);

	_tny_camel_queue_launch_wflags (TNY_FOLDER_PRIV_GET_QUEUE (priv), 
		tny_camel_folder_get_headers_async_thread, 
		tny_camel_folder_get_headers_async_callback,
		tny_camel_folder_get_headers_async_destroyer, 
		tny_camel_folder_get_headers_async_cancelled_callback,
		tny_camel_folder_get_headers_async_cancelled_destroyer, 
		&info->cancelled,
		info, sizeof (GetHeadersInfo),
		TNY_CAMEL_QUEUE_PRIORITY_ITEM/*|TNY_CAMEL_QUEUE_CANCELLABLE_ITEM*/|
		TNY_CAMEL_QUEUE_GET_HEADERS_ITEM, 
		__FUNCTION__);

	return;
}

static void
tny_camel_folder_get_headers (TnyFolder *self, TnyList *headers, gboolean refresh, GError **err)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->get_headers(self, headers, refresh, err);
	return;
}

static void
tny_camel_folder_get_headers_default (TnyFolder *self, TnyList *headers, gboolean refresh, GError **err)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;
	FldAndPriv *ptr = NULL;

	g_assert (TNY_IS_LIST (headers));

	if (!_tny_session_check_operation (TNY_FOLDER_PRIV_GET_SESSION(priv), priv->account, err, 
			TNY_ERROR_DOMAIN, TNY_SERVICE_ERROR_REFRESH))
		return;

	if (!priv->account) {
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_REFRESH,
			_("Folder not ready for getting headers"));
		return;
	}

	g_static_rec_mutex_lock (priv->folder_lock);

	if (!load_folder_no_lock (priv))
	{
		_tny_camel_exception_to_tny_error (&priv->load_ex, err);
		camel_exception_clear (&priv->load_ex);
		g_static_rec_mutex_unlock (priv->folder_lock);
		_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
		return;
	}

	/* We reason the folder to make sure it does not lose all the references
	 * and uncache, causing an interlock */
	_tny_camel_folder_reason (priv);

	g_object_ref (headers);
	ptr = g_slice_new (FldAndPriv);
	ptr->self = self;
	ptr->priv = priv;
	ptr->headers = headers;

	if (refresh && priv->folder && CAMEL_IS_FOLDER (priv->folder))
	{
		priv->want_changes = FALSE;
		camel_folder_refresh_info (priv->folder, &ex);
		priv->want_changes = TRUE;

		if (camel_exception_is_set (&ex)) {
			_tny_camel_exception_to_tny_error (&ex, err);
			camel_exception_clear (&ex);
		}

	}

	if (priv->folder && CAMEL_IS_FOLDER (priv->folder)) {
		GPtrArray *array;;

		array = priv->folder->summary->messages;
		if (array->len > 0) {
			guint i;
			for (i = array->len - 1; i < array->len ; i--) {
				add_message_with_uid (array->pdata[i], ptr);
			}
		}
	}

	g_slice_free (FldAndPriv, ptr);

	g_object_unref (headers);

	_tny_camel_folder_unreason (priv);
	g_static_rec_mutex_unlock (priv->folder_lock);

	reset_local_size (priv);

	_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));

	return;
}



typedef struct 
{
	TnyCamelQueueable parent;

	TnyFolder *self;
	TnyMsg *msg;
	TnyHeader *header;
	GError *err;
	gpointer user_data;
	TnyGetMsgCallback callback;
	TnyStatusCallback status_callback;
	TnySessionCamel *session;
	TnyIdleStopper *stopper;
	gboolean cancelled;

} GetMsgInfo;


static void
tny_camel_folder_get_msg_async_destroyer (gpointer thr_user_data)
{
	GetMsgInfo *info = (GetMsgInfo *) thr_user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);

	/* thread reference */
	_tny_camel_folder_unreason (priv);
	g_object_unref (info->self);

	if (info->err)
		g_error_free (info->err);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	/**/

	camel_object_unref (info->session);

	return;
}


static gboolean
tny_camel_folder_get_msg_async_callback (gpointer thr_user_data)
{
	GetMsgInfo *info = (GetMsgInfo *) thr_user_data;
	TnyFolderChange *change;

	if (info->msg) 
	{
		change = tny_folder_change_new (info->self);
		tny_folder_change_set_check_duplicates (change, TRUE);
		tny_folder_change_set_received_msg (change, info->msg);
		notify_folder_observers_about (info->self, change, info->session);
		g_object_unref (change);
	}

	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, info->cancelled, info->msg, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}

	if (info->msg)
		g_object_unref (info->msg);

	tny_idle_stopper_stop (info->stopper);

	return FALSE;
}


static void
tny_camel_folder_get_msg_async_status (struct _CamelOperation *op, const char *what, int sofar, int oftotal, void *thr_user_data)
{
	GetMsgInfo *oinfo = thr_user_data;
	TnyProgressInfo *info = NULL;

	info = tny_progress_info_new (G_OBJECT (oinfo->self), oinfo->status_callback, 
		TNY_FOLDER_STATUS, TNY_FOLDER_STATUS_CODE_GET_MSG, what, sofar, 
		oftotal, oinfo->stopper, oinfo->session->priv->ui_lock, oinfo->user_data);

	g_idle_add_full (TNY_PRIORITY_LOWER_THAN_GTK_REDRAWS, 
			  tny_progress_info_idle_func, info, 
			  tny_progress_info_destroy);

	return;
}

static gpointer 
tny_camel_folder_get_msg_async_thread (gpointer thr_user_data)
{
	GetMsgInfo *info = (GetMsgInfo *) thr_user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);
	CamelOperation *cancel;

	/* TNY TODO: Ensure load_folder here */

	/* This one doesn't use the _tny_camel_account_start_camel_operation 
	 * infrastructure because it doesn't need to cancel existing operations
	 * due to a patch to camel-lite allowing messages to be fetched while
	 * other operations are happening */

	cancel = camel_operation_new (tny_camel_folder_get_msg_async_status, info);

	if (priv->account && TNY_IS_CAMEL_ACCOUNT (priv->account)) {
		TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (priv->account);
		apriv->getmsg_cancel = cancel;
		camel_operation_ref (cancel);
	}

	/* To disable parallel getting of messages while summary is being retreived,
	 * restore this lock (A) */
	/* g_static_rec_mutex_lock (priv->folder_lock); */

	camel_operation_register (cancel);
	camel_operation_start (cancel, (char *) "Getting message");

	info->cancelled = FALSE;

	info->msg = tny_msg_receive_strategy_perform_get_msg (priv->receive_strat, 
			info->self, info->header, &info->err);

	reset_local_size (priv);

	info->cancelled = camel_operation_cancel_check (cancel);

	camel_operation_unregister (cancel);
	camel_operation_end (cancel);
	camel_operation_unref (cancel);

	if (priv->account && TNY_IS_CAMEL_ACCOUNT (priv->account)) {
		TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (priv->account);
		apriv->getmsg_cancel = NULL;
		camel_operation_unref (cancel);
	}

	/* To disable parallel getting of messages while summary is being retreived,
	 * restore this lock (B) */
	/* g_static_rec_mutex_unlock (priv->folder_lock);  */

	if (info->err != NULL) {
		if (camel_strstrcase (info->err->message, "cancel") != NULL)
			info->cancelled = TRUE;
		if (info->msg && G_IS_OBJECT (info->msg))
			g_object_unref (info->msg);
		info->msg = NULL;
	}

	/* thread reference header */
	g_object_unref (info->header);

	return NULL;

}

static void
tny_camel_folder_get_msg_async_cancelled_destroyer (gpointer thr_user_data)
{
	GetMsgInfo *info = (GetMsgInfo *) thr_user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);

	/* thread reference */
	_tny_camel_folder_unreason (priv);
	g_object_unref (info->self);

	if (info->err)
		g_error_free (info->err);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	/**/

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_get_msg_async_cancelled_callback (gpointer thr_user_data)
{
	GetMsgInfo *info = (GetMsgInfo *) thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, TRUE, NULL, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}

static void
tny_camel_folder_get_msg_async (TnyFolder *self, TnyHeader *header, TnyGetMsgCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->get_msg_async(self, header, callback, status_callback, user_data);
	return;
}


static void
tny_camel_folder_get_msg_async_default (TnyFolder *self, TnyHeader *header, TnyGetMsgCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	GetMsgInfo *info;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyCamelQueue *queue;

	/* Idle info for the callbacks */
	info = g_slice_new (GetMsgInfo);
	info->cancelled = FALSE;
	info->session = TNY_FOLDER_PRIV_GET_SESSION (priv);
	camel_object_ref (info->session);
	info->self = self;
	info->header = header;
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;
	info->err = NULL;

	info->stopper = tny_idle_stopper_new();

	/* thread reference */
	_tny_camel_folder_reason (priv);
	g_object_ref (info->self);

	/* thread reference header */
	g_object_ref (info->header);

	if (!TNY_IS_CAMEL_POP_FOLDER (self) && 
	    !_tny_camel_queue_has_items (TNY_FOLDER_PRIV_GET_QUEUE (priv), 
					 TNY_CAMEL_QUEUE_RECONNECT_ITEM | TNY_CAMEL_QUEUE_CONNECT_ITEM))
		queue = TNY_FOLDER_PRIV_GET_MSG_QUEUE (priv);
	else
		queue = TNY_FOLDER_PRIV_GET_QUEUE (priv);

	_tny_camel_queue_launch (queue, 
		tny_camel_folder_get_msg_async_thread, 
		tny_camel_folder_get_msg_async_callback,
		tny_camel_folder_get_msg_async_destroyer, 
		tny_camel_folder_get_msg_async_cancelled_callback,
		tny_camel_folder_get_msg_async_cancelled_destroyer, 
		&info->cancelled,
		info, sizeof (GetMsgInfo), 
		__FUNCTION__);

	return;
}


typedef struct 
{
	TnyCamelQueueable parent;

	TnyFolder *self;
	TnyMsg *msg;
	gchar *url_string;
	GError *err;
	gpointer user_data;
	TnyGetMsgCallback callback;
	TnyStatusCallback status_callback;
	TnySessionCamel *session;
	TnyIdleStopper *stopper;
	gboolean cancelled;

} FindMsgInfo;

static void
tny_camel_folder_find_msg_async_destroyer (gpointer thr_user_data)
{
	FindMsgInfo *info = (FindMsgInfo *) thr_user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);

	/* thread reference */
	_tny_camel_folder_unreason (priv);
	g_object_unref (info->self);

	if (info->err)
		g_error_free (info->err);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	/**/

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_find_msg_async_callback (gpointer thr_user_data)
{
	FindMsgInfo *info = (FindMsgInfo *) thr_user_data;
	TnyFolderChange *change;

	if (info->msg) 
	{
		change = tny_folder_change_new (info->self);
		tny_folder_change_set_check_duplicates (change, TRUE);
		tny_folder_change_set_received_msg (change, info->msg);
		notify_folder_observers_about (info->self, change, info->session);
		g_object_unref (change);
	}

	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, info->cancelled, info->msg, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}

	if (info->msg)
		g_object_unref (info->msg);

	tny_idle_stopper_stop (info->stopper);

	return FALSE;
}


static void
tny_camel_folder_find_msg_async_cancelled_destroyer (gpointer thr_user_data)
{
	FindMsgInfo *info = (FindMsgInfo *) thr_user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);

	/* thread reference */
	_tny_camel_folder_unreason (priv);
	g_object_unref (info->self);

	if (info->err)
		g_error_free (info->err);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	/**/

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_find_msg_async_cancelled_callback (gpointer thr_user_data)
{
	FindMsgInfo *info = (FindMsgInfo *) thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, TRUE, NULL, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}

static gpointer 
tny_camel_folder_find_msg_async_thread (gpointer thr_user_data)
{
	FindMsgInfo *info = (FindMsgInfo *) thr_user_data;

	info->msg = tny_folder_find_msg (info->self, info->url_string, &info->err);

	info->cancelled = FALSE;
	if (info->err != NULL) {
		if (camel_strstrcase (info->err->message, "cancel") != NULL)
			info->cancelled = TRUE;
	}

	/* Free the URL */
	g_free (info->url_string);

	return NULL;
}

static void
tny_camel_folder_find_msg_async (TnyFolder *self, const gchar *url_string, TnyGetMsgCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->find_msg_async(self, url_string, callback, status_callback, user_data);
	return;
}


static void
tny_camel_folder_find_msg_async_default (TnyFolder *self, const gchar *url_string, TnyGetMsgCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	FindMsgInfo *info;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyCamelQueue *queue;

	/* Idle info for the callbacks */
	info = g_slice_new (FindMsgInfo);
	info->cancelled = FALSE;
	info->session = TNY_FOLDER_PRIV_GET_SESSION (priv);
	camel_object_ref (info->session);
	info->self = self;
	info->url_string = g_strdup (url_string);
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;
	info->err = NULL;

	info->stopper = tny_idle_stopper_new();

	/* thread reference */
	_tny_camel_folder_reason (priv);
	g_object_ref (info->self);

	if (!TNY_IS_CAMEL_POP_FOLDER (self) && 
	    !_tny_camel_queue_has_items (TNY_FOLDER_PRIV_GET_QUEUE (priv), 
					 TNY_CAMEL_QUEUE_RECONNECT_ITEM | TNY_CAMEL_QUEUE_CONNECT_ITEM))
		queue = TNY_FOLDER_PRIV_GET_MSG_QUEUE (priv);
	else
		queue = TNY_FOLDER_PRIV_GET_QUEUE (priv);

	_tny_camel_queue_launch (queue, 
		tny_camel_folder_find_msg_async_thread, 
		tny_camel_folder_find_msg_async_callback,
		tny_camel_folder_find_msg_async_destroyer, 
		tny_camel_folder_find_msg_async_cancelled_callback,
		tny_camel_folder_find_msg_async_cancelled_destroyer, 
		&info->cancelled,
		info, sizeof (FindMsgInfo), 
		__FUNCTION__);

	return;
}

static TnyMsgReceiveStrategy* 
tny_camel_folder_get_msg_receive_strategy (TnyFolder *self)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->get_msg_receive_strategy(self);
}

static TnyMsgReceiveStrategy* 
tny_camel_folder_get_msg_receive_strategy_default (TnyFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	return TNY_MSG_RECEIVE_STRATEGY (g_object_ref (G_OBJECT (priv->receive_strat)));
}

static void 
tny_camel_folder_set_msg_receive_strategy (TnyFolder *self, TnyMsgReceiveStrategy *st)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->set_msg_receive_strategy(self, st);
	return;
}

static void 
tny_camel_folder_set_msg_receive_strategy_default (TnyFolder *self, TnyMsgReceiveStrategy *st)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	if (priv->receive_strat)
		g_object_unref (G_OBJECT (priv->receive_strat));

	priv->receive_strat = TNY_MSG_RECEIVE_STRATEGY (g_object_ref (G_OBJECT (st)));

	return;
}

static TnyMsg*
tny_camel_folder_get_msg (TnyFolder *self, TnyHeader *header, GError **err)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->get_msg(self, header, err);
}

static TnyMsg*
tny_camel_folder_get_msg_default (TnyFolder *self, TnyHeader *header, GError **err)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyMsg *retval = NULL;
	TnyFolderChange *change;

	if (!_tny_session_check_operation (TNY_FOLDER_PRIV_GET_SESSION(priv), 
			priv->account, err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_GET_MSG))
		return NULL;

	if (!priv->account) {
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_GET_MSG,
			_("Folder not ready for getting messages"));
		return NULL;
	}

	g_assert (TNY_IS_HEADER (header));

	if (!priv->receive_strat) {
		_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
		return NULL;
	}

	g_static_rec_mutex_lock (priv->folder_lock);

	if (!priv->folder || !priv->loaded || !CAMEL_IS_FOLDER (priv->folder))
		if (!load_folder_no_lock (priv))
		{
			_tny_camel_exception_to_tny_error (&priv->load_ex, err);
			camel_exception_clear (&priv->load_ex);
			g_static_rec_mutex_unlock (priv->folder_lock);
			_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
			return NULL;
		}

	retval = tny_msg_receive_strategy_perform_get_msg (priv->receive_strat, self, header, err);

	reset_local_size (priv);

	if (retval)
	{
		change = tny_folder_change_new (self);
		tny_folder_change_set_check_duplicates (change, TRUE);
		tny_folder_change_set_received_msg (change, retval);
		notify_folder_observers_about_in_idle (self, change,
			TNY_FOLDER_PRIV_GET_SESSION (priv));
		g_object_unref (change);
	}

	g_static_rec_mutex_unlock (priv->folder_lock);

	_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));

	return retval;
}




static TnyMsg*
tny_camel_folder_find_msg (TnyFolder *self, const gchar *url_string, GError **err)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->find_msg(self, url_string, err);
}

static TnyMsg*
tny_camel_folder_find_msg_default (TnyFolder *self, const gchar *url_string, GError **err)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyMsg *retval = NULL;
	TnyHeader *hdr;
	CamelMessageInfo *info;
	const gchar *uid;

	if (!_tny_session_check_operation (TNY_FOLDER_PRIV_GET_SESSION(priv), 
			priv->account, err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_GET_MSG))
		return NULL;

	if (!priv->account) {
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_GET_MSG,
			_("Folder not ready for finding messages"));
		return NULL;
	}

	if (!priv->receive_strat) {
		_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
		return NULL;
	}

	g_static_rec_mutex_lock (priv->folder_lock);

	if (!priv->folder || !priv->loaded || !CAMEL_IS_FOLDER (priv->folder))
		if (!load_folder_no_lock (priv))
		{
			_tny_camel_exception_to_tny_error (&priv->load_ex, err);
			camel_exception_clear (&priv->load_ex);
			g_static_rec_mutex_unlock (priv->folder_lock);
			_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
			return NULL;
		}

	uid = strrchr (url_string, '/');

	/* Skip over the '/': */
	if (uid && strlen (uid))
		++uid;
	
	if (uid && uid[0] != '/' && strlen (uid) > 0)
	{
		TnyHeader *nhdr;
		info = camel_folder_get_message_info (priv->folder, uid);
		if (info == NULL) {
			g_set_error (err, TNY_ERROR_DOMAIN, TNY_SERVICE_ERROR_NO_SUCH_MESSAGE,
				     _("Message uid not found in folder"));
			retval = NULL;
		} else {
			hdr = _tny_camel_header_new ();
			_tny_camel_header_set_folder ((TnyCamelHeader *) hdr, (TnyCamelFolder *) self, priv);
			_tny_camel_header_set_camel_message_info ((TnyCamelHeader *) hdr, info, FALSE);
			
			retval = tny_msg_receive_strategy_perform_get_msg (priv->receive_strat, self, hdr, err);
			if (retval) {
				nhdr = tny_msg_get_header (retval);
				/* This trick is for forcing owning a TnyCamelHeader reference */
				if (hdr != nhdr && TNY_IS_CAMEL_MSG_HEADER (nhdr)) {
					_tny_camel_msg_header_set_decorated ((TnyCamelMsgHeader *) nhdr, hdr, TRUE);
				}
				g_object_unref (nhdr);
			}
			g_object_unref (hdr);
			reset_local_size (priv);
		}

	} else {
		g_set_error (err, TNY_ERROR_DOMAIN,
				TNY_SERVICE_ERROR_GET_MSG,
				_("This url string is malformed"));
		retval = NULL;
	}

	g_static_rec_mutex_unlock (priv->folder_lock);

	_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));

	return retval;
}

static const gchar*
tny_camel_folder_get_name (TnyFolder *self)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->get_name(self);
}

static const gchar*
tny_camel_folder_get_name_default (TnyFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	const gchar *name = NULL;

	if (G_UNLIKELY (!priv->cached_name))
	{
		if (!load_folder (priv))
			return NULL;
		name = camel_folder_get_name (priv->folder);
	} else
		name = priv->cached_name;

	return name;
}

static TnyFolderType
tny_camel_folder_get_folder_type (TnyFolder *self)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->get_folder_type(self);
}

static TnyFolderType
tny_camel_folder_get_folder_type_default (TnyFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	return priv->cached_folder_type;
}

static const gchar*
tny_camel_folder_get_id (TnyFolder *self)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->get_id(self);
}

static const gchar*
tny_camel_folder_get_id_default (TnyFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	return priv->folder_name;
}



static TnyFolder*
tny_camel_folder_copy (TnyFolder *self, TnyFolderStore *into, const gchar *new_name, gboolean del, GError **err)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->copy(self, into, new_name, del, err);
}

typedef struct {
	GList *rems;
	GList *adds;
	TnyFolder *created;
	gboolean observers_ready;
} CpyRecRet;

typedef struct {
	TnyFolderStore *str;
	TnyFolder *fol;
} CpyEvent;

static CpyEvent*
cpy_event_new (TnyFolderStore *str, TnyFolder *fol)
{
	CpyEvent *e = g_slice_new (CpyEvent);
	e->str = (TnyFolderStore *) g_object_ref (str);
	e->fol = (TnyFolder *) g_object_ref (fol);
	return e;
}

static void
cpy_event_free (CpyEvent *e)
{
	g_object_unref (e->str);
	g_object_unref (e->fol);
	g_slice_free (CpyEvent, e);
}

static CpyRecRet*
recurse_copy (TnyFolder *folder, TnyFolderStore *into, const gchar *new_name, gboolean del, GError **err, GList *rems, GList *adds)
{
	CpyRecRet *cpyr = g_slice_new0 (CpyRecRet);

	TnyFolderStore *a_store=NULL;
	TnyFolder *retval = NULL;
	TnyStoreAccount *acc_to;
	TnyCamelFolderPriv *fpriv = TNY_CAMEL_FOLDER_GET_PRIVATE (folder);
	TnyList *headers = NULL, *new_headers = NULL;

	GError *nerr = NULL;

	g_static_rec_mutex_lock (fpriv->folder_lock);

	load_folder_no_lock (fpriv);

	tny_debug ("tny_folder_copy: create %s\n", new_name);

	retval = tny_folder_store_create_folder (into, new_name, &nerr);
	if (nerr != NULL) {
		if (retval)
			g_object_unref (retval);
		goto exception;
	}

	/* tny_debug ("recurse_copy: adding to adds: %s\n", tny_folder_get_name (retval));
	 * adds = g_list_append (adds, cpy_event_new (TNY_FOLDER_STORE (into), retval)); */

	if (TNY_IS_FOLDER_STORE (folder))
	{
		TnyList *folders = tny_simple_list_new ();
		TnyIterator *iter;

		tny_folder_store_get_folders (TNY_FOLDER_STORE (folder), folders, NULL, TRUE, &nerr);

		if (nerr != NULL)
		{
			g_object_unref (folders);
			g_object_unref (retval);
			goto exception;
		}

		iter = tny_list_create_iterator (folders);
		while (!tny_iterator_is_done (iter))
		{
			TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));
			TnyFolder *mnew;

			CpyRecRet *rt = recurse_copy (cur, TNY_FOLDER_STORE (retval), 
				tny_folder_get_name (cur), del, &nerr, rems, adds);

			mnew = rt->created;
			rems = rt->rems;
			adds = rt->adds;

			g_slice_free (CpyRecRet, rt);

			if (nerr != NULL)
			{
				if (mnew) g_object_unref (mnew);
				g_object_unref (cur);
				g_object_unref (iter);
				g_object_unref (folders);
				g_object_unref (retval);
				goto exception;
			}

			g_object_unref (mnew);
			g_object_unref (cur);
			tny_iterator_next (iter);
		}
		g_object_unref (iter);
		g_object_unref (folders);
	}

	headers = tny_simple_list_new ();
	tny_folder_get_headers (folder, headers, TRUE, &nerr);
	if (nerr != NULL) {
		g_object_unref (headers);
		g_object_unref (retval);
		retval = NULL;
		goto exception;
	}

	tny_debug ("tny_folder_copy: transfer %s to %s\n", tny_folder_get_name (folder), tny_folder_get_name (retval));

	new_headers = tny_simple_list_new ();
	tny_camel_folder_transfer_msgs_shared (folder, headers, retval, del, new_headers, &nerr);

	/* TNY TODO: notify observers of retval about new_headers? */
	g_object_unref (new_headers);
	g_object_unref (headers);

	if (nerr != NULL) {
		g_object_unref (retval);
		retval = NULL;
		goto exception;
	}

	acc_to = TNY_STORE_ACCOUNT (tny_folder_get_account (retval));
	if (acc_to) {

		if (tny_folder_is_subscribed (folder))
		{
			CamelFolder *cfolder = NULL;
			TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (acc_to);
			const gchar *folder_full_name = NULL;
			CamelException ex = CAMEL_EXCEPTION_INITIALISER;
			CamelStore *store = NULL;

			cfolder = _tny_camel_folder_get_folder (TNY_CAMEL_FOLDER (retval));
			folder_full_name = camel_folder_get_full_name (cfolder);
			store = CAMEL_STORE (apriv->service);

			if (store && folder_full_name && camel_store_supports_subscriptions (store))
				camel_store_subscribe_folder (store, folder_full_name, &ex);
			
		}
		g_object_unref (acc_to);
	}

	if (del)
	{
		tny_debug ("tny_folder_copy: del orig %s\n", tny_folder_get_name (folder));

		a_store = tny_folder_get_folder_store (folder);
		if (a_store) {
			/* tny_debug ("recurse_copy: prepending to rems: %s\n", tny_folder_get_name (folder));
			rems = g_list_append (rems, cpy_event_new (a_store, folder)); */
			tny_folder_store_remove_folder (a_store, folder, &nerr);
			g_object_unref (a_store);
		} else {
			g_set_error (&nerr, TNY_ERROR_DOMAIN,
				TNY_SERVICE_ERROR_UNKNOWN,
				"The folder (%s) didn't have a parent, therefore "
				"failed to remove it while moving. This problem "
				"indicates a bug in the software.", 
				folder ? tny_folder_get_name (folder):"none");
		}
	}

exception:

	if (nerr != NULL)
		g_propagate_error (err, nerr);

	g_static_rec_mutex_unlock (fpriv->folder_lock);

	cpyr->created = retval;
	cpyr->adds = adds;
	cpyr->rems = rems;

	return cpyr;
}

typedef GList * (*lstmodfunc) (GList *list, gpointer data);

static GList*
recurse_evt (TnyFolder *folder, TnyFolderStore *into, GList *list, lstmodfunc func, gboolean rem)
{
	TnyList *folders = tny_simple_list_new ();
	TnyIterator *iter;
	TnyStoreAccount *acc;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (folder);

	if (!priv->folder)
		load_folder_no_lock (priv);

	acc = TNY_STORE_ACCOUNT (tny_folder_get_account (folder));

	if (rem) {
		if (acc) {
			CamelFolder *cfolder = NULL;
			TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (acc);
			const gchar *folder_full_name = NULL;
			CamelException ex = CAMEL_EXCEPTION_INITIALISER;
			CamelStore *store = NULL;

			cfolder = _tny_camel_folder_get_folder (TNY_CAMEL_FOLDER (folder));
			folder_full_name = camel_folder_get_full_name (cfolder);
			store = CAMEL_STORE (apriv->service);

			if (store && folder_full_name && camel_store_supports_subscriptions (store))
				camel_store_unsubscribe_folder (store, folder_full_name, &ex);
		}
	} else {
		if (acc) {
			CamelFolder *cfolder = NULL;
			TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (acc);
			const gchar *folder_full_name = NULL;
			CamelException ex = CAMEL_EXCEPTION_INITIALISER;
			CamelStore *store = NULL;

			cfolder = _tny_camel_folder_get_folder (TNY_CAMEL_FOLDER (folder));
			folder_full_name = camel_folder_get_full_name (cfolder);
			store = CAMEL_STORE (apriv->service);

			if (store && folder_full_name && camel_store_supports_subscriptions (store))
				camel_store_subscribe_folder (store, folder_full_name, &ex);
		}
	}

	if (acc)
		g_object_unref (acc);

	list = func (list, cpy_event_new (TNY_FOLDER_STORE (into), folder));
	tny_folder_store_get_folders (TNY_FOLDER_STORE (folder), folders, NULL, TRUE, NULL);
	iter = tny_list_create_iterator (folders);
	while (!tny_iterator_is_done (iter))
	{
		TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));
		list = recurse_evt (cur, TNY_FOLDER_STORE (folder), list, func, rem);
		g_object_unref (cur);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);
	g_object_unref (folders);

	return list;
}


static void
notify_folder_observers_about_copy (GList *adds, GList *rems, gboolean del, gboolean in_idle, TnySessionCamel *session)
{

	if (rems) {
		rems = g_list_first (rems);
		while (rems) {
			CpyEvent *evt = rems->data;
			if (del) {
				TnyFolderStoreChange *change = tny_folder_store_change_new (evt->str);

				tny_folder_store_change_add_removed_folder (change, evt->fol);

				if (TNY_IS_CAMEL_STORE_ACCOUNT (evt->str)) {
					if (in_idle)
						notify_folder_store_observers_about_for_store_acc_in_idle (evt->str, change, session);
					else
						notify_folder_store_observers_about_for_store_acc (evt->str, change, session);
				} else {
					if (in_idle)
						notify_folder_store_observers_about_in_idle (evt->str, change, session);
					else
						notify_folder_store_observers_about (evt->str, change, session);
				}

				g_object_unref (change);

			}
			cpy_event_free (evt);
			rems = g_list_next (rems);
		}
	}

	if (adds) {
		adds = g_list_first (adds);
		while (adds) {
			CpyEvent *evt = adds->data;
			TnyFolderStoreChange *change = tny_folder_store_change_new (evt->str);

			tny_folder_store_change_add_created_folder (change, evt->fol);
			if (TNY_IS_CAMEL_STORE_ACCOUNT (evt->str)) {
				if (in_idle) {
					notify_folder_store_observers_about_for_store_acc_in_idle (evt->str, change, session);
				} else {
					notify_folder_store_observers_about_for_store_acc (evt->str, change, session);
				}
			} else {
				if (in_idle) {
					notify_folder_store_observers_about_in_idle (evt->str, change, session);
				} else {
					notify_folder_store_observers_about (evt->str, change, session);
				}
			}
			g_object_unref (change);

			cpy_event_free (evt);
			adds = g_list_next (adds);
		}
	}

	return;
}

static CpyRecRet*
tny_camel_folder_copy_shared (TnyFolder *self, TnyFolderStore *into, const gchar *new_name, gboolean del, GError **err, GList *rems, GList *adds)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyFolder *retval = NULL;
	gboolean succeeded = FALSE, tried=FALSE, did_rename=FALSE;
	TnyAccount *a, *b = NULL;
	GError *nerr = NULL;
	GError *terr = NULL;
	CpyRecRet *retc;

	retc = g_slice_new0 (CpyRecRet);

	if (del && priv->reason_to_live != 0)
	{
		g_set_error (&nerr, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_STATE,
			"You should not use this operation with del=TRUE "
			"while the folder is still in use. There are "
			"still %d users of this folder. This problem "
			"indicates a bug in the software.", 
			priv->reason_to_live);

		g_propagate_error (err, nerr);
		return retc;
	}

	g_static_rec_mutex_lock (priv->folder_lock);

	retc->observers_ready = FALSE;

	if (TNY_IS_CAMEL_FOLDER (into) || TNY_IS_CAMEL_STORE_ACCOUNT (into))
	{
		a = tny_folder_get_account (self);

		if (TNY_IS_FOLDER (into))
			b = tny_folder_get_account (TNY_FOLDER (into));
		else /* it's a TnyCamelStoreAccount in this case */
			b = g_object_ref (into);

		if (a && b && (del && (a == b)))
		{
			TnyFolderStore *a_store;
			TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (a);
			CamelException ex = CAMEL_EXCEPTION_INITIALISER;
			gchar *from, *to;

			/* load_folder_no_lock (priv);
			   load_folder_no_lock (tpriv); */

			from = priv->folder_name;

			if (TNY_IS_CAMEL_FOLDER (into)) {
				TnyCamelFolderPriv *tpriv = TNY_CAMEL_FOLDER_GET_PRIVATE (into);
				tpriv->cant_reuse_iter = TRUE;
				to = g_strdup_printf ("%s/%s", tpriv->folder_name, new_name);
			} else {
				TnyCamelStoreAccountPriv *sapriv = TNY_CAMEL_STORE_ACCOUNT_GET_PRIVATE (into);
				sapriv->cant_reuse_iter = TRUE;
				to = g_strdup (new_name);
			}

			tny_debug ("tny_folder_copy: rename %s to %s\n", from, to);

			a_store = tny_folder_get_folder_store (self);
			if (a_store) {
				rems = recurse_evt (self, a_store,
					rems, g_list_prepend, TRUE);
				g_object_unref (a_store);
			}

			/* This does a g_rename on the mmap()ed file! */
			camel_store_rename_folder (CAMEL_STORE (apriv->service), from, to, &ex);

			did_rename = TRUE;

			if (!camel_exception_is_set (&ex))
			{
				CamelFolderInfo *iter;

				gboolean was_new=FALSE;

				retval = tny_camel_store_account_factor_folder 
					(TNY_CAMEL_STORE_ACCOUNT (a), to, &was_new);
				_tny_camel_folder_set_parent (TNY_CAMEL_FOLDER (retval), 
					into);

				if (priv->folder_name)
					g_free (priv->folder_name);
				priv->folder_name = g_strdup (to);
				priv->iter = NULL; /* Known leak */

				succeeded = TRUE;

				if (TRUE || was_new)
				{
					CamelStore *store = priv->store;
					CamelException ex = CAMEL_EXCEPTION_INITIALISER;
					int flags = CAMEL_STORE_FOLDER_INFO_RECURSIVE|CAMEL_STORE_FOLDER_INFO_NO_VIRTUAL;

					iter = camel_store_get_folder_info (store, to, flags, &ex);
					if (camel_exception_is_set (&ex) || !iter)
					{
						if (!camel_exception_is_set (&ex))
							g_set_error (&terr, TNY_ERROR_DOMAIN,
								TNY_SERVICE_ERROR_COPY, 
								_("Folder not ready for copy"));
						else {
							_tny_camel_exception_to_tny_error (&ex, &terr);
							camel_exception_clear (&ex);
						}
						succeeded = FALSE; tried=TRUE;
					}
					if (succeeded) {
						TnyCamelFolderPriv *rpriv = TNY_CAMEL_FOLDER_GET_PRIVATE (retval);
						rpriv->loaded = 0;
						_tny_camel_folder_set_folder_info (TNY_FOLDER_STORE (a), 
							TNY_CAMEL_FOLDER (retval), iter);
						if (!rpriv->folder_name || strlen (rpriv->folder_name) <= 0)
							rpriv->folder_name = g_strdup (to);
						_tny_camel_folder_set_parent (TNY_CAMEL_FOLDER (retval), into);
												
						if (G_LIKELY (rpriv->folder) && CAMEL_IS_FOLDER (rpriv->folder))
							{
								if (rpriv->folder_changed_id != 0)
  									camel_object_remove_event (rpriv->folder, rpriv->folder_changed_id);
							 	camel_object_unref (CAMEL_OBJECT (rpriv->folder));
							}
						rpriv->folder = NULL;
					} 
				}

				if (succeeded)
					adds = recurse_evt (retval, TNY_FOLDER_STORE (into), 
						adds, g_list_append, FALSE);


			} else {
				_tny_camel_exception_to_tny_error (&ex, &terr);
				camel_exception_clear (&ex);
				tried=TRUE;
			}

			g_free (to);
		}

		if (a)
			g_object_unref (a);

		if (b)
			g_object_unref (b);
	}

	if (!succeeded && !did_rename)
	{
		CpyRecRet *cpyr;

		tny_debug ("tny_folder_copy: recurse_copy\n");

		/* The recurse_copy call deletes the original folder if del==TRUE */
		cpyr = recurse_copy (self, into, new_name, del, &nerr, adds, rems);

		if (nerr != NULL) {
			if (!tried)
				g_propagate_error (err, nerr);
			else
				g_error_free (nerr);
		} else {
			/* Unload the folder. This way we'll get the
			   right CamelFolderInfo objects for the
			   TnyFolders */
			TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (cpyr->created);
			g_static_rec_mutex_lock (priv->folder_lock);
			camel_folder_info_free  (priv->iter);
			priv->iter = NULL;

			/* PVH investigated this and noticed that it made things
			 * crash in the following situation:
			 * You rename folder A to B, you copy B to C
			 *
			 * unload_folder_no_lock (priv, FALSE); 
			 **/
			succeeded = TRUE;

			g_static_rec_mutex_unlock (priv->folder_lock);
		}

		retval = cpyr->created;
		adds = cpyr->adds;
		rems = cpyr->rems;

		g_slice_free (CpyRecRet, cpyr);
	}

	if (succeeded) {
		TnyFolderStore *folder_store = tny_folder_get_folder_store (self);

		if (TNY_IS_ACCOUNT (folder_store)) {
			TnyCamelStoreAccountPriv *apriv = TNY_CAMEL_STORE_ACCOUNT_GET_PRIVATE (folder_store);
			apriv->iter = NULL;
		} else if (TNY_IS_FOLDER (folder_store)) {
			TnyCamelFolderPriv *ppriv = TNY_CAMEL_FOLDER_GET_PRIVATE (folder_store);
			ppriv->iter = NULL;
		}
		g_object_unref (folder_store);
	}

	if (tried && terr)
		g_propagate_error (err, terr);

	if (!tried && terr)
		g_error_free (terr);

	retc->created = retval;
	retc->adds = adds;
	retc->rems = rems;

	g_static_rec_mutex_unlock (priv->folder_lock);

	return retc;
}

static TnyFolder*
tny_camel_folder_copy_default (TnyFolder *self, TnyFolderStore *into, const gchar *new_name, gboolean del, GError **err)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	GList *rems=NULL, *adds=NULL;
	TnyFolder *retval = NULL;
	GError *nerr = NULL;
	CpyRecRet *cpyr;
	TnyFolderStore *orig_store;

	if (!_tny_session_check_operation (TNY_FOLDER_PRIV_GET_SESSION(priv), 
			priv->account, err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_COPY))
		return NULL;


	if (!priv->account) {
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_COPY,
			_("Folder not ready for copy"));
		return NULL;
	}

	orig_store = tny_folder_get_folder_store (self);

	/* If the caller is trying to move the folder to the location where it
	 * already is, we'll just return self ... */

	if (orig_store && orig_store == into && (!strcmp (new_name, tny_folder_get_name (self)))) {
		g_object_unref (orig_store);
		return TNY_FOLDER (g_object_ref (self));
	}

	if (orig_store)
		g_object_unref (orig_store);

	cpyr = tny_camel_folder_copy_shared (self, into, new_name, del, &nerr, rems, adds);

	retval = cpyr->created;
	rems = cpyr->rems;
	adds = cpyr->adds;

	g_slice_free (CpyRecRet, cpyr);

	if (nerr != NULL)
		g_propagate_error (err, nerr);
	else
		notify_folder_observers_about_copy (adds, rems, del, TRUE,
			TNY_FOLDER_PRIV_GET_SESSION (priv));

	if (adds)
		g_list_free (adds);
	if (rems)
		g_list_free (rems);

	_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));

	return retval;
}


typedef struct 
{
	TnyCamelQueueable parent;

	TnyFolder *self;
	TnyFolderStore *into;
	gchar *new_name;
	gboolean delete_originals;
	GError *err;
	gpointer user_data;
	TnyCopyFolderCallback callback;
	TnyStatusCallback status_callback;
	TnyFolder *new_folder;
	TnySessionCamel *session;
	TnyIdleStopper *stopper;
	gboolean cancelled;
	GList *rems, *adds;

} CopyFolderInfo;


static void
tny_camel_folder_copy_async_destroyer (gpointer thr_user_data)
{
	CopyFolderInfo *info = (CopyFolderInfo *) thr_user_data;

	if (info->new_folder)
		g_object_unref (info->new_folder);

	/* thread reference */
	g_object_unref (info->into);
	g_object_unref (info->self);

	if (info->err)
		g_error_free (info->err);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	if (info->new_name)
		g_free (info->new_name);

	if (info->rems)
		g_list_free (info->rems);
	if (info->adds)
		g_list_free (info->adds);

	/**/

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_copy_async_callback (gpointer thr_user_data)
{
	CopyFolderInfo *info = (CopyFolderInfo *) thr_user_data;

	if (info->err == NULL) {
		notify_folder_observers_about_copy (info->adds, info->rems, 
			info->delete_originals, FALSE, info->session);
	}

	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, info->cancelled, info->into, 
			info->new_folder, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}

	tny_idle_stopper_stop (info->stopper);

	return FALSE;
}

static void
tny_camel_folder_copy_async_status (struct _CamelOperation *op, const char *what, int sofar, int oftotal, void *thr_user_data)
{
	CopyFolderInfo *oinfo = thr_user_data;
	TnyProgressInfo *info = NULL;

	/* Send back progress data only for these internal operations */

	if (!what)
		return;

	if ((g_ascii_strcasecmp(what, "Renaming folder")) &&
	    (g_ascii_strcasecmp(what, "Moving messages")) &&
	    (g_ascii_strcasecmp(what, "Copying messages"))) 
		return;
	
	info = tny_progress_info_new (G_OBJECT (oinfo->self), oinfo->status_callback, 
		TNY_FOLDER_STATUS, TNY_FOLDER_STATUS_CODE_COPY_FOLDER, what, sofar, 
		oftotal, oinfo->stopper, oinfo->session->priv->ui_lock, oinfo->user_data);

	g_idle_add_full (TNY_PRIORITY_LOWER_THAN_GTK_REDRAWS,
			  tny_progress_info_idle_func, info, 
			  tny_progress_info_destroy);

	return;
}


static gpointer 
tny_camel_folder_copy_async_thread (gpointer thr_user_data)
{
	CopyFolderInfo *info = thr_user_data;
	TnyFolder *self = info->self;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyCamelAccountPriv *apriv = NULL;
	GError *nerr = NULL;
	CpyRecRet *cpyr;
	TnyFolderStore *orig_store = NULL;

	if (!priv->account) {
		info->cancelled = TRUE;
		return NULL;
	}

	apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (priv->account);

	g_static_rec_mutex_lock (priv->folder_lock);

	info->cancelled = FALSE;

	_tny_camel_account_start_camel_operation (TNY_CAMEL_ACCOUNT (priv->account), 
		tny_camel_folder_copy_async_status, 
		info, "Copying folder");

	info->adds = NULL; 
	info->rems = NULL;
	info->new_folder = NULL;

	/* If the caller is trying to move the folder to the location where it
	 * already is, we'll just do nothing  ... */

	orig_store = tny_folder_get_folder_store (self);
	if (!(orig_store && orig_store == info->into && (!strcmp (info->new_name, tny_folder_get_name (self)))))
	{
		cpyr = tny_camel_folder_copy_shared (info->self, info->into, 
				info->new_name, info->delete_originals, &nerr, 
				info->rems, info->adds);

		info->new_folder = cpyr->created;
		info->rems = cpyr->rems;
		info->adds = cpyr->adds;

		g_slice_free (CpyRecRet, cpyr);
	}

	if (orig_store)
		g_object_unref (orig_store);

	info->cancelled = camel_operation_cancel_check (apriv->cancel);

	_tny_camel_account_stop_camel_operation (TNY_CAMEL_ACCOUNT (priv->account));

	info->err = NULL;
	if (nerr != NULL)
		g_propagate_error (&info->err, nerr);

	g_static_rec_mutex_unlock (priv->folder_lock);

	return NULL;
}

static void
tny_camel_folder_copy_async_cancelled_destroyer (gpointer thr_user_data)
{
	CopyFolderInfo *info = (CopyFolderInfo *) thr_user_data;

	if (info->new_folder)
		g_object_unref (info->new_folder);

	if (info->new_name)
		g_free (info->new_name);
	if (info->err)
		g_error_free (info->err);
	g_object_unref (info->self);
	g_object_unref (info->into);

	if (info->rems)
		g_list_free (info->rems);
	if (info->adds)
		g_list_free (info->adds);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	/**/

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_copy_async_cancelled_callback (gpointer thr_user_data)
{
	CopyFolderInfo *info = (CopyFolderInfo *) thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, TRUE, info->into, NULL, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}

static void
tny_camel_folder_copy_async (TnyFolder *self, TnyFolderStore *into, const gchar *new_name, gboolean del, TnyCopyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->copy_async(self, into, new_name, del, callback, status_callback, user_data);
	return;
}

static void
tny_camel_folder_copy_async_default (TnyFolder *self, TnyFolderStore *into, const gchar *new_name, gboolean del, TnyCopyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyCamelFolderPriv *dest_priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	CopyFolderInfo *info;

	/* Idle info for the callbacks */
	info = g_slice_new (CopyFolderInfo);
	info->cancelled = FALSE;
	info->session = TNY_FOLDER_PRIV_GET_SESSION (priv);
	camel_object_ref (info->session);
	info->self = self;
	info->new_folder = NULL;
	info->into = into;
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;
	info->err = NULL;
	info->delete_originals = del;
	info->new_name = g_strdup (new_name);
	info->err = NULL;

	info->stopper = tny_idle_stopper_new();

	/* thread reference */
	g_object_ref (info->self);
	g_object_ref (info->into);

	_tny_camel_queue_launch (TNY_FOLDER_PRIV_GET_QUEUE (priv), 
		tny_camel_folder_copy_async_thread, 
		tny_camel_folder_copy_async_callback,
		tny_camel_folder_copy_async_destroyer, 
		tny_camel_folder_copy_async_cancelled_callback,
		tny_camel_folder_copy_async_cancelled_destroyer, 
		&info->cancelled,
		info, sizeof (CopyFolderInfo),
		__FUNCTION__);

	return;
}


typedef struct 
{
	TnyCamelQueueable parent;

	GError *err;
	TnyFolder *self;
	TnyTransferMsgsCallback callback;
	TnyStatusCallback status_callback;
	TnyIdleStopper *stopper;
	gpointer user_data;
	TnyList *header_list;
	TnyList *new_header_list;
	TnyFolder *folder_dst;
	gboolean delete_originals;
	TnySessionCamel *session;
	gint from_all; 
	gint to_all;
	gint from_unread;
	gint to_unread;
	gboolean cancelled;
	TnySessionCamel *dsession;

} TransferMsgsInfo;


static void 
notify_folder_observers_about_transfer (TnyFolder *from, TnyFolder *to, gboolean del_orig, TnyList *headers, TnyList *new_header_list, gint from_all, gint to_all, gint from_unread, gint to_unread, gboolean in_idle, TnySessionCamel *session)
{
	TnyFolderChange *tochange = tny_folder_change_new (to);
	TnyFolderChange *fromchange = tny_folder_change_new (from);
	TnyIterator *iter;

	tny_folder_change_set_check_duplicates (tochange, TRUE);
	tny_folder_change_set_check_duplicates (fromchange, TRUE);

	iter = tny_list_create_iterator (new_header_list);
	while (!tny_iterator_is_done (iter)) 
	{
		TnyHeader *header = TNY_HEADER (tny_iterator_get_current (iter));
		if (del_orig)
			tny_folder_change_add_expunged_header (fromchange, header);
		tny_folder_change_add_added_header (tochange, header);
		g_object_unref (header);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);

	tny_folder_change_set_new_all_count (tochange, to_all);
	tny_folder_change_set_new_unread_count (tochange, to_unread);

	tny_folder_change_set_new_all_count (fromchange, from_all);
	tny_folder_change_set_new_unread_count (fromchange, from_unread);

	if (in_idle)
	{
		notify_folder_observers_about_in_idle (to, tochange, session);
		notify_folder_observers_about_in_idle (from, fromchange, session);
	} else {
		notify_folder_observers_about (to, tochange, session);
		notify_folder_observers_about (from, fromchange, session);
	}

	g_object_unref (tochange);
	g_object_unref (fromchange);

	return;
}

static void
tny_camel_folder_transfer_msgs_async_destroyer (gpointer thr_user_data)
{
	TransferMsgsInfo *info = thr_user_data;
	TnyCamelFolderPriv *priv_src = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);
	TnyCamelFolderPriv *priv_dst = TNY_CAMEL_FOLDER_GET_PRIVATE (info->folder_dst);

	/* thread reference */
	_tny_camel_folder_unreason (priv_src);
	g_object_unref (info->self);
	g_object_unref (info->header_list);
	if (info->new_header_list)
		g_object_unref (info->new_header_list);
	_tny_camel_folder_unreason (priv_dst);
	g_object_unref (info->folder_dst);

	if (info->err) 
		g_error_free (info->err);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	/**/

	camel_object_unref (info->session);
	camel_object_unref (info->dsession);

	return;
}

static gboolean
tny_camel_folder_transfer_msgs_async_callback (gpointer thr_user_data)
{
	TransferMsgsInfo *info = thr_user_data;
	
	notify_folder_observers_about_transfer (info->self, info->folder_dst, info->delete_originals,
		info->header_list, info->new_header_list, info->from_all, 
		info->to_all, info->from_unread, info->to_unread, FALSE, info->session);

	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, info->cancelled, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}

	tny_idle_stopper_stop (info->stopper);

	return FALSE;
}


static void
transfer_msgs_thread_clean (TnyFolder *self, TnyList *headers, TnyList *new_headers, TnyFolder *folder_dst, gboolean delete_originals, GError **err)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyFolder *folder_src = self;
	TnyCamelFolderPriv *priv_src, *priv_dst;
	TnyIterator *iter;
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;
	CamelFolder *cfol_src, *cfol_dst;
	guint list_length;
	GPtrArray *uids = NULL;
	GPtrArray *transferred_uids = NULL;
	GPtrArray *dst_orig_uids = NULL;
	gboolean no_uidplus = FALSE, did_refresh = FALSE;

	g_assert (TNY_IS_LIST (headers));
	g_assert (TNY_IS_FOLDER (folder_src));
	g_assert (TNY_IS_FOLDER (folder_dst));

	if (!priv->account) {
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_TRANSFER,
			_("Folder not ready for transfer"));
		return;
	}

	if (!_tny_session_check_operation (TNY_FOLDER_PRIV_GET_SESSION(priv), 
			priv->account, err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_TRANSFER))
		return;

	list_length = tny_list_get_length (headers);

	if (list_length < 1) {
		_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
		return;
	}

	/* Get privates */
	priv_src = TNY_CAMEL_FOLDER_GET_PRIVATE (folder_src);
	priv_dst = TNY_CAMEL_FOLDER_GET_PRIVATE (folder_dst);

	g_static_rec_mutex_lock (priv_src->folder_lock);
	g_static_rec_mutex_lock (priv_dst->folder_lock);

	if (!priv_src->folder || !priv_src->loaded || !CAMEL_IS_FOLDER (priv_src->folder))
		if (!load_folder_no_lock (priv_src)) {
			_tny_camel_exception_to_tny_error (&priv_src->load_ex, err);
			camel_exception_clear (&priv_src->load_ex);
			g_static_rec_mutex_unlock (priv_src->folder_lock);
			g_static_rec_mutex_unlock (priv_dst->folder_lock);
			_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
			return;
		}

	if (!priv_dst->folder || !priv_dst->loaded || !CAMEL_IS_FOLDER (priv_dst->folder))
		if (!load_folder_no_lock (priv_dst)) {
			_tny_camel_exception_to_tny_error (&priv_dst->load_ex, err);
			camel_exception_clear (&priv_dst->load_ex);
			g_static_rec_mutex_unlock (priv_src->folder_lock);
			g_static_rec_mutex_unlock (priv_dst->folder_lock);
			_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
			return;
		}

	iter = tny_list_create_iterator (headers);
	uids = g_ptr_array_sized_new (list_length);

	/* Get camel folders */
	cfol_src = _tny_camel_folder_get_camel_folder (TNY_CAMEL_FOLDER (folder_src));
	cfol_dst = _tny_camel_folder_get_camel_folder (TNY_CAMEL_FOLDER (folder_dst));

	/* Create uids */
	while (!tny_iterator_is_done (iter)) 
	{
		TnyHeader *header;
		gchar *uid;

		header = TNY_HEADER (tny_iterator_get_current (iter));
		uid = tny_header_dup_uid (header);

		if (G_UNLIKELY (uid == NULL)) 
		{
			g_set_error (err, TNY_ERROR_DOMAIN,
				TNY_SERVICE_ERROR_TRANSFER,
				"You can only pass summary items as headers. "
				"These are instances that you got with the "
				"tny_folder_get_headers API. You can't use "
				"the header instances that tny_msg_get_header "
				"will return you. This problem indicates a bug "
				"in the software.");

			g_object_unref (header);
			g_object_unref (iter);
			g_ptr_array_free (uids, TRUE);

			g_static_rec_mutex_unlock (priv_dst->folder_lock);
			g_static_rec_mutex_unlock (priv_src->folder_lock);

			_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
			return;
		} else
			g_ptr_array_add (uids, (gpointer) uid);

		g_object_unref (header);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);

	priv_src->handle_changes = FALSE;
	priv_dst->handle_changes = FALSE;

	camel_folder_freeze (cfol_src);
	camel_folder_freeze (cfol_dst);


	/* We can't yet know whether the more efficient way will be 
	 * possible. So we'll just always copy the uids temporarily,
	 * just in case we'll need the less efficient way . . . (sorry,
	 * we'll clean it up from your memory, don't worry) */
	if (cfol_dst && cfol_dst->summary && cfol_dst->summary->messages)
	{
		gint a = 0, len = 0;
		len = cfol_dst->summary->messages->len;
		dst_orig_uids = g_ptr_array_sized_new (len);
		for (a = 0; a < len; a++) {
			CamelMessageInfo *om = camel_folder_summary_index (cfol_dst->summary, a);
			if (om && om->uid)
				g_ptr_array_add (dst_orig_uids, g_strdup (om->uid));
			if (om)
				camel_message_info_free (om);
		}
	}

	camel_folder_transfer_messages_to (cfol_src, uids, cfol_dst, 
			&transferred_uids, delete_originals, &ex);

	if (!camel_exception_is_set (&ex) && uids && delete_originals)
	{
		int i;
		for (i = 0; i < uids->len; i++) {
			camel_folder_set_message_flags (cfol_src, uids->pdata[i],
				CAMEL_MESSAGE_SEEN, CAMEL_MESSAGE_SEEN);
		}
	}

	camel_folder_thaw (cfol_src);
	camel_folder_thaw (cfol_dst);

	if (camel_exception_is_set (&ex))
		goto err_goto_lbl;

	camel_folder_sync (cfol_dst, FALSE, NULL);

	if (new_headers && transferred_uids)
	{
		/* The more efficient way: We rely on the information that the 
		 * service's implementation of 'append_message' gave us. If 
		 * something, anything, goes wrong here, we'll fall back to a 
		 * less efficient method (see below for that one) */

		int i = 0;
		CamelException ex = CAMEL_EXCEPTION_INITIALISER;

		/* Refreshing is needed to get the new summary early, I know 
		 * this pulls bandwidth. */

		camel_folder_refresh_info (cfol_dst, &ex);
		did_refresh = TRUE;

		if (!camel_exception_is_set (&ex)) 
		{
		  GList *succeeded_news = NULL, *copy = NULL;
		  no_uidplus = FALSE;

		  for (i = 0; i < transferred_uids->len; i++) 
		  {
			const gchar *uid = transferred_uids->pdata[i];
			CamelMessageInfo *minfo = NULL;

			if (uid == NULL) {
				/* Fallback to the less efficient way */
				no_uidplus = TRUE;
				break;
			}

			minfo = camel_folder_summary_uid (cfol_dst->summary, uid);
			if (minfo)
			{
				TnyHeader *hdr = _tny_camel_header_new ();
				/* This adds a reason to live for folder_dst */
				_tny_camel_header_set_folder (TNY_CAMEL_HEADER (hdr), 
					TNY_CAMEL_FOLDER (folder_dst), priv_dst);
				/* hdr will take care of the freeup */
				_tny_camel_header_set_as_memory (TNY_CAMEL_HEADER (hdr), minfo);
				succeeded_news = g_list_prepend (succeeded_news, hdr);
			} else {
				/* Fallback to the less efficient way */
				no_uidplus = TRUE;
				break;
			}

		  }

		  copy = succeeded_news;
		  while (copy) {
		  	TnyHeader *hdr = copy->data;
		  	if (hdr) {
		  		if (!no_uidplus)
		  			tny_list_prepend (new_headers, (GObject *) hdr);
		  		g_object_unref (hdr);
		  	}
		  	copy = g_list_next (copy);
		  }
		  g_list_free (succeeded_news);

		} else {
			g_warning ("Oeps from Tinymail: refreshing the summary "
				"failed after transferring messages to a folder");
			no_uidplus = TRUE;
		}
	} 

	if ((dst_orig_uids) && ((new_headers && !transferred_uids) || no_uidplus))
	{
		/* The less efficient way: We'll compare the old uids with the
		 * new list of uids. If we find new items, we'll create headers
		 * for it */
		CamelException ex = CAMEL_EXCEPTION_INITIALISER;
		gint nlen = 0, a = 0;

		if (!did_refresh)
			camel_folder_refresh_info (cfol_dst, &ex);

		if (did_refresh || (!did_refresh && !camel_exception_is_set (&ex))) 
		{
		  nlen = cfol_dst->summary->messages->len;

		  for (a = 0; a < nlen; a++) 
		  {
			CamelMessageInfo *om = camel_folder_summary_index (cfol_dst->summary, a);
			if (om && om->uid) {
				gint b = 0;
				gboolean found = FALSE;

				for (b = 0; b < dst_orig_uids->len; b++) {
					/* Finding the needle ... */
					if (!strcmp (dst_orig_uids->pdata[b], om->uid)) {
						found = TRUE;
						break;
					}
				}

				if (!found) { 
					/* Jeej! a new one! */

					TnyHeader *hdr = _tny_camel_header_new ();

					/* This adds a reason to live for folder_dst */
					_tny_camel_header_set_folder (TNY_CAMEL_HEADER (hdr), 
						TNY_CAMEL_FOLDER (folder_dst), priv_dst);
					/* hdr will take care of the freeup */
					_tny_camel_header_set_as_memory (TNY_CAMEL_HEADER (hdr), om);
					tny_list_prepend (new_headers, (GObject *) hdr);
					g_object_unref (hdr);
				} else /* Not-new message, freeup */
					camel_message_info_free (om);
			} else if (om) /* arg? */
				camel_message_info_free (om);
		  } 
		}
	}


err_goto_lbl:

	if (camel_exception_is_set (&ex)) 
	{
		_tny_camel_exception_to_tny_error (&ex, err);
		camel_exception_clear (&ex);
	} else {
		/* if (delete_originals) 
			camel_folder_sync (cfol_src, TRUE, &ex); */
	}

	priv_src->handle_changes = TRUE;
	priv_dst->handle_changes = TRUE;

	if (transferred_uids) {
		g_ptr_array_foreach (transferred_uids, (GFunc) g_free, NULL);
		g_ptr_array_free (transferred_uids, TRUE);
	}

	if (dst_orig_uids) {
		g_ptr_array_foreach (dst_orig_uids, (GFunc) g_free, NULL);
		g_ptr_array_free (dst_orig_uids, TRUE);
	}

	g_ptr_array_foreach (uids, (GFunc) g_free, NULL);
	g_ptr_array_free (uids, TRUE);

	priv_src->unread_length = camel_folder_get_unread_message_count (cfol_src);
	priv_src->cached_length = camel_folder_get_message_count (cfol_src);
	update_iter_counts (priv_src);
	priv_dst->unread_length = camel_folder_get_unread_message_count (cfol_dst);
	priv_dst->cached_length = camel_folder_get_message_count (cfol_dst);
	update_iter_counts (priv_dst);

	g_static_rec_mutex_unlock (priv_dst->folder_lock);
	g_static_rec_mutex_unlock (priv_src->folder_lock);

	_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));

	return;
}

static void
tny_camel_folder_transfer_msgs_async_status (struct _CamelOperation *op, const char *what, int sofar, int oftotal, void *thr_user_data)
{
	TransferMsgsInfo *oinfo = thr_user_data;
	TnyProgressInfo *info = NULL;

	info = tny_progress_info_new (G_OBJECT (oinfo->self), oinfo->status_callback, 
		TNY_FOLDER_STATUS, TNY_FOLDER_STATUS_CODE_XFER_MSGS, what, sofar, 
		oftotal, oinfo->stopper, oinfo->session->priv->ui_lock, oinfo->user_data);

	g_idle_add_full (TNY_PRIORITY_LOWER_THAN_GTK_REDRAWS,
			  tny_progress_info_idle_func, info, 
			  tny_progress_info_destroy);

	return;
}

static gpointer 
tny_camel_folder_transfer_msgs_async_thread (gpointer thr_user_data)
{
	TransferMsgsInfo *info = (TransferMsgsInfo*) thr_user_data;
	TnyCamelFolderPriv *priv_src = NULL, *priv_dst = NULL;
	TnyCamelAccountPriv *apriv = NULL;
	gboolean on_err = FALSE;

	priv_src = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);
	priv_dst = TNY_CAMEL_FOLDER_GET_PRIVATE (info->folder_dst);

	if (!priv_src->folder || !priv_src->loaded || !CAMEL_IS_FOLDER (priv_src->folder))
		if (!load_folder_no_lock (priv_src)) {
			on_err = TRUE;
			_tny_camel_exception_to_tny_error (&priv_src->load_ex, &info->err);
			camel_exception_clear (&priv_src->load_ex);
		}

	if (!priv_dst->folder || !priv_dst->loaded || !CAMEL_IS_FOLDER (priv_dst->folder))
		if (!load_folder_no_lock (priv_dst)) {
			on_err = TRUE;
			_tny_camel_exception_to_tny_error (&priv_dst->load_ex, &info->err);
			camel_exception_clear (&priv_dst->load_ex);
		}

	if (!on_err) 
	{
		info->cancelled = FALSE;
		apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (priv_src->account);

		/* Start operation */
		_tny_camel_account_start_camel_operation (TNY_CAMEL_ACCOUNT (priv_src->account), 
			tny_camel_folder_transfer_msgs_async_status, info, 
			"Transfer messages between two folders");

		transfer_msgs_thread_clean (info->self, info->header_list, info->new_header_list, info->folder_dst, 
				info->delete_originals, &info->err);

		/* Check cancelation and stop operation */
		info->cancelled = camel_operation_cancel_check (apriv->cancel);
		_tny_camel_account_stop_camel_operation (TNY_CAMEL_ACCOUNT (priv_src->account));

		/* Reset local size info */
		reset_local_size (priv_src);
		reset_local_size (priv_dst);

		/* Get data */
		info->from_all = camel_folder_get_message_count (priv_src->folder);
		info->to_all = camel_folder_get_message_count (priv_dst->folder);
		info->from_unread = camel_folder_get_unread_message_count (priv_src->folder);
		info->to_unread = camel_folder_get_unread_message_count (priv_dst->folder);
	} else
		info->cancelled = TRUE;


	return NULL;
}

static void
tny_camel_folder_transfer_msgs_async_cancelled_destroyer (gpointer thr_user_data)
{
	TransferMsgsInfo *info = thr_user_data;
	TnyCamelFolderPriv *priv_src = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);
	TnyCamelFolderPriv *priv_dst = TNY_CAMEL_FOLDER_GET_PRIVATE (info->folder_dst);

	/* thread reference */
	_tny_camel_folder_unreason (priv_src);
	g_object_unref (info->self);
	g_object_unref (info->header_list);
	if (info->new_header_list)
		g_object_unref (info->new_header_list);
	_tny_camel_folder_unreason (priv_dst);
	g_object_unref (info->folder_dst);

	if (info->err)
		g_error_free (info->err);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	/**/

	camel_object_unref (info->session);
	camel_object_unref (info->dsession);

	return;
}

static gboolean
tny_camel_folder_transfer_msgs_async_cancelled_callback (gpointer thr_user_data)
{
	TransferMsgsInfo *info = thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, TRUE, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}


static void
tny_camel_folder_transfer_msgs_async (TnyFolder *self, TnyList *header_list, TnyFolder *folder_dst, gboolean delete_originals, TnyTransferMsgsCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->transfer_msgs_async(self, header_list, folder_dst, delete_originals, callback, status_callback, user_data);
	return;
}

static void
tny_camel_folder_transfer_msgs_async_default (TnyFolder *self, TnyList *header_list, TnyFolder *folder_dst, gboolean delete_originals, TnyTransferMsgsCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TransferMsgsInfo *info;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyCamelFolderPriv *priv_src = priv;
	TnyCamelFolderPriv *priv_dst = TNY_CAMEL_FOLDER_GET_PRIVATE (folder_dst);

	/* Idle info for the callbacks */
	info = g_slice_new (TransferMsgsInfo);
	info->cancelled = FALSE;
	info->session = TNY_FOLDER_PRIV_GET_SESSION (priv);
	camel_object_ref (info->session);
	info->dsession = TNY_FOLDER_PRIV_GET_SESSION (priv_dst);
	camel_object_ref (info->dsession);
	info->self = self;
	info->new_header_list = tny_simple_list_new ();
	info->header_list = header_list; 
	info->folder_dst = folder_dst;
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;
	info->delete_originals = delete_originals;
	info->err = NULL;

	info->stopper = tny_idle_stopper_new();

	/* thread reference */
	g_object_ref (info->header_list);
	_tny_camel_folder_reason (priv_src);
	g_object_ref (info->self);
	_tny_camel_folder_reason (priv_dst);
	g_object_ref (info->folder_dst);

	_tny_camel_queue_launch_wflags (TNY_FOLDER_PRIV_GET_QUEUE (priv),
		tny_camel_folder_transfer_msgs_async_thread, 
		tny_camel_folder_transfer_msgs_async_callback,
		tny_camel_folder_transfer_msgs_async_destroyer, 
		tny_camel_folder_transfer_msgs_async_cancelled_callback,
		tny_camel_folder_transfer_msgs_async_cancelled_destroyer, 
		&info->cancelled,
		info, sizeof (TransferMsgsInfo), TNY_CAMEL_QUEUE_CANCELLABLE_ITEM,
		__FUNCTION__);

	return;
}


static void
tny_camel_folder_transfer_msgs (TnyFolder *self, TnyList *headers, TnyFolder *folder_dst, gboolean delete_originals, GError **err)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->transfer_msgs(self, headers, folder_dst, delete_originals, err);
	return;
}


static void
tny_camel_folder_transfer_msgs_shared (TnyFolder *self, TnyList *headers, TnyFolder *folder_dst, gboolean delete_originals, TnyList *new_headers, GError **err)
{
	TnyCamelFolderPriv *priv_src, *priv_dst;
	gboolean on_err = FALSE;

	g_assert (TNY_IS_LIST (headers));
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_IS_FOLDER (folder_dst));

	priv_src = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	priv_dst = TNY_CAMEL_FOLDER_GET_PRIVATE (folder_dst);

	if (!priv_src->folder || !priv_src->loaded || !CAMEL_IS_FOLDER (priv_src->folder))
		if (!load_folder (priv_src)) {
			on_err = TRUE;
			_tny_camel_exception_to_tny_error (&priv_src->load_ex, err);
			camel_exception_clear (&priv_src->load_ex);
		}

	if (!priv_dst->folder || !priv_dst->loaded || !CAMEL_IS_FOLDER (priv_dst->folder))
		if (!load_folder (priv_dst)) {
			_tny_camel_exception_to_tny_error (&priv_dst->load_ex, err);
			camel_exception_clear (&priv_dst->load_ex);
			on_err = TRUE;
		}

	if (!on_err) {
		_tny_camel_folder_reason (priv_dst);
		transfer_msgs_thread_clean (self, headers, new_headers, folder_dst, delete_originals, err);
		_tny_camel_folder_unreason (priv_dst);
	}

	return;
}

static void
tny_camel_folder_transfer_msgs_default (TnyFolder *self, TnyList *headers, TnyFolder *folder_dst, gboolean delete_originals, GError **err)
{
	GError *nerr = NULL;
	TnyList *new_headers = tny_simple_list_new ();
	gint from = 0, to = 0, ufrom = 0, uto = 0;
	TnyCamelFolderPriv *priv_src, *priv_dst;

	priv_src = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	priv_dst = TNY_CAMEL_FOLDER_GET_PRIVATE (folder_dst);

	tny_camel_folder_transfer_msgs_shared (self, headers, folder_dst, delete_originals, new_headers, &nerr);

	if (nerr == NULL)
	{
		from = camel_folder_get_message_count (priv_src->folder);
		to = camel_folder_get_message_count (priv_dst->folder);
		ufrom = camel_folder_get_unread_message_count (priv_src->folder);
		uto = camel_folder_get_unread_message_count (priv_dst->folder);
		notify_folder_observers_about_transfer (self, folder_dst, delete_originals, 
			headers, new_headers, from, to, ufrom, uto, TRUE,
				TNY_FOLDER_PRIV_GET_SESSION (priv_src));
	} else 
		g_propagate_error (err, nerr);

	g_object_unref (new_headers);

	return;
}

void
_tny_camel_folder_set_id (TnyCamelFolder *self, const gchar *id)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	g_static_rec_mutex_lock (priv->folder_lock);

	/* unload_folder_no_lock (priv, TRUE); */

	if (G_UNLIKELY (priv->folder_name))
		g_free (priv->folder_name);

	priv->folder_name = g_strdup (id);

	g_static_rec_mutex_unlock (priv->folder_lock);

	return;
}

void
_tny_camel_folder_set_name (TnyCamelFolder *self, const gchar *name)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	if (G_UNLIKELY (priv->cached_name))
		g_free (priv->cached_name);

	priv->cached_name = g_strdup (name);

	return;
}



static void
tny_camel_folder_uncache (TnyCamelFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	if (G_LIKELY (priv->folder != NULL))
		unload_folder (priv, FALSE);

	return;
}

static void
tny_camel_folder_uncache_nl (TnyCamelFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	if (G_LIKELY (priv->folder != NULL))
		unload_folder_no_lock (priv, FALSE);

	return;
}

static void 
folder_tracking_finalize (CamelObject *folder, gpointer event_data, gpointer user_data)
{
	TnyCamelFolderPriv *priv =  (TnyCamelFolderPriv *) user_data;
	if (priv->folder_tracking_id)
		priv->folder_tracking_id = 0;

	if (priv->folder_tracking)
		priv->folder_tracking = NULL;
}

void
_tny_camel_folder_track_folder_changed (TnyCamelFolder *self, 
					CamelFolder *folder)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	if (priv->folder_tracking == folder)
		return;

	if (priv->folder_tracking) {
		camel_object_remove_event (priv->folder_tracking, priv->folder_tracking_id);
		camel_object_unhook_event (priv->folder_tracking, "finalize", folder_tracking_finalize, priv);
		priv->folder_tracking_id = 0;
	}
	priv->folder_tracking = folder;
	if  (priv->folder_tracking != NULL) {
		priv->folder_tracking_id = camel_object_hook_event (priv->folder_tracking, "folder_changed", folder_tracking_changed, self);
		camel_object_hook_event (priv->folder_tracking, "finalize", folder_tracking_finalize, priv);
	}

}



void 
_tny_camel_folder_unreason (TnyCamelFolderPriv *priv)
{
	g_static_rec_mutex_lock (priv->reason_lock);
	priv->reason_to_live--;

#ifdef DEBUG_EXTRA
	g_print ("_tny_camel_folder_unreason (%s) : %d\n", 
		priv->folder_name?priv->folder_name:"(cleared)", 
			priv->reason_to_live);

	g_print ("Folder's reason to live -> %d\n", priv->reason_to_live);
#endif


	if (priv->reason_to_live == 0) 
	{
		/* The special case is for when the amount of items is ZERO
		 * while we are listening for Push E-mail events. That's a
		 * reason by itself not to destroy priv->folder
		 *
		 * For any other folder that has no more reason to live,
		 * we'll uncache (this means destroying the CamelFolder
		 * instance and freeing up memory */

		/* If we can't do Push E-mail, we don't need to keep the folder
		 * alive, because nothing will happen. */
		if (priv->folder)
		{
			if (!(priv->folder->folder_flags & CAMEL_FOLDER_HAS_PUSHEMAIL_CAPABILITY))
				tny_camel_folder_uncache_nl ((TnyCamelFolder *)priv->self);
			/* Else we should only close if there are not zero messages */
			else if (!(priv->push && priv->folder->summary && 
				priv->folder->summary->messages && 
				priv->folder->summary->messages->len == 0)) {
				tny_camel_folder_uncache ((TnyCamelFolder *)priv->self);
			}
		}
	}
	g_static_rec_mutex_unlock (priv->reason_lock);
}

void 
_tny_camel_folder_reason (TnyCamelFolderPriv *priv)
{
	//g_static_rec_mutex_lock (priv->reason_lock);
	priv->reason_to_live++;
	//g_static_rec_mutex_unlock (priv->reason_lock);
}

static TnyMsgRemoveStrategy* 
tny_camel_folder_get_msg_remove_strategy (TnyFolder *self)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->get_msg_remove_strategy(self);
}

static TnyMsgRemoveStrategy* 
tny_camel_folder_get_msg_remove_strategy_default (TnyFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	return TNY_MSG_REMOVE_STRATEGY (g_object_ref (G_OBJECT (priv->remove_strat)));
}

static void 
tny_camel_folder_set_msg_remove_strategy (TnyFolder *self, TnyMsgRemoveStrategy *st)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->set_msg_remove_strategy(self, st);
	return;
}

static void 
tny_camel_folder_set_msg_remove_strategy_default (TnyFolder *self, TnyMsgRemoveStrategy *st)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	if (priv->remove_strat)
		g_object_unref (G_OBJECT (priv->remove_strat));

	priv->remove_strat = TNY_MSG_REMOVE_STRATEGY (g_object_ref (G_OBJECT (st)));

	return;
}

void 
_tny_camel_folder_remove_folder_actual (TnyFolderStore *self, TnyFolder *folder, TnyFolderStoreChange *change, GError **err)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	TnyCamelStoreAccountPriv *apriv = NULL;
	CamelStore *store = priv->store;
	TnyCamelFolder *cfol = TNY_CAMEL_FOLDER (folder);
	TnyCamelFolderPriv *cpriv = TNY_CAMEL_FOLDER_GET_PRIVATE (cfol);
	gchar *cfolname; gchar *folname; gint parlen;
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;
	gboolean changed = FALSE;

	if (!priv->account) {
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_FOLDER_REMOVE,
			_("Folder store not ready for removal"));
		return;
	}

	apriv = TNY_CAMEL_STORE_ACCOUNT_GET_PRIVATE (priv->account);

	g_static_rec_mutex_lock (apriv->factory_lock);

	g_static_rec_mutex_lock (priv->folder_lock);
	g_static_rec_mutex_lock (cpriv->folder_lock);

	if (apriv->iter)
	{
		/* Known memleak
		camel_store_free_folder_info (apriv->iter_store, apriv->iter); */
		apriv->iter = NULL;
	}

	if (TNY_IS_STORE_ACCOUNT (self)) {
		TnyCamelStoreAccountPriv *ppriv = TNY_CAMEL_STORE_ACCOUNT_GET_PRIVATE (self);
		ppriv->iter = NULL;
	} else {
		TnyCamelFolderPriv *ppriv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
		ppriv->iter = NULL;
	}

	cfolname = cpriv->folder_name;
	folname = priv->folder_name;
	parlen = strlen (folname);

	/* /INBOX/test      *
	 * /INBOX/test/test */

	if (!strncmp (folname, cfolname, parlen))
	{
		gchar *ccfoln = cfolname + parlen;
		if ((*ccfoln == '/') && (strrchr (ccfoln, '/') == ccfoln))
		{
			CamelException subex = CAMEL_EXCEPTION_INITIALISER;

			/* g_static_rec_mutex_lock (cpriv->obs_lock); */
			_tny_camel_folder_freeup_observers (cfol, cpriv);
			/* g_static_rec_mutex_unlock (priv->obs_lock); */

			if (camel_store_supports_subscriptions (store))
				camel_store_subscribe_folder (store, cfolname, &subex);

			camel_store_delete_folder (store, cfolname, &ex);

			if (camel_exception_is_set (&ex))
			{
				_tny_camel_exception_to_tny_error (&ex, err);
				camel_exception_clear (&ex);
				changed = FALSE;
			} else 
			{
				if (camel_store_supports_subscriptions (store))
					camel_store_unsubscribe_folder (store, cfolname, &subex);

				changed = TRUE;

				/* g_free (cpriv->folder_name); 
				   cpriv->folder_name = NULL; */

				_tny_camel_store_account_remove_from_managed_folders (TNY_CAMEL_STORE_ACCOUNT (priv->account), cfol);
			}
		}
	}

	g_static_rec_mutex_unlock (cpriv->folder_lock);
	g_static_rec_mutex_unlock (priv->folder_lock);

	g_static_rec_mutex_unlock (apriv->factory_lock);


	if (changed)
		tny_folder_store_change_add_removed_folder (change, folder);

	return;
}

static GList*
recurse_remove (TnyFolderStore *from, TnyFolder *folder, GList *changes, GError **err)
{
	TnyCamelFolderPriv *fpriv = TNY_CAMEL_FOLDER_GET_PRIVATE (from);
	GError *nerr = NULL;
	TnyFolderStoreChange *change = NULL;

	g_static_rec_mutex_lock (fpriv->folder_lock);

	if (TNY_IS_FOLDER_STORE (folder))
	{
		TnyList *folders = tny_simple_list_new ();
		TnyIterator *iter;

		tny_folder_store_get_folders (TNY_FOLDER_STORE (folder),
				folders, NULL, TRUE, &nerr);

		if (nerr != NULL)
		{
			g_object_unref (folders);
			goto exception;
		}

		iter = tny_list_create_iterator (folders);
		while (!tny_iterator_is_done (iter))
		{
			TnyFolder *cur = TNY_FOLDER (tny_iterator_get_current (iter));

			changes = recurse_remove (TNY_FOLDER_STORE (folder), cur, changes, &nerr);

			if (nerr != NULL)
			{
				g_object_unref (cur);
				g_object_unref (iter);
				g_object_unref (folders);
				goto exception;
			}

			g_object_unref (cur);
			tny_iterator_next (iter);
		}
		g_object_unref (iter);
		g_object_unref (folders);
	}

	tny_debug ("tny_folder_store_remove: actual removal of %s\n", 
			tny_folder_get_name (folder));

	change = tny_folder_store_change_new (from);
	_tny_camel_folder_remove_folder_actual (from, folder, change, &nerr);
	changes = g_list_append (changes, change);

exception:

	if (nerr != NULL)
		g_propagate_error (err, nerr);

	g_static_rec_mutex_unlock (fpriv->folder_lock);

	return changes;
}

static void 
tny_camel_folder_remove_folder (TnyFolderStore *self, TnyFolder *folder, GError **err)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->remove_folder(self, folder, err);
}


typedef struct {
	GList *list;
	TnySessionCamel *session;
} NObsAbRem;

static gboolean
notify_folder_store_observers_about_remove_in_idle (gpointer user_data)
{
	NObsAbRem *info = (NObsAbRem *) user_data;
	GList *list = info->list;

	while (list) {
		TnyFolderStoreChange *change = (TnyFolderStoreChange *) list->data;
		TnyFolderStore *store = tny_folder_store_change_get_folder_store (change);
		notify_folder_store_observers_about (store, change, info->session);

		g_object_unref (store);
		g_object_unref (change);
		list = g_list_next (list);
	}
	return FALSE;
}

static void
notify_folder_store_observers_about_remove_in_idle_destroy (gpointer user_data)
{
	NObsAbRem *info = (NObsAbRem *) user_data;

	g_list_free (info->list);
	camel_object_unref (info->session);
	g_slice_free (NObsAbRem, info);
}

static void 
tny_camel_folder_remove_folder_default (TnyFolderStore *self, TnyFolder *folder, GError **err)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	GError *nerr = NULL;
	GList *changes = NULL;

	if (!_tny_session_check_operation (TNY_FOLDER_PRIV_GET_SESSION(priv), priv->account, err, 
			TNY_ERROR_DOMAIN, TNY_SERVICE_ERROR_FOLDER_REMOVE))
		return;

	if (!priv->account) {
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_FOLDER_REMOVE,
			_("Folder store not ready for removing folders"));
		return;
	}

	changes = recurse_remove (self, folder, changes, &nerr);

	if (changes) {
		NObsAbRem *info = g_slice_new (NObsAbRem);

		info->list = changes;
		info->session = TNY_FOLDER_PRIV_GET_SESSION (priv);
		camel_object_ref (info->session);

		g_idle_add_full (G_PRIORITY_HIGH, 
			notify_folder_store_observers_about_remove_in_idle, info,
			notify_folder_store_observers_about_remove_in_idle_destroy);
	}

	if (nerr != NULL)
		g_propagate_error (err, nerr);

	_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));

	return;
}

void
_tny_camel_folder_set_folder_info (TnyFolderStore *self, TnyCamelFolder *folder, CamelFolderInfo *info)
{
	if (!info) {
		g_critical ("Creating invalid folder. This indicates a problem "
			    "in the software\n");
		return;
	}

	_tny_camel_folder_set_id (folder, info->full_name);
	_tny_camel_folder_set_folder_type (folder, info);
	_tny_camel_folder_set_unread_count (folder, info->unread);
	_tny_camel_folder_set_all_count (folder, info->total);
	_tny_camel_folder_set_local_size (folder, info->local_size);

	if (!info->name)
		g_critical ("Creating invalid folder. This indicates a problem "
			    "in the software\n");

	_tny_camel_folder_set_name (folder, info->name);
	_tny_camel_folder_set_iter (folder, info);

	if (TNY_IS_CAMEL_FOLDER (self)) {
		TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
		_tny_camel_folder_set_account (folder, priv->account);
	} else if (TNY_IS_CAMEL_STORE_ACCOUNT (self)){
		_tny_camel_folder_set_account (folder, TNY_ACCOUNT (self));
	}

	_tny_camel_folder_set_parent (folder, self);
}

static TnyFolder*
tny_camel_folder_create_folder (TnyFolderStore *self, const gchar *name, GError **err)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->create_folder(self, name, err);
}


static TnyFolder*
tny_camel_folder_create_folder_default (TnyFolderStore *self, const gchar *name, GError **err)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;
	CamelStore *store; gchar *folname;
	TnyFolder *folder; CamelFolderInfo *info;
	TnyFolderStoreChange *change;
	CamelException subex = CAMEL_EXCEPTION_INITIALISER;
	gboolean was_new = FALSE;

	if (!_tny_session_check_operation (TNY_FOLDER_PRIV_GET_SESSION(priv), 
			priv->account, err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_FOLDER_CREATE))
		return NULL;

	if (!name || strlen (name) <= 0) {
		g_set_error (err, TNY_ERROR_DOMAIN,
				TNY_SERVICE_ERROR_FOLDER_CREATE,
				_("Failed to create folder with no name"));
		_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
		return NULL;
	}

	if (!priv->folder_name) {
		g_set_error (err, TNY_ERROR_DOMAIN,
				TNY_SERVICE_ERROR_FOLDER_CREATE,
				_("Failed to create folder. Invalid parent folder"));
		_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
		return NULL;
	}

	store = (CamelStore*) priv->store;

	g_assert (CAMEL_IS_STORE (store));

	g_static_rec_mutex_lock (priv->folder_lock);
	folname = priv->folder_name;
	info = camel_store_create_folder (store, priv->folder_name, name, &ex);
	g_static_rec_mutex_unlock (priv->folder_lock);

	if (!info || camel_exception_is_set (&ex) || !priv->account) {
		if (camel_exception_is_set (&ex)) {
			_tny_camel_exception_to_tny_error (&ex, err);
			camel_exception_clear (&ex);
		} else
			g_set_error (err, TNY_ERROR_DOMAIN,
				TNY_SERVICE_ERROR_FOLDER_CREATE,
				_("Unknown error while trying to create folder"));

		if (info)
			camel_store_free_folder_info (store, info);
		_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));
		return NULL;
	}

	g_assert (info != NULL);

	if (camel_store_supports_subscriptions (store))
		camel_store_subscribe_folder (store, info->full_name, &subex);

	folder = tny_camel_store_account_factor_folder  
		(TNY_CAMEL_STORE_ACCOUNT (priv->account), info->full_name, &was_new);

	_tny_camel_folder_set_folder_info (self, TNY_CAMEL_FOLDER (folder), info);
	_tny_camel_folder_set_parent (TNY_CAMEL_FOLDER (folder), TNY_FOLDER_STORE (self));


	/* So that the next call to get_folders includes the newly
	 * created folder */
	priv->iter = camel_store_get_folder_info (store, priv->folder_name, 0, &subex);

	change = tny_folder_store_change_new (self);
	tny_folder_store_change_add_created_folder (change, folder);
	notify_folder_store_observers_about_in_idle (self, change,
		TNY_FOLDER_PRIV_GET_SESSION (priv));
	g_object_unref (change);

	_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));

	return folder;
}

typedef struct 
{
	TnyCamelQueueable parent;

	GError *err;
	TnyFolderStore *self;
	gchar *name;
	TnyFolder *new_folder;
	TnyCreateFolderCallback callback;
	gpointer user_data;
	TnySessionCamel *session;
	gboolean cancelled;

} CreateFolderInfo;

static gpointer 
tny_camel_folder_create_folder_async_thread (gpointer thr_user_data)
{
	CreateFolderInfo *info = (CreateFolderInfo*) thr_user_data;

	info->new_folder = tny_camel_folder_create_folder (TNY_FOLDER_STORE (info->self), 
							   (const gchar *) info->name, 
							   &info->err);

	info->cancelled = FALSE;
	if (info->err != NULL) {
		if (camel_strstrcase (info->err->message, "cancel") != NULL)
			info->cancelled = TRUE;
	}

	return NULL;
}

static gboolean
tny_camel_folder_create_folder_async_callback (gpointer thr_user_data)
{
	CreateFolderInfo *info = thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, info->cancelled, info->new_folder, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}

static gboolean
tny_camel_folder_create_folder_async_cancelled_callback (gpointer thr_user_data)
{
	CreateFolderInfo *info = thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, TRUE, info->new_folder, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}

static void
tny_camel_folder_create_folder_async_destroyer (gpointer thr_user_data)
{
	CreateFolderInfo *info = (CreateFolderInfo *) thr_user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);

	/* thread reference */
	_tny_camel_folder_unreason (priv);
	g_object_unref (info->self);
	g_free (info->name);

	if (info->err)
		g_error_free (info->err);

	_tny_session_stop_operation (info->session);

	camel_object_unref (info->session);

	return;
}

static void
tny_camel_folder_create_folder_async (TnyFolderStore *self, const gchar *name, TnyCreateFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->create_folder_async(self, name, callback, status_callback, user_data);
}

static void
tny_camel_folder_create_folder_async_default (TnyFolderStore *self, const gchar *name, TnyCreateFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	CreateFolderInfo *info;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	/* Idle info for the callbacks */
	info = g_slice_new (CreateFolderInfo);
	info->session = TNY_FOLDER_PRIV_GET_SESSION (priv);
	camel_object_ref (info->session);
	info->self = self;
	info->name = g_strdup (name);
	info->callback = callback;
	info->user_data = user_data;
	info->new_folder = NULL;
	info->err = NULL;
	info->cancelled = FALSE;

	/* thread reference */
	_tny_camel_folder_reason (priv);
	g_object_ref (info->self);

	_tny_camel_queue_launch (TNY_FOLDER_PRIV_GET_QUEUE (priv), 
		tny_camel_folder_create_folder_async_thread, 
		tny_camel_folder_create_folder_async_callback,
		tny_camel_folder_create_folder_async_destroyer, 
		tny_camel_folder_create_folder_async_cancelled_callback,
		tny_camel_folder_create_folder_async_destroyer, 
		&info->cancelled, 
		info, sizeof (CreateFolderInfo),
		__FUNCTION__);

	return;
}

/* Sets a TnyFolderStore as the parent of a TnyCamelFolder. Note that
 * this code could cause a cross-reference situation, if the parent
 * was used to create the child. */

static void 
notify_parent_del (gpointer user_data, GObject *parent)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (user_data);
	priv->parent = NULL;
}


void
_tny_camel_folder_set_parent (TnyCamelFolder *self, TnyFolderStore *parent)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	if (priv->parent)
		g_object_weak_unref (G_OBJECT (priv->parent), notify_parent_del, self);
	g_object_weak_ref (G_OBJECT (parent), notify_parent_del, self);

	priv->parent = parent;
	return;
}

static void
_tny_camel_folder_guess_folder_type (TnyCamelFolder *folder, CamelFolderInfo *folder_info)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (folder);

	if (folder_info->name) {
		if (!g_ascii_strcasecmp (folder_info->name, "inbox")) {
			/* Needed as some dovecot servers report the inbox as
			 * normal */
			TnyFolderStore *store = tny_folder_get_folder_store (folder);
			if (store) {
				if (TNY_IS_ACCOUNT (store))
					priv->cached_folder_type = TNY_FOLDER_TYPE_INBOX;
				else
					priv->cached_folder_type = TNY_FOLDER_TYPE_NORMAL;
				g_object_unref (store);
			} else {
				priv->cached_folder_type = TNY_FOLDER_TYPE_NORMAL;
			}
		}
		else if (!g_ascii_strcasecmp (folder_info->name, "drafts")) {
			priv->cached_folder_type = TNY_FOLDER_TYPE_DRAFTS;
		} else if (!g_ascii_strcasecmp (folder_info->name, "sent")) {
			priv->cached_folder_type = TNY_FOLDER_TYPE_SENT;
		} else if (!g_ascii_strcasecmp (folder_info->name, "outbox")) {
			priv->cached_folder_type = TNY_FOLDER_TYPE_OUTBOX;
		} else {
			priv->cached_folder_type = TNY_FOLDER_TYPE_NORMAL;
		}
	} else {
		priv->cached_folder_type = TNY_FOLDER_TYPE_NORMAL;
	}
}

void 
_tny_camel_folder_set_folder_type (TnyCamelFolder *folder, CamelFolderInfo *folder_info)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (folder);

	if (!folder_info)
		priv->cached_folder_type = TNY_FOLDER_TYPE_NORMAL;
	else {
		switch (folder_info->flags & CAMEL_FOLDER_TYPE_MASK) 
		{
			case CAMEL_FOLDER_TYPE_INBOX:
				priv->cached_folder_type = TNY_FOLDER_TYPE_INBOX;
			break;
			case CAMEL_FOLDER_TYPE_OUTBOX:
				priv->cached_folder_type = TNY_FOLDER_TYPE_OUTBOX; 
			break;
			case CAMEL_FOLDER_TYPE_TRASH:
				priv->cached_folder_type = TNY_FOLDER_TYPE_TRASH; 
			break;
			case CAMEL_FOLDER_TYPE_JUNK:
				priv->cached_folder_type = TNY_FOLDER_TYPE_JUNK; 
			break;
			case CAMEL_FOLDER_TYPE_SENT:
				priv->cached_folder_type = TNY_FOLDER_TYPE_SENT; 
			break;
			default:
				_tny_camel_folder_guess_folder_type (folder, folder_info);
			break;
		}
	}
}

void 
_tny_camel_folder_set_iter (TnyCamelFolder *folder, CamelFolderInfo *iter)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (folder);

	priv->cant_reuse_iter = FALSE;
	priv->iter = iter;

	if (iter->flags & CAMEL_FOLDER_NOCHILDREN)
		priv->caps |= TNY_FOLDER_CAPS_NOCHILDREN;
	if (iter->flags & CAMEL_FOLDER_NOSELECT)
		priv->caps |= TNY_FOLDER_CAPS_NOSELECT;

	priv->iter_parented = TRUE;

	return;
}

static void
tny_camel_folder_get_folders (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, gboolean refresh, GError **err)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->get_folders(self, list, query, refresh, err);
}

static void 
tny_camel_folder_get_folders_default (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, gboolean refresh, GError **err)
{
	gboolean cant_reuse_iter = TRUE;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	CamelFolderInfo *iter;
	TnyAccount *account = NULL;

	if (!_tny_session_check_operation (TNY_FOLDER_PRIV_GET_SESSION(priv), 
			priv->account, err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_GET_FOLDERS))
		return;

	account = tny_folder_get_account (TNY_FOLDER (self));

	if (account) {
		TnyCamelStoreAccountPriv *aspriv = TNY_CAMEL_STORE_ACCOUNT_GET_PRIVATE (account);
		if (aspriv)
			cant_reuse_iter = aspriv->cant_reuse_iter;
		g_object_unref (account);
	}

	if (!cant_reuse_iter)
		cant_reuse_iter = priv->cant_reuse_iter;

	if (!cant_reuse_iter)
		cant_reuse_iter = refresh;

	if ((!priv->iter && priv->iter_parented) || cant_reuse_iter)
	{
		CamelStore *store = priv->store;
		CamelException ex = CAMEL_EXCEPTION_INITIALISER;

		g_return_if_fail (priv->folder_name != NULL);

		if (!refresh && CAMEL_IS_DISCO_STORE(store)) {
			priv->iter = CAMEL_DISCO_STORE_CLASS(CAMEL_OBJECT_GET_CLASS(store))->get_folder_info_offline(store,  priv->folder_name, 0, &ex);
		} else {
			priv->iter = camel_store_get_folder_info (store, priv->folder_name, 0, &ex);
		}

		priv->cant_reuse_iter = FALSE;

		if (camel_exception_is_set (&ex))
		{
			_tny_camel_exception_to_tny_error (&ex, err);
			camel_exception_clear (&ex);
			_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));

			if (priv->iter == NULL)
				return;
		}

		priv->iter_parented = FALSE;
	} 

	iter = priv->iter;
 
	if (iter)
	{
	  iter = iter->child;
	  while (iter)
	  {
		/* Also take a look at camel-maildir-store.c:525 */
		if (!(iter->flags & CAMEL_FOLDER_VIRTUAL) && _tny_folder_store_query_passes (query, iter) && priv->account)
		{
			gboolean was_new = FALSE;
			TnyCamelFolder *folder = (TnyCamelFolder *) tny_camel_store_account_factor_folder (
				TNY_CAMEL_STORE_ACCOUNT (priv->account), 
				iter->full_name, &was_new);
			if (was_new)
				_tny_camel_folder_set_folder_info (self, folder, iter);
			tny_list_prepend (list, G_OBJECT (folder));
			g_object_unref (folder);
		}
		iter = iter->next;
	  }
	}

#ifdef MERGEFOLDERTEST
	if (tny_list_get_length (list) > 1)
	{
		int i=0;
		TnyFolder *merge = tny_merge_folder_new ("MERGE TESTER");
		TnyIterator *iter = tny_list_create_iterator (list);
		g_warning ("CREATING MERGE TEST (%d)\n", tny_list_get_length (list) );
		while (!tny_iterator_is_done (iter) && i < 2) {
			GObject *obj = tny_iterator_get_current (iter);
			tny_merge_folder_add_folder (TNY_MERGE_FOLDER (merge), 
				TNY_FOLDER (obj));
			g_object_unref (obj);
			i++; tny_iterator_next (iter);
		}
		g_object_unref (iter);
		tny_list_prepend (list, (GObject *) merge);
	}
#endif

	_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));

	return;
}

typedef struct 
{
	TnyCamelQueueable parent;

	GError *err;
	TnyFolderStore *self;
	TnyList *list;
	TnyGetFoldersCallback callback;
	TnyFolderStoreQuery *query;
	gboolean refresh;
	gpointer user_data;
	TnySessionCamel *session;
	gboolean cancelled;

} GetFoldersInfo;


static void
tny_camel_folder_get_folders_async_destroyer (gpointer thr_user_data)
{
	GetFoldersInfo *info = thr_user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);

	/* thread reference */
	_tny_camel_folder_unreason (priv);
	g_object_unref (info->self);
	g_object_unref (info->list);

	if (info->query)
		g_object_unref (G_OBJECT (info->query));

	if (info->err)
		g_error_free (info->err);

	_tny_session_stop_operation (info->session);

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_get_folders_async_callback (gpointer thr_user_data)
{
	GetFoldersInfo *info = thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, info->cancelled, info->list, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}


static gpointer 
tny_camel_folder_get_folders_async_thread (gpointer thr_user_data)
{
	GetFoldersInfo *info = (GetFoldersInfo*) thr_user_data;

	tny_folder_store_get_folders (TNY_FOLDER_STORE (info->self),
		info->list, info->query, info->refresh, &info->err);

	info->cancelled = FALSE;
	if (info->err != NULL) {
		if (camel_strstrcase (info->err->message, "cancel") != NULL)
			info->cancelled = TRUE;
	}

	return NULL;
}

static void
tny_camel_folder_get_folders_async_cancelled_destroyer (gpointer thr_user_data)
{
	GetFoldersInfo *info = thr_user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);

	/* thread reference */
	_tny_camel_folder_unreason (priv);
	g_object_unref (info->self);
	g_object_unref (info->list);

	if (info->err)
		g_error_free (info->err);
	if (info->query)
		g_object_unref (info->query);

	/**/

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_get_folders_async_cancelled_callback (gpointer thr_user_data)
{
	GetFoldersInfo *info = thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, TRUE, info->list, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}

static void 
tny_camel_folder_get_folders_async (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, gboolean refresh, TnyGetFoldersCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->get_folders_async(self, list, query, refresh, callback, status_callback, user_data);
}


static void 
tny_camel_folder_get_folders_async_default (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, gboolean refresh, TnyGetFoldersCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	GetFoldersInfo *info;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	/* Idle info for the callbacks */
	info = g_slice_new (GetFoldersInfo);
	info->session = TNY_FOLDER_PRIV_GET_SESSION (priv);
	camel_object_ref (info->session);
	info->self = self;
	info->list = list;
	info->callback = callback;
	info->user_data = user_data;
	info->query = query;
	info->refresh = refresh;
	info->err = NULL;
	info->cancelled = FALSE;

	/* thread reference */
	_tny_camel_folder_reason (priv);
	g_object_ref (info->self);
	g_object_ref (info->list);
	if (info->query)
		g_object_ref (G_OBJECT (info->query));

	_tny_camel_queue_launch (TNY_FOLDER_PRIV_GET_QUEUE (priv), 
		tny_camel_folder_get_folders_async_thread, 
		tny_camel_folder_get_folders_async_callback,
		tny_camel_folder_get_folders_async_destroyer, 
		tny_camel_folder_get_folders_async_cancelled_callback,
		tny_camel_folder_get_folders_async_cancelled_destroyer, 
		&info->cancelled, 
		info, sizeof (GetFoldersInfo),
		__FUNCTION__);

	return;
}

static void
tny_camel_folder_store_refresh (TnyFolderStore *self, GError **err)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	CamelFolderInfo *iter;
	TnyAccount *account = NULL;
	CamelStore *store = priv->store;
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;

	if (!_tny_session_check_operation (TNY_FOLDER_PRIV_GET_SESSION(priv), 
			priv->account, err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_GET_FOLDERS))
		return;

	account = tny_folder_get_account (TNY_FOLDER (self));

	g_return_if_fail (priv->folder_name != NULL);

	priv->iter = camel_store_get_folder_info (store, priv->folder_name, 0, &ex);
	priv->cant_reuse_iter = FALSE;

	if (camel_exception_is_set (&ex))
	{
		_tny_camel_exception_to_tny_error (&ex, err);
		camel_exception_clear (&ex);
		_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));

		if (priv->iter == NULL)
			return;
	}

	priv->iter_parented = FALSE;

	iter = priv->iter;
 
	if (iter)
	{
	  iter = iter->child;
	  while (iter)
	  {
		/* Also take a look at camel-maildir-store.c:525 */
		if (!(iter->flags & CAMEL_FOLDER_VIRTUAL) && priv->account)
		{
			gboolean was_new = FALSE;
			TnyCamelFolder *folder = (TnyCamelFolder *) tny_camel_store_account_factor_folder (
				TNY_CAMEL_STORE_ACCOUNT (priv->account),
				iter->full_name, &was_new);
			if (was_new) {
				TnyFolderStoreChange *change;
				_tny_camel_folder_set_folder_info (self, folder, iter);
				change = tny_folder_store_change_new (TNY_FOLDER_STORE(self));
				tny_folder_store_change_add_created_folder (change, TNY_FOLDER(folder));
				notify_folder_store_observers_about_in_idle (self,
					change,
					TNY_FOLDER_PRIV_GET_SESSION (priv));
				g_object_unref(change);
			}
			g_object_unref (folder);
		}
		iter = iter->next;
	  }
	}


	_tny_session_stop_operation (TNY_FOLDER_PRIV_GET_SESSION (priv));

	return;
}

typedef struct
{
	TnyCamelQueueable parent;

	GError *err;
	TnyFolderStore *self;
	TnyFolderStoreCallback callback;
	gpointer user_data;
	TnySessionCamel *session;
	gboolean cancelled;
} StoreRefreshInfo;


static void
tny_camel_folder_store_refresh_async_destroyer (gpointer thr_user_data)
{
	StoreRefreshInfo *info = thr_user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);

	/* thread reference */
	_tny_camel_folder_unreason (priv);
	g_object_unref (info->self);

	if (info->err)
		g_error_free (info->err);

	_tny_session_stop_operation (info->session);

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_store_refresh_async_callback (gpointer thr_user_data)
{
	StoreRefreshInfo *info = thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, info->cancelled, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}


static gpointer 
tny_camel_folder_store_refresh_async_thread (gpointer thr_user_data)
{
	StoreRefreshInfo *info = (StoreRefreshInfo*) thr_user_data;

	tny_camel_folder_store_refresh (TNY_FOLDER_STORE (info->self), &info->err);

	info->cancelled = FALSE;
	if (info->err != NULL) {
		if (camel_strstrcase (info->err->message, "cancel") != NULL)
			info->cancelled = TRUE;
	}

	return NULL;
}

static void
tny_camel_folder_store_refresh_async_cancelled_destroyer (gpointer thr_user_data)
{
	StoreRefreshInfo *info = thr_user_data;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (info->self);

	/* thread reference */
	_tny_camel_folder_unreason (priv);
	g_object_unref (info->self);

	if (info->err)
		g_error_free (info->err);

	/**/

	camel_object_unref (info->session);

	return;
}

static gboolean
tny_camel_folder_store_refresh_async_cancelled_callback (gpointer thr_user_data)
{
	StoreRefreshInfo *info = thr_user_data;
	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, TRUE, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}

static void
tny_camel_folder_store_refresh_async (TnyFolderStore *self, TnyFolderStoreCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	StoreRefreshInfo *info;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	/* Idle info for the callbacks */
	info = g_slice_new (StoreRefreshInfo);
	info->session = TNY_FOLDER_PRIV_GET_SESSION (priv);
	camel_object_ref (info->session);
	info->self = self;
	info->callback = callback;
	info->user_data = user_data;
	info->err = NULL;
	info->cancelled = FALSE;

	/* thread reference */
	_tny_camel_folder_reason (priv);
	g_object_ref (info->self);

	_tny_camel_queue_launch (TNY_FOLDER_PRIV_GET_QUEUE (priv),
		tny_camel_folder_store_refresh_async_thread,
		tny_camel_folder_store_refresh_async_callback,
		tny_camel_folder_store_refresh_async_destroyer,
		tny_camel_folder_store_refresh_async_cancelled_callback,
		tny_camel_folder_store_refresh_async_cancelled_destroyer,
		&info->cancelled,
		info, sizeof (StoreRefreshInfo),
		__FUNCTION__);

	return;
}

void
_tny_camel_folder_set_folder (TnyCamelFolder *self, CamelFolder *camel_folder)
{
	_tny_camel_folder_set_id (self, camel_folder_get_full_name (camel_folder));

	return;
}

/**
 * tny_camel_folder_get_full_name:
 * @self: A #TnyCamelFolder object
 *
 * Get a camel-lite specific full name of the folder
 *
 * Return value: The full name of the folder
 **/
const gchar*
tny_camel_folder_get_full_name (TnyCamelFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	return priv->folder_name;
}


CamelFolder*
_tny_camel_folder_get_folder (TnyCamelFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	CamelFolder *retval = NULL;

	if (G_UNLIKELY (!priv->loaded))
		if (!load_folder (priv))
			return NULL;

	retval = priv->folder;
	if (retval)
		camel_object_ref (CAMEL_OBJECT (retval));

	return retval;
}


static void 
tny_camel_folder_poke_status (TnyFolder *self)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->poke_status(self);
	return;
}

typedef struct {
	TnyCamelQueueable parent;

	TnyFolder *self;
	gint unread;
	gint total;
	gboolean do_status;
	gboolean cancelled;
	TnySessionCamel *session;
} PokeStatusInfo;

static gboolean
tny_camel_folder_poke_status_callback (gpointer data)
{
	PokeStatusInfo *info = (PokeStatusInfo *) data;
	TnyFolder *self = (TnyFolder *) info->self;
	TnyFolderChange *change = NULL;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	priv->ongoing_poke_status = FALSE;

	if (info->cancelled)
		return FALSE;

	if (info->total != -1) {
		priv->cached_length = (guint) info->total;
		if (!change)
			change = tny_folder_change_new (self);
		tny_folder_change_set_new_all_count (change, priv->cached_length);
	}
	if (info->unread != -1) {
		priv->unread_length = (guint) info->unread;
		if (!change)
			change = tny_folder_change_new (self);
		tny_folder_change_set_new_unread_count (change, priv->unread_length);
	}

	if (change) {
		notify_folder_observers_about (self, change, info->session);
		g_object_unref (change);
	}

	return FALSE;
}

static void
tny_camel_folder_poke_status_destroyer (gpointer data)
{
	PokeStatusInfo *info = (PokeStatusInfo *) data;
	/* Thread reference */
	g_object_unref (info->self);

	/**/

	camel_object_unref (info->session);

	return;
}


static gpointer
tny_camel_folder_poke_status_thread (gpointer user_data)
{
	PokeStatusInfo *info = (PokeStatusInfo *) user_data;
	TnyFolder *folder = info->self;
	TnyCamelFolderPriv *priv = NULL;
	CamelStore *store = NULL;
	int newlen = -1, newurlen = -1, uidnext = -1;

	priv = TNY_CAMEL_FOLDER_GET_PRIVATE (folder);
	store = priv->store;

	if (info->do_status && ((CamelService *) store)->status == CAMEL_SERVICE_CONNECTED) {
		camel_store_get_folder_status (store, priv->folder_name, 
			&newurlen, &newlen, &uidnext);
	}

	if (newurlen == -1 || newlen == -1)
	{
		if (priv->iter) {
			info->unread = priv->iter->unread;
			info->total = priv->iter->total;
		} else {
			info->unread = priv->unread_length;
			info->total = priv->cached_length;
		}
	} else {
		info->unread = newurlen;
		info->total = newlen;

		priv->unread_length = newurlen;
		priv->cached_length = newlen;
		update_iter_counts (priv);
	}

	return NULL;
}


static void 
tny_camel_folder_poke_status_default (TnyFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	CamelStore *store;
	PokeStatusInfo *info;

	if (priv->ongoing_poke_status)
		return;

	store = priv->store;
	info = g_slice_new (PokeStatusInfo);

	priv->ongoing_poke_status = TRUE;

	info->do_status = FALSE;
	/* Thread reference */
	info->self = TNY_FOLDER (g_object_ref (self));
	info->session = TNY_FOLDER_PRIV_GET_SESSION (priv);
	info->cancelled = FALSE;
	camel_object_ref (info->session);

	if (priv->folder)
	{
		info->unread = camel_folder_get_unread_message_count (priv->folder);
		info->total = camel_folder_get_message_count (priv->folder);

	} else {

		if (store && CAMEL_IS_DISCO_STORE (store)  && priv->folder_name 
			&& camel_disco_store_status (CAMEL_DISCO_STORE (store)) == CAMEL_DISCO_STORE_ONLINE)
		{
			info->do_status = TRUE;
		} else {
			if (priv->iter) {
				info->unread = priv->iter->unread;
				info->total = priv->iter->total;
			}
		}
	}

	_tny_camel_queue_launch_wflags (TNY_FOLDER_PRIV_GET_QUEUE (priv), 
		tny_camel_folder_poke_status_thread, 
		tny_camel_folder_poke_status_callback, 
		tny_camel_folder_poke_status_destroyer, 
		tny_camel_folder_poke_status_callback, 
		tny_camel_folder_poke_status_destroyer, 
		&info->cancelled,
		info, sizeof (PokeStatusInfo),
		TNY_CAMEL_QUEUE_CANCELLABLE_ITEM,
		__FUNCTION__);

	return;
}

static TnyFolderStore*  
tny_camel_folder_get_folder_store (TnyFolder *self)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->get_folder_store(self);
}

static TnyFolderStore*  
tny_camel_folder_get_folder_store_default (TnyFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	if (priv->parent)
		g_object_ref (priv->parent);

	return priv->parent;
}


static void
tny_camel_folder_add_observer (TnyFolder *self, TnyFolderObserver *observer)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->add_observer(self, observer);
}

static void 
notify_observer_del (gpointer user_data, GObject *observer)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (user_data);
	g_static_rec_mutex_lock (priv->obs_lock);
	priv->obs = g_list_remove (priv->obs, observer);
	g_static_rec_mutex_unlock (priv->obs_lock);
}

static void
tny_camel_folder_add_observer_default (TnyFolder *self, TnyFolderObserver *observer)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	g_assert (TNY_IS_FOLDER_OBSERVER (observer));

	g_static_rec_mutex_lock (priv->obs_lock);
	if (!g_list_find (priv->obs, observer)) {
		priv->obs = g_list_prepend (priv->obs, observer);
		g_object_weak_ref (G_OBJECT (observer), notify_observer_del, self);
	}
	g_static_rec_mutex_unlock (priv->obs_lock);

	return;
}


static void
tny_camel_folder_remove_observer (TnyFolder *self, TnyFolderObserver *observer)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->remove_observer(self, observer);
}

static void
tny_camel_folder_remove_observer_default (TnyFolder *self, TnyFolderObserver *observer)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	GList *found = NULL;

	g_assert (TNY_IS_FOLDER_OBSERVER (observer));

	g_static_rec_mutex_lock (priv->obs_lock);

	if (!priv->obs) {
		g_static_rec_mutex_unlock (priv->obs_lock);
		return;
	}

	found = g_list_find (priv->obs, observer);
	if (found) {
		priv->obs = g_list_remove_link (priv->obs, found);
		g_object_weak_unref (found->data, notify_observer_del, self);
		g_list_free (found);
	}

	g_static_rec_mutex_unlock (priv->obs_lock);

	return;
}


static TnyFolderStats *
tny_camel_folder_get_stats (TnyFolder *self)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->get_stats(self);
}

static TnyFolderStats * 
tny_camel_folder_get_stats_default (TnyFolder *self)
{
	TnyFolderStats *retval = NULL;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	if (!load_folder (priv))
		return NULL;

	retval = tny_folder_stats_new (self);

	priv->unread_length = camel_folder_get_unread_message_count (priv->folder);
	priv->cached_length = camel_folder_get_message_count (priv->folder);
	update_iter_counts (priv);
	priv->local_size = camel_folder_get_local_size (priv->folder);
	tny_folder_stats_set_local_size (retval, priv->local_size);

	return retval;
}

static void
tny_camel_folder_store_add_observer (TnyFolderStore *self, TnyFolderStoreObserver *observer)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->add_store_observer(self, observer);
}


static void 
notify_store_observer_del (gpointer user_data, GObject *observer)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (user_data);
	g_static_rec_mutex_lock (priv->obs_lock);
	priv->sobs = g_list_remove (priv->sobs, observer);
	g_static_rec_mutex_unlock (priv->obs_lock);
}

static void
tny_camel_folder_store_add_observer_default (TnyFolderStore *self, TnyFolderStoreObserver *observer)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	g_assert (TNY_IS_FOLDER_STORE_OBSERVER (observer));

	g_static_rec_mutex_lock (priv->obs_lock);
	if (!g_list_find (priv->sobs, observer)) {
		priv->sobs = g_list_prepend (priv->sobs, observer);
		g_object_weak_ref (G_OBJECT (observer), notify_store_observer_del, self);
	}
	g_static_rec_mutex_unlock (priv->obs_lock);

	return;
}


static void
tny_camel_folder_store_remove_observer (TnyFolderStore *self, TnyFolderStoreObserver *observer)
{
	TNY_CAMEL_FOLDER_GET_CLASS (self)->remove_store_observer(self, observer);
}

static void
tny_camel_folder_store_remove_observer_default (TnyFolderStore *self, TnyFolderStoreObserver *observer)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	GList *found = NULL;

	g_assert (TNY_IS_FOLDER_STORE_OBSERVER (observer));

	g_static_rec_mutex_lock (priv->obs_lock);

	if (!priv->sobs) {
		g_static_rec_mutex_unlock (priv->obs_lock);
		return;
	}

	found = g_list_find (priv->sobs, observer);
	if (found) {
		priv->sobs = g_list_remove_link (priv->sobs, found);
		g_object_weak_unref (found->data, notify_store_observer_del, self);
		g_list_free (found);
	}

	g_static_rec_mutex_unlock (priv->obs_lock);

	return;
}


static gchar* 
tny_camel_folder_get_url_string (TnyFolder *self)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->get_url_string(self);
}

static gchar* 
tny_camel_folder_get_url_string_default (TnyFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	gchar *retval = NULL;

	/* iter->uri is a cache.
	 * Check for strlen(), because camel produces an empty (non-null) 
	 * uri for POP. */

	if (priv->iter && priv->iter->uri && (strlen (priv->iter->uri) > 0)) {

		retval = g_strdup_printf ("%s", priv->iter->uri);

	} else if (priv->account) {
		TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (priv->account);
		if (apriv->service) {
			const char *foln;
			char *urls = camel_service_get_url (apriv->service);
			if (!priv->folder)
				load_folder_no_lock (priv);
			foln = camel_folder_get_full_name (priv->folder);
			retval = g_strdup_printf ("%s/%s", urls, foln);
			g_free (urls);
		}
	}

	if (!retval) { /* Strange, a local one? */
		g_warning ("tny_folder_get_url_string does not have an "
				"iter nor account. Using maildir as type.\n");
		retval = g_strdup_printf ("maildir://%s", priv->folder_name);
	}

	return retval;
}

static TnyFolderCaps 
tny_camel_folder_get_caps (TnyFolder *self)
{
	return TNY_CAMEL_FOLDER_GET_CLASS (self)->get_caps(self);
}

static TnyFolderCaps 
tny_camel_folder_get_caps_default (TnyFolder *self)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	return priv->caps;
}

TnyFolder*
_tny_camel_folder_new_with_folder (CamelFolder *camel_folder)
{
	TnyCamelFolder *self = g_object_new (TNY_TYPE_CAMEL_FOLDER, NULL);

	_tny_camel_folder_set_folder (self, camel_folder);

	return TNY_FOLDER (self);
}



TnyFolder*
_tny_camel_folder_new (void)
{
	TnyCamelFolder *self = g_object_new (TNY_TYPE_CAMEL_FOLDER, NULL);

	return TNY_FOLDER (self);
}

void 
_tny_camel_folder_freeup_observers (TnyCamelFolder *self, TnyCamelFolderPriv *priv)
{
	/* Commented because they should not be really needed but some
	   times they're causing locking problems. TODO: review the
	   source of this behaviour */

	g_static_rec_mutex_lock (priv->obs_lock);
	if (priv->obs) {
		GList *copy = priv->obs;
		while (copy) {
			g_object_weak_unref ((GObject *) copy->data, notify_observer_del, self);
			copy = g_list_next (copy);
		}
		g_list_free (priv->obs);
		priv->obs = NULL;
	}

	if (priv->sobs) {
		GList *copy = priv->sobs;
		while (copy) {
			g_object_weak_unref ((GObject *) copy->data, notify_store_observer_del, self);
			copy = g_list_next (copy);
		}
		g_list_free (priv->sobs);
		priv->sobs = NULL;
	}
	g_static_rec_mutex_unlock (priv->obs_lock);
}

static void
tny_camel_folder_dispose (GObject *object)
{
	TnyCamelFolder *self = (TnyCamelFolder*) object;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	if (priv->store) {
		camel_object_unref (priv->store);
		priv->store = NULL;
	}

	if (priv->account && TNY_IS_CAMEL_STORE_ACCOUNT (priv->account)) {
		_tny_camel_store_account_remove_from_managed_folders (TNY_CAMEL_STORE_ACCOUNT (priv->account), 
								      self);
	}

#ifdef ACCOUNT_WEAK_REF
	if (priv->account) {
		g_object_weak_unref (G_OBJECT (priv->account), notify_account_del, self);
		priv->account = NULL;
	}
#else
	if (priv->account) {
		g_object_unref (priv->account);
		priv->account = NULL;
	}
#endif

	_tny_camel_folder_freeup_observers (self, priv);

	g_static_rec_mutex_lock (priv->folder_lock);
	priv->dont_fkill = FALSE;

	if (!priv->iter_parented && priv->iter) {
		CamelStore *store = priv->store;
		camel_store_free_folder_info (store, priv->iter);
	}

	unload_folder_no_lock (priv, TRUE);

	if (priv->folder) {
		camel_object_unref (priv->folder);
		priv->folder = NULL;
	}

	if (priv->remove_strat) {
		g_object_unref (G_OBJECT (priv->remove_strat));
		priv->remove_strat = NULL;
	}

	if (priv->receive_strat) {
		g_object_unref (G_OBJECT (priv->receive_strat));
		priv->receive_strat = NULL;
	}

	if (priv->parent) {
		g_object_weak_unref (G_OBJECT (priv->parent), notify_parent_del, self);
		priv->parent = NULL;
	}

	g_static_rec_mutex_unlock (priv->folder_lock);

	return;
}


static void
tny_camel_folder_finalize (GObject *object)
{
	TnyCamelFolder *self = (TnyCamelFolder*) object;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	if (priv->folder_tracking) {
		camel_object_remove_event (priv->folder_tracking, priv->folder_tracking_id);
		camel_object_unhook_event (priv->folder_tracking, "finalize", folder_tracking_finalize, priv);
		priv->folder_tracking_id = 0;
	}

#ifdef DEBUG
	g_print ("Finalizing TnyCamelFolder: %s\n", 
		priv->folder_name?priv->folder_name:"(cleared)");

	if (priv->reason_to_live != 0)
		g_print ("Finalizing TnyCamelFolder, yet TnyHeader instances "
		"are still alive: %d\n", priv->reason_to_live);
#endif

	g_static_rec_mutex_lock (priv->folder_lock);
	priv->dont_fkill = FALSE;

	if (priv->account && TNY_IS_CAMEL_STORE_ACCOUNT (priv->account)) {
		_tny_camel_store_account_remove_from_managed_folders (TNY_CAMEL_STORE_ACCOUNT (priv->account), 
								      self);
	}

	if (G_LIKELY (priv->folder))
	{
		camel_object_unref (priv->folder);
		priv->folder = NULL;
	}

	if (G_LIKELY (priv->cached_name))
		g_free (priv->cached_name);
	priv->cached_name = NULL;

	g_static_rec_mutex_unlock (priv->folder_lock);

	/* g_static_rec_mutex_free (priv->folder_lock); */
	g_free (priv->folder_lock);
	priv->folder_lock = NULL;

	/* g_static_rec_mutex_free (priv->obs_lock); */
	g_free (priv->obs_lock);
	priv->obs_lock = NULL;

	g_free (priv->reason_lock);
	priv->reason_lock = NULL;

	if (priv->folder_name)
		g_free (priv->folder_name);
	priv->folder_name = NULL;

	(*parent_class->finalize) (object);

	return;
}

static void
tny_folder_init (gpointer g, gpointer iface_data)
{
	TnyFolderIface *klass = (TnyFolderIface *)g;

	klass->add_msg_async= tny_camel_folder_add_msg_async;
	klass->get_msg_remove_strategy= tny_camel_folder_get_msg_remove_strategy;
	klass->set_msg_remove_strategy= tny_camel_folder_set_msg_remove_strategy;
	klass->get_msg_receive_strategy= tny_camel_folder_get_msg_receive_strategy;
	klass->set_msg_receive_strategy= tny_camel_folder_set_msg_receive_strategy;
	klass->get_headers= tny_camel_folder_get_headers;
	klass->get_headers_async= tny_camel_folder_get_headers_async;
	klass->get_msg= tny_camel_folder_get_msg;
	klass->find_msg= tny_camel_folder_find_msg;
	klass->get_msg_async= tny_camel_folder_get_msg_async;
	klass->find_msg_async= tny_camel_folder_find_msg_async;
	klass->get_id= tny_camel_folder_get_id;
	klass->get_name= tny_camel_folder_get_name;
	klass->get_folder_type= tny_camel_folder_get_folder_type;
	klass->get_unread_count= tny_camel_folder_get_unread_count;
	klass->get_local_size= tny_camel_folder_get_local_size;
	klass->get_all_count= tny_camel_folder_get_all_count;
	klass->get_account= tny_camel_folder_get_account;
	klass->is_subscribed= tny_camel_folder_is_subscribed;
	klass->refresh_async= tny_camel_folder_refresh_async;
	klass->refresh= tny_camel_folder_refresh;
	klass->remove_msg= tny_camel_folder_remove_msg;
	klass->remove_msgs= tny_camel_folder_remove_msgs;
	klass->sync= tny_camel_folder_sync;
	klass->sync_async= tny_camel_folder_sync_async;
	klass->add_msg= tny_camel_folder_add_msg;
	klass->transfer_msgs= tny_camel_folder_transfer_msgs;
	klass->transfer_msgs_async= tny_camel_folder_transfer_msgs_async;
	klass->copy= tny_camel_folder_copy;
	klass->copy_async= tny_camel_folder_copy_async;
	klass->poke_status= tny_camel_folder_poke_status;
	klass->add_observer= tny_camel_folder_add_observer;
	klass->remove_observer= tny_camel_folder_remove_observer;
	klass->get_folder_store= tny_camel_folder_get_folder_store;
	klass->get_stats= tny_camel_folder_get_stats;
	klass->get_url_string= tny_camel_folder_get_url_string;
	klass->get_caps= tny_camel_folder_get_caps;
	klass->remove_msgs_async= tny_camel_folder_remove_msgs_async;

	return;
}

static void
tny_folder_store_init (gpointer g, gpointer iface_data)
{
	TnyFolderStoreIface *klass = (TnyFolderStoreIface *)g;

	klass->remove_folder= tny_camel_folder_remove_folder;
	klass->create_folder= tny_camel_folder_create_folder;
	klass->create_folder_async= tny_camel_folder_create_folder_async;
	klass->get_folders= tny_camel_folder_get_folders;
	klass->get_folders_async= tny_camel_folder_get_folders_async;
	klass->add_observer= tny_camel_folder_store_add_observer;
	klass->remove_observer= tny_camel_folder_store_remove_observer;
	klass->refresh_async = tny_camel_folder_store_refresh_async;

	return;
}

static void 
tny_camel_folder_class_init (TnyCamelFolderClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	object_class->dispose = tny_camel_folder_dispose;
	object_class->finalize = tny_camel_folder_finalize;

	class->add_msg_async= tny_camel_folder_add_msg_async_default;
	class->get_msg_receive_strategy= tny_camel_folder_get_msg_receive_strategy_default;
	class->set_msg_receive_strategy= tny_camel_folder_set_msg_receive_strategy_default;
	class->get_msg_remove_strategy= tny_camel_folder_get_msg_remove_strategy_default;
	class->set_msg_remove_strategy= tny_camel_folder_set_msg_remove_strategy_default;
	class->get_headers= tny_camel_folder_get_headers_default;
	class->get_headers_async= tny_camel_folder_get_headers_async_default;
	class->get_msg= tny_camel_folder_get_msg_default;
	class->find_msg= tny_camel_folder_find_msg_default;
	class->get_msg_async= tny_camel_folder_get_msg_async_default;
	class->find_msg_async= tny_camel_folder_find_msg_async_default;
	class->get_id= tny_camel_folder_get_id_default;
	class->get_name= tny_camel_folder_get_name_default;
	class->get_folder_type= tny_camel_folder_get_folder_type_default;
	class->get_unread_count= tny_camel_folder_get_unread_count_default;
	class->get_local_size= tny_camel_folder_get_local_size_default;
	class->get_all_count= tny_camel_folder_get_all_count_default;
	class->get_account= tny_camel_folder_get_account_default;
	class->is_subscribed= tny_camel_folder_is_subscribed_default;
	class->refresh_async= tny_camel_folder_refresh_async_default;
	class->refresh= tny_camel_folder_refresh_default;
	class->remove_msg= tny_camel_folder_remove_msg_default;
	class->remove_msgs= tny_camel_folder_remove_msgs_default;
	class->add_msg= tny_camel_folder_add_msg_default;
	class->sync= tny_camel_folder_sync_default;
	class->sync_async= tny_camel_folder_sync_async_default;
	class->transfer_msgs= tny_camel_folder_transfer_msgs_default;
	class->transfer_msgs_async= tny_camel_folder_transfer_msgs_async_default;
	class->copy= tny_camel_folder_copy_default;
	class->copy_async= tny_camel_folder_copy_async_default;
	class->poke_status= tny_camel_folder_poke_status_default;
	class->add_observer= tny_camel_folder_add_observer_default;
	class->remove_observer= tny_camel_folder_remove_observer_default;
	class->get_folder_store= tny_camel_folder_get_folder_store_default;
	class->get_stats= tny_camel_folder_get_stats_default;
	class->get_url_string= tny_camel_folder_get_url_string_default;
	class->get_caps= tny_camel_folder_get_caps_default;
	class->remove_msgs_async= tny_camel_folder_remove_msgs_async_default;

	class->get_folders_async= tny_camel_folder_get_folders_async_default;
	class->get_folders= tny_camel_folder_get_folders_default;
	class->create_folder= tny_camel_folder_create_folder_default;
	class->create_folder_async= tny_camel_folder_create_folder_async_default;
	class->remove_folder= tny_camel_folder_remove_folder_default;
	class->add_store_observer= tny_camel_folder_store_add_observer_default;
	class->remove_store_observer= tny_camel_folder_store_remove_observer_default;

	g_type_class_add_private (object_class, sizeof (TnyCamelFolderPriv));

	return;
}



/**
 * tny_camel_folder_set_strict_retrieval:
 * @self: a #TnyCamelFolder instance
 * @setting: whether or not to enforce strict retrieval
 * 
 * API WARNING: This API might change
 *
 * Sets whether or not the message retrieve strategies need to strictly enforce
 * the retrieval type. For example in case of a partial retrieval strategy,
 * enforce a removal of a full previously retrieved message and retrieve a
 * new message. In case of a full retrieval strategy and a partial cache, remove
 * the partial cache and retrieve the message again.
 *
 **/
void 
tny_camel_folder_set_strict_retrieval (TnyCamelFolder *self, gboolean setting)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);
	priv->strict_retrieval = setting;
}

static void
tny_camel_folder_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyCamelFolder *self = (TnyCamelFolder *)instance;
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (self);

	priv->cant_reuse_iter = TRUE;
	priv->unread_read = FALSE;
	priv->account = NULL;
	priv->store = NULL;
	priv->parent = NULL;
	priv->strict_retrieval = FALSE;
	priv->self = (TnyFolder *) self;
	priv->want_changes = TRUE;
	priv->handle_changes = TRUE;
	priv->caps = 0;
	priv->local_size = 0;
	priv->unread_sync = 0;
	priv->dont_fkill = FALSE;
	priv->obs = NULL;
	priv->sobs = NULL;
	priv->iter = NULL;
	priv->iter_parented = FALSE;
	priv->reason_to_live = 0;
	priv->loaded = FALSE;
	priv->folder_changed_id = 0;
	priv->folder = NULL;
	priv->folder_tracking = NULL;
	priv->folder_tracking_id = 0;
	priv->cached_name = NULL;
	priv->cached_folder_type = TNY_FOLDER_TYPE_UNKNOWN;
	priv->remove_strat = tny_camel_msg_remove_strategy_new ();
	priv->receive_strat = tny_camel_full_msg_receive_strategy_new ();
	priv->reason_lock = g_new0 (GStaticRecMutex, 1);
	g_static_rec_mutex_init (priv->reason_lock);

	priv->folder_lock = g_new0 (GStaticRecMutex, 1);
	g_static_rec_mutex_init (priv->folder_lock);
	priv->obs_lock = g_new0 (GStaticRecMutex, 1);
	g_static_rec_mutex_init (priv->obs_lock);

	priv->ongoing_poke_status = FALSE;

	return;
}

static gpointer 
tny_camel_folder_register_type (gpointer notused)
{
	GType type = 0;
	
	static const GTypeInfo info = 
		{
			sizeof (TnyCamelFolderClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_folder_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelFolder),
			0,      /* n_preallocs */
			tny_camel_folder_instance_init    /* instance_init */
		};
	
	static const GInterfaceInfo tny_folder_info = 
		{
			(GInterfaceInitFunc) tny_folder_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	static const GInterfaceInfo tny_folder_store_info = 
		{
			(GInterfaceInitFunc) tny_folder_store_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyCamelFolder",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_FOLDER_STORE, 
				     &tny_folder_store_info);
	
	g_type_add_interface_static (type, TNY_TYPE_FOLDER, 
				     &tny_folder_info);

	return GSIZE_TO_POINTER (type);
}

/**
 * tny_camel_folder_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
GType 
tny_camel_folder_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);

		camel_type_init ();
		_camel_type_init_done = TRUE;
	}

	g_once (&once, tny_camel_folder_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
