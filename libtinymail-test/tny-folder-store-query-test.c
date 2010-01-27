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
#include <tny-account-store.h>
#include <tny-store-account.h>
#include <tny-folder.h>

#include <account-store.h>

#include <tny-folder-store.h>
#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-simple-list.h>
#include <tny-camel-header.h>

#include <tny-folder-store-query.h>

static TnyAccountStore *account_store;
static TnyList *accounts;
static TnyIterator *aiter;
static TnyStoreAccount *account=NULL;
static gboolean online_tests=FALSE;
static gchar *str;

static void
tny_folder_store_query_test_setup (void)
{
	if (online_tests) {
		accounts = tny_simple_list_new ();
		account_store = tny_test_account_store_new (TRUE, NULL);
		tny_account_store_get_accounts (account_store, accounts, 
				TNY_ACCOUNT_STORE_STORE_ACCOUNTS);
		aiter = tny_list_create_iterator (accounts);
		tny_iterator_first (aiter);
		account = TNY_STORE_ACCOUNT (tny_iterator_get_current (aiter));
		g_object_unref (aiter);

		if (!account)
			online_tests = FALSE;
	}

	return;
}

static void 
tny_folder_store_query_test_teardown (void)
{
	if (online_tests) {
		g_object_unref (account);
		g_object_unref (accounts);
	}
	return;
}

START_TEST (tny_folder_store_query_test_match_on_name)
{
	TnyFolderStoreQuery *query = NULL;
	TnyList *folders = NULL, *subfolders;
	TnyIterator *iter = NULL;
	TnyFolder *folder;
	gint length=0;
    
	if (!online_tests)
		return;

	query = tny_folder_store_query_new ();
	tny_folder_store_query_add_item (query, "^tny.*$", TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_NAME);

	folders = tny_simple_list_new();
	subfolders = tny_simple_list_new();

	tny_folder_store_get_folders (TNY_FOLDER_STORE (account),
			folders, NULL, TRUE, NULL);
	length = tny_list_get_length (folders);

	str = g_strdup_printf ("Root should have exactly one folder in the test account, it matches %d\n", length);
	fail_unless (length == 1, str);
	g_free (str);

	if (length >= 1)  {
		iter = tny_list_create_iterator (folders);
		folder = (TnyFolder *) tny_iterator_get_current (iter);
		tny_folder_store_get_folders (TNY_FOLDER_STORE (folder),
				subfolders, query, TRUE, NULL);
		g_object_unref (G_OBJECT (folder));
		length = tny_list_get_length (subfolders);
		str = g_strdup_printf ("^tny.*$ should match exactly one folder in the test account, it matches %d\n", length);
		fail_unless (tny_list_get_length (subfolders) == 1, str);
		g_free (str);
		g_object_unref (iter);
	}

	g_object_unref (folders);
	g_object_unref (subfolders);
	g_object_unref (query);

}
END_TEST

START_TEST (tny_folder_store_query_test_match_on_id)
{
	TnyFolderStoreQuery *query = NULL;
	TnyList *folders = NULL, *subfolders;
	TnyIterator *iter = NULL;
	TnyFolder *folder;
	gint length=0;

	if (!online_tests)
		return;

	query = tny_folder_store_query_new ();
	tny_folder_store_query_add_item (query, "^INBOX/tny.*$", TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_ID);

	folders = tny_simple_list_new();
	subfolders = tny_simple_list_new();

	tny_folder_store_get_folders (TNY_FOLDER_STORE (account),
			folders, NULL, TRUE, NULL);
	length = tny_list_get_length (folders);

	str = g_strdup_printf ("Root should have exactly one folder in the test account, it matches %d\n", length);
	fail_unless (length == 1, str);
	g_free (str);

	if (length >= 1) {
		iter = tny_list_create_iterator (folders);
		folder = (TnyFolder *) tny_iterator_get_current (iter);
		tny_folder_store_get_folders (TNY_FOLDER_STORE (folder),
				subfolders, query, TRUE, NULL);
		g_object_unref (G_OBJECT (folder));
		length = tny_list_get_length (subfolders);
		str = g_strdup_printf ("^INBOX/tny.*$ should match exactly one folder in the test account, it matches %d\n", length);
		fail_unless (tny_list_get_length (subfolders) == 1, str);
		g_object_unref (iter);
	}

	g_object_unref (folders);
	g_object_unref (subfolders);
	g_object_unref (query);
}
END_TEST


