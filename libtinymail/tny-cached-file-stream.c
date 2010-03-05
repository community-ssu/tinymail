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

/**
 * TnyCachedFileStream:
 *
 * A stream that accesses a cached file stream taking into account when
 * specific parts are available.
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <tny-cached-file-stream.h>

static GObjectClass *parent_class = NULL;
static GTypeInterface *parent_stream_iface = NULL;
static GTypeInterface *parent_seekable_iface = NULL;

typedef struct _TnyCachedFileStreamPriv TnyCachedFileStreamPriv;

struct _TnyCachedFileStreamPriv
{
	TnyCachedFile *cached_file;
	gssize position;
	GMutex *position_mutex;
};

#define TNY_CACHED_FILE_STREAM_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_CACHED_FILE_STREAM, TnyCachedFileStreamPriv))


static gssize
tny_cached_file_stream_read (TnyStream *self, char *buffer, gsize n)
{
	TnyCachedFileStreamPriv *priv = TNY_CACHED_FILE_STREAM_GET_PRIVATE (self);
	gssize nread;
	gint64 fetchable;

	fetchable = tny_cached_file_wait_fetchable (priv->cached_file, priv->position);

	if (fetchable < priv->position) {
		return -1;
	} else {
		n = MIN (fetchable - priv->position, n);
		nread = (*((TnyStreamIface *) parent_stream_iface)->read) (self, buffer, n);
		g_mutex_lock (priv->position_mutex);
		if (priv->position == -1) {
			nread = -1;
		} else {
			priv->position += nread;
		}
		g_mutex_unlock (priv->position_mutex);
	}
	return nread;
}

static gssize
tny_cached_file_stream_write (TnyStream *self, const char *buffer, gsize n)
{
	g_warning (  "You can't use the tny_stream_write API on cached file "
		     "streams. This problem indicates a bug in the software");

	return -1;
}


static gint
tny_cached_file_stream_close (TnyStream *self)
{
	TnyCachedFileStreamPriv *priv = TNY_CACHED_FILE_STREAM_GET_PRIVATE (self);
	gboolean unregister = FALSE;

	g_mutex_lock (priv->position_mutex);
	if (priv->position > -1) {
		priv->position = -1;
		unregister = TRUE;
	}
	g_mutex_unlock (priv->position_mutex);

	if (unregister) {
		tny_cached_file_unregister_stream (priv->cached_file, TNY_CACHED_FILE_STREAM (self));
	}

	return (*((TnyStreamIface *) parent_stream_iface)->close) (self);
}

/**
 * tny_cached_file_stream_new:
 * @cached_file: a #TnyCachedFile
 * @fd: The file descriptor to write to or read from
 *
 * returns: (caller-owns): a new #TnyStream instance
 * since: 1.0
 * audience: tinymail-developer
 **/
TnyStream*
tny_cached_file_stream_new (TnyCachedFile *cached_file, gint fd)
{
	TnyCachedFileStream *self = g_object_new (TNY_TYPE_CACHED_FILE_STREAM, NULL);
	TnyCachedFileStreamPriv *priv = TNY_CACHED_FILE_STREAM_GET_PRIVATE (self);
	tny_fs_stream_set_fd (TNY_FS_STREAM (self), fd);
	priv->cached_file = g_object_ref (cached_file);
	return TNY_STREAM (self);
}

static void
tny_cached_file_stream_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyCachedFileStream *self = (TnyCachedFileStream *)instance;
	TnyCachedFileStreamPriv *priv = TNY_CACHED_FILE_STREAM_GET_PRIVATE (self);

	priv->position = 0;
	priv->position_mutex = g_mutex_new ();

	return;
}

static void
tny_cached_file_stream_finalize (GObject *object)
{
	TnyCachedFileStream *self = (TnyCachedFileStream *)object;
	TnyCachedFileStreamPriv *priv = TNY_CACHED_FILE_STREAM_GET_PRIVATE (self);

	g_mutex_free (priv->position_mutex);
	priv->position = -1;

	(*parent_class->finalize) (object);
	return;
}

