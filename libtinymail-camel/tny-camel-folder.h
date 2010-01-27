#ifndef TNY_CAMEL_FOLDER_H
#define TNY_CAMEL_FOLDER_H

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
#include <glib-object.h>

#include <tny-folder.h>
#include <tny-msg.h>
#include <tny-header.h>

#include <tny-msg-remove-strategy.h>
#include <tny-msg-receive-strategy.h>

G_BEGIN_DECLS

#define TNY_TYPE_CAMEL_FOLDER             (tny_camel_folder_get_type ())
#define TNY_CAMEL_FOLDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_CAMEL_FOLDER, TnyCamelFolder))
#define TNY_CAMEL_FOLDER_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_CAMEL_FOLDER, TnyCamelFolderClass))
#define TNY_IS_CAMEL_FOLDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CAMEL_FOLDER))
#define TNY_IS_CAMEL_FOLDER_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_CAMEL_FOLDER))
#define TNY_CAMEL_FOLDER_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_CAMEL_FOLDER, TnyCamelFolderClass))

typedef struct _TnyCamelFolder TnyCamelFolder;
typedef struct _TnyCamelFolderClass TnyCamelFolderClass;

struct _TnyCamelFolder
{
	GObject parent;
};

struct _TnyCamelFolderClass 
{
	GObjectClass parent;

	/* virtual methods */
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
	void (*transfer_msgs) (TnyFolder *self, TnyList *headers, TnyFolder *folder_dst, gboolean delete_originals, GError **err);
	void (*transfer_msgs_async) (TnyFolder *self, TnyList *header_list, TnyFolder *folder_dst, gboolean delete_originals, TnyTransferMsgsCallback callback, TnyStatusCallback status_callback, gpointer user_data);
	TnyFolder* (*copy) (TnyFolder *self, TnyFolderStore *into, const gchar *new_name, gboolean del, GError **err);
	void (*copy_async) (TnyFolder *self, TnyFolderStore *into, const gchar *new_name, gboolean del, TnyCopyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data);
	void (*poke_status) (TnyFolder *self);
	void (*add_observer) (TnyFolder *self, TnyFolderObserver *observer);
	void (*remove_observer) (TnyFolder *self, TnyFolderObserver *observer);
	TnyFolderStats* (*get_stats) (TnyFolder *self);
	gchar* (*get_url_string) (TnyFolder *self);
	TnyFolderCaps (*get_caps) (TnyFolder *self);
	void (*remove_msgs_async) (TnyFolder *self, TnyList *headers, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data);

	void (*get_folders_async) (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, gboolean refresh, TnyGetFoldersCallback callback, TnyStatusCallback status_callback, gpointer user_data);
	void (*get_folders) (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, gboolean refresh, GError **err);
	void (*remove_folder) (TnyFolderStore *self, TnyFolder *folder, GError **err);
	TnyFolder* (*create_folder) (TnyFolderStore *self, const gchar *name, GError **err);
	void (*create_folder_async) (TnyFolderStore *self, const gchar *name, TnyCreateFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data);
	TnyFolderStore* (*get_folder_store) (TnyFolder *self);
	void (*add_store_observer) (TnyFolderStore *self, TnyFolderStoreObserver *observer);
	void (*remove_store_observer) (TnyFolderStore *self, TnyFolderStoreObserver *observer);

};

GType tny_camel_folder_get_type (void);


const gchar* tny_camel_folder_get_full_name (TnyCamelFolder *self);

G_END_DECLS

#endif

