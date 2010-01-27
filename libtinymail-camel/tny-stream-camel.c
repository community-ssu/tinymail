/* libtinymail-camel - The Tiny Mail base library for Camel
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

#include <tny-stream-camel.h>
#include <tny-camel-stream.h>
#include <tny-camel-shared.h>

static CamelStreamClass *parent_class = NULL;

/**
 * tny_stream_camel_write_to_stream:
 * @self: a #TnyCamelStream object
 * @output: a #TnyStream object to write to
 * 
 * Write self to output (copy it) in an efficient way
 *
 * Return value: the number of bytes written to the output stream, or -1 on 
 * error along with setting errno.
 **/
gssize 
tny_stream_camel_write_to_stream (TnyStreamCamel *self, TnyStream *output)
{
	CamelStream *stream = CAMEL_STREAM (self);
	char tmp_buf[4096];
	gssize total = 0;
	gssize nb_read;
	gssize nb_written;
	g_return_val_if_fail (CAMEL_IS_STREAM (stream), -1);
	g_return_val_if_fail (TNY_IS_STREAM (output), -1);

	while (G_LIKELY (!camel_stream_eos (stream)))
	{
		nb_read = camel_stream_read (stream, tmp_buf, sizeof (tmp_buf));
		if (G_UNLIKELY (nb_read < 0))
			return -1;
		else if (G_LIKELY (nb_read > 0))
		{
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
tny_stream_camel_read (CamelStream *stream, char *buffer, gsize n)
{
	TnyStreamCamel *self = (TnyStreamCamel *)stream;
	return tny_stream_read (self->stream, buffer, n);
}

static gssize
tny_stream_camel_write (CamelStream *stream, const char *buffer, gsize n)
{
	TnyStreamCamel *self = (TnyStreamCamel *)stream;
	return tny_stream_write (self->stream, buffer, n);
}

static int
tny_stream_camel_close (CamelStream *stream)
{
	TnyStreamCamel *self = (TnyStreamCamel *)stream;
	return tny_stream_close (self->stream);
}

static gboolean
tny_stream_camel_eos (CamelStream *stream)
{
	TnyStreamCamel *self = (TnyStreamCamel *)stream;
	return tny_stream_is_eos (self->stream);
}

static int
tny_stream_camel_flush (CamelStream *stream)
{
	TnyStreamCamel *self = (TnyStreamCamel *)stream;
	return tny_stream_flush (self->stream);
}


static int
tny_stream_camel_reset (CamelStream *stream)
{
	TnyStreamCamel *self = (TnyStreamCamel *)stream;
	return tny_stream_reset (self->stream);
}


static off_t 
tny_stream_camel_seek (CamelSeekableStream *stream, off_t offset, CamelStreamSeekPolicy policy)
{
	TnyStreamCamel *self = (TnyStreamCamel *)stream;
	off_t retval = -1;

	if (TNY_IS_SEEKABLE (self->stream))
		retval = tny_seekable_seek ((TnySeekable *) self->stream, offset, (int) policy);

	return retval;
}

static off_t 
tny_stream_camel_tell (CamelSeekableStream *stream)
{
	TnyStreamCamel *self = (TnyStreamCamel *)stream;
	off_t retval = -1;

	if (TNY_IS_SEEKABLE (self->stream))
		retval = tny_seekable_tell ((TnySeekable *) self->stream);

	return retval;
}

int 
tny_stream_camel_set_bouds (CamelSeekableStream *stream, off_t start, off_t end)
{
	TnyStreamCamel *self = (TnyStreamCamel *)stream;
	int retval = -1;

	if (TNY_IS_SEEKABLE (self->stream))
		retval = tny_seekable_set_bounds ((TnySeekable *) self->stream, start, end);

	return retval;
}

static void
tny_stream_camel_init (CamelObject *object)
{
	TnyStreamCamel *self = (TnyStreamCamel *)object;
	self->stream = NULL;
	return;
}


static void
tny_stream_camel_finalize (CamelObject *object)
{
	TnyStreamCamel *self = (TnyStreamCamel *)object;

	if (self->stream)
		g_object_unref (self->stream);

	return;
}


static void
tny_stream_camel_class_init (TnyStreamCamelClass *klass)
{
	((CamelStreamClass *)klass)->read = tny_stream_camel_read;
	((CamelStreamClass *)klass)->write = tny_stream_camel_write;
	((CamelStreamClass *)klass)->close = tny_stream_camel_close;
	((CamelStreamClass *)klass)->eos = tny_stream_camel_eos;
	((CamelStreamClass *)klass)->reset = tny_stream_camel_reset;
	((CamelStreamClass *)klass)->flush = tny_stream_camel_flush;


	((CamelSeekableStreamClass *)klass)->seek = tny_stream_camel_seek;
	((CamelSeekableStreamClass *)klass)->tell = tny_stream_camel_tell;
	((CamelSeekableStreamClass *)klass)->set_bounds = tny_stream_camel_set_bouds;

	return;
}

/**
 * tny_stream_camel_get_type:
 *
 * CamelType system helper function
 *
 * Return value: a CamelType
 **/
CamelType
tny_stream_camel_get_type (void)
{
	static CamelType type = CAMEL_INVALID_TYPE;
	
	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);

		camel_type_init ();
		_camel_type_init_done = TRUE;
	}

	if (G_UNLIKELY (type == CAMEL_INVALID_TYPE)) 
	{
		parent_class = (CamelStreamClass *)camel_stream_get_type();
		type = camel_type_register ((CamelType)parent_class,
					    "TnyStreamCamel",
					    sizeof (TnyStreamCamel),
					    sizeof (TnyStreamCamelClass),
					    (CamelObjectClassInitFunc) tny_stream_camel_class_init,
					    NULL,
					    (CamelObjectInitFunc) tny_stream_camel_init,
					    (CamelObjectFinalizeFunc) tny_stream_camel_finalize);
	}
	
	return type;
}

/**
 * tny_stream_camel_set_stream:
 * @self: A #TnyStreamCamel object
 * @stream: A #TnyStream object
 *
 * Set the stream to play proxy for
 *
 **/
void
tny_stream_camel_set_stream (TnyStreamCamel *self, TnyStream *stream)
{

	if (self->stream)
		g_object_unref (G_OBJECT (self->stream));

	g_object_ref (G_OBJECT (stream)); 

	self->stream = stream;

	return;
}


/**
 * tny_stream_camel_new:
 * @stream: A #TnyStream stream to play proxy for
 *
 * Create a new #CamelStream instance implemented as a proxy
 * for a #TnyStream
 * 
 * Return value: A new #CamelStream instance implemented as a proxy
 * for a #TnyStream
 **/
CamelStream *
tny_stream_camel_new (TnyStream *stream)
{
	TnyStreamCamel *self = (TnyStreamCamel *) camel_object_new (tny_stream_camel_get_type());

	tny_stream_camel_set_stream (self, stream);

	return (CamelStream*) self;
}
