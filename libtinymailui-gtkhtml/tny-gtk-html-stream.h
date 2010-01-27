#ifndef TNY_GTK_HTML_STREAM_H
#define TNY_GTK_HTML_STREAM_H

/* libtinymailui-gtkhtml - The Tiny Mail UI library for GtkHtml
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
#include <tny-mime-part-view.h>

#include <gtkhtml/gtkhtml.h>


G_BEGIN_DECLS

#define TNY_TYPE_GTK_HTML_STREAM             (tny_gtk_html_stream_get_type ())
#define TNY_GTK_HTML_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_GTK_HTML_STREAM, TnyGtkHtmlStream))
#define TNY_GTK_HTML_STREAM_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_GTK_HTML_STREAM, TnyGtkHtmlStreamClass))
#define TNY_IS_GTK_HTML_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_GTK_HTML_STREAM))
#define TNY_IS_GTK_HTML_STREAM_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_GTK_HTML_STREAM))
#define TNY_GTK_HTML_STREAM_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_GTK_HTML_STREAM, TnyGtkHtmlStreamClass))

typedef struct _TnyGtkHtmlStream TnyGtkHtmlStream;
typedef struct _TnyGtkHtmlStreamClass TnyGtkHtmlStreamClass;
typedef struct _TnyGtkHtmlStreamPriv TnyGtkHtmlStreamPriv;

struct _TnyGtkHtmlStream
{
	GObject parent;
	TnyGtkHtmlStreamPriv *priv;
};

struct _TnyGtkHtmlStreamClass 
{
	GObjectClass parent;
};

GType tny_gtk_html_stream_get_type (void);
TnyStream* tny_gtk_html_stream_new (GtkHTMLStream* stream);


G_END_DECLS

#endif

