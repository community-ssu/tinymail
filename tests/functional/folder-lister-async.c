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
#include <gtk/gtk.h>
#include <tny-shared.h>
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
static gboolean online=FALSE, mainloop=FALSE;

static const GOptionEntry options[] = 
{
	{ "cachedir", 'c', 0, G_OPTION_ARG_STRING, &cachedir,
		"Cache directory", NULL },
	{ "online", 'o', 0, G_OPTION_ARG_NONE, &online,
		"Online or offline", NULL },
	{ "mainloop", 'm', 0, G_OPTION_ARG_NONE, &mainloop,
		"Use the Gtk+ mainloop", NULL },
    
	{ NULL }
};


static void 
callback (TnyFolderStore *self, gboolean cancelled, TnyList *list, GError *err, gpointer user_data)
{
	TnyIterator *iter = tny_list_create_iterator (list);

	while (!tny_iterator_is_done (iter))
	{
		TnyFolderStore *folder = (TnyFolderStore*) tny_iterator_get_current (iter);
		TnyList *folders = tny_simple_list_new ();
		g_print ("%s\n", tny_folder_get_name (TNY_FOLDER (folder)));
		tny_folder_store_get_folders_async (folder,
			folders, NULL, TRUE, callback, NULL, NULL);
		g_object_unref (G_OBJECT (folder));
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (list);
}

static gboolean
time_s_up (gpointer data)
{
	gtk_main_quit ();
	return FALSE;
}

static gboolean
dance (gpointer data)
{
	TnyList *folders;
	TnyStoreAccount *account = data;
    
	folders = tny_simple_list_new ();
    	tny_folder_store_get_folders_async (TNY_FOLDER_STORE (account),
		folders, NULL, TRUE, callback, NULL, NULL);
    
	return FALSE;
}

int 
main (int argc, char **argv)
{
	GOptionContext *context;
	TnyAccountStore *account_store;
	TnyList *accounts;
	TnyStoreAccount *account;
	TnyIterator *iter;
    
	free (malloc (10));
	g_type_init ();
    
    
    	context = g_option_context_new ("- The tinymail functional tester");
	g_option_context_add_main_entries (context, options, "tinymail");

    	g_option_context_parse (context, &argc, &argv, NULL);

    	if (mainloop)
		gtk_init (&argc, &argv);

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


    if (mainloop)
	{
		g_print ("Using the Gtk+ mainloop (will wait 4 seconds in the loop)\n");
	    
	    	g_timeout_add (1, dance, account);	    
	    	g_timeout_add (1000 * 4, time_s_up, NULL);
	    
		gtk_main ();
	    
	} else {
		g_print ("Not using a mainloop (will sleep 4 seconds)\n");
	    
		dance (account);
		sleep (4);
	}
    
	g_object_unref (account);
	g_object_unref (iter);
	g_object_unref (accounts);
    
	return 0;
}

