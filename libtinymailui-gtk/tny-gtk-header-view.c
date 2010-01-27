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

/**
 * TnyGtkHeaderView:
 *
 * A simple view for a #TnyHeader
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <glib/gi18n-lib.h>

#include <string.h>
#include <gtk/gtk.h>
#include <tny-gtk-header-view.h>

static GObjectClass *parent_class = NULL;

typedef struct _TnyGtkHeaderViewPriv TnyGtkHeaderViewPriv;

struct _TnyGtkHeaderViewPriv
{
	TnyHeader *header;
	GtkWidget *from_label;
	GtkWidget *to_label;
	GtkWidget *subject_label;
	GtkWidget *date_label;
};

#define TNY_GTK_HEADER_VIEW_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_GTK_HEADER_VIEW, TnyGtkHeaderViewPriv))


static gchar *
_get_readable_date (time_t file_time_raw)
{
	struct tm file_time;
	gchar readable_date[64];
	gsize readable_date_size;

	gmtime_r (&file_time_raw, &file_time);

	readable_date_size = strftime (readable_date, 63, _("%Y-%m-%d, %-I:%M %p"), &file_time);
	
	return g_strdup (readable_date);
}

static void 
tny_gtk_header_view_set_header (TnyHeaderView *self, TnyHeader *header)
{
	TNY_GTK_HEADER_VIEW_GET_CLASS (self)->set_header(self, header);
	return;
}

static void 
tny_gtk_header_view_set_header_default (TnyHeaderView *self, TnyHeader *header)
{
	TnyGtkHeaderViewPriv *priv = TNY_GTK_HEADER_VIEW_GET_PRIVATE (self);

	if (header)
		g_assert (TNY_IS_HEADER (header));

	if (G_LIKELY (priv->header))
 		g_object_unref (G_OBJECT (priv->header));
	priv->header = NULL;

	if (header && G_IS_OBJECT (header))
	{
		gchar *str;
		gchar *to, *from, *subject;
		g_object_ref (G_OBJECT (header)); 
		priv->header = header;

		to = tny_header_dup_to (header);
		from = tny_header_dup_from (header);
		subject = tny_header_dup_subject (header);

		if (to) {
			gtk_label_set_text (GTK_LABEL (priv->to_label), to);
			g_free (to);
		} else {
			gtk_label_set_text (GTK_LABEL (priv->to_label), "");
		}

		if (from) {
			gtk_label_set_text (GTK_LABEL (priv->from_label), from);
			g_free (from);
		} else {
			gtk_label_set_text (GTK_LABEL (priv->from_label), "");
		}

		if (subject) {
			gtk_label_set_text (GTK_LABEL (priv->subject_label), subject);
			g_free (subject);
		} else {
			gtk_label_set_text (GTK_LABEL (priv->subject_label), "");
		}

		str = _get_readable_date (tny_header_get_date_sent (header));
		gtk_label_set_text (GTK_LABEL (priv->date_label), (const gchar*)str);
		g_free (str);
	}

	return;
}

static void 
tny_gtk_header_view_clear (TnyHeaderView *self)
{
	TNY_GTK_HEADER_VIEW_GET_CLASS (self)->clear(self);
	return;
}

static void 
tny_gtk_header_view_clear_default (TnyHeaderView *self)
{
	TnyGtkHeaderViewPriv *priv = TNY_GTK_HEADER_VIEW_GET_PRIVATE (self);

	if (G_LIKELY (priv->header))
		g_object_unref (G_OBJECT (priv->header));
	priv->header = NULL;

	gtk_label_set_text (GTK_LABEL (priv->to_label), "");
	gtk_label_set_text (GTK_LABEL (priv->from_label), "");
	gtk_label_set_text (GTK_LABEL (priv->subject_label), "");
	gtk_label_set_text (GTK_LABEL (priv->date_label), "");

	return;
}



/**
 * tny_gtk_header_view_new:
 *
 * Create a new #TnyHeaderView
 *
 * returns: (caller-owns): a new #TnyHeaderView
 * since: 1.0
 * audience: application-developer
 **/