static gboolean 
tny_cached_file_stream_is_eos (TnyStream *self)
{
	TnyCachedFileStreamPriv *priv = TNY_CACHED_FILE_STREAM_GET_PRIVATE (self);

	return (priv->position >= tny_cached_file_get_expected_size (TNY_CACHED_FILE (priv->cached_file)));
}

static gint 
tny_cached_file_stream_reset (TnyStream *self)
{
	TnyCachedFileStreamPriv *priv = TNY_CACHED_FILE_STREAM_GET_PRIVATE (self);
	priv->position = 0;

	return (*((TnyStreamIface *) parent_stream_iface)->reset) (self);
}

static off_t 
tny_cached_file_seek (TnySeekable *self, off_t offset, int policy)
{
	TnyCachedFileStreamPriv *priv = TNY_CACHED_FILE_STREAM_GET_PRIVATE (self);
	off_t real = 0;
	gint64 final_pos;

	g_mutex_lock (priv->position_mutex);
	switch (policy) {
	case SEEK_SET:
		real = offset;
		break;
	case SEEK_CUR:
		g_mutex_lock (priv->position_mutex);
		real = priv->position + offset;
		g_mutex_unlock (priv->position_mutex);
		break;
	case SEEK_END:
		real = tny_cached_file_get_expected_size (priv->cached_file);
		break;
	}

	final_pos = tny_cached_file_wait_fetchable (priv->cached_file, real);
	if (final_pos == -1)
		return -1;

	final_pos = (*((TnySeekableIface *) parent_seekable_iface)->seek) (self, offset, policy);
	g_mutex_lock (priv->position_mutex);
	priv->position = MIN (priv->position, final_pos);
	final_pos = priv->position;
	g_mutex_unlock (priv->position_mutex);

	return final_pos;
}

static void
tny_stream_init (gpointer g, gpointer iface_data)
{
	TnyStreamIface *klass = (TnyStreamIface *)g;

	klass->is_eos= tny_cached_file_stream_is_eos;
	klass->read= tny_cached_file_stream_read;
	klass->write= tny_cached_file_stream_write;
	klass->close= tny_cached_file_stream_close;
	klass->reset= tny_cached_file_stream_reset;

	return;
}

static void
tny_seekable_init (gpointer g, gpointer iface_data)
{
	TnySeekableIface *klass = (TnySeekableIface *)g;

	klass->seek= tny_cached_file_seek;

	return;
}

static void 
tny_cached_file_stream_class_init (TnyCachedFileStreamClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	parent_stream_iface = g_type_interface_peek (parent_class, TNY_TYPE_STREAM);
	parent_seekable_iface = g_type_interface_peek (parent_class, TNY_TYPE_SEEKABLE);
	object_class = (GObjectClass*) class;
	object_class->finalize = tny_cached_file_stream_finalize;
	g_type_class_add_private (object_class, sizeof (TnyCachedFileStreamPriv));

	return;
}

static gpointer
tny_cached_file_stream_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCachedFileStreamClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_cached_file_stream_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCachedFileStream),
			0,      /* n_preallocs */
			tny_cached_file_stream_instance_init,   /* instance_init */
			NULL
		};
	
	static const GInterfaceInfo tny_stream_info = 
		{
			(GInterfaceInitFunc) tny_stream_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	static const GInterfaceInfo tny_seekable_info = 
		{
			(GInterfaceInitFunc) tny_seekable_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (TNY_TYPE_FS_STREAM,
				       "TnyCachedFileStream",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_STREAM, 
				     &tny_stream_info);
	
	g_type_add_interface_static (type, TNY_TYPE_SEEKABLE, 
				     &tny_seekable_info);
	
	return GSIZE_TO_POINTER (type);
}

GType 
tny_cached_file_stream_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_cached_file_stream_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
