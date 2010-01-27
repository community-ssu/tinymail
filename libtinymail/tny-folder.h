#ifndef TNY_FOLDER_H
#define TNY_FOLDER_H

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

#include <glib.h>

#include <tny-shared.h>
#include <tny-msg.h>
#include <tny-header.h>
#include <tny-account.h>
#include <tny-msg-remove-strategy.h>
#include <tny-list.h>

G_BEGIN_DECLS

#define TNY_TYPE_FOLDER             (tny_folder_get_type ())
#define TNY_FOLDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_FOLDER, TnyFolder))
#define TNY_IS_FOLDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_FOLDER))
#define TNY_FOLDER_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_FOLDER, TnyFolderIface))

#ifndef TNY_SHARED_H
typedef struct _TnyFolder TnyFolder;
typedef struct _TnyFolderIface TnyFolderIface;
#endif


#define TNY_TYPE_FOLDER_SIGNAL (tny_folder_signal_get_type())

typedef enum
{
	TNY_FOLDER_FOLDER_INSERTED,
	TNY_FOLDER_FOLDERS_RELOADED,
	TNY_FOLDER_LAST_SIGNAL
} TnyFolderSignal;


extern guint tny_folder_signals[TNY_FOLDER_LAST_SIGNAL];

#define TNY_TYPE_FOLDER_TYPE (tny_folder_type_get_type())

#define TNY_TYPE_FOLDER_CAPS (tny_folder_caps_get_type())

typedef enum
{
	TNY_FOLDER_CAPS_WRITABLE = 1<<0,
	TNY_FOLDER_CAPS_PUSHEMAIL = 1<<1,
	TNY_FOLDER_CAPS_NOSELECT = 1<<2,
	TNY_FOLDER_CAPS_NOCHILDREN = 1<<3
} TnyFolderCaps;

typedef enum
{
	TNY_FOLDER_TYPE_UNKNOWN,
	TNY_FOLDER_TYPE_NORMAL,
	TNY_FOLDER_TYPE_INBOX,
	TNY_FOLDER_TYPE_OUTBOX,
	TNY_FOLDER_TYPE_TRASH,
	TNY_FOLDER_TYPE_JUNK,
	TNY_FOLDER_TYPE_SENT,
	TNY_FOLDER_TYPE_ROOT,
	TNY_FOLDER_TYPE_NOTES,
	TNY_FOLDER_TYPE_DRAFTS,
	TNY_FOLDER_TYPE_CONTACTS,
	TNY_FOLDER_TYPE_CALENDAR,
	TNY_FOLDER_TYPE_ARCHIVE,
	TNY_FOLDER_TYPE_MERGE,
	TNY_FOLDER_TYPE_NUM
} TnyFolderType;

struct _TnyFolderIface
{
	GTypeInterface parent;
	
	/* Methods */
	void (*remove_msg) (TnyFolder *self, TnyHeader *header, GError **err);
	void (*remove_msgs) (TnyFolder *self, TnyList *headers, GError **err);
	void (*add_msg) (TnyFolder *self, TnyMsg *msg, GError **err);
	void (*add_msg_async) (TnyFolder *self, TnyMsg *msg, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data);
	void (*sync) (TnyFolder *self, gboolean expunge, GError **err);
	void (*sync_async) (TnyFolder *self, gboolean expunge, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data);
	TnyMsgRemoveStrategy* (*get_msg_remove_strategy) (TnyFolder *self);
	void (*set_msg_remove_strategy) (TnyFolder *self, TnyMsgRemoveStrategy *st);
	TnyMsgReceiveStrategy* (*get_msg_receive_strategy) (TnyFolder *self);
	void (*set_msg_receive_strategy) (TnyFolder *self, TnyMsgReceiveStrategy *st);
	TnyMsg* (*get_msg) (TnyFolder *self, TnyHeader *header, GError **err);
	TnyMsg* (*find_msg) (TnyFolder *self, const gchar *url_string, GError **err);
	void (*get_msg_async) (TnyFolder *self, TnyHeader *header, TnyGetMsgCallback callback, TnyStatusCallback status_callback, gpointer user_data);
	void (*find_msg_async) (TnyFolder *self, const gchar *url_string, TnyGetMsgCallback callback, TnyStatusCallback status_callback, gpointer user_data);
	void (*get_headers) (TnyFolder *self, TnyList *headers, gboolean refresh, GError **err);
	void (*get_headers_async) (TnyFolder *self, TnyList *headers, gboolean refresh, TnyGetHeadersCallback callback, TnyStatusCallback status_callback, gpointer user_data);
	const gchar* (*get_name) (TnyFolder *self);
	const gchar* (*get_id) (TnyFolder *self);
	TnyAccount* (*get_account) (TnyFolder *self);
	TnyFolderType (*get_folder_type) (TnyFolder *self);
	guint (*get_all_count) (TnyFolder *self);
	guint (*get_unread_count) (TnyFolder *self);
	guint (*get_local_size) (TnyFolder *self);
	gboolean (*is_subscribed) (TnyFolder *self);
	void (*refresh_async) (TnyFolder *self, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data);
	void (*refresh) (TnyFolder *self, GError **err);
	void (*transfer_msgs) (TnyFolder *self, TnyList *header_list, TnyFolder *folder_dst, gboolean delete_originals, GError **err);
	void (*transfer_msgs_async) (TnyFolder *self, TnyList *header_list, TnyFolder *folder_dst, gboolean delete_originals, TnyTransferMsgsCallback callback, TnyStatusCallback status_callback, gpointer user_data);
	TnyFolder* (*copy) (TnyFolder *self, TnyFolderStore *into, const gchar *new_name, gboolean del, GError **err);
	void (*copy_async) (TnyFolder *self, TnyFolderStore *into, const gchar *new_name, gboolean del, TnyCopyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data);
	void (*poke_status) (TnyFolder *self);
	void (*add_observer) (TnyFolder *self, TnyFolderObserver *observer);
	void (*remove_observer) (TnyFolder *self, TnyFolderObserver *observer);
	TnyFolderStore* (*get_folder_store) (TnyFolder *self);
	TnyFolderStats* (*get_stats) (TnyFolder *self);
	gchar* (*get_url_string) (TnyFolder *self);
	TnyFolderCaps (*get_caps) (TnyFolder *self);
	void (*remove_msgs_async) (TnyFolder *self, TnyList *headers, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data);

};

