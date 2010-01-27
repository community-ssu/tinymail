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
#include <tny-header.h>

#include <tny-camel-msg.h>

#include <camel/camel-folder.h>
#include <camel/camel.h>
#include <camel/camel-folder-summary.h>

/* We are going to test the camel implementation */
#include <tny-camel-header.h>

static TnyHeader *iface = NULL;
static TnyMsg *msg = NULL;

/* TODO: Check wether we are missing testing properties here */

static void
tny_header_test_setup (void)
{
	/* Don't ask me why, I think this is a Camel bug */
	CamelInternetAddress *addr = camel_internet_address_new ();
	camel_object_unref (CAMEL_OBJECT (addr));

	msg = TNY_MSG (tny_camel_msg_new ());
	iface = tny_msg_get_header (msg);

	return;
}

static void
tny_header_test_teardown (void)
{
	g_object_unref (G_OBJECT (iface));
	g_object_unref (G_OBJECT (msg));


	return;
}

START_TEST (tny_header_test_set_from)
{
	const gchar *str_in = "Me myself and I <me.myself@and.i.com>", *str_out;
	
	tny_header_set_from (iface, str_in);
	str_out = tny_header_dup_from (iface);

	fail_unless(!strcmp (str_in, str_out), "Unable to set from!\n");

        if (NULL != str_out)
        {
            g_free (str_out);
        }

	return;
}
END_TEST

START_TEST (tny_header_test_set_to)
{
	gchar *str_in = g_strdup ("Myself <this@is.me>, You Do Die Daa <you.doe.die@daa.com>; patrick@test.com");
	const gchar *str_out;
	int i=0;
	
	tny_header_set_to (iface, (const gchar*)str_in);
	str_out = tny_header_dup_to (iface);

	/* The implementation will always return a comma separated list
	 * but should also accept ; separated lists. Even mixed (both
	 * characters have the same meaning). */

	for (i=0; i < strlen(str_in); i++)
		if (str_in[i] == ';')
			str_in[i] = ',';

	fail_unless(!strcmp (str_in, str_out), "Unable to set to!\n");

	g_free (str_in);

        if (NULL != str_out)
        {
             g_free (str_out);
        }

	return;
}
END_TEST

START_TEST (tny_header_test_set_cc)
{
	const gchar *str_in = "First user <first@user.be>, Second user <second@user.com>", *str_out;

	tny_header_set_cc (iface, str_in);
	str_out = tny_header_dup_cc (iface);

	fail_unless(!strcmp (str_in, str_out), "Unable to set cc!\n");

        if (NULL != str_out)
        {
             g_free (str_out);
        }

	return;
}
END_TEST

START_TEST (tny_header_test_set_bcc)
{
	const gchar *str_in = "The Invisible man <the.invisible@man.com>, mark@here.there.com", *str_out;

	tny_header_set_bcc (iface, str_in);
	str_out = tny_header_dup_bcc (iface);

	fail_unless(!strcmp (str_in, str_out), "Unable to set bcc!\n");

        if (NULL != str_out)
        {
             g_free (str_out);
        }

	return;
}
END_TEST

START_TEST (tny_header_test_set_subject)
{
	const gchar *str_in = "I'm the nice subject", *str_out;
	
	tny_header_set_subject (iface, str_in);
	str_out = tny_header_dup_subject (iface);

	fail_unless(!strcmp (str_in, str_out), "Unable to set subject!\n");

        if (NULL != str_out)
        {
             g_free (str_out);
        }

        return;
        
}
END_TEST

START_TEST (tny_header_test_set_replyto)
{

	/* g_warning ("TODO"); */

	return;
}
END_TEST

START_TEST (tny_header_test_set_priority_flags)
{
	TnyHeaderFlags flags;

	tny_header_set_priority (iface, TNY_HEADER_FLAG_HIGH_PRIORITY);
	flags = tny_header_get_priority (iface);
	fail_unless (flags == TNY_HEADER_FLAG_HIGH_PRIORITY, "Unable to set high priority.\n");

	tny_header_set_priority (iface, TNY_HEADER_FLAG_NORMAL_PRIORITY);
	flags = tny_header_get_priority (iface);
	fail_unless (flags == TNY_HEADER_FLAG_NORMAL_PRIORITY, "Unable to set normal priority.\n");

	tny_header_set_priority (iface, TNY_HEADER_FLAG_LOW_PRIORITY);
	flags = tny_header_get_priority (iface);
	fail_unless (flags == TNY_HEADER_FLAG_LOW_PRIORITY, "Unable to set low priority.\n");

	return;
}
END_TEST

Suite *
create_tny_header_suite (void)
{
     Suite *s = suite_create ("Header");
     TCase *tc = NULL;

     tc = tcase_create ("Set Bcc");
     tcase_add_checked_fixture (tc, tny_header_test_setup, tny_header_test_teardown);
     tcase_add_test (tc, tny_header_test_set_bcc);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Set Cc");
     tcase_add_checked_fixture (tc, tny_header_test_setup, tny_header_test_teardown);
     tcase_add_test (tc, tny_header_test_set_cc);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Set To");
     tcase_add_checked_fixture (tc, tny_header_test_setup, tny_header_test_teardown);
     tcase_add_test (tc, tny_header_test_set_to);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Set From");
     tcase_add_checked_fixture (tc, tny_header_test_setup, tny_header_test_teardown);
     tcase_add_test (tc, tny_header_test_set_from);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Set Replyto");
     tcase_add_checked_fixture (tc, tny_header_test_setup, tny_header_test_teardown);
     tcase_add_test (tc, tny_header_test_set_replyto);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Set Subject");
     tcase_add_checked_fixture (tc, tny_header_test_setup, tny_header_test_teardown);
     tcase_add_test (tc, tny_header_test_set_subject);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Set priority flags");
     tcase_add_checked_fixture (tc, tny_header_test_setup, tny_header_test_teardown);
     tcase_add_test (tc, tny_header_test_set_priority_flags);
     suite_add_tcase (s, tc);

     return s;
}
