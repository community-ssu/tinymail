/* tinymail - Tiny Mail
 * Copyright (C) 2006-2008 Philip Van Hoof <pvanhoof@gnome.org>
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

#include <string.h>
#include <camel/camel.h>

#include <gtk/gtk.h>

#define TEST "This is a test to compres to compress to compres"

int main (int argc, char **argv)
{
	CamelStream *in, *out, *com, *de;
	CamelInternetAddress *addr;

	g_thread_init (NULL);
	camel_type_init ();

	addr = camel_internet_address_new ();

	out = camel_stream_fs_new_with_name ("/tmp/testing", O_CREAT, S_IRWXU);
	in = camel_stream_mem_new_with_buffer (TEST, strlen (TEST));

	com = camel_stream_gzip_new (in, 7, CAMEL_STREAM_GZIP_ZIP, CAMEL_STREAM_GZIP_ZIP);
	de = camel_stream_gzip_new (out, 7, CAMEL_STREAM_GZIP_UNZIP, CAMEL_STREAM_GZIP_UNZIP);

	camel_stream_write_to_stream (com, de);

	camel_object_unref (CAMEL_OBJECT (com));
	camel_object_unref (CAMEL_OBJECT (de));
	camel_object_unref (CAMEL_OBJECT (out));
	camel_object_unref (CAMEL_OBJECT (in));

	return 0;

}

