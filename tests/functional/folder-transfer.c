/* tinymail - Tiny Mail
 * Copyright (C) 2006-2007 Sergio Villar Senin <svillar@igalia.com>
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

#include <glib.h>
#include <stdlib.h>

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
static TnyFolder *src_folder = NULL;
static TnyFolder *dst_folder = NULL;
static gint recursion_level=0;

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

static void
recurse_folders (TnyFolderStore *store, TnyFolderStoreQuery *query)
{
	TnyIterator *iter;
	TnyList *folders = tny_simple_list_new ();

	tny_folder_store_get_folders (store, folders, query, TRUE, NULL);
	iter = tny_list_create_iterator (folders);

	while (!tny_iterator_is_done (iter))
	{
		TnyFolderStore *folder = (TnyFolderStore*) tny_iterator_get_current (iter);
		gint i=0;
		const gchar *folder_name = NULL;

		for (i=0; i<recursion_level; i++)
			g_print ("\t");

		folder_name = tny_folder_get_name (TNY_FOLDER (folder));
		g_print ("%s\n", folder_name);

		if (!strcmp (folder_name, src_name))
			src_folder = g_object_ref (folder);
		
		if (!strcmp (folder_name, dst_name))
			dst_folder = g_object_ref (folder);

		recursion_level++;
		recurse_folders (folder, query);
		recursion_level--;
	    
 		g_object_unref (folder);

		tny_iterator_next (iter);
	}

	 g_object_unref (iter);
	 g_object_unref (folders);
}

int 
main (int argc, char **argv)
{
	GOptionContext *context;
	TnyAccountStore *account_store;
	TnyList *accounts;
	TnyStoreAccount *account;
	TnyIterator *iter;
	gint i;
	GError *err = NULL;

	free (malloc (10));

	g_type_init ();

	context = g_option_context_new ("- The tinymail functional tester");
	g_option_context_add_main_entries (context, options, "tinymail");
	g_option_context_parse (context, &argc, &argv, NULL);

	account_store = tny_test_account_store_new (online, cachedir);

	if (cachedir)
		g_print ("Using %s as cache directory\n", cachedir);

	g_option_context_free (context);
	accounts = tny_simple_list_new ();

	tny_account_store_get_accounts (account_store, accounts, 
		TNY_ACCOUNT_STORE_STORE_ACCOUNTS);
	g_object_unref (G_OBJECT (account_store));
	iter = tny_list_create_iterator (accounts);
	account = (TnyStoreAccount*) tny_iterator_get_current (iter);

	recursion_level = 0;
	for (i=0; i<1; i++) 
		recurse_folders (TNY_FOLDER_STORE (account), NULL);

	/* Test 1 */
	if (!(src_folder && dst_folder)) {
		g_message ("Folders not found");
		goto cleanup;
	}

	g_print ("%s folder %s to %s\n",
		 (move) ? "Moving" : "Copying",
		 tny_folder_get_name (src_folder),
		 tny_folder_get_name (dst_folder));

	tny_folder_copy (src_folder,
			 TNY_FOLDER_STORE (dst_folder),
			 tny_folder_get_name (src_folder),
			 move,
			 &err);

	if (err)
		g_warning ("%s", err->message);

	for (i=0; i<1; i++) 
		recurse_folders (TNY_FOLDER_STORE (account), NULL);

 cleanup:
	g_object_unref (account);
	g_object_unref (iter);
	g_object_unref (accounts);

	return 0;
}

