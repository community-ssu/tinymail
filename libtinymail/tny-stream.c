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
 * TnyStream:
 *
 * A stream type
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <tny-stream.h>


/**
 * tny_stream_write_to_stream:
 * @self: a #TnyStream
 * @output: a #TnyStream
 * 
 * Write @self to @output in an efficient way
 *
 * returns: the number of bytes written to the output stream, or -1 on error along with setting errno.
 * since: 1.0
 * audience: application-developer
 **/
gssize
tny_stream_write_to_stream (TnyStream *self, TnyStream *output)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_STREAM (self));
	g_assert (output);
	g_assert (TNY_IS_STREAM (output));
	g_assert (TNY_STREAM_GET_IFACE (self)->write_to_stream!= NULL);
#endif

	return TNY_STREAM_GET_IFACE (self)->write_to_stream(self, output);
}

/**
 * tny_stream_read:
 * @self: a #TnyStream
 * @buffer: a buffer that is at least n in size
 * @n: the max amount of bytes to read from self and to write into buffer
 * 
 * Read n bytes from @self and write it into @buffer. It's your responsibility
 * to pass a buffer that is large enough to hold n bytes.
 *
 * returns: the number of bytes actually read, or -1 on error and set errno.
 * since: 1.0
 * audience: application-developer
 **/
gssize
tny_stream_read  (TnyStream *self, char *buffer, gsize n)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_STREAM (self));
	g_assert (TNY_STREAM_GET_IFACE (self)->read != NULL);
#endif

	return TNY_STREAM_GET_IFACE (self)->read(self, buffer, n);
}

/**
 * tny_stream_write:
 * @self: a #TnyStream
 * @buffer: a buffer that has at least n bytes
 * @n: the amount of bytes to read from buffer and to write to self
 * 
 * Write n bytes of @buffer into @self. It's your responsibility to pass
 * a buffer that has at least n bytes.
 *
 * returns: the number of bytes written to the stream, or -1 on error along with setting errno.
 * since: 1.0
 * audience: application-developer
 **/
gssize
tny_stream_write (TnyStream *self, const char *buffer, gsize n)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_STREAM (self));
	g_assert (TNY_STREAM_GET_IFACE (self)->write != NULL);
#endif

	return TNY_STREAM_GET_IFACE (self)->write(self, buffer, n);
}


/**
 * tny_stream_flush:
 * @self: a #TnyStream
 * 
 * Flushes any buffered data to the stream's backing store. 
 *
 * returns: 0 on success or -1 on fail along with setting errno.
 * since: 1.0
 * audience: application-developer
 **/
gint
tny_stream_flush (TnyStream *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_STREAM (self));
	g_assert (TNY_STREAM_GET_IFACE (self)->flush != NULL);
#endif

	return TNY_STREAM_GET_IFACE (self)->flush(self);
}

/**
 * tny_stream_close:
 * @self: a #TnyStream
 * 
 * Closes the stream
 *
 * returns: 0 on success or -1 on fail.
 * since: 1.0
 * audience: application-developer
 **/
gint
tny_stream_close (TnyStream *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_STREAM (self));
	g_assert (TNY_STREAM_GET_IFACE (self)->close != NULL);
#endif

	return TNY_STREAM_GET_IFACE (self)->close(self);
}


/**
 * tny_stream_is_eos:
 * @self: a #TnyStream
 * 
 * Tests if there are bytes left to read on @self.
 *
 * returns: TRUE on EOS or FALSE otherwise.
 * since: 1.0
 * audience: application-developer
 **/
gboolean
tny_stream_is_eos   (TnyStream *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_STREAM (self));
	g_assert (TNY_STREAM_GET_IFACE (self)->is_eos != NULL);
#endif

	return TNY_STREAM_GET_IFACE (self)->is_eos(self);
}

/**
 * tny_stream_reset:
 * @self: a #TnyStream
 * 
 * Resets @self. That is, put it in a state where it can be read from 
 * the beginning again.
 *
 * returns: 0 on success or -1 on error along with setting errno.
 * since: 1.0
 * audience: application-developer
 **/
gint
tny_stream_reset (TnyStream *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_STREAM (self));
	g_assert (TNY_STREAM_GET_IFACE (self)->reset != NULL);
#endif

	return TNY_STREAM_GET_IFACE (self)->reset(self);
}

static void
tny_stream_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_stream_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyStreamIface),
			tny_stream_base_init,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL,   /* instance_init */
			NULL
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyStream", &info, 0);
	
	return GUINT_TO_POINTER (type);
}

GType
tny_stream_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_stream_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
