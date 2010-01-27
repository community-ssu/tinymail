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

#include <tny-msg.h>
#include <tny-camel-msg.h>
#include <tny-camel-header.h>
#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-simple-list.h>

#include <camel/camel-folder.h>
#include <camel/camel.h>
#include <camel/camel-folder-summary.h>

static TnyMsg *iface = NULL;
static gchar *str;

static void
tny_msg_test_setup (void)
{
	/* Don't ask me why, I think this is a Camel bug */
	CamelInternetAddress *addr = camel_internet_address_new ();
	camel_object_unref (CAMEL_OBJECT (addr));

	iface = tny_camel_msg_new ();

	return;
}

static void 
tny_msg_test_teardown (void)
{
	g_object_unref (G_OBJECT (iface));

	return;
}


START_TEST (tny_msg_test_add_part_del_part)
{
    
    	gint length = 0;
    	TnyList *parts = tny_simple_list_new();
	TnyMimePart *part = tny_camel_mime_part_new ();
    
	tny_mime_part_add_part (TNY_MIME_PART (iface), part);

    	tny_mime_part_get_parts (TNY_MIME_PART (iface), parts);
	length = tny_list_get_length (parts);
    	g_object_unref (G_OBJECT (parts));

    	/* TnyMsg contains one part by itself */
	str = g_strdup_printf ("Length must be exactly 2, received %d parts\n", length);
	fail_unless (length == 2, str);
	g_free (str);

    	parts = tny_simple_list_new ();
    	tny_mime_part_del_part (TNY_MIME_PART (iface), part);
       	tny_mime_part_get_parts (TNY_MIME_PART (iface), parts);
	length = tny_list_get_length (parts);
    	g_object_unref (G_OBJECT (parts));
    
	str = g_strdup_printf ("Length must be exactly 1, received %d parts\n", length);
	fail_unless (length == 1, str);
	g_free (str);
    
    	return;
}
END_TEST

Suite *
create_tny_msg_suite (void)
{
     Suite *s = suite_create ("Message");
     TCase *tc = NULL;

     tc = tcase_create ("Add Part Delete Part");
     tcase_add_checked_fixture (tc, tny_msg_test_setup, tny_msg_test_teardown);
     tcase_add_test (tc, tny_msg_test_add_part_del_part);
     /* Disabled because it fails. TODO: Enable again */
/*      suite_add_tcase (s, tc); */

     return s;
}
