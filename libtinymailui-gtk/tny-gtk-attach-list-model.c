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
 * TnyGtkAttachListModel:
 *
 * A #GtkTreeModel for #TnyMimePart instances that happen to be attachments too.
 *
 * Note that a #TnyGtkAttachListModel is a #TnyList too. You can use the
 * #TnyList API on instances of this type too.
 *
 * Note that you must make sure that you unreference #TnyMimePart instances
 * that you get out of the instance column of this type using the #GtkTreeModel
 * API gtk_tree_model_get().
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <glib.h>
#include <gtk/gtk.h>

#ifdef GNOME
#include <libgnomeui/libgnomeui.h>
#endif

#include <tny-gtk-attach-list-model.h>
#include <tny-mime-part.h>
#include <tny-iterator.h>
#include <tny-mime-part.h>
#include <tny-folder.h>

#include "tny-gtk-attach-list-model-priv.h"
#include "tny-gtk-attach-list-model-iterator-priv.h"


static GObjectClass *parent_class = NULL;


#define TNY_GTK_ATTACH_LIST_MODEL_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_GTK_ATTACH_LIST_MODEL, TnyGtkAttachListModelPriv))

typedef void (*listaddfunc) (GtkListStore *list_store, GtkTreeIter *iter);

static void
tny_gtk_attach_list_model_add (TnyGtkAttachListModel *self, TnyMimePart *part, listaddfunc func)
{
	GtkListStore *model = GTK_LIST_STORE (self);
	GtkTreeIter iter;
	TnyGtkAttachListModelPriv *priv = TNY_GTK_ATTACH_LIST_MODEL_GET_PRIVATE (self);
	static GdkPixbuf *stock_file_pixbuf = NULL;
	GdkPixbuf *pixbuf;
	gchar *icon;

	if (tny_mime_part_get_content_type (part) &&
			tny_mime_part_is_attachment (part))
	{

		if (!priv->theme || !GTK_IS_ICON_THEME (priv->theme))
		{
			priv->theme = gtk_icon_theme_get_default ();
			g_object_ref (priv->theme);
		}

#ifdef GNOME
		if (priv->theme && GTK_IS_ICON_THEME (priv->theme))
		{
			icon = gnome_icon_lookup (priv->theme, NULL, 
				tny_mime_part_get_filename (part), NULL, NULL,
				tny_mime_part_get_content_type (part), 0, NULL);
		}
#else
		icon = GTK_STOCK_FILE;
#endif

		if (G_LIKELY (icon) && priv->theme && GTK_IS_ICON_THEME (priv->theme))
		{
			pixbuf = gtk_icon_theme_load_icon (priv->theme, icon, 
				GTK_ICON_SIZE_LARGE_TOOLBAR, 0, NULL);
#ifdef GNOME
			g_free (icon);
#endif
		} else {
			if (G_UNLIKELY (!stock_file_pixbuf) && priv->theme && GTK_IS_ICON_THEME (priv->theme))
				stock_file_pixbuf = gtk_icon_theme_load_icon (priv->theme, 
					GTK_STOCK_FILE, GTK_ICON_SIZE_LARGE_TOOLBAR, 
					0, NULL);

			pixbuf = stock_file_pixbuf;
		}

		func (model, &iter);

		gtk_list_store_set (model, &iter,
			TNY_GTK_ATTACH_LIST_MODEL_PIXBUF_COLUMN, 
			pixbuf,
			TNY_GTK_ATTACH_LIST_MODEL_FILENAME_COLUMN, 
			tny_mime_part_get_filename (part),
			TNY_GTK_ATTACH_LIST_MODEL_INSTANCE_COLUMN,
			part, -1);
	} else {

		func (model, &iter);

		gtk_list_store_set (model, &iter,
			TNY_GTK_ATTACH_LIST_MODEL_FILENAME_COLUMN, 
			tny_mime_part_get_description (part)?
				tny_mime_part_get_description (part):
				"Unknown attachment",
			TNY_GTK_ATTACH_LIST_MODEL_INSTANCE_COLUMN,
			part, -1);
	}

	return;
}


/**
 * tny_gtk_attach_list_model_new:
 *
 * Get a new #GtkTreeModel for #TnyMimePart instances that are attachments.
 *
 * returns: (caller-owns) a new #GtkTreeModel #TnyMimePart instances
 * since: 1.0
 * audience: application-developer
 **/
