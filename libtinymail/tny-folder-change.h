#ifndef TNY_FOLDER_CHANGE_H
#define TNY_FOLDER_CHANGE_H

/* libtinymail- The Tiny Mail base library
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

#include <tny-shared.h>
#include <tny-folder.h>

G_BEGIN_DECLS

#define TNY_TYPE_FOLDER_CHANGE             (tny_folder_change_get_type ())
#define TNY_FOLDER_CHANGE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_FOLDER_CHANGE, TnyFolderChange))
#define TNY_FOLDER_CHANGE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_FOLDER_CHANGE, TnyFolderChangeClass))
#define TNY_IS_FOLDER_CHANGE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_FOLDER_CHANGE))
#define TNY_IS_FOLDER_CHANGE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_FOLDER_CHANGE))
#define TNY_FOLDER_CHANGE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_FOLDER_CHANGE, TnyFolderChangeClass))

#ifndef TNY_SHARED_H
typedef struct _TnyFolderChange TnyFolderChange;
typedef struct _TnyFolderChangeClass TnyFolderChangeClass;
#endif


#define TNY_TYPE_FOLDER_CHANGE_CHANGED (tny_folder_change_changed_get_type())

typedef enum
{
	TNY_FOLDER_CHANGE_CHANGED_ALL_COUNT = 1<<0,
	TNY_FOLDER_CHANGE_CHANGED_UNREAD_COUNT = 1<<1,
	TNY_FOLDER_CHANGE_CHANGED_ADDED_HEADERS = 1<<2,
	TNY_FOLDER_CHANGE_CHANGED_EXPUNGED_HEADERS = 1<<3,
	TNY_FOLDER_CHANGE_CHANGED_FOLDER_RENAME = 1<<4,
	TNY_FOLDER_CHANGE_CHANGED_MSG_RECEIVED = 1<<5
} TnyFolderChangeChanged;


struct _TnyFolderChange
{
	GObject parent;
};

struct _TnyFolderChangeClass 
{
	GObjectClass parent;
};

GType  tny_folder_change_get_type (void);
GType tny_folder_change_changed_get_type (void);

TnyFolderChange* tny_folder_change_new (TnyFolder *folder);

const gchar* tny_folder_change_get_rename (TnyFolderChange *self, const gchar **oldname);
void tny_folder_change_set_rename (TnyFolderChange *self, const gchar *newname);
void tny_folder_change_set_received_msg (TnyFolderChange *self, TnyMsg *msg);
TnyMsg* tny_folder_change_get_received_msg (TnyFolderChange *self);
void tny_folder_change_set_new_all_count (TnyFolderChange *self, guint new_all_count);
void tny_folder_change_set_new_unread_count (TnyFolderChange *self, guint new_unread_count);
guint tny_folder_change_get_new_unread_count (TnyFolderChange *self);
guint tny_folder_change_get_new_all_count (TnyFolderChange *self);
void tny_folder_change_add_added_header (TnyFolderChange *self, TnyHeader *header);
void tny_folder_change_add_expunged_header (TnyFolderChange *self, TnyHeader *header);
void tny_folder_change_get_added_headers (TnyFolderChange *self, TnyList *headers);
void tny_folder_change_get_expunged_headers (TnyFolderChange *self, TnyList *headers);
void tny_folder_change_reset (TnyFolderChange *self);
TnyFolder* tny_folder_change_get_folder (TnyFolderChange *self);
gboolean tny_folder_change_get_check_duplicates (TnyFolderChange *self);
void tny_folder_change_set_check_duplicates (TnyFolderChange *self, gboolean check_duplicates);

TnyFolderChangeChanged tny_folder_change_get_changed  (TnyFolderChange *self);

G_END_DECLS

#endif

