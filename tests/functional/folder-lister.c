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

#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>

#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-simple-list.h>
#include <tny-account-store.h>
#include <tny-store-account.h>
#include <tny-folder.h>
#include <tny-folder-store.h>
#include <tny-folder-store-query.h>

#include <account-store.h>

static gint recursion_level=0;
static gchar *cachedir=NULL;
static gboolean online=FALSE;

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

		for (i=0; i<recursion_level; i++)
			g_print ("\t");

		g_print ("%s\n", tny_folder_get_name (TNY_FOLDER (folder)));

		recursion_level++;
		recurse_folders (folder, query);
		recursion_level--;
	    
 		g_object_unref (folder);

		tny_iterator_next (iter);
	}

	 g_object_unref (iter);
	 g_object_unref (folders);
}

static const GOptionEntry options[] = 
{
	{ "cachedir", 'c', 0, G_OPTION_ARG_STRING, &cachedir,
		"Cache directory", NULL },
	{ "online", 'o', 0, G_OPTION_ARG_NONE, &online,
		"Online or offline", NULL },
    
	{ NULL }
};

int 
main (int argc, char **argv)
{
	GOptionContext *context;
	TnyAccountStore *account_store;
	TnyList *accounts;
	TnyStoreAccount *account;
	TnyIterator *iter;
	gint i;

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

	gtk_main();

	g_object_unref (account);
	g_object_unref (iter);
	g_object_unref (accounts);

	return 0;
}

