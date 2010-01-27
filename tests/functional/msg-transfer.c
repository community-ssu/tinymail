/* tinymail - Tiny Mail
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

#include <glib.h>

#include <string.h>

#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-simple-list.h>
#include <tny-account-store.h>
#include <tny-store-account.h>
#include <tny-folder.h>
#include <tny-folder-store.h>
#include <tny-folder-store-query.h>

#include <account-store.h>

static gchar *cachedir=NULL;
static gboolean online=FALSE;
static gboolean move=FALSE;
static const gchar *src_name = NULL;
static const gchar *dst_name = NULL;

static void
find_folders (TnyFolderStore *store, TnyFolderStoreQuery *query,
	      TnyFolder **folder_src, TnyFolder **folder_dst)
{
	TnyIterator *iter;
	TnyList *folders;

	if ((*folder_src != NULL) && (*folder_dst != NULL))
		return;

	folders = tny_simple_list_new ();
	tny_folder_store_get_folders (store, folders, query, TRUE, NULL);
	iter = tny_list_create_iterator (folders);

	while (!tny_iterator_is_done (iter) && (!*folder_src || !*folder_dst))
	{
		TnyFolderStore *folder = (TnyFolderStore*) tny_iterator_get_current (iter);
		const gchar *folder_name = NULL;

		folder_name = tny_folder_get_name (TNY_FOLDER (folder));

		if (!strcmp (folder_name, src_name))
		    *folder_src = g_object_ref (folder);

		if (!strcmp (folder_name, dst_name))
		    *folder_dst = g_object_ref (folder);

		find_folders (folder, query, folder_src, folder_dst);
	    
 		g_object_unref (G_OBJECT (folder));

		tny_iterator_next (iter);
	}

	 g_object_unref (G_OBJECT (iter));
	 g_object_unref (G_OBJECT (folders));
}

static const GOptionEntry options[] = {
		{ "from",  'f', 0, G_OPTION_ARG_STRING, &src_name,
		  "Source folder", NULL},
		{ "to", 't', 0, G_OPTION_ARG_STRING, &dst_name,
		  "Destination folder", NULL},
		{ "cachedir", 'c', 0, G_OPTION_ARG_STRING, &cachedir,
		  "Cache directory", NULL },
		{ "online", 'o', 0, G_OPTION_ARG_NONE, &online,
		  "Online or offline", NULL },
		{ "move", 'm', 0, G_OPTION_ARG_NONE, &move,
		  "Move the messages instead of copy them", NULL },
		{ NULL }
};

int 
main (int argc, char **argv)
{
	GOptionContext *context;
	TnyAccountStore *account_store;
	TnyList *accounts, *src_headers;
	TnyFolderStoreQuery *query;
	TnyStoreAccount *account;
	TnyIterator *iter;
	TnyFolder *folder_src = NULL, *folder_dst = NULL;
	guint src_num_headers = 0, dst_num_headers = 0;
	GError *err;
    
	g_type_init ();

    	context = g_option_context_new ("- The tinymail functional tester");
	g_option_context_add_main_entries (context, options, "tinymail");
	if (!g_option_context_parse (context, &argc, &argv, &err)) {
		g_printerr ("Error in command line parameter(s): '%s', exiting\n",
			    err ? err->message : "");
		return 1;
	}
	g_option_context_free (context);

	account_store = tny_test_account_store_new (online, cachedir);

	if (cachedir)
		g_print ("Using %s as cache directory\n", cachedir);

	if (!src_name || !dst_name) {
		g_printerr ("Error in command line parameter(s), specify source and target folders\n");	
		return 1;
	}

	/* Get accounts */    
	accounts = tny_simple_list_new ();

	tny_account_store_get_accounts (account_store, accounts, 
	      TNY_ACCOUNT_STORE_STORE_ACCOUNTS);
	g_object_unref (G_OBJECT (account_store));
    
	iter = tny_list_create_iterator (accounts);
	account = (TnyStoreAccount*) tny_iterator_get_current (iter);

	g_object_unref (G_OBJECT (iter));
	g_object_unref (G_OBJECT (accounts));

	query = tny_folder_store_query_new ();
	tny_folder_store_query_add_item (query, NULL, TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED);

	/* Find the two folders */
	find_folders (TNY_FOLDER_STORE (account), query, 
		      &folder_src, &folder_dst);

	if (!folder_src || !folder_dst)
		goto cleanup;

	/* Refresh folders */
	tny_folder_refresh (folder_src, NULL);
	src_num_headers = tny_folder_get_all_count (folder_src);

	tny_folder_refresh (folder_dst, NULL);
	dst_num_headers = tny_folder_get_all_count (folder_dst);

	/* Get all the headers of the source & target folder */
	src_headers = tny_simple_list_new ();
	tny_folder_get_headers (folder_src, src_headers, TRUE, NULL);
		
	g_print ("%s %d messages from %s to %s\n",
		  move ? "Moving" : "Copying",
		  src_num_headers,
		  tny_folder_get_name (folder_src),
		  tny_folder_get_name (folder_dst));

	/* Copy/move messages */
	tny_folder_transfer_msgs (folder_src, src_headers, folder_dst, move, NULL);

	/* Check that all the messages have been transferred */
	tny_folder_refresh (folder_dst, NULL);
	g_print ("Transferred %d of %d messages\n",
		  tny_folder_get_all_count (folder_dst) - dst_num_headers,
		  src_num_headers);
	
	g_object_unref (G_OBJECT (src_headers));

 cleanup:
	g_object_unref (account);
	g_object_unref (query);
    
	return 0;
}
