/* libtinymail-gnomevfs - The Tiny Mail base library for GnomeVFS
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
 *
 * A GnomeVFS to CamelStream mapper.
 */

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <tny-vfs-stream.h>

static GObjectClass *parent_class = NULL;

typedef struct _TnyVfsStreamPriv TnyVfsStreamPriv;

struct _TnyVfsStreamPriv
{
	GnomeVFSHandle *handle;
	gboolean eos;
	off_t position;		/* current postion in the stream */
	off_t bound_start;	/* first valid position */
	off_t bound_end;	/* first invalid position */
};

#define TNY_VFS_STREAM_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_VFS_STREAM, TnyVfsStreamPriv))


static void
tny_vfs_stream_set_errno (GnomeVFSResult res)
{
	switch(res) {
	case GNOME_VFS_OK:
		g_warning("tny-vfs-stream: calling set_errno with no error");
		break;
	case GNOME_VFS_ERROR_NOT_FOUND:
	case GNOME_VFS_ERROR_HOST_NOT_FOUND:
	case GNOME_VFS_ERROR_INVALID_HOST_NAME:
	case GNOME_VFS_ERROR_HOST_HAS_NO_ADDRESS:
	case GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE:
		errno = ENOENT;
		break;
	case GNOME_VFS_ERROR_GENERIC:
	case GNOME_VFS_ERROR_INTERNAL:
	case GNOME_VFS_ERROR_IO:
	case GNOME_VFS_ERROR_EOF: /* will be caught by read before here anyway */
	case GNOME_VFS_ERROR_SERVICE_OBSOLETE:
	case GNOME_VFS_ERROR_PROTOCOL_ERROR:
	default:
		errno = EIO;
		break;
	case GNOME_VFS_ERROR_BAD_PARAMETERS:
	case GNOME_VFS_ERROR_NOT_SUPPORTED:
	case GNOME_VFS_ERROR_INVALID_URI:
	case GNOME_VFS_ERROR_NOT_OPEN:
	case GNOME_VFS_ERROR_INVALID_OPEN_MODE:
	case GNOME_VFS_ERROR_NOT_SAME_FILE_SYSTEM:
		errno = EINVAL;
		break;
	case GNOME_VFS_ERROR_CORRUPTED_DATA: /* not sure about these */
	case GNOME_VFS_ERROR_WRONG_FORMAT:
	case GNOME_VFS_ERROR_BAD_FILE:
		errno = EBADF;
		break;
	case GNOME_VFS_ERROR_TOO_BIG:
		errno = E2BIG;
		break;
	case GNOME_VFS_ERROR_NO_SPACE:
		errno = ENOSPC;
		break;
	case GNOME_VFS_ERROR_READ_ONLY:
	case GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM:
		errno = EROFS;
		break;
	case GNOME_VFS_ERROR_TOO_MANY_OPEN_FILES:
		errno = EMFILE;
		break;
	case GNOME_VFS_ERROR_NOT_A_DIRECTORY:
		errno = ENOTDIR;
		break;
	case GNOME_VFS_ERROR_IN_PROGRESS:
		errno = EINPROGRESS;
		break;
	case GNOME_VFS_ERROR_INTERRUPTED:
		errno = EINTR;
		break;
	case GNOME_VFS_ERROR_FILE_EXISTS:
		errno = EEXIST;
		break;
	case GNOME_VFS_ERROR_LOOP:
		errno = ELOOP;
		break;
	case GNOME_VFS_ERROR_ACCESS_DENIED:
	case GNOME_VFS_ERROR_NOT_PERMITTED:
	case GNOME_VFS_ERROR_LOGIN_FAILED:
		errno = EPERM;
		break;
	case GNOME_VFS_ERROR_IS_DIRECTORY:
	case GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY: /* ?? */
		errno = EISDIR;
		break;
	case GNOME_VFS_ERROR_NO_MEMORY:
		errno = ENOMEM;
		break;
	case GNOME_VFS_ERROR_CANCELLED:
		errno = EINTR;
		break;
	case GNOME_VFS_ERROR_DIRECTORY_BUSY:
		errno = EBUSY;
		break;
	case GNOME_VFS_ERROR_TOO_MANY_LINKS:
		errno = EMLINK;
		break;
	case GNOME_VFS_ERROR_NAME_TOO_LONG:
		errno = ENAMETOOLONG;
		break;
	}
}

