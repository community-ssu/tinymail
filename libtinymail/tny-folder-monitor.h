#ifndef TNY_FOLDER_MONITOR_H
#define TNY_FOLDER_MONITOR_H

/* libtinymailui-gtk - The Tiny Mail library
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
#include <tny-shared.h>
#include <tny-folder-observer.h>
#include <tny-list.h>
#include <tny-folder-change.h>

G_BEGIN_DECLS

#define TNY_TYPE_FOLDER_MONITOR             (tny_folder_monitor_get_type ())
#define TNY_FOLDER_MONITOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_FOLDER_MONITOR, TnyFolderMonitor))
#define TNY_FOLDER_MONITOR_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_FOLDER_MONITOR, TnyFolderMonitorClass))
#define TNY_IS_FOLDER_MONITOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_FOLDER_MONITOR))
#define TNY_IS_FOLDER_MONITOR_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_FOLDER_MONITOR))
#define TNY_FOLDER_MONITOR_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_FOLDER_MONITOR, TnyFolderMonitorClass))

#ifndef TNY_SHARED_H
typedef struct _TnyFolderMonitor TnyFolderMonitor;
typedef struct _TnyFolderMonitorClass TnyFolderMonitorClass;
#endif

struct _TnyFolderMonitor 
{
	GObject parent;
};

struct _TnyFolderMonitorClass 
{
	GObjectClass parent;

	/* virtuals */
	void (*update) (TnyFolderObserver *self, TnyFolderChange *change);
	void (*poke_status) (TnyFolderMonitor *self);
	void (*add_list) (TnyFolderMonitor *self, TnyList *list);
	void (*remove_list) (TnyFolderMonitor *self, TnyList *list);
	void (*stop) (TnyFolderMonitor *self);
	void (*start) (TnyFolderMonitor *self);

};

GType tny_folder_monitor_get_type (void);
TnyFolderObserver* tny_folder_monitor_new (TnyFolder *folder);

void tny_folder_monitor_poke_status (TnyFolderMonitor *self);
void tny_folder_monitor_add_list (TnyFolderMonitor *self, TnyList *list);
void tny_folder_monitor_remove_list (TnyFolderMonitor *self, TnyList *list);
void tny_folder_monitor_stop (TnyFolderMonitor *self);
void tny_folder_monitor_start (TnyFolderMonitor *self);

G_END_DECLS

#endif
