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

#include "check_libtinymailui.h"

#include <tny-platform-factory.h>

#include <platfact.h>

static TnyPlatformFactory *iface = NULL;
static gchar *str;

static void
tny_platform_factory_test_setup (void)
{
	iface = tny_test_platform_factory_get_instance ();

	return;
}

static void 
tny_platform_factory_test_teardown (void)
{

	g_object_unref (G_OBJECT (iface));
	return;
}


START_TEST (tny_platform_factory_test_new_device)
{
	GObject *obj = (GObject*)tny_platform_factory_new_device (iface);

	str = g_strdup_printf ("Returned instance doesn't implement TnyDevice\n");
	fail_unless (TNY_IS_DEVICE (obj), str);
	g_free (str);

	g_object_unref (G_OBJECT (obj));
}
END_TEST

START_TEST (tny_platform_factory_test_new_account_store)
{
	GObject *obj = (GObject*)tny_platform_factory_new_account_store (iface);

	str = g_strdup_printf ("Returned instance doesn't implement TnyAccountStore\n");
	fail_unless (TNY_IS_ACCOUNT_STORE (obj), str);
	g_free (str);

	g_object_unref (G_OBJECT (obj));
}
END_TEST

START_TEST(tny_platform_factory_test_new_msg_view)
{
	GObject *obj = (GObject*)tny_platform_factory_new_msg_view (iface);

	str = g_strdup_printf ("Returned instance doesn't implement TnyMsgView\n");
	fail_unless (TNY_IS_MSG_VIEW (obj), str);
	g_free (str);

	/* It's a floating object that gets unreferenced by 
	  gtk_widget_destroy() and likes

	  TODO: Make tny-msg-view finalize properly

	g_object_unref (G_OBJECT (obj)); */
}
END_TEST

Suite *
create_tny_platform_factory_suite (void)
{
     Suite *s = suite_create ("Platform Factory");
     TCase *tc = NULL;

     tc = tcase_create ("New Device");
     tcase_add_checked_fixture (tc, tny_platform_factory_test_setup, tny_platform_factory_test_teardown);
     tcase_add_test (tc, tny_platform_factory_test_new_device);
     suite_add_tcase (s, tc);

     tc = tcase_create ("New Account Store");
     tcase_add_checked_fixture (tc, tny_platform_factory_test_setup, tny_platform_factory_test_teardown);
     tcase_add_test (tc, tny_platform_factory_test_new_account_store);
     suite_add_tcase (s, tc);

     tc = tcase_create ("New Message View");
     tcase_add_checked_fixture (tc, tny_platform_factory_test_setup, tny_platform_factory_test_teardown);
     tcase_add_test (tc, tny_platform_factory_test_new_msg_view);
     suite_add_tcase (s, tc);

     return s;
}
