#ifndef TNY_NOOP_LOCKABLE_H
#define TNY_NOOP_LOCKABLE_H

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

#include <tny-lockable.h>

G_BEGIN_DECLS

#define TNY_TYPE_NOOP_LOCKABLE             (tny_noop_lockable_get_type ())
#define TNY_NOOP_LOCKABLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_NOOP_LOCKABLE, TnyNoopLockable))
#define TNY_NOOP_LOCKABLE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_NOOP_LOCKABLE, TnyNoopLockableClass))
#define TNY_IS_NOOP_LOCKABLE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_NOOP_LOCKABLE))
#define TNY_IS_NOOP_LOCKABLE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_NOOP_LOCKABLE))
#define TNY_NOOP_LOCKABLE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_NOOP_LOCKABLE, TnyNoopLockableClass))

#ifndef TNY_SHARED_H
typedef struct _TnyNoopLockable TnyNoopLockable;
typedef struct _TnyNoopLockableClass TnyNoopLockableClass;
#endif

struct _TnyNoopLockable
{
	GObject parent;
};

struct _TnyNoopLockableClass 
{
	GObjectClass parent;
};

GType  tny_noop_lockable_get_type (void);
TnyLockable* tny_noop_lockable_new (void);

G_END_DECLS

#endif

