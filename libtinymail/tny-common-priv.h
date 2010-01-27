#ifndef TNY_COMMON_PRIV_H
#define TNY_COMMON_PRIV_H

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

#include <tny-shared.h>
#include <tny-status.h>
#include <tny-error.h>
#include <tny-lockable.h>

#ifndef TINYMAIL_ENABLE_PRIVATE_API
#error "This is private API that should only be used by tinymail internally."
#endif

G_BEGIN_DECLS

/* Implementations in tny-progress-info.c and tny-idle-stopper.c */

typedef struct _TnyIdleStopper TnyIdleStopper;
typedef struct _TnyProgressInfo TnyProgressInfo;

TnyIdleStopper* tny_idle_stopper_new (void);
TnyIdleStopper* tny_idle_stopper_copy (TnyIdleStopper *stopper);

TnyProgressInfo* tny_progress_info_new (GObject *self, TnyStatusCallback status_callback, TnyStatusDomain domain, TnyStatusCode code, const gchar *what, gint sofar, gint oftotal, TnyIdleStopper* stopper, TnyLockable *uid_lock, gpointer user_data);
gboolean tny_progress_info_idle_func (gpointer data);
void tny_progress_info_destroy (gpointer data);

void tny_idle_stopper_stop (TnyIdleStopper *stopper);
void tny_idle_stopper_destroy(TnyIdleStopper *stopper);
gboolean tny_idle_stopper_is_stopped (TnyIdleStopper* stopper);





#define TNY_TYPE_EXPUNGED_HEADER             (tny_expunged_header_get_type ())
#define TNY_EXPUNGED_HEADER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_EXPUNGED_HEADER, TnyExpungedHeader))
#define TNY_EXPUNGED_HEADER_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_EXPUNGED_HEADER, TnyExpungedHeaderClass))
#define TNY_IS_EXPUNGED_HEADER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_EXPUNGED_HEADER))
#define TNY_IS_EXPUNGED_HEADER_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_EXPUNGED_HEADER))
#define TNY_EXPUNGED_HEADER_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_EXPUNGED_HEADER, TnyExpungedHeaderClass))

typedef struct _TnyExpungedHeader TnyExpungedHeader;
typedef struct _TnyExpungedHeaderClass TnyExpungedHeaderClass;

struct _TnyExpungedHeader {
	GObject parent;
};

struct _TnyExpungedHeaderClass {
	GObjectClass parent;
};

GType tny_expunged_header_get_type (void);
TnyHeader* tny_expunged_header_new (void);


G_END_DECLS

#endif 
