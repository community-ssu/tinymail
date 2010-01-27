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
 */

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <tny-stream.h>
#include <tny-test-stream.h>

static GObjectClass *parent_class = NULL;

static gint 
tny_test_stream_reset (TnyStream *self)
{
	TnyTestStream *me = TNY_TEST_STREAM (self);

	if (me->wrote)
		g_free (me->wrote);

	me->wrote = NULL;
	me->eos_count = 0;

	return 0;
}


static ssize_t
tny_test_stream_write_to_stream (TnyStream *self, TnyStream *output)
{
	char tmp_buf[42];
	ssize_t total = 0;
	ssize_t nb_read;
	ssize_t nb_written;
	
	while (!tny_stream_is_eos (self)) 
	{

		nb_read = tny_stream_read (self, tmp_buf, sizeof (tmp_buf));

		if (nb_read < 0)
		{
			return -1;
		} else if (nb_read > 0) 
		{
			nb_written = 0;
	
			while (nb_written < nb_read) {
				ssize_t len = tny_stream_write (output, tmp_buf + nb_written,
								  nb_read - nb_written);
				if (len < 0)
					return -1;
				nb_written += len;
			}
			total += nb_written;
		}
	}
	return total;
}

static ssize_t
tny_test_stream_read  (TnyStream *self, char *buffer, size_t n)
{
	int i = 0;

	for (i = 0; i < n; i++)
	{
		if (i % 2 == 1)
			buffer [i] = '2';
		else
			buffer [i] = '4';		
	}

	buffer [i] = '\0';

	return i;
}

static ssize_t
tny_test_stream_write (TnyStream *self, const char *buffer, size_t n)
{

	gint i=0;
	TnyTestStream *me = TNY_TEST_STREAM (self);

	if (me->wrote)
		g_free (me->wrote);

	me->wrote = NULL;

	me->wrote = (gchar*) g_malloc (sizeof (gchar) * n);

	for (i=0; i < n; i++)
		me->wrote [i] = buffer [i];

	me->wrote[i] = '\0';

	return i;
}

static gint
tny_test_stream_close (TnyStream *self)
{
	return tny_test_stream_reset (self);
}

static gboolean
tny_test_stream_is_eos (TnyStream *self)
{
	TnyTestStream *me = TNY_TEST_STREAM (self);

	me->eos_count++;

	return (me->eos_count > 1);
}


/**
 * tny_test_stream_new:
 *
 * Create an test stream that streams the answer to all questions 
 *
 * Return value: a new #TnyStream instance
 **/
TnyTestStream*
tny_test_stream_new (void)
{
	TnyTestStream *self = g_object_new (TNY_TYPE_TEST_STREAM, NULL);

	self->wrote = NULL;
	self->eos_count = 0;

	return self;
}

static void
tny_test_stream_instance_init (GTypeInstance *instance, gpointer g_class)
{
	return;
}

static void
tny_test_stream_finalize (GObject *object)
{
	return;
}

static void
tny_stream_init (gpointer g, gpointer iface_data)
{
	TnyStreamIface *klass = (TnyStreamIface *)g;

	klass->read= tny_test_stream_read;
	klass->write= tny_test_stream_write;
	klass->close= tny_test_stream_close;
	klass->write_to_stream= tny_test_stream_write_to_stream;
	klass->is_eos= tny_test_stream_is_eos;
	klass->reset= tny_test_stream_reset;

	return;
}

static void 
tny_test_stream_class_init (TnyTestStreamClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_test_stream_finalize;

	return;
}

GType 
tny_test_stream_get_type (void)
{
	static GType type = 0;

	if (type == 0) 
	{
		static const GTypeInfo info = 
		{
		  sizeof (TnyTestStreamClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_test_stream_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyTestStream),
		  0,      /* n_preallocs */
		  tny_test_stream_instance_init    /* instance_init */
		};

		static const GInterfaceInfo tny_stream_info = 
		{
		  (GInterfaceInitFunc) tny_stream_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		type = g_type_register_static (G_TYPE_OBJECT,
			"TnyTestStream",
			&info, 0);

		g_type_add_interface_static (type, TNY_TYPE_STREAM, 
			&tny_stream_info);
	}

	return type;
}
