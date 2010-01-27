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
 * TnyGtkMimePartSaveStrategy:
 *
 * a #TnyMimePartSaveStrategy that saves a #TnyMimePart using a file dialog
 * in Gtk+ and GnomeVFS if available.
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <glib/gi18n-lib.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string.h>
#include <gtk/gtk.h>
#include <tny-gtk-mime-part-save-strategy.h>
#include <tny-gtk-text-buffer-stream.h>
#include <tny-gtk-attach-list-model.h>

#include <tny-mime-part.h>

#ifdef GNOME
#include <tny-vfs-stream.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#else
#include <tny-fs-stream.h>
#endif

static GObjectClass *parent_class = NULL;

typedef struct _TnyGtkMimePartSaveStrategyPriv TnyGtkMimePartSaveStrategyPriv;

struct _TnyGtkMimePartSaveStrategyPriv
{
	TnyStatusCallback status_callback;
	gpointer status_user_data;
};

#define TNY_GTK_MIME_PART_SAVE_STRATEGY_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_GTK_MIME_PART_SAVE_STRATEGY, TnyGtkMimePartSaveStrategyPriv))


#ifdef GNOME

static gboolean
gtk_save_to_file (const gchar *uri, TnyMimePart *part, TnyMimePartSaveStrategy *self)
{
	TnyGtkMimePartSaveStrategyPriv *priv = TNY_GTK_MIME_PART_SAVE_STRATEGY_GET_PRIVATE (self);
	GnomeVFSResult result;
	GnomeVFSHandle *handle;
	TnyStream *stream = NULL;

	result = gnome_vfs_create (&handle, uri, 
		GNOME_VFS_OPEN_WRITE, FALSE, 0777);

	if (G_UNLIKELY (result != GNOME_VFS_OK))
		return FALSE;

	stream = tny_vfs_stream_new (handle);
	tny_mime_part_decode_to_stream_async (part, stream, NULL, 
		priv->status_callback, priv->status_user_data);

	/* This also closes the handle */    
	g_object_unref (stream);

	return TRUE;
}
#else
static gboolean
gtk_save_to_file (const gchar *local_filename, TnyMimePart *part, TnyMimePartSaveStrategy *self)
{
	TnyGtkMimePartSaveStrategyPriv *priv = TNY_GTK_MIME_PART_SAVE_STRATEGY_GET_PRIVATE (self);
	int fd = open (local_filename, O_WRONLY | O_CREAT,  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (fd != -1)
	{
		TnyStream *stream = tny_fs_stream_new (fd);
		tny_mime_part_decode_to_stream_async (part, stream, NULL, 
			priv->status_callback, priv->status_user_data);

		/* This also closes the file descriptor */
		g_object_unref (stream);
		return TRUE;
	}

	return FALSE;
}
#endif


static void
tny_gtk_mime_part_save_strategy_perform_save (TnyMimePartSaveStrategy *self, TnyMimePart *part)
{
	TNY_GTK_MIME_PART_SAVE_STRATEGY_GET_CLASS (self)->perform_save(self, part);
	return;
}

static void
tny_gtk_mime_part_save_strategy_perform_save_default (TnyMimePartSaveStrategy *self, TnyMimePart *part)
{
	GtkFileChooserDialog *dialog;
	gboolean destr=FALSE;

	g_assert (TNY_IS_MIME_PART (part));

	dialog = GTK_FILE_CHOOSER_DIALOG 
		(gtk_file_chooser_dialog_new (_("Save File"), NULL,
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, 
		GTK_RESPONSE_ACCEPT, NULL));

	/* gtk_file_chooser_set_do_overwrite_confirmation 
		(GTK_FILE_CHOOSER (dialog), TRUE); */

	gtk_file_chooser_set_current_folder 
		(GTK_FILE_CHOOSER (dialog), g_get_home_dir ());

/* TNY TODO: detect this at runtime */
#ifndef GNOME
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);
#endif

	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), 
		tny_mime_part_get_filename (part));

	if (G_LIKELY (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT))
	{
		gchar *uri;

/* TNY TODO: detect this at runtime */
#ifdef GNOME
		uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
#else
		uri = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
#endif
		if (uri)
		{
			if (!gtk_save_to_file (uri, part, self))
			{
				GtkWidget *errd;

				gtk_widget_destroy (GTK_WIDGET (dialog));
				destr = TRUE;
				errd = gtk_message_dialog_new (NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					_("Saving to %s failed\n"), uri);
				gtk_dialog_run (GTK_DIALOG (errd));
				gtk_widget_destroy (GTK_WIDGET (errd));
			}

			g_free (uri);
		}
	}

	if (!destr)
		gtk_widget_destroy (GTK_WIDGET (dialog));

	return;
}