GType tny_folder_get_type (void);
GType tny_folder_type_get_type (void);
GType tny_folder_caps_get_type (void);
GType tny_folder_signal_get_type (void);

TnyMsgRemoveStrategy* tny_folder_get_msg_remove_strategy (TnyFolder *self);
void tny_folder_set_msg_remove_strategy (TnyFolder *self, TnyMsgRemoveStrategy *st);
TnyMsgReceiveStrategy* tny_folder_get_msg_receive_strategy (TnyFolder *self);
void tny_folder_set_msg_receive_strategy (TnyFolder *self, TnyMsgReceiveStrategy *st);
void tny_folder_add_msg_async (TnyFolder *self, TnyMsg *msg, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data);
void tny_folder_sync_async (TnyFolder *self, gboolean expunge, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data);
void tny_folder_get_msg_async (TnyFolder *self, TnyHeader *header, TnyGetMsgCallback callback, TnyStatusCallback status_callback, gpointer user_data);
void tny_folder_find_msg_async (TnyFolder *self, const gchar *url_string, TnyGetMsgCallback callback, TnyStatusCallback status_callback, gpointer user_data);
void tny_folder_get_headers_async (TnyFolder *self, TnyList *headers, gboolean refresh, TnyGetHeadersCallback callback, TnyStatusCallback status_callback, gpointer user_data);
TnyAccount* tny_folder_get_account (TnyFolder *self);
const gchar* tny_folder_get_id (TnyFolder *self);
const gchar* tny_folder_get_name (TnyFolder *self);
TnyFolderType tny_folder_get_folder_type (TnyFolder *self);
guint tny_folder_get_all_count (TnyFolder *self);
guint tny_folder_get_unread_count (TnyFolder *self);
guint tny_folder_get_local_size (TnyFolder *self);
gboolean tny_folder_is_subscribed (TnyFolder *self);
void tny_folder_refresh_async (TnyFolder *self, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data);
void tny_folder_transfer_msgs_async (TnyFolder *self, TnyList *header_list, TnyFolder *folder_dst, gboolean delete_originals, TnyTransferMsgsCallback callback, TnyStatusCallback status_callback, gpointer user_data);
void tny_folder_copy_async (TnyFolder *self, TnyFolderStore *into, const gchar *new_name, gboolean del,  TnyCopyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data);
void tny_folder_poke_status (TnyFolder *self);
void tny_folder_add_observer (TnyFolder *self, TnyFolderObserver *observer);
void tny_folder_remove_observer (TnyFolder *self, TnyFolderObserver *observer);
TnyFolderStore* tny_folder_get_folder_store (TnyFolder *self);
TnyFolderStats* tny_folder_get_stats (TnyFolder *self);
TnyFolderCaps tny_folder_get_caps (TnyFolder *self);
gchar* tny_folder_get_url_string (TnyFolder *self);
void tny_folder_remove_msgs_async (TnyFolder *self, TnyList *headers, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data);

#ifndef TNY_DISABLE_DEPRECATED
TnyFolder* tny_folder_copy (TnyFolder *self, TnyFolderStore *into, const gchar *new_name, gboolean del, GError **err);
void tny_folder_add_msg (TnyFolder *self, TnyMsg *msg, GError **err);
void tny_folder_remove_msg (TnyFolder *self, TnyHeader *header, GError **err);
void tny_folder_remove_msgs (TnyFolder *self, TnyList *headers, GError **err);
TnyMsg* tny_folder_get_msg (TnyFolder *self, TnyHeader *header, GError **err);
TnyMsg* tny_folder_find_msg (TnyFolder *self, const gchar *url_string, GError **err);
void tny_folder_refresh (TnyFolder *self, GError **err);
void tny_folder_transfer_msgs (TnyFolder *self, TnyList *header_list, TnyFolder *folder_dst, gboolean delete_originals, GError **err);
void tny_folder_get_headers (TnyFolder *self, TnyList *headers, gboolean refresh, GError **err);
void tny_folder_sync (TnyFolder *self, gboolean expunge, GError **err);
#endif

G_END_DECLS

#endif