static gssize
tny_vfs_stream_write_to_stream (TnyStream *self, TnyStream *output)
{
	char tmp_buf[4096];
	gssize total = 0;
	gssize nb_read;
	gssize nb_written;

	g_assert (TNY_IS_STREAM (output));

	while (G_UNLIKELY (!tny_stream_is_eos (self))) {
		nb_read = tny_stream_read (self, tmp_buf, sizeof (tmp_buf));
		if (G_UNLIKELY (nb_read < 0))
			return -1;
		else if (G_LIKELY (nb_read > 0)) {
			nb_written = 0;
	
			while (G_LIKELY (nb_written < nb_read))
			{
				gssize len = tny_stream_write (output, tmp_buf + nb_written,
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

static gssize
tny_vfs_stream_read  (TnyStream *self, char *buffer, gsize n)
{
	TnyVfsStreamPriv *priv = TNY_VFS_STREAM_GET_PRIVATE (self);
	GnomeVFSFileSize nread = 0;
	GnomeVFSResult result;

	if (priv->bound_end != (~0))
		n = MIN (priv->bound_end - priv->position, n);

	result = gnome_vfs_read (priv->handle, buffer, n, &nread);

	if (nread > 0 && result == GNOME_VFS_OK)
		priv->position += nread;
	else {
		if ((result != GNOME_VFS_OK) && (result != GNOME_VFS_ERROR_EOF))
			nread = -1;
		else if (nread == 0)
			priv->eos = TRUE;
		tny_vfs_stream_set_errno (result);
	}

	return nread;
}

static gssize
tny_vfs_stream_write (TnyStream *self, const char *buffer, gsize n)
{
	TnyVfsStreamPriv *priv = TNY_VFS_STREAM_GET_PRIVATE (self);
	GnomeVFSFileSize nwritten = 0;
	GnomeVFSResult result;

	if (priv->bound_end != (~0))
		n = MIN (priv->bound_end - priv->position, n);

	result = gnome_vfs_write (priv->handle, buffer, n, &nwritten);

	if (nwritten > 0 && result == GNOME_VFS_OK) {
		priv->position += nwritten;
	} else {
		if ((result != GNOME_VFS_OK) && (result != GNOME_VFS_ERROR_EOF))
			nwritten = -1;
		else if (nwritten == 0)
			priv->eos = TRUE;
		tny_vfs_stream_set_errno (result);
	}

	return nwritten;
}

static gint
tny_vfs_stream_close (TnyStream *self)
{
	TnyVfsStreamPriv *priv = TNY_VFS_STREAM_GET_PRIVATE (self);
	GnomeVFSResult res;

	if (priv->handle == NULL)  {
		errno = EINVAL;
		return -1;
	}

	res = gnome_vfs_close (priv->handle);

	priv->handle = NULL;
	priv->eos = TRUE;

	if (res == GNOME_VFS_OK)
		return 0;

	tny_vfs_stream_set_errno (res);

	return -1;
}


/**
 * tny_vfs_stream_set_handle:
 * @self: A #TnyVfsStream instance
 * @handle: The #GnomeVFSHandle to write to or read from
 *
 * Set the #GnomeVFSHandle to play adaptor for
 *
 **/
void
tny_vfs_stream_set_handle (TnyVfsStream *self, GnomeVFSHandle *handle)
{
	TnyVfsStreamPriv *priv = TNY_VFS_STREAM_GET_PRIVATE (self);

	if (priv->handle) {
		gnome_vfs_close (priv->handle);
		priv->handle = NULL;
	}

	if (!handle)
		return;

	priv->handle = handle;
	priv->eos = FALSE;
	priv->position = 0;
	gnome_vfs_seek (handle, GNOME_VFS_SEEK_CURRENT, priv->position);

	return;
}

/**
 * tny_vfs_stream_new:
 * @handle: The #GnomeVFSHandle to write to or read from
 *
 * Create an adaptor instance between #TnyStream and #GnomeVFSHandle
 *
 * Return value: a new #TnyStream instance
 **/
TnyStream*
tny_vfs_stream_new (GnomeVFSHandle *handle)
{
	TnyVfsStream *self = g_object_new (TNY_TYPE_VFS_STREAM, NULL);

	tny_vfs_stream_set_handle (self, handle);

	return TNY_STREAM (self);
}

static void
tny_vfs_stream_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyVfsStream *self = (TnyVfsStream *)instance;
	TnyVfsStreamPriv *priv = TNY_VFS_STREAM_GET_PRIVATE (self);

	priv->eos = FALSE;
	priv->handle = NULL;
	priv->bound_start = 0;
	priv->bound_end = (~0);
	priv->position = 0;

	return;
}

static void
tny_vfs_stream_finalize (GObject *object)
{
	TnyVfsStream *self = (TnyVfsStream *)object;	
	TnyVfsStreamPriv *priv = TNY_VFS_STREAM_GET_PRIVATE (self);

	if (G_LIKELY (priv->handle))
		gnome_vfs_close (priv->handle);

	(*parent_class->finalize) (object);

	return;
}

static gint 
tny_vfs_flush (TnyStream *self)
{
	return 0;
}

static gboolean 
tny_vfs_is_eos (TnyStream *self)
{
	TnyVfsStreamPriv *priv = TNY_VFS_STREAM_GET_PRIVATE (self);

	return priv->eos;
}

static gint 
tny_vfs_reset (TnyStream *self)
{
	TnyVfsStreamPriv *priv = TNY_VFS_STREAM_GET_PRIVATE (self);
	gint retval = 0;
	GnomeVFSResult res;

	if (priv->handle == NULL) 
	{
		errno = EINVAL;
		return -1;
	}

	res = gnome_vfs_seek (priv->handle, GNOME_VFS_SEEK_START, 0);

	if (res == GNOME_VFS_OK) {
		priv->position = 0;
	} else {
		tny_vfs_stream_set_errno (res);
		retval = -1;
	}
	priv->eos = FALSE;

	return retval;
}



static off_t 
tny_vfs_seek (TnySeekable *self, off_t offset, int policy)
{
	TnyVfsStreamPriv *priv = TNY_VFS_STREAM_GET_PRIVATE (self);
	GnomeVFSFileSize real = 0;
	GnomeVFSResult result;
	GnomeVFSHandle *handle = priv->handle;

	switch (policy) {
	case SEEK_SET:
		real = offset;
		break;
	case SEEK_CUR:
		real = priv->position + offset;
		break;
	case SEEK_END:
		if (priv->bound_end == (~0)) {
			result = gnome_vfs_seek (handle, GNOME_VFS_SEEK_END, offset);
			if (result != GNOME_VFS_OK)
				return -1;
			gnome_vfs_tell (handle, &real);
			if (real != -1) {
				if (real<priv->bound_start)
					real = priv->bound_start;
				priv->position = real;
			}
			return real;
		}
		real = priv->bound_end + offset;
		break;
	}

	if (priv->bound_end != (~0))
		real = MIN (real, priv->bound_end);
	real = MAX (real, priv->bound_start);

	result = gnome_vfs_seek (handle, GNOME_VFS_SEEK_START, real);
	if (result != GNOME_VFS_OK)
		return -1;

	if (real != priv->position && priv->eos)
		priv->eos = FALSE;

	priv->position = real;

	return real;
}

static off_t
tny_vfs_tell (TnySeekable *self)
{
	TnyVfsStreamPriv *priv = TNY_VFS_STREAM_GET_PRIVATE (self);
	return priv->position;
}

static gint 
tny_vfs_set_bounds (TnySeekable *self, off_t start, off_t end)
{
	TnyVfsStreamPriv *priv = TNY_VFS_STREAM_GET_PRIVATE (self);
	priv->bound_end = end;
	priv->bound_start = start;
	return 0;
}

static void
tny_stream_init (gpointer g, gpointer iface_data)
{
	TnyStreamIface *klass = (TnyStreamIface *)g;

	klass->reset= tny_vfs_reset;
	klass->flush= tny_vfs_flush;
	klass->is_eos= tny_vfs_is_eos;
	klass->read= tny_vfs_stream_read;
	klass->write= tny_vfs_stream_write;
	klass->close= tny_vfs_stream_close;
	klass->write_to_stream= tny_vfs_stream_write_to_stream;

	return;
}


static void
tny_seekable_init (gpointer g, gpointer iface_data)
{
	TnySeekableIface *klass = (TnySeekableIface *)g;

	klass->seek= tny_vfs_seek;
	klass->tell= tny_vfs_tell;
	klass->set_bounds= tny_vfs_set_bounds;

	return;
}

static void 
tny_vfs_stream_class_init (TnyVfsStreamClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_vfs_stream_finalize;

	g_type_class_add_private (object_class, sizeof (TnyVfsStreamPriv));

	return;
}

static gpointer 
tny_vfs_stream_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyVfsStreamClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_vfs_stream_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyVfsStream),
			0,      /* n_preallocs */
			tny_vfs_stream_instance_init,/* instance_init */
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
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyVfsStream",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_STREAM, 
				     &tny_stream_info);
	
	g_type_add_interface_static (type, TNY_TYPE_SEEKABLE, 
				     &tny_seekable_info);
	
	return GUINT_TO_POINTER (type);
}

GType 
tny_vfs_stream_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_vfs_stream_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