START_TEST (tny_folder_store_query_test_match_subscribed)
{
	TnyFolderStoreQuery *query = NULL;
	TnyList *folders = NULL, *subfolders;
	TnyIterator *iter = NULL;
	TnyFolder *folder;
	gint length=0;
    
	if (!online_tests)
		return;
    
	query = tny_folder_store_query_new ();
	tny_folder_store_query_add_item (query, NULL, TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED);

	folders = tny_simple_list_new();
	subfolders = tny_simple_list_new();

	tny_folder_store_get_folders (TNY_FOLDER_STORE (account),
			folders, NULL, TRUE, NULL);
    	length = tny_list_get_length (folders);
    
	str = g_strdup_printf ("Root should have exactly one folder in the test account, it matches %d\n", length);
	fail_unless (length == 1, str);
	g_free (str);
    
    	if (length >= 1) 
	{	
		iter = tny_list_create_iterator (folders);
		folder = (TnyFolder *) tny_iterator_get_current (iter);
		tny_folder_store_get_folders (TNY_FOLDER_STORE (folder),
				subfolders, query, TRUE, NULL);
		g_object_unref (G_OBJECT (folder));
		length = tny_list_get_length (subfolders);
	    
		str = g_strdup_printf ("There's 17 subscribed folders in the test account, I received %d\n", length);
		fail_unless (tny_list_get_length (subfolders) == 17, str);
		g_free (str);
		g_object_unref (G_OBJECT (iter));
	}

	g_object_unref (G_OBJECT (folders));
	g_object_unref (G_OBJECT (subfolders));
	g_object_unref (G_OBJECT (query));
}		
END_TEST


START_TEST (tny_folder_store_query_test_match_unsubscribed)
{
	TnyFolderStoreQuery *query = NULL;
	TnyList *folders = NULL, *subfolders;
	TnyIterator *iter = NULL;
	TnyFolder *folder;
	gint length=0;
    
	if (!online_tests)
		return;
    
	query = tny_folder_store_query_new ();
	tny_folder_store_query_add_item (query, NULL, TNY_FOLDER_STORE_QUERY_OPTION_UNSUBSCRIBED);

	folders = tny_simple_list_new();
	subfolders = tny_simple_list_new();

	tny_folder_store_get_folders (TNY_FOLDER_STORE (account),
			folders, NULL, TRUE, NULL);
    	length = tny_list_get_length (folders);
    
	str = g_strdup_printf ("Root should have exactly one folder in the test account, it matches %d\n", length);
	fail_unless (length == 1, str);
	g_free (str);
    
    	if (length >= 1) 
	{	
		iter = tny_list_create_iterator (folders);
		folder = (TnyFolder *) tny_iterator_get_current (iter);
		tny_folder_store_get_folders (TNY_FOLDER_STORE (folder),
				subfolders, query, TRUE, NULL);
		g_object_unref (G_OBJECT (folder));
		length = tny_list_get_length (subfolders);
	    
		str = g_strdup_printf ("There's 1 subscribed folder in the test account, I received %d\n", length);
		fail_unless (tny_list_get_length (subfolders) == 1, str);
		g_free (str);
	    
		g_object_unref (G_OBJECT (iter));
	}

	g_object_unref (G_OBJECT (folders));
	g_object_unref (G_OBJECT (subfolders));
	g_object_unref (G_OBJECT (query));
}
END_TEST

Suite *
create_tny_folder_store_query_suite (void)
{
     TCase *tc = NULL;
     Suite *s = suite_create ("Folder Store Query");

     tc = tcase_create ("Match Name");
     tcase_add_checked_fixture (tc, tny_folder_store_query_test_setup, tny_folder_store_query_test_teardown);
     tcase_add_test (tc, tny_folder_store_query_test_match_on_name);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Match Id");
     tcase_add_checked_fixture (tc, tny_folder_store_query_test_setup, tny_folder_store_query_test_teardown);
     tcase_add_test (tc, tny_folder_store_query_test_match_on_id);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Match Subscribed");
     tcase_add_checked_fixture (tc, tny_folder_store_query_test_setup, tny_folder_store_query_test_teardown);
     tcase_add_test (tc, tny_folder_store_query_test_match_subscribed);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Match Unsubscribed");
     tcase_add_checked_fixture (tc, tny_folder_store_query_test_setup, tny_folder_store_query_test_teardown);
     tcase_add_test (tc, tny_folder_store_query_test_match_unsubscribed);
     suite_add_tcase (s, tc);

     return s;
}
