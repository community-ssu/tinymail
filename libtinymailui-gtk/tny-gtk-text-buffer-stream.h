#ifndef TNY_GTK_TEXT_BUFFER_STREAM_H
#define TNY_GTK_TEXT_BUFFER_STREAM_H

/* libtinymailui-gtk - The Tiny Mail UI library for Gtk+
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
#include <gtk/gtk.h>
#include <glib-object.h>

#include <tny-stream.h>

G_BEGIN_DECLS

#define TNY_TYPE_GTK_TEXT_BUFFER_STREAM             (tny_gtk_text_buffer_stream_get_type ())
#define TNY_GTK_TEXT_BUFFER_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_GTK_TEXT_BUFFER_STREAM, TnyGtkTextBufferStream))
#define TNY_GTK_TEXT_BUFFER_STREAM_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_GTK_TEXT_BUFFER_STREAM, TnyGtkTextBufferStreamClass))
#define TNY_IS_GTK_TEXT_BUFFER_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_GTK_TEXT_BUFFER_STREAM))
#define TNY_IS_GTK_TEXT_BUFFER_STREAM_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_GTK_TEXT_BUFFER_STREAM))
#define TNY_GTK_TEXT_BUFFER_STREAM_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_GTK_TEXT_BUFFER_STREAM, TnyGtkTextBufferStreamClass))

typedef struct _TnyGtkTextBufferStream TnyGtkTextBufferStream;
typedef struct _TnyGtkTextBufferStreamClass TnyGtkTextBufferStreamClass;

struct _TnyGtkTextBufferStream
{
	GObject parent;
};

struct _TnyGtkTextBufferStreamClass 
{
	GObjectClass parent;

	/* virtual methods */
	gssize (*read) (TnyStream *self, char *buffer, gsize n);
	gssize (*write) (TnyStream *self, const char *buffer, gsize n);
	gint (*flush) (TnyStream *self);
	gint (*close) (TnyStream *self);
	gboolean (*is_eos) (TnyStream *self);
	gint (*reset) (TnyStream *self);
	gssize (*write_to_stream) (TnyStream *self, TnyStream *output);
};

GType tny_gtk_text_buffer_stream_get_type (void);
TnyStream* tny_gtk_text_buffer_stream_new (GtkTextBuffer *buffer);

void tny_gtk_text_buffer_stream_set_text_buffer (TnyGtkTextBufferStream *self, GtkTextBuffer *buffer);

G_END_DECLS

#endif