/**
 * tny_gtk_mime_part_save_strategy_new:
 * @status_callback: (null-ok): a #TnyStatusCallback for when status information happens or NULL
 * @status_user_data: (null-ok): user data for @status_callback
 *
 * Create a new #TnyMimePartSaveStrategy It will use the #GtkFileChooserDialog type and if
 * available consume its support for GnomeVFS.
 *
 * Whenever data must be retrieved or takes long to load, @status_callback will
 * be called to let the outside world know about what this compenent is doing.
 *
 * returns: (caller-owns): a new #TnyMimePartSaveStrategy
 * since: 1.0
 * audience: application-developer
 **/
TnyMimePartSaveStrategy*
tny_gtk_mime_part_save_strategy_new (TnyStatusCallback status_callback, gpointer status_user_data)
{
	TnyGtkMimePartSaveStrategy *self = g_object_new (TNY_TYPE_GTK_MIME_PART_SAVE_STRATEGY, NULL);
	TnyGtkMimePartSaveStrategyPriv *priv= TNY_GTK_MIME_PART_SAVE_STRATEGY_GET_PRIVATE (self);

	priv->status_callback = status_callback;
	priv->status_user_data = status_user_data;

	return TNY_MIME_PART_SAVE_STRATEGY (self);
}

static void
tny_gtk_mime_part_save_strategy_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyGtkMimePartSaveStrategyPriv *priv = TNY_GTK_MIME_PART_SAVE_STRATEGY_GET_PRIVATE (instance);

	priv->status_callback = NULL;
	priv->status_user_data = NULL;

	return;
}

static void
tny_gtk_mime_part_save_strategy_finalize (GObject *object)
{
	(*parent_class->finalize) (object);

	return;
}

static void
tny_gtk_mime_part_save_strategy_init (gpointer g, gpointer iface_data)
{
	TnyMimePartSaveStrategyIface *klass = (TnyMimePartSaveStrategyIface *)g;

	klass->perform_save= tny_gtk_mime_part_save_strategy_perform_save;

	return;
}

static void 
tny_gtk_mime_part_save_strategy_class_init (TnyGtkMimePartSaveStrategyClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	class->perform_save= tny_gtk_mime_part_save_strategy_perform_save_default;

	object_class->finalize = tny_gtk_mime_part_save_strategy_finalize;

	g_type_class_add_private (object_class, sizeof (TnyGtkMimePartSaveStrategyPriv));

	return;
}

static gpointer 
tny_gtk_mime_part_save_strategy_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyGtkMimePartSaveStrategyClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_gtk_mime_part_save_strategy_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyGtkMimePartSaveStrategy),
		  0,      /* n_preallocs */
		  tny_gtk_mime_part_save_strategy_instance_init,    /* instance_init */
		  NULL
		};

	static const GInterfaceInfo tny_gtk_mime_part_save_strategy_info = 
		{
		  (GInterfaceInitFunc) tny_gtk_mime_part_save_strategy_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyGtkMimePartSaveStrategy",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_MIME_PART_SAVE_STRATEGY, 
				     &tny_gtk_mime_part_save_strategy_info);

	return GUINT_TO_POINTER (type);
}

GType 
tny_gtk_mime_part_save_strategy_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gtk_mime_part_save_strategy_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
