/* tinymail - Tiny Mail unit test
 * Copyright (C) 2006 Øystein Gisnås <oystein@gisnas.net>
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

#include <gtk/gtk.h>

#include <tny-folder.h>
#include <tny-camel-folder.h>
#include <tny-folder-store.h>
#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-simple-list.h>
#include <tny-account-store.h>
#include <tny-store-account.h>
#include <tny-folder.h>
#include <tny-camel-header.h>

#include <account-store.h>

static TnyFolderStore *account;

static void
tny_folder_store_test_setup (void)
{
	TnyIterator *aiter;
	TnyList *accounts = tny_simple_list_new ();
	TnyAccountStore *account_store = tny_test_account_store_new (TRUE, NULL);
	tny_account_store_get_accounts (account_store, accounts, TNY_ACCOUNT_STORE_STORE_ACCOUNTS);
	g_object_unref (account_store);
	aiter = tny_list_create_iterator (accounts);
	tny_iterator_first (aiter);
	account = TNY_FOLDER_STORE (tny_iterator_get_current (aiter));
	g_object_unref (aiter);
	g_object_unref (accounts);
}

static void 
tny_folder_store_test_teardown (void)
{
    	g_object_unref (G_OBJECT (account));
}

START_TEST (tny_folder_store_test_get_folders)
{
     TnyList *folders;
#if 0
     GError *err = NULL;
#endif

     if (account == NULL)
     {
	     g_warning ("Test cannot continue (are you online?)");
	     return;
     }

     /* Test server has only one root folder */
     folders = tny_simple_list_new ();
     tny_folder_store_get_folders (account, folders, NULL, TRUE, NULL);
     fail_unless (tny_list_get_length (folders) == 1, "There should be only one root folder");
     g_object_unref (G_OBJECT (folders));

     /* Make sure errors are set - invalid URL
     folders = tny_simple_list_new ();
     tny_account_set_url_string (TNY_ACCOUNT (account), "trigger://error");
     tny_folder_store_get_folders (account, folders, NULL, &err);
     fail_unless (err != NULL, "An error should be set when the account is invalid");
     g_object_unref (G_OBJECT (folders)); */

}
END_TEST

static void
callback (TnyFolderStore *self, TnyList *list, GError **err, gpointer user_data)
{
     TnyFolder *folder;
     TnyIterator *iter = tny_list_create_iterator (list);

     fail_unless (tny_list_get_length (list) == 1, "Did not find one root folder as expected in callback");     folder = TNY_FOLDER (tny_iterator_get_current (iter));
     fail_unless (strcmp (tny_folder_get_id (folder), "INBOX") == 0, "Did not find INBOX");
     g_object_unref (G_OBJECT (iter));
     g_object_unref (G_OBJECT (folder));
     gtk_main_quit ();
}

static gboolean
timeout (gpointer data)
{
     gtk_main_quit ();
     return FALSE;
}

static void status_cb (GObject *self, TnyStatus *status, gpointer user_data) {}

START_TEST (tny_folder_store_test_get_folders_async)
{
     TnyList *folders;

     if (account == NULL)
     {
	     g_warning ("Test cannot continue (are you online?)");
	     return;
     }

     folders = tny_simple_list_new ();
     tny_folder_store_get_folders_async (account, folders, NULL, TRUE, callback, status_cb, NULL);
     g_timeout_add (1000*4, timeout, NULL);
     gtk_main ();
     fail_unless (tny_list_get_length (folders) == 1, "Did not find one root folder as expected");

     g_object_unref (G_OBJECT (folders));
}
END_TEST

TnyFolderStore *
get_inbox (void)
{
     TnyList *folders = tny_simple_list_new ();
     TnyIterator *iter;
     TnyFolder *folder;

     tny_folder_store_get_folders (account, folders, NULL, TRUE, NULL);
     /*fail_unless (tny_list_get_length (folders) == 1, "Asserted that there was only one root folder");*/
     iter = tny_list_create_iterator (TNY_LIST (folders));
     folder = TNY_FOLDER (tny_iterator_get_current (iter));

     g_object_unref (G_OBJECT (iter));
     g_object_unref (G_OBJECT (folders));

     return (TnyFolderStore *) folder;
}

/* It might be an idea to refactor remove_folder into a separate test case */
/* in case the folder exists. We will never get rid of it the way it is now. */
START_TEST (tny_folder_store_test_create_remove_folder)
{
     GError *err;
     TnyFolderStore *inbox;
     TnyFolder *new_folder;
     const gchar *new_folder_name = "tny-folder-store-test_temp-folder";

     /* Make sure errors are set - cannot add folder to root */
     err = NULL;
/*
	 tny_folder_store_create_folder (account, "tny-folder-store-test_temp-folder", &err);
     fail_unless (err != NULL, "Expected an error when trying to create a top level folder");
*/

     /* Create a folder under Inbox. Hopefully we're able to remove it later. */
     inbox = get_inbox();
     err = NULL;
     new_folder = tny_folder_store_create_folder (inbox, new_folder_name, &err);
	fail_unless (err != NULL, "Errrr");

/*     if (err != NULL)
	  fail (g_strdup_printf ("The attempt to create folder %s in %s failed. Error message: %s\n", new_folder_name, tny_folder_get_name (TNY_FOLDER (inbox)), err->message));
     fail_unless (strcmp (tny_folder_get_name (new_folder), new_folder_name) == 0, "A new folder was not created. Did it already exist?");
*/

     /* Attempt to remove the new folder */
     /*err = NULL;
     tny_folder_store_remove_folder (inbox, new_folder, &err);
     if (err != NULL)
	  fail (g_strdup_printf ("Could not remove folder %s from %s. Error message: %s\n", new_folder_name, tny_folder_get_name (TNY_FOLDER (inbox)), err->message));
     fail_unless (tny_folder_get_id (new_folder) == NULL, "The folder ID of a removed folder was still set after removal");*/


     g_object_unref (G_OBJECT (new_folder));
     g_object_unref (G_OBJECT (inbox));

}
END_TEST

Suite *
create_tny_folder_store_suite (void)
{
     TCase *tc = NULL;
     Suite *s = suite_create ("Folder Store");

     tc = tcase_create ("Get Folders");
     tcase_add_checked_fixture (tc, tny_folder_store_test_setup, tny_folder_store_test_teardown);
     tcase_add_test (tc, tny_folder_store_test_get_folders);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Get Folders Async");
     tcase_set_timeout (tc, 5);
     tcase_add_checked_fixture (tc, tny_folder_store_test_setup, tny_folder_store_test_teardown);
     tcase_add_test (tc, tny_folder_store_test_get_folders_async);
     suite_add_tcase (s, tc);

     tc = tcase_create ("Create and Remove Folder");
     tcase_set_timeout (tc, 10);
     tcase_add_checked_fixture (tc, tny_folder_store_test_setup, tny_folder_store_test_teardown);
     tcase_add_test (tc, tny_folder_store_test_create_remove_folder);
     suite_add_tcase (s, tc);

     return s;
}
