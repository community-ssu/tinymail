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

#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-simple-list.h>
#include <tny-account-store.h>
#include <tny-store-account.h>
#include <tny-folder.h>
#include <tny-camel-header.h>
#include <tny-folder-store.h>

#include <account-store.h>

#include <camel/camel.h>

typedef void (*performer_t) (TnyFolder *folder);
static gchar *cachedir=NULL;
static gboolean online=FALSE, justget=FALSE;

static void
do_get_folder (TnyFolder *folder)
{
	g_print ("Getting headers of %s ...\n", tny_folder_get_id (folder));
	tny_folder_refresh (folder, NULL);
}


static void
do_test_folder (TnyFolder *folder)
{
	TnyList *headers = tny_simple_list_new ();
	gint length, bytes;
	gdouble kbytes, mbytes;

	g_print ("Loading headers for %s ...\n", tny_folder_get_id (folder));
	tny_folder_get_headers (folder, headers, FALSE, NULL);
	length=tny_list_get_length (headers);
	
	bytes = (sizeof (TnyCamelHeader) + sizeof (CamelMessageInfo) + 
		 sizeof (CamelMessageInfoBase) + 
		 sizeof (CamelMessageContentInfo));

	kbytes = ((gdouble)bytes) / 1024;
	mbytes = kbytes / 1024;

	g_print ("Loaded %d headers\n\n", length);

	g_print ("\tsizeof (TnyHeader) = %d - accounts for %d bytes (~%.2lfK)\n", sizeof (TnyCamelHeader), length * sizeof (TnyCamelHeader), ((gdouble)length * sizeof (TnyCamelHeader))/1024);
	g_print ("\tsizeof (CamelMessageInfo) = %d - accounts for %d bytes (~%.2lfK)\n", sizeof (CamelMessageInfo), length * sizeof (CamelMessageInfo), ((gdouble)length * sizeof (CamelMessageInfo))/1024);
	g_print ("\tsizeof (CamelMessageInfoBase) = %d - accounts for %d bytes (~%.2lfK)\n", sizeof (CamelMessageInfoBase), length * sizeof (CamelMessageInfoBase), ((gdouble)length * sizeof (CamelMessageInfoBase))/1024);
	g_print ("\tsizeof (CamelMessageContentInfo) = %d - accounts for %d bytes (~%.2lfK)\n", sizeof (CamelMessageContentInfo), length * sizeof (CamelMessageContentInfo), ((gdouble)length * sizeof (CamelMessageContentInfo))/1024);

	g_print ("\nThis means that (at least) %d bytes or ~%.2lfK or ~%.2lfM are needed for this folder\n", bytes, kbytes, mbytes);

	g_print ("Sleeping to allow your valgrind to see this...\n");
	sleep (5);
	g_print ("Unloading headers ...\n");
	g_object_unref (G_OBJECT (headers));
		g_print ("Sleeping to allow your valgrind to see this...\n");
	sleep (5);
}

static const GOptionEntry options[] = 
{
	{ "cachedir", 'c', 0, G_OPTION_ARG_STRING, &cachedir,
		"Cache directory", NULL },
	{ "online", 'o', 0, G_OPTION_ARG_NONE, &online,
		"Online or offline", NULL },
	{ "justget", 'j', 0, G_OPTION_ARG_NONE, &justget,
		"Just get the messages", NULL },

	{ NULL }
};

int 
main (int argc, char **argv)
{
	GOptionContext *context;
	TnyAccountStore *account_store;
	TnyList *accounts;
	TnyStoreAccount *account;
	TnyIterator *iter, *topiter;
	TnyList *folders, *topfolders;
	TnyFolder *inbox;

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

	topfolders = tny_simple_list_new ();
	folders = tny_simple_list_new ();

	tny_folder_store_get_folders (TNY_FOLDER_STORE (account), topfolders, NULL, TRUE, NULL);
	topiter = tny_list_create_iterator (topfolders);
	inbox = TNY_FOLDER (tny_iterator_get_current (topiter));

	tny_folder_store_get_folders (TNY_FOLDER_STORE (inbox), folders, NULL, TRUE, NULL);

	if (iter)
		g_object_unref (iter);
	iter = tny_list_create_iterator (folders);

	while (!tny_iterator_is_done (iter))
	{
		TnyFolder *folder = (TnyFolder*) tny_iterator_get_current (iter);

		printf ("NAME=%s\n", tny_folder_get_name (folder));

		if (online)
			do_get_folder (folder);
		if (!justget)
			do_test_folder (folder);
		g_object_unref (G_OBJECT (folder));
		tny_iterator_next (iter);
	}

	g_object_unref (G_OBJECT (iter));
	g_object_unref (G_OBJECT (folders));

	g_object_unref (G_OBJECT (inbox));
	g_object_unref (G_OBJECT (topiter));
	g_object_unref (G_OBJECT (topfolders));

	g_object_unref (G_OBJECT (account));
	g_object_unref (G_OBJECT (accounts));

	return 0;
}


