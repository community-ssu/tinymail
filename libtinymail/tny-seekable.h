#ifndef TNY_SEEKABLE_H
#define TNY_SEEKABLE_H

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

#include <stdarg.h>
#include <unistd.h>
#include <glib.h>
#include <glib-object.h>
#include <tny-shared.h>
#include <sys/types.h>

G_BEGIN_DECLS

#define TNY_TYPE_SEEKABLE             (tny_seekable_get_type ())
#define TNY_SEEKABLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_SEEKABLE, TnySeekable))
#define TNY_IS_SEEKABLE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_SEEKABLE))
#define TNY_SEEKABLE_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_SEEKABLE, TnySeekableIface))

#ifndef TNY_SHARED_H
typedef struct _TnySeekable TnySeekable;
typedef struct _TnySeekableIface TnySeekableIface;
#endif

struct _TnySeekableIface
{
	GTypeInterface parent;

	off_t (*seek)		(TnySeekable *self, off_t offset, int policy);
	off_t (*tell)		(TnySeekable *self);
	gint  (*set_bounds)	(TnySeekable *self, off_t start, off_t end);
};

GType tny_seekable_get_type (void);

off_t tny_seekable_seek (TnySeekable *self, off_t offset, int policy);
off_t tny_seekable_tell (TnySeekable *self);
gint tny_seekable_set_bounds (TnySeekable *self, off_t start, off_t end);

G_END_DECLS

#endif
