#ifndef TNY_CAMEL_FOLDER_PRIV_H
#define TNY_CAMEL_FOLDER_PRIV_H

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
#include <glib.h>
#include <camel/camel-folder.h>
#include <camel/camel-store.h>
#include <tny-account.h>
#include <tny-folder.h>

typedef struct _TnyCamelFolderPriv TnyCamelFolderPriv;

struct _TnyCamelFolderPriv
{
	gboolean loaded;
	GType headers_list_type;
	guint folder_changed_id;
	guint reason_to_live;
	GStaticRecMutex *reason_lock;
	GStaticRecMutex *folder_lock, *obs_lock;
	CamelFolder *folder;
	CamelFolder *folder_tracking;
	guint folder_tracking_id;
	gchar *folder_name;
	TnyAccount *account; CamelStore *store;
	guint cached_length, unread_length, unread_sync, local_size;
	gboolean subscribed, strict_retrieval;
	gboolean has_summary_cap;
	CamelFolderInfo *iter;
	gboolean iter_parented, push, unread_read;
	gchar *cached_name;
	TnyFolderType cached_folder_type;
	TnyMsgRemoveStrategy *remove_strat;
	TnyMsgReceiveStrategy *receive_strat;
	TnyFolder *self;
	gboolean want_changes, handle_changes, dont_fkill;
	TnyFolderStore *parent;
	TnyFolderCaps caps; 
	CamelException load_ex;
	GList *obs, *sobs;
	gboolean cant_reuse_iter;
	gboolean ongoing_poke_status;
};

CamelFolder* _tny_camel_folder_get_camel_folder (TnyCamelFolder *self);
void _tny_camel_folder_set_id (TnyCamelFolder *self, const gchar *id);
void _tny_camel_folder_set_subscribed (TnyCamelFolder *self, gboolean subscribed);
void _tny_camel_folder_set_name (TnyCamelFolder *self, const gchar *name);
void _tny_camel_folder_set_folder_type (TnyCamelFolder *folder, CamelFolderInfo *folder_info);
void _tny_camel_folder_set_iter (TnyCamelFolder *folder, CamelFolderInfo *iter);
void _tny_camel_folder_set_account (TnyCamelFolder *self, TnyAccount *account);
gboolean _tny_camel_folder_load_folder_no_lock (TnyCamelFolderPriv *priv);
void _tny_camel_folder_set_folder (TnyCamelFolder *self, CamelFolder *camel_folder);
void _tny_camel_folder_set_parent (TnyCamelFolder *self, TnyFolderStore *parent);
void _tny_camel_folder_set_folder_info (TnyFolderStore *self, TnyCamelFolder *folder, CamelFolderInfo *info);
void _tny_camel_folder_set_local_size (TnyCamelFolder *self, guint len);
void _tny_camel_folder_set_unread_count (TnyCamelFolder *self, guint len);
void _tny_camel_folder_set_all_count (TnyCamelFolder *self, guint len);
void _tny_camel_folder_check_unread_count (TnyCamelFolder *self);

void _tny_camel_folder_unreason (TnyCamelFolderPriv *priv);
void _tny_camel_folder_reason (TnyCamelFolderPriv *priv);

void _tny_camel_folder_uncache_attachments (TnyCamelFolder *self, const gchar *uid);
void _tny_camel_folder_rewrite_cache (TnyCamelFolder *self, const gchar *uid, CamelMimeMessage *msg);

gboolean _tny_camel_folder_get_allow_external_images (TnyCamelFolder *self, const gchar *uid);
void _tny_camel_folder_set_allow_external_images (TnyCamelFolder *self, const gchar *uid, gboolean allow);

void _tny_camel_folder_remove_folder_actual (TnyFolderStore *self, TnyFolder *folder, TnyFolderStoreChange *change, GError **err);

void _tny_camel_folder_freeup_observers (TnyCamelFolder *self, TnyCamelFolderPriv *priv);
void _tny_camel_folder_track_folder_changed (TnyCamelFolder *self, CamelFolder *folder);

CamelFolder* _tny_camel_folder_get_folder (TnyCamelFolder *self);

#define TNY_CAMEL_FOLDER_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_CAMEL_FOLDER, TnyCamelFolderPriv))

#define TNY_FOLDER_PRIV_GET_SESSION(o) \
	((o)&&(o)->account?TNY_CAMEL_ACCOUNT_GET_PRIVATE ((o)->account)->session:NULL)

#define TNY_FOLDER_PRIV_GET_QUEUE(o) \
	((o)&&(o)->account?TNY_CAMEL_STORE_ACCOUNT_GET_PRIVATE ((o)->account)->queue:NULL)

#define TNY_FOLDER_PRIV_GET_MSG_QUEUE(o) \
	((o)&&(o)->account?TNY_CAMEL_STORE_ACCOUNT_GET_PRIVATE ((o)->account)->msg_queue:NULL)


TnyFolder* _tny_camel_folder_new_with_folder (CamelFolder *camel_folder);
TnyFolder* _tny_camel_folder_new (void);

#endif
