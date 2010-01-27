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

#include <tny-account-store.h>
#include <tny-platform-factory.h>
#include <platfact.h>

static TnyAccountStore *iface = NULL;
static TnyPlatformFactory *platfact = NULL;

static void
tny_account_store_test_setup (void)
{
	/* platfact is a singleton */
    
	platfact = tny_test_platform_factory_get_instance ();
	iface = tny_platform_factory_new_account_store (platfact);

	return;
}

static void 
tny_account_store_test_teardown (void)
{
	g_object_unref (G_OBJECT (iface));

	return;
}

/* TODO: 
	test signals: account_changed, account_inserted, account_removed, accounts_reloaded
	test methods: get_accounts, add_store_account, add_transport_account, get_cache_dir, get_device
	test callback: alert
*/


START_TEST (tny_account_store_test_something)
{
}
END_TEST

Suite *
create_tny_account_store_suite (void)
{
     Suite *s = suite_create ("Account Store");

     TCase *tc = tcase_create ("Empty test");
     tcase_add_checked_fixture (tc, tny_account_store_test_setup, tny_account_store_test_teardown);
     tcase_add_test (tc, tny_account_store_test_something);
     suite_add_tcase (s, tc);

     return s;
}
