#ifndef TNY_STREAM_H
#define TNY_STREAM_H

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

G_BEGIN_DECLS

#define TNY_TYPE_STREAM             (tny_stream_get_type ())
#define TNY_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_STREAM, TnyStream))
#define TNY_IS_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_STREAM))
#define TNY_STREAM_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_STREAM, TnyStreamIface))

#ifndef TNY_SHARED_H
typedef struct _TnyStream TnyStream;
typedef struct _TnyStreamIface TnyStreamIface;
#endif

struct _TnyStreamIface
{
	GTypeInterface parent;

	gssize (*read) (TnyStream *self, char *buffer, gsize n);
	gssize (*write) (TnyStream *self, const char *buffer, gsize n);
	gint (*flush) (TnyStream *self);
	gint (*close) (TnyStream *self);
	gboolean (*is_eos) (TnyStream *self);
	gint (*reset) (TnyStream *self);
	gssize (*write_to_stream) (TnyStream *self, TnyStream *output);
};

GType tny_stream_get_type (void);

gssize tny_stream_read (TnyStream *self, char *buffer, gsize n);
gssize tny_stream_write (TnyStream *self, const char *buffer, gsize n);
gint tny_stream_flush (TnyStream *self);
gint tny_stream_close (TnyStream *self);
gboolean tny_stream_is_eos (TnyStream *self);
gint tny_stream_reset (TnyStream *self);
gssize tny_stream_write_to_stream (TnyStream *self, TnyStream *output);

G_END_DECLS

#endif
