#ifndef TNY_MOZ_EMBED_STREAM_H
#define TNY_MOZ_EMBED_STREAM_H

/* libtinymailui-mozembed - The Tiny Mail UI library for Mozembed
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
#include <gtkmozembed.h>


G_BEGIN_DECLS

#define TNY_TYPE_MOZ_EMBED_STREAM             (tny_moz_embed_stream_get_type ())
#define TNY_MOZ_EMBED_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_MOZ_EMBED_STREAM, TnyMozEmbedStream))
#define TNY_MOZ_EMBED_STREAM_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_MOZ_EMBED_STREAM, TnyMozEmbedStreamClass))
#define TNY_IS_MOZ_EMBED_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_MOZ_EMBED_STREAM))
#define TNY_IS_MOZ_EMBED_STREAM_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_MOZ_EMBED_STREAM))
#define TNY_MOZ_EMBED_STREAM_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_MOZ_EMBED_STREAM, TnyMozEmbedStreamClass))

typedef struct _TnyMozEmbedStream TnyMozEmbedStream;
typedef struct _TnyMozEmbedStreamClass TnyMozEmbedStreamClass;
typedef struct _TnyMozEmbedStreamPriv TnyMozEmbedStreamPriv;

struct _TnyMozEmbedStream
{
	GObject parent;
	TnyMozEmbedStreamPriv *priv;
};

struct _TnyMozEmbedStreamClass 
{
	GObjectClass parent;
};

GType tny_moz_embed_stream_get_type (void);
TnyStream* tny_moz_embed_stream_new (GtkMozEmbed *embed);
void tny_moz_embed_stream_set_moz_embed (TnyMozEmbedStream *self, GtkMozEmbed *embed);


G_END_DECLS

#endif