GtkTreeModel*
tny_gtk_attach_list_model_new (void)
{
	TnyGtkAttachListModel *self = g_object_new (TNY_TYPE_GTK_ATTACH_LIST_MODEL, NULL);

	return GTK_TREE_MODEL (self);
}

static void
tny_gtk_attach_list_model_finalize (GObject *object)
{
	TnyGtkAttachListModelPriv *priv = TNY_GTK_ATTACH_LIST_MODEL_GET_PRIVATE (object);
	TnyGtkAttachListModel *me = (TnyGtkAttachListModel*) object;

	g_mutex_lock (me->iterator_lock);
	if (me->first) {
		if (me->first_needs_unref)
			g_list_foreach (me->first, (GFunc)g_object_unref, NULL);
		me->first_needs_unref = FALSE;
		g_list_free (me->first);
	}
	me->first = NULL;
	g_mutex_unlock (me->iterator_lock);

	g_mutex_free (me->iterator_lock);
	me->iterator_lock = NULL;

	if (priv->theme)
		g_object_unref (G_OBJECT (priv->theme));

	(*parent_class->finalize) (object);
}

static void
tny_gtk_attach_list_model_class_init (TnyGtkAttachListModelClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_gtk_attach_list_model_finalize;

	g_type_class_add_private (object_class, sizeof (TnyGtkAttachListModelPriv));

	return;
}

static void
tny_gtk_attach_list_model_instance_init (GTypeInstance *instance, gpointer g_class)
{
	GtkListStore *store = (GtkListStore*) instance;
	TnyGtkAttachListModel *me = (TnyGtkAttachListModel*) instance;
	TnyGtkAttachListModelPriv *priv = TNY_GTK_ATTACH_LIST_MODEL_GET_PRIVATE (instance);
	static GType types[] = { G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_OBJECT };


	priv->theme = NULL;
	types[0] = GDK_TYPE_PIXBUF;
	me->iterator_lock = g_mutex_new ();
	me->first = NULL;
	me->first_needs_unref = FALSE;

	gtk_list_store_set_column_types (store, 
		TNY_GTK_ATTACH_LIST_MODEL_N_COLUMNS, types);

	return;
}

static TnyIterator*
tny_gtk_attach_list_model_create_iterator (TnyList *self)
{
	TnyGtkAttachListModel *me = (TnyGtkAttachListModel*)self;

	/* Return a new iterator */

	return TNY_ITERATOR (_tny_gtk_attach_list_model_iterator_new (me));
}



static void
tny_gtk_attach_list_model_prepend (TnyList *self, GObject* item)
{
	TnyGtkAttachListModel *me = (TnyGtkAttachListModel*)self;

	g_mutex_lock (me->iterator_lock);

	/* Prepend something to the list */

	tny_gtk_attach_list_model_add (me, TNY_MIME_PART (item), 
		gtk_list_store_prepend);

	me->first = g_list_prepend (me->first, item);

	g_mutex_unlock (me->iterator_lock);
}

static void
tny_gtk_attach_list_model_append (TnyList *self, GObject* item)
{
	TnyGtkAttachListModel *me = (TnyGtkAttachListModel*)self;

	g_mutex_lock (me->iterator_lock);

	/* Append something to the list */

	tny_gtk_attach_list_model_add (me, TNY_MIME_PART (item), 
		gtk_list_store_append);
	me->first = g_list_append (me->first, item);

	g_mutex_unlock (me->iterator_lock);
}

static guint
tny_gtk_attach_list_model_get_length (TnyList *self)
{
	TnyGtkAttachListModel *me = (TnyGtkAttachListModel*)self;
	guint retval = 0;

	g_mutex_lock (me->iterator_lock);

	retval = me->first?g_list_length (me->first):0;

	g_mutex_unlock (me->iterator_lock);

	return retval;
}

static void
tny_gtk_attach_list_model_remove (TnyList *self, GObject* item)
{
	TnyGtkAttachListModel *me = (TnyGtkAttachListModel*)self;
	GtkTreeModel *model = GTK_TREE_MODEL (me);
	GtkTreeIter iter;

	g_return_if_fail (G_IS_OBJECT (item));
	g_return_if_fail (G_IS_OBJECT (me));

	/* Remove something from the list */

	g_mutex_lock (me->iterator_lock);

	me->first = g_list_remove (me->first, (gconstpointer)item);

	if (gtk_tree_model_get_iter_first (model, &iter))
	  do
	  {
		GObject *citem;

		gtk_tree_model_get (model, &iter, 
			TNY_GTK_ATTACH_LIST_MODEL_INSTANCE_COLUMN, 
			&citem, -1);

		if (citem == item)
		{
			gtk_list_store_remove (GTK_LIST_STORE (me), &iter);
			g_object_unref (item);
			break;
		}
		g_object_unref (citem);
	  } while (gtk_tree_model_iter_next (model, &iter));

	g_mutex_unlock (me->iterator_lock);
}


