/* libtinymailui-mozembed - The Tiny Mail UI library for Gtk+
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

#include <config.h>
#include <glib/gi18n-lib.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include <tny-stream.h>
#include <tny-moz-embed-stream.h>

static GObjectClass *parent_class = NULL;


struct _TnyMozEmbedStreamPriv
{
	GtkMozEmbed *embed;
};

#define TNY_MOZ_EMBED_STREAM_GET_PRIVATE(o) ((TnyMozEmbedStream*)(o))->priv

/*	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_MOZ_EMBED_STREAM, TnyMozEmbedStreamPriv)) */

static gint tny_moz_embed_stream_close (TnyStream *self);

static ssize_t
tny_moz_embed_write_to_stream (TnyStream *self, TnyStream *output)
{
	char tmp_buf[4096];
	ssize_t total = 0;
	ssize_t nb_read;
	ssize_t nb_written;

	g_assert (TNY_IS_STREAM (output));

	while (G_LIKELY (!tny_stream_is_eos (self))) 
	{
		nb_read = tny_stream_read (self, tmp_buf, sizeof (tmp_buf));
		if (G_UNLIKELY (nb_read < 0))
			return -1;
		else if (G_LIKELY (nb_read > 0)) {
			nb_written = 0;
	
			while (G_LIKELY (nb_written < nb_read))
			{
				ssize_t len = tny_stream_write (output, tmp_buf + nb_written,
								  nb_read - nb_written);
				if (G_UNLIKELY (len < 0))
					return -1;
				nb_written += len;
			}
			total += nb_written;
		}
	}
	return total;
}

static ssize_t
tny_moz_embed_stream_read  (TnyStream *self, char *data, size_t n)
{
	ssize_t retval = -1;
	return retval;
}

static gint
tny_moz_embed_stream_reset (TnyStream *self)
{
	return 0;
}

#ifdef NO_MOZ_PREFS

/* Ad-Hoc tag commenter */

static gchar*
replace_things (const char *data, const char *tag, size_t on, size_t *nn, gboolean *is_copy)
{
	char *p, *r, *ttag = g_strdup_printf ("<%s", tag), 
		*with = g_strdup_printf ("<!-- %s", tag);
	char *retval;
	unsigned int oi=0,og=0,occ=0,news;
	unsigned int withl = strlen (with), ttagl = strlen (ttag);

	p = (char*) data;
	while (p)
	{
		p = strcasestr (p, (const char*) ttag);
		if (p) { occ++; p++; }
			else break;
	}

	if (occ == 0)
	{
		*nn = on;
		*is_copy = FALSE;
		g_free (ttag); g_free (with);
		return (gchar *) data;
	}

	*is_copy = TRUE;

	news = (sizeof (char)) * (on + (occ* ((withl-ttagl) + (4-1))));
	retval = g_malloc0 (news);
	*nn = news;

	while (oi < on)
	{
		char *p = (char *) &data[oi];
		char *o = &retval[og];

		if (strncasecmp (p, (const char *) ttag, ttagl) == 0)
		{
			char *np, *e;

			memcpy (o, (const char *) with, withl);
			oi += ttagl;
			og += withl;
			e = (char *) &data[oi];
			np = strchr (e, '>');
			if (np)
			{
				while (e < np)
				{
					retval[og] = data[oi];
					oi++; og++; e++;
				}
				o = &retval[og];
				memcpy (o, " -->", 4);
				og += 4;
				oi++;
			}
		} else {
			retval[og] = data[oi];
			og++; oi++;
		}

	}

	g_free (ttag); g_free (with);

	return retval;
}

#endif

static ssize_t
tny_moz_embed_stream_write (TnyStream *self, const char *data, size_t n)
{
	TnyMozEmbedStreamPriv *priv = TNY_MOZ_EMBED_STREAM_GET_PRIVATE (self);

	if (priv->embed)
	{
#ifdef NO_MOZ_PREFS
		size_t tn = 0, nn = 0;
		gboolean bis_copy = FALSE, ais_copy = FALSE;
		gchar *nndata, *ndata;

		ndata = replace_things (data, "img", n, &nn, &ais_copy);
		nndata = replace_things (ndata, "script", nn, &tn, &bis_copy);

		if (ais_copy)
			g_free (ndata);

		gtk_moz_embed_append_data (priv->embed, (const char*) nndata, tn);

		if (bis_copy)
			g_free (nndata);
#else
		gtk_moz_embed_append_data (priv->embed, data, n);

#endif
	}
	return (ssize_t) n;
}

