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

#include <glib.h>

#include <gtk/gtk.h>

#include <gtk/gtkwindow.h>
#include <gtk/gtkprogressbar.h>
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

static void
recurse_folders (TnyFolderStore *store, TnyFolderStoreQuery *query, TnyList *all_folders)
{
	TnyIterator *iter;
	TnyList *folders = tny_simple_list_new ();

	tny_folder_store_get_folders (store, folders, query, TRUE, NULL);
	iter = tny_list_create_iterator (folders);

	while (!tny_iterator_is_done (iter)) {

		TnyFolderStore *folder = (TnyFolderStore*) tny_iterator_get_current (iter);
		tny_list_prepend (all_folders, G_OBJECT (folder));
		recurse_folders (folder, query, all_folders);    
 		g_object_unref (G_OBJECT (folder));

		tny_iterator_next (iter);
	}
	 g_object_unref (iter);
	 g_object_unref (folders);
}

static gboolean
notify_update_account_observers (gpointer data)
{
	g_print ("Refresing %s\n", tny_folder_get_name(TNY_FOLDER(data)));
	g_object_unref (data);

	return FALSE;
}

static gboolean
update_account_quit (gpointer data)
{
	gtk_main_quit ();

	return FALSE;
}

static gpointer
update_account_thread (gpointer thr_user_data)
{
	TnyList *all_folders = NULL;
	TnyIterator *iter = NULL;
	TnyStoreAccount *account;
	GError *error = NULL;

	account = (TnyStoreAccount *) (thr_user_data);

	all_folders = tny_simple_list_new ();
	tny_folder_store_get_folders (TNY_FOLDER_STORE (account),
				      all_folders,
				      NULL,
				      TRUE,
				      &error);
	if (error)
		goto out;

	iter = tny_list_create_iterator (all_folders);
	while (!tny_iterator_is_done (iter)) {
		TnyFolderStore *folder = TNY_FOLDER_STORE (tny_iterator_get_current (iter));

		recurse_folders (folder, NULL, all_folders);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);

	/* Refresh folders */
	iter = tny_list_create_iterator (all_folders);
	while (!tny_iterator_is_done (iter)) {

		TnyFolderStore *folder = TNY_FOLDER_STORE (tny_iterator_get_current (iter));

		/* Refresh the folder */
		tny_folder_refresh (TNY_FOLDER (folder), &error);
/* 		sleep (1); */

		if (!error) {
			if (mainloop)
				g_idle_add (notify_update_account_observers, g_object_ref (folder));
			else
				notify_update_account_observers (folder);
		} else {
			g_print ("%s %s\n", __FUNCTION__, error->message);
			g_clear_error (&error);
		}
		g_object_unref (folder);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);

 out:
	/* Frees */
	g_object_unref (all_folders);
	g_object_unref (account);

	if (mainloop)
		g_timeout_add (1, update_account_quit, NULL);

	return NULL;
}

static gboolean
update_account (gpointer data)
{
	GThread *thread;

	thread = g_thread_create (update_account_thread, g_object_ref (data), FALSE, NULL);

	return FALSE;
}

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

int main (int argc, char **argv)
{
	GOptionContext *context;
	TnyAccountStore *account_store;
	TnyList *accounts;
	TnyStoreAccount *account;
	TnyIterator *iter;

	g_type_init ();

	if (mainloop)
		gtk_init (&argc, &argv);

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

	if (mainloop) {

		g_print ("Using the Gtk+ mainloop\n");
		g_timeout_add (1, update_account, account);

		gtk_main ();
	} else {
		g_print ("Not using a mainloop (will sleep 20 secs)\n");
		update_account (account);
		sleep (20);
	}

	g_object_unref (G_OBJECT (account));
	g_object_unref (G_OBJECT (iter));
	g_object_unref (G_OBJECT (accounts));

	return 0;
}