TnyHeaderView*
tny_gtk_header_view_new (void)
{
	TnyGtkHeaderView *self = g_object_new (TNY_TYPE_GTK_HEADER_VIEW, NULL);

	return TNY_HEADER_VIEW (self);
}

static void
tny_gtk_header_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyGtkHeaderView *self = (TnyGtkHeaderView *)instance;
	TnyGtkHeaderViewPriv *priv = TNY_GTK_HEADER_VIEW_GET_PRIVATE (self);
	GtkWidget *label2, *label3, *label4, *label1;

	priv->header = NULL;

	gtk_table_set_homogeneous (GTK_TABLE (self), FALSE);
	gtk_table_resize (GTK_TABLE (self), 4, 2);

	gtk_table_set_col_spacings (GTK_TABLE (self), 4);

	label2 = gtk_label_new (_("<b>To:</b>"));
	gtk_widget_show (label2);
	gtk_table_attach (GTK_TABLE (self), label2, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label2), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

	label3 = gtk_label_new (_("<b>Subject:</b>"));
	gtk_widget_show (label3);
	gtk_table_attach (GTK_TABLE (self), label3, 0, 1, 2, 3,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label3), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);

	label4 = gtk_label_new (_("<b>Date:</b>"));
	gtk_widget_show (label4);
	gtk_table_attach (GTK_TABLE (self), label4, 0, 1, 3, 4,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label4), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

	priv->from_label = gtk_label_new ("");
	gtk_widget_show (priv->from_label);
	gtk_table_attach (GTK_TABLE (self), priv->from_label, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (priv->from_label), 0, 0.5);

	priv->to_label = gtk_label_new ("");
	gtk_widget_show (priv->to_label);
	gtk_table_attach (GTK_TABLE (self), priv->to_label, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (priv->to_label), 0, 0.5);

	priv->subject_label = gtk_label_new ("");
	gtk_widget_show (priv->subject_label);
	gtk_table_attach (GTK_TABLE (self), priv->subject_label, 1, 2, 2, 3,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (priv->subject_label), 0, 0.5);

	priv->date_label = gtk_label_new ("");
	gtk_widget_show (priv->date_label);
	gtk_table_attach (GTK_TABLE (self), priv->date_label, 1, 2, 3, 4,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (priv->date_label), 0, 0.5);

	label1 = gtk_label_new (_("<b>From:</b>"));
	gtk_widget_show (label1);
	gtk_table_attach (GTK_TABLE (self), label1, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label1), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);

	return;
}

static void
tny_gtk_header_view_finalize (GObject *object)
{
	TnyGtkHeaderView *self = (TnyGtkHeaderView *)object;	
	TnyGtkHeaderViewPriv *priv = TNY_GTK_HEADER_VIEW_GET_PRIVATE (self);

	if (G_LIKELY (priv->header))
		g_object_unref (G_OBJECT (priv->header));
	priv->header = NULL;

	(*parent_class->finalize) (object);

	return;
}

static void
tny_header_view_init (gpointer g, gpointer iface_data)
{
	TnyHeaderViewIface *klass = (TnyHeaderViewIface *)g;

	klass->set_header= tny_gtk_header_view_set_header;
	klass->clear= tny_gtk_header_view_clear;

	return;
}

static void 
tny_gtk_header_view_class_init (TnyGtkHeaderViewClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	class->set_header= tny_gtk_header_view_set_header_default;
	class->clear= tny_gtk_header_view_clear_default;

	object_class->finalize = tny_gtk_header_view_finalize;

	g_type_class_add_private (object_class, sizeof (TnyGtkHeaderViewPriv));

	return;
}

static gpointer
tny_gtk_header_view_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyGtkHeaderViewClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_gtk_header_view_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyGtkHeaderView),
		  0,      /* n_preallocs */
		  tny_gtk_header_view_instance_init    /* instance_init */
		};

	static const GInterfaceInfo tny_header_view_info = 
		{
		  (GInterfaceInitFunc) tny_header_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

	type = g_type_register_static (GTK_TYPE_TABLE,
				       "TnyGtkHeaderView",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_HEADER_VIEW, 
				     &tny_header_view_info);

	return GUINT_TO_POINTER (type);
}

/**
 * tny_gtk_header_view_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType 
tny_gtk_header_view_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gtk_header_view_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
