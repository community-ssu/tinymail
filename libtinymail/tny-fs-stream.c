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

/* Parts of this file were copied from the Evolution project. This is the
 * original copyright of that file. The file's source code was distributed as 
 * LGPL content. Therefore the copyright of Tinymail is compatible with this.
 *
 * Authors: Bertrand Guiheneuf <bertrand@helixcode.com>
 *          Michael Zucchi <notzed@ximian.com>
 *          Philip Van Hoof <pvanhoof@gnome.org>
 *
 * Copyright 1999-2003 Ximian, Inc. (www.ximian.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

/**
 * TnyFsStream:
 *
 * A stream that adapts a filedescriptor to the #TnyStream API
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <tny-fs-stream.h>

static GObjectClass *parent_class = NULL;

typedef struct _TnyFsStreamPriv TnyFsStreamPriv;

struct _TnyFsStreamPriv
{
	int fd;
	off_t position;		/* current postion in the stream */
	off_t bound_start;	/* first valid position */
	off_t bound_end;	/* first invalid position */
	gboolean eos;
};

#define TNY_FS_STREAM_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_FS_STREAM, TnyFsStreamPriv))


static gssize
tny_fs_stream_write_to_stream (TnyStream *self, TnyStream *output)
{
	char tmp_buf[4096];
	gssize total = 0;
	gssize nb_read;
	gssize nb_written;
	
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
tny_fs_stream_read (TnyStream *self, char *buffer, gsize n)
{
	TnyFsStreamPriv *priv = TNY_FS_STREAM_GET_PRIVATE (self);
	gssize nread;

	if (priv->bound_end != (~0))
		n = MIN (priv->bound_end - priv->position, n);

	if ((nread = read (priv->fd, buffer, n)) > 0)
		priv->position += nread;
	else if (nread == 0)
		priv->eos = TRUE;

	return nread;
}

static gssize
tny_fs_stream_write (TnyStream *self, const char *buffer, gsize n)
{
	TnyFsStreamPriv *priv = TNY_FS_STREAM_GET_PRIVATE (self);
	gssize nwritten;

	if (priv->bound_end != (~0))
		n = MIN (priv->bound_end - priv->position, n);

	if ((nwritten = write (priv->fd, buffer, n)) > 0)
		priv->position += nwritten;

	return nwritten;
}


static gint
tny_fs_stream_close (TnyStream *self)
{
	TnyFsStreamPriv *priv = TNY_FS_STREAM_GET_PRIVATE (self);
	if (close (priv->fd) == -1)
		return -1;
	priv->fd = -1;
	return 0;
}


/**
 * tny_fs_stream_set_fd:
 * @self: A #TnyFsStream instance
 * @fd: The file descriptor to write to or read from
 *
 * Set the file descriptor to play adaptor for
 *
 * since: 1.0
 * audience: tinymail-developer
 **/
void
tny_fs_stream_set_fd (TnyFsStream *self, int fd)
{
	TnyFsStreamPriv *priv = TNY_FS_STREAM_GET_PRIVATE (self);
	if (fd == -1)
		return;
	if (priv->fd != -1)
		close (priv->fd);
	priv->fd = fd;
	priv->position = lseek (priv->fd, 0, SEEK_CUR);
	if (priv->position == -1)
		priv->position = 0;
	return;
}

/**
 * tny_fs_stream_new:
 * @fd: The file descriptor to write to or read from
 *
 * Create an adaptor instance between #TnyStream and a file descriptor. Note 
 * that you must not to close() fd yourself. The destructor will do that for
 * you.
 *
 * Therefore use it with care. It's more or less an exception in the framework,
 * although whether or not you call it an exception depends on your point of
 * view.
 *
 * The the instance will on top of close() when destructing also fsync() the
 * filedescriptor. It does this depending on its mood, the weather and your
 * wive's periods using a complex algorithm that abuses your privacy and might
 * kill your cat and dog (yes, both of them).
 *
 * returns: (caller-owns): a new #TnyStream instance
 * since: 1.0
 * audience: tinymail-developer
 **/
TnyStream*
tny_fs_stream_new (int fd)
{
	TnyFsStream *self = g_object_new (TNY_TYPE_FS_STREAM, NULL);
	tny_fs_stream_set_fd (self, fd);
	return TNY_STREAM (self);
}

static void
tny_fs_stream_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyFsStream *self = (TnyFsStream *)instance;
	TnyFsStreamPriv *priv = TNY_FS_STREAM_GET_PRIVATE (self);

	priv->fd = -1;
	priv->position = 0;
	priv->bound_start = 0;
	priv->bound_end = (~0);

	return;
}

