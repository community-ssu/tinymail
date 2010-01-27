/*
 * Author:
 *  Philip Van Hoof <pvanhoof@gnome.org>
 *
 * Copyright 1999, 2007 Philip Van Hoof
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "camel-stream-gzip.h"

static CamelObjectClass *parent_class = NULL;

#define CSZ_CLASS(so) CAMEL_STREAM_GZIP_CLASS(CAMEL_OBJECT_GET_CLASS(so))


static ssize_t z_stream_read (CamelStream *stream, char *buffer, size_t n)
{
	ssize_t haveread = 0, retval = 0;
	CamelStreamGZip *self = (CamelStreamGZip *) stream;
	z_stream c_stream = * (self->r_stream);

	if (self->read_mode == CAMEL_STREAM_GZIP_ZIP)
	{
		char *mem = g_malloc0 (n);
		c_stream.next_out = (Bytef *) buffer;
		c_stream.avail_out = n;
		haveread = camel_stream_read (self->real, mem, n);
		c_stream.next_in = (Bytef *) mem;
		c_stream.avail_in = haveread;
		deflate (&c_stream, Z_FINISH);
		retval = n - c_stream.avail_out;
		g_free (mem);
	} else
	{
		int block_size = (n < 1000) ? (n < 10 ? 2 : 10) : n / 100;
		char *mem = g_malloc0 (block_size);

		haveread = block_size;
		c_stream.next_out = (Bytef *) buffer;
		c_stream.avail_out = n;

		while (haveread == block_size && c_stream.avail_out > 0)
		{
			haveread = camel_stream_read (self->real, mem, block_size);
			c_stream.next_in  = (Bytef *) mem;
			c_stream.avail_in = haveread;
			inflate (&c_stream, Z_NO_FLUSH);
		}
		retval = n - c_stream.avail_out;
		g_free (mem);
	}

	return retval;
}


static ssize_t z_stream_write (CamelStream *stream, const char *buffer, size_t n)
{
	CamelStreamGZip *self = (CamelStreamGZip *) stream;
	z_stream c_stream = * (self->w_stream);
	ssize_t retval = 0;

	if (self->write_mode == CAMEL_STREAM_GZIP_ZIP)
	{
		char *mem = g_malloc0 (n);

		c_stream.next_in  = (Bytef *) buffer;
		c_stream.avail_in = n;
		c_stream.next_out = (Bytef *) mem;
		c_stream.avail_out = n;

		deflate (&c_stream, Z_FINISH);

		camel_stream_write (self->real, mem, n - c_stream.avail_out);

		retval = n;
		g_free (mem);

	} else
	{
		char *mem = g_malloc0 (n);

		c_stream.next_in = (Bytef *) buffer;
		c_stream.avail_in = n;
		c_stream.next_out = (Bytef *) mem;
		c_stream.avail_out = n;

		while (c_stream.avail_in > 0)
		{
			inflate (&c_stream, Z_NO_FLUSH);

			camel_stream_write (self->real, mem, n - c_stream.avail_out);
			retval += n - c_stream.avail_out;

			c_stream.next_out = (Bytef *) mem;
			c_stream.avail_out = n;
		}
		g_free (mem);
	}

	return retval;
}

static int z_stream_flush (CamelStream *stream)
{
	CamelStreamGZip *self = (CamelStreamGZip *) stream;
	return camel_stream_flush (self->real);
}

static int z_stream_close (CamelStream *stream)
{
	CamelStreamGZip *self = (CamelStreamGZip *) stream;
	z_stream_flush (stream);
	return camel_stream_close (self->real);
}

static gboolean z_stream_eos (CamelStream *stream)
{
	CamelStreamGZip *self = (CamelStreamGZip *) stream;
	return camel_stream_eos (self->real);
}

static int z_stream_reset (CamelStream *stream)
{
	CamelStreamGZip *self = (CamelStreamGZip *) stream;

	if (self->read_mode == CAMEL_STREAM_GZIP_ZIP)
		deflateReset (self->r_stream);
	else
		inflateReset (self->r_stream);


	if (self->write_mode == CAMEL_STREAM_GZIP_ZIP)
		deflateReset (self->w_stream);
	else
		inflateReset (self->w_stream);


	return 0;
}

static void
camel_stream_gzip_class_init (CamelStreamClass *camel_stream_gzip_class)
{
	CamelStreamClass *camel_stream_class = (CamelStreamClass *)camel_stream_gzip_class;

	parent_class = camel_type_get_global_classfuncs( CAMEL_OBJECT_TYPE );

	/* virtual method definition */
	camel_stream_class->read = z_stream_read;
	camel_stream_class->write = z_stream_write;
	camel_stream_class->close = z_stream_close;
	camel_stream_class->flush = z_stream_flush;
	camel_stream_class->eos = z_stream_eos;
	camel_stream_class->reset = z_stream_reset;
}

static void
camel_stream_gzip_finalize (CamelObject *object)
{
	CamelStreamGZip *self = (CamelStreamGZip *) object;

	camel_object_unref (CAMEL_OBJECT (self->real));

	if (self->read_mode == CAMEL_STREAM_GZIP_ZIP)
		deflateEnd (self->r_stream);
	else
		inflateEnd (self->r_stream);


	if (self->write_mode == CAMEL_STREAM_GZIP_ZIP)
		deflateEnd (self->w_stream);
	else
		inflateEnd (self->w_stream);

	g_free (self->r_stream);
	g_free (self->w_stream);

	return;
}

CamelType
camel_stream_gzip_get_type (void)
{
	static CamelType camel_stream_gzip_type = CAMEL_INVALID_TYPE;

	if (camel_stream_gzip_type == CAMEL_INVALID_TYPE)
	{
		camel_stream_gzip_type = camel_type_register(
			camel_stream_get_type(),
			"CamelStreamGZip",
			sizeof( CamelStreamGZip ),
			sizeof( CamelStreamGZipClass ),
			(CamelObjectClassInitFunc) camel_stream_gzip_class_init,
			NULL,
			NULL,
			(CamelObjectFinalizeFunc) camel_stream_gzip_finalize );
	}

	return camel_stream_gzip_type;
}

static int
set_mode (z_stream *stream, int level, int mode)
{
	int retval;
	if (mode == CAMEL_STREAM_GZIP_ZIP)
		retval = deflateInit2 (stream, level, Z_DEFLATED, -MAX_WBITS,
			MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	else
		retval = inflateInit2 (stream, -MAX_WBITS);
	return retval;
}

CamelStream *
camel_stream_gzip_new (CamelStream *real, int level, int read_mode, int write_mode)
{
	CamelStreamGZip *self = (CamelStreamGZip *) camel_object_new (camel_stream_gzip_get_type ());
	int retval;

	camel_object_ref (CAMEL_OBJECT (real));
	self->real = real;

	self->r_stream = g_new0 (z_stream, 1);
	self->w_stream = g_new0 (z_stream, 1);
	self->level = level;
	self->read_mode = read_mode;
	self->write_mode = write_mode;

	retval = set_mode (self->r_stream, level, read_mode);
	if (retval != Z_OK)
	{
		camel_object_unref (self);
		return NULL;
	}


	retval = set_mode (self->w_stream, level, write_mode);
	if (retval != Z_OK)
	{
		camel_object_unref (self);
		return NULL;
	}


	return CAMEL_STREAM (self);
}
