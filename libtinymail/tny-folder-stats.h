#ifndef TNY_FOLDER_STATS_H
#define TNY_FOLDER_STATS_H

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

#define TNY_TYPE_FOLDER_STATS             (tny_folder_stats_get_type ())
#define TNY_FOLDER_STATS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_FOLDER_STATS, TnyFolderStats))
#define TNY_FOLDER_STATS_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_FOLDER_STATS, TnyFolderStatsClass))
#define TNY_IS_FOLDER_STATS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_FOLDER_STATS))
#define TNY_IS_FOLDER_STATS_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_FOLDER_STATS))
#define TNY_FOLDER_STATS_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_FOLDER_STATS, TnyFolderStatsClass))

#ifndef TNY_SHARED_H
typedef struct _TnyFolderStats TnyFolderStats;
typedef struct _TnyFolderStatsClass TnyFolderStatsClass;
#endif


struct _TnyFolderStats
{
	GObject parent;
};

struct _TnyFolderStatsClass 
{
	GObjectClass parent;
};

GType  tny_folder_stats_get_type (void);
TnyFolderStats* tny_folder_stats_new (TnyFolder *folder);

guint tny_folder_stats_get_unread_count (TnyFolderStats *self);
guint tny_folder_stats_get_all_count (TnyFolderStats *self);
gsize tny_folder_stats_get_local_size (TnyFolderStats *self);
void tny_folder_stats_set_local_size (TnyFolderStats *self, gsize local_size);

TnyFolder* tny_folder_stats_get_folder (TnyFolderStats *self);

G_END_DECLS

#endif