static gint
tny_moz_embed_stream_flush (TnyStream *self)
{
	return 0;
}


static gint
tny_moz_embed_stream_close (TnyStream *self)
{
	TnyMozEmbedStreamPriv *priv = TNY_MOZ_EMBED_STREAM_GET_PRIVATE (self);

	if (priv->embed)
	{
		gtk_moz_embed_close_stream (priv->embed);
		/* g_timeout_add (2000, (GSourceFunc) timeout_update_gok, priv->embed); */
		gtk_moz_embed_reload (priv->embed, GTK_MOZ_EMBED_FLAG_RELOADNORMAL);
	}

	return 0;
}

static gboolean
tny_moz_embed_stream_is_eos   (TnyStream *self)
{
	return TRUE;
}


/**
 * tny_moz_embed_stream_set_moz_embed:
 * @self: A #TnyMozEmbedStream instance
 * @embed: The #GtkMozEmbed to write to or read from
 *
 * Set the #GtkMozEmbed to play adaptor for
 *
 **/
void
tny_moz_embed_stream_set_moz_embed (TnyMozEmbedStream *self, GtkMozEmbed *embed)
{
	TnyMozEmbedStreamPriv *priv = TNY_MOZ_EMBED_STREAM_GET_PRIVATE (self);

	if (priv->embed)
		g_object_unref (G_OBJECT (priv->embed));
	g_object_ref (G_OBJECT (embed));

	priv->embed = embed;
	gtk_moz_embed_open_stream (priv->embed, "file:///", "text/html");

	return;
}

/**
 * tny_moz_embed_stream_new:
 * @embed: The #GtkMozEmbed to write to or read from
 *
 * Create an adaptor instance between #TnyStream and #GtkMozEmbed
 *
 * Return value: a new #TnyStream instance
 **/
TnyStream*
tny_moz_embed_stream_new (GtkMozEmbed *embed)
{
	TnyMozEmbedStream *self = g_object_new (TNY_TYPE_MOZ_EMBED_STREAM, NULL);

	tny_moz_embed_stream_set_moz_embed (self, embed);

	return TNY_STREAM (self);
}

static void
tny_moz_embed_stream_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyMozEmbedStream *self = (TnyMozEmbedStream *)instance;
	TnyMozEmbedStreamPriv *priv = g_slice_new (TnyMozEmbedStreamPriv);

	priv->embed = NULL;
	self->priv = priv;

	return;
}

static void
tny_moz_embed_stream_finalize (GObject *object)
{
	TnyMozEmbedStream *self = (TnyMozEmbedStream *)object;	
	TnyMozEmbedStreamPriv *priv = TNY_MOZ_EMBED_STREAM_GET_PRIVATE (self);

	tny_moz_embed_stream_close (TNY_STREAM (self));
	
	if (priv->embed)
		g_object_unref (G_OBJECT (priv->embed));

	g_slice_free (TnyMozEmbedStreamPriv, priv);
	
	(*parent_class->finalize) (object);

	return;
}

static void
tny_stream_init (gpointer g, gpointer iface_data)
{
	TnyStreamIface *klass = (TnyStreamIface *)g;

	klass->read= tny_moz_embed_stream_read;
	klass->write= tny_moz_embed_stream_write;
	klass->flush= tny_moz_embed_stream_flush;
	klass->close= tny_moz_embed_stream_close;
	klass->is_eos= tny_moz_embed_stream_is_eos;
	klass->reset= tny_moz_embed_stream_reset;
	klass->write_to_stream= tny_moz_embed_write_to_stream;

	return;
}

static void 
tny_moz_embed_stream_class_init (TnyMozEmbedStreamClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_moz_embed_stream_finalize;

	/* g_type_class_add_private (object_class, sizeof (TnyMozEmbedStreamPriv)); */

	return;
}

static gpointer
tny_moz_embed_stream_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyMozEmbedStreamClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_moz_embed_stream_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyMozEmbedStream),
		  0,      /* n_preallocs */
		  tny_moz_embed_stream_instance_init    /* instance_init */
		};

	static const GInterfaceInfo tny_stream_info = 
		{
		  (GInterfaceInitFunc) tny_stream_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyMozEmbedStream",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_STREAM, 
				     &tny_stream_info);

	return GUINT_TO_POINTER (type);
}

GType 
tny_moz_embed_stream_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_moz_embed_stream_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
