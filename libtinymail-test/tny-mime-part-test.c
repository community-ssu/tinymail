/* tinymail - Tiny Mail unit test
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

#include "check_libtinymail.h"

/* We are going to test the camel implementation */
#include <tny-camel-mime-part.h>
#include <tny-stream.h>
#include <tny-test-stream.h>
#include <tny-stream-camel.h>
#include <tny-camel-stream.h>

#include <camel/camel-folder.h>
#include <camel/camel.h>
#include <camel/camel-folder-summary.h>


static TnyMimePart *iface = NULL;
static gchar *str;

static void
tny_mime_part_test_setup (void)
{
	/* Don't ask me why, I think this is a Camel bug */
	CamelInternetAddress *addr = camel_internet_address_new ();
	camel_object_unref (CAMEL_OBJECT (addr));

	iface = tny_camel_mime_part_new ();

	return;
}

static void 
tny_mime_part_test_teardown (void)
{
	g_object_unref (G_OBJECT (iface));

	return;
}

START_TEST (tny_mime_part_test_set_content_location)
{
	const gchar *str_in = "testcontentlocation", *str_out;
	
	tny_mime_part_set_content_location (iface, str_in);
	str_out = tny_mime_part_get_content_location (iface);

	str = g_strdup_printf ("Unable to set content location! (%s) vs. (%s)\n",
		str_in, str_out);

	fail_unless(!strcmp (str_in, str_out), str);

	g_free (str);

	return;
}
END_TEST


START_TEST (tny_mime_part_test_set_description)
{
	const gchar *str_in = "test description", *str_out;
	
	tny_mime_part_set_description (iface, str_in);
	str_out = tny_mime_part_get_description (iface);

	fail_unless(!strcmp (str_in, str_out), "Unable to set description!\n");

	return;
}
END_TEST

START_TEST (tny_mime_part_test_set_content_id)
{
	const gchar *str_in = "testcontentid", *str_out;
	
	tny_mime_part_set_content_id (iface, str_in);
	str_out = tny_mime_part_get_content_id (iface);

	fail_unless(!strcmp (str_in, str_out), "Unable to set content id!\n");

	return;
}
END_TEST
    
START_TEST (tny_mime_part_test_set_filename)
{
	const gchar *str_in = "test_filename.txt", *str_out;
	
	tny_mime_part_set_filename (iface, str_in);
	str_out = tny_mime_part_get_filename (iface);

	fail_unless(!strcmp (str_in, str_out), "Unable to set filename!\n");

	return;
}
END_TEST
       
START_TEST (tny_mime_part_test_set_content_type)
{
	const gchar *str_in = "text/html", *str_out;
	
	tny_mime_part_set_content_type (iface, str_in);
	str_out = tny_mime_part_get_content_type (iface);

	fail_unless(!strcmp (str_in, str_out), "Unable to content type!\n");

	return;
}
END_TEST

START_TEST (tny_mime_part_test_stream)
{
	CamelStream *real_to = camel_stream_mem_new ();
	TnyStream *to = TNY_STREAM (tny_camel_stream_new (real_to));

/* TODO (this one crashes)

	tny_mime_part_construct (iface, from, "text/plain");
	tny_mime_part_write_to_stream (iface, to, NULL);

	while (!tny_stream_is_eos (to) && n < 1)
	{
		gchar buf[2];
		tny_stream_read (to, buf, sizeof (buf));

		buf[2] = '\0';

		fail_unless(!strcmp (buf, "42"), "Unable to stream!\n");

		n++;
	}
*/
	g_object_unref (G_OBJECT (to));
	camel_object_unref (CAMEL_OBJECT (real_to));

	return;
}
END_TEST

Suite *
create_tny_mime_part_suite (void)
{
     Suite *s = suite_create ("MIME Part");
     TCase *tc = NULL;

     tc = tcase_create ("Set Content Location");
     tcase_add_checked_fixture (tc, tny_mime_part_test_setup, tny_mime_part_test_teardown);
     tcase_add_test (tc, tny_mime_part_test_set_content_location);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Set Description");
     tcase_add_checked_fixture (tc, tny_mime_part_test_setup, tny_mime_part_test_teardown);
     tcase_add_test (tc, tny_mime_part_test_set_description);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Set Content ID");
     tcase_add_checked_fixture (tc, tny_mime_part_test_setup, tny_mime_part_test_teardown);
     tcase_add_test (tc, tny_mime_part_test_set_content_id);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Set Content Type");
     tcase_add_checked_fixture (tc, tny_mime_part_test_setup, tny_mime_part_test_teardown);
     tcase_add_test (tc, tny_mime_part_test_set_content_type);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Set Filename");
     tcase_add_checked_fixture (tc, tny_mime_part_test_setup, tny_mime_part_test_teardown);
     tcase_add_test (tc, tny_mime_part_test_set_filename);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Stream");
     tcase_add_checked_fixture (tc, tny_mime_part_test_setup, tny_mime_part_test_teardown);
     tcase_add_test (tc, tny_mime_part_test_stream);
     suite_add_tcase (s, tc);

     return s;
}
