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

#include <tny-account.h>
#include <tny-folder-store.h>

#include <tny-camel-store-account.h>
#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-simple-list.h>
#include <tny-account-store.h>
#include <tny-store-account.h>
#include <tny-folder.h>
#include <tny-camel-header.h>

#include <account-store.h>

static TnyAccount *iface = NULL;
static TnyAccountStore *account_store;
static gboolean online_tests=FALSE;
static gchar *str;

static void
tny_account_test_setup (void)
{
    TnyList *accounts;
    TnyIterator *aiter;
    iface = NULL;
    
    if (online_tests)
    {
	accounts = tny_simple_list_new ();
	account_store = tny_test_account_store_new (TRUE, NULL);
	tny_account_store_get_accounts (account_store, accounts, 
			TNY_ACCOUNT_STORE_STORE_ACCOUNTS);

	aiter = tny_list_create_iterator (accounts);
	iface = TNY_ACCOUNT (tny_iterator_get_current (aiter));
    
    	if (!iface)
		online_tests = FALSE;
	
    }
    
    if (!iface)
	    iface = TNY_ACCOUNT (tny_camel_store_account_new ());
    
   return;
}

static void 
tny_account_test_teardown (void)
{
     /* TODO: Find out why tests fail when objects are unref'ed */
/*     	g_object_unref (G_OBJECT (iface)); */
/* 	g_object_unref (G_OBJECT (aiter)); */
/* 	g_object_unref (G_OBJECT (accounts)); */
    
	return;
}

START_TEST (tny_store_account_test_get_folders)
{
    	TnyList *folders = NULL;
    
      	if (!online_tests)
	    	return;
    
    	folders = tny_simple_list_new ();
    
    	tny_folder_store_get_folders (TNY_FOLDER_STORE (iface),
			folders, NULL, TRUE, NULL);
        
    	fail_unless (tny_list_get_length (folders) == 1, 
		"Account should have at least an inbox folder\n");
    
    	g_object_unref (G_OBJECT (folders));
    
    	return;
}
END_TEST

START_TEST (tny_account_test_get_account_type)
{
	fail_unless (tny_account_get_account_type (iface) == TNY_ACCOUNT_TYPE_STORE,
		"Account type should be store\n");
}
END_TEST

START_TEST (tny_account_test_set_hostname)
{
	const gchar *str_in = "imap.imapserver.com", *str_out;

	tny_account_set_hostname (iface, str_in);
	str_out = tny_account_get_hostname (iface);

	str = g_strdup_printf ("Unable to set hostname to %s, it became %s\n", str_in, str_out);
	fail_unless (!strcmp (str_in, str_out), str);
	g_free (str);
}
END_TEST

START_TEST (tny_account_test_set_user)
{
	const gchar *str_in = "myusername", *str_out;

	tny_account_set_user (iface, str_in);
	str_out = tny_account_get_user (iface);

	str = g_strdup_printf ("Unable to set user to %s, it became %s\n", str_in, str_out);
	fail_unless (!strcmp (str_in, str_out), str);
	g_free (str);
}
END_TEST

START_TEST (tny_account_test_set_id)
{
	const gchar *str_in = "THE_ID", *str_out;

	tny_account_set_id (iface, str_in);
	str_out = tny_account_get_id (iface);

	str = g_strdup_printf ("Unable to set id to %s, it became %s\n", str_in, str_out);
	fail_unless (!strcmp (str_in, str_out), str);
	g_free (str);
}
END_TEST

START_TEST (tny_account_test_set_name)
{
	const gchar *str_in = "The name of the account", *str_out;

	tny_account_set_name (iface, str_in);
	str_out = tny_account_get_name (iface);

	str = g_strdup_printf ("Unable to set name to %s, it became %s\n", str_in, str_out);
	fail_unless (!strcmp (str_in, str_out), str);
	g_free (str);
}
END_TEST

START_TEST (tny_account_test_set_proto)
{
	const gchar *str_in = "imap", *str_out;

	tny_account_set_proto (iface, str_in);
	str_out = tny_account_get_proto (iface);

	str = g_strdup_printf ("Unable to set proto to %s, it became %s\n", str_in, str_out);
	fail_unless (!strcmp (str_in, str_out), str);
	g_free (str);
}
END_TEST

Suite *
create_tny_account_suite (void)
{
     Suite *s = suite_create ("Account");
     TCase *tc = NULL;

     tc = tcase_create ("Get Account Type");
     tcase_add_checked_fixture (tc, tny_account_test_setup, tny_account_test_teardown);
     tcase_add_test (tc, tny_account_test_get_account_type);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Set Hostname");
     tcase_add_checked_fixture (tc, tny_account_test_setup, tny_account_test_teardown);
     tcase_add_test (tc, tny_account_test_set_hostname);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Set User");
     tcase_add_checked_fixture (tc, tny_account_test_setup, tny_account_test_teardown);
     tcase_add_test (tc, tny_account_test_set_user);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Set ID");
     tcase_add_checked_fixture (tc, tny_account_test_setup, tny_account_test_teardown);
     tcase_add_test (tc, tny_account_test_set_id);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Set Protocol");
     tcase_add_checked_fixture (tc, tny_account_test_setup, tny_account_test_teardown);
     tcase_add_test (tc, tny_account_test_set_proto);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Set Name");
     tcase_add_checked_fixture (tc, tny_account_test_setup, tny_account_test_teardown);
     tcase_add_test (tc, tny_account_test_set_name);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Store Account Get Folders");
     tcase_add_checked_fixture (tc, tny_account_test_setup, tny_account_test_teardown);
     tcase_add_test (tc, tny_store_account_test_get_folders);
     suite_add_tcase (s, tc);

     return s;
}