static void
tny_fs_stream_finalize (GObject *object)
{
	TnyFsStream *self = (TnyFsStream *)object;
	TnyFsStreamPriv *priv = TNY_FS_STREAM_GET_PRIVATE (self);
	if (priv->fd != -1) {
		fsync (priv->fd);
		close (priv->fd);
	}
	priv->fd = -1;
	priv->eos = TRUE;
	(*parent_class->finalize) (object);
	return;
}

static gint 
tny_fs_flush (TnyStream *self)
{
	TnyFsStreamPriv *priv = TNY_FS_STREAM_GET_PRIVATE (self);
	return fsync(priv->fd);
}

static gboolean 
tny_fs_is_eos (TnyStream *self)
{
	TnyFsStreamPriv *priv = TNY_FS_STREAM_GET_PRIVATE (self);
	return priv->eos;
}

static gint 
tny_fs_reset (TnyStream *self)
{
	TnyFsStreamPriv *priv = TNY_FS_STREAM_GET_PRIVATE (self);
	priv->position = lseek (priv->fd, 0, SEEK_SET);
	priv->eos = FALSE;
	return 0;
}

static off_t 
tny_fs_seek (TnySeekable *self, off_t offset, int policy)
{
	TnyFsStreamPriv *priv = TNY_FS_STREAM_GET_PRIVATE (self);
	off_t real = 0;

	switch (policy) {
	case SEEK_SET:
		real = offset;
		break;
	case SEEK_CUR:
		real = priv->position + offset;
		break;
	case SEEK_END:
		if (priv->bound_end == (~0)) {
			real = lseek(priv->fd, offset, SEEK_END);
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

	real = lseek(priv->fd, real, SEEK_SET);
	if (real == -1)
		return -1;

	if (real != priv->position && priv->eos)
		priv->eos = FALSE;

	priv->position = real;

	return real;
}

static off_t
tny_fs_tell (TnySeekable *self)
{
	TnyFsStreamPriv *priv = TNY_FS_STREAM_GET_PRIVATE (self);
	return priv->position;
}

static gint 
tny_fs_set_bounds (TnySeekable *self, off_t start, off_t end)
{
	TnyFsStreamPriv *priv = TNY_FS_STREAM_GET_PRIVATE (self);
	priv->bound_end = end;
	priv->bound_start = start;
	return 0;
}


static void
tny_stream_init (gpointer g, gpointer iface_data)
{
	TnyStreamIface *klass = (TnyStreamIface *)g;

	klass->reset= tny_fs_reset;
	klass->flush= tny_fs_flush;
	klass->is_eos= tny_fs_is_eos;
	klass->read= tny_fs_stream_read;
	klass->write= tny_fs_stream_write;
	klass->close= tny_fs_stream_close;
	klass->write_to_stream= tny_fs_stream_write_to_stream;

	return;
}

static void
tny_seekable_init (gpointer g, gpointer iface_data)
{
	TnySeekableIface *klass = (TnySeekableIface *)g;

	klass->seek= tny_fs_seek;
	klass->tell= tny_fs_tell;
	klass->set_bounds= tny_fs_set_bounds;

	return;
}

static void 
tny_fs_stream_class_init (TnyFsStreamClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	object_class->finalize = tny_fs_stream_finalize;
	g_type_class_add_private (object_class, sizeof (TnyFsStreamPriv));

	return;
}

static gpointer
tny_fs_stream_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyFsStreamClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_fs_stream_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyFsStream),
			0,      /* n_preallocs */
			tny_fs_stream_instance_init,   /* instance_init */
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
				       "TnyFsStream",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_STREAM, 
				     &tny_stream_info);
	
	g_type_add_interface_static (type, TNY_TYPE_SEEKABLE, 
				     &tny_seekable_info);
	
	return GSIZE_TO_POINTER (type);
}

GType 
tny_fs_stream_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_fs_stream_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