static TnyList*
tny_gtk_attach_list_model_copy_the_list (TnyList *self)
{
	TnyGtkAttachListModel *me = (TnyGtkAttachListModel*)self;
	TnyGtkAttachListModel *copy = g_object_new (TNY_TYPE_GTK_ATTACH_LIST_MODEL, NULL);
	GList *list_copy = NULL;

	/* This only copies the TnyList pieces. The result is not a
	   correct or good TnyHeaderListModel. But it will be a correct
	   TnyList instance. It is the only thing the user of this
	   method expects.

	   The new list will point to the same instances, of course. It's
	   only a copy of the list-nodes of course. */

	g_mutex_lock (me->iterator_lock);
	list_copy = g_list_copy (me->first);
	g_list_foreach (list_copy, (GFunc)g_object_ref, NULL);
	copy->first_needs_unref = TRUE;
	copy->first = list_copy;
	g_mutex_unlock (me->iterator_lock);

	return TNY_LIST (copy);
}

static void 
tny_gtk_attach_list_model_foreach_in_the_list (TnyList *self, GFunc func, gpointer user_data)
{
	TnyGtkAttachListModel *me = (TnyGtkAttachListModel*)self;

	/* Foreach item in the list (without using a slower iterator) */

	g_mutex_lock (me->iterator_lock);
	g_list_foreach (me->first, func, user_data);
	g_mutex_unlock (me->iterator_lock);

	return;
}

static void
tny_list_init (TnyListIface *klass)
{
	klass->get_length= tny_gtk_attach_list_model_get_length;
	klass->prepend= tny_gtk_attach_list_model_prepend;
	klass->append= tny_gtk_attach_list_model_append;
	klass->remove= tny_gtk_attach_list_model_remove;
	klass->create_iterator= tny_gtk_attach_list_model_create_iterator;
	klass->copy= tny_gtk_attach_list_model_copy_the_list;
	klass->foreach= tny_gtk_attach_list_model_foreach_in_the_list;

	return;
}

static gpointer
tny_gtk_attach_list_model_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyGtkAttachListModelClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_gtk_attach_list_model_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyGtkAttachListModel),
		  0,      /* n_preallocs */
		  tny_gtk_attach_list_model_instance_init,    /* instance_init */
		  NULL
		};


	static const GInterfaceInfo tny_list_info = {
		(GInterfaceInitFunc) tny_list_init,
		NULL,
		NULL
	};

	type = g_type_register_static (GTK_TYPE_LIST_STORE, "TnyGtkAttachListModel",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_LIST,
				     &tny_list_info);

	return GUINT_TO_POINTER (type);
}

/**
 * tny_gtk_attach_list_model_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType
tny_gtk_attach_list_model_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gtk_attach_list_model_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}

static gpointer
tny_gtk_attach_list_model_column_register_type (gpointer notused)
{
  GType etype = 0;
  static const GEnumValue values[] = {
      { TNY_GTK_ATTACH_LIST_MODEL_PIXBUF_COLUMN, "TNY_GTK_ATTACH_LIST_MODEL_PIXBUF_COLUMN", "pixbuf" },
      { TNY_GTK_ATTACH_LIST_MODEL_FILENAME_COLUMN, "TNY_GTK_ATTACH_LIST_MODEL_FILENAME_COLUMN", "filename" },
      { TNY_GTK_ATTACH_LIST_MODEL_INSTANCE_COLUMN, "TNY_GTK_ATTACH_LIST_MODEL_INSTANCE_COLUMN", "instance" },
      { TNY_GTK_ATTACH_LIST_MODEL_N_COLUMNS, "TNY_GTK_ATTACH_LIST_MODEL_N_COLUMNS", "n" },
      { 0, NULL, NULL }
  };
  etype = g_enum_register_static ("TnyGtkAttachListModelColumn", values);
  return GUINT_TO_POINTER (etype);
}

/**
 * tny_gtk_attach_list_model_column_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType 
tny_gtk_attach_list_model_column_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gtk_attach_list_model_column_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
