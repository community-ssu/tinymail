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

#include <tny-simple-list.h>
#include <tny-gtk-account-list-model.h>
#include <tny-gtk-attach-list-model.h>
#include <tny-gtk-folder-store-tree-model.h>
#include <tny-gtk-header-list-model.h>

#include <tny-camel-imap-store-account.h>
#include <tny-camel-header.h>
#include <tny-camel-mime-part.h>

#include <account-store.h>
#include <tny-session-camel.h>
#include <tny-test-object.h>

#include <camel/camel-folder.h>
#include <camel/camel.h>
#include <camel/camel-folder-summary.h>

static TnyList *ifaces[5];
static gchar *str;

static void
tny_list_test_setup (void)
{
	ifaces[0] = tny_simple_list_new ();
	ifaces[1] = TNY_LIST (tny_gtk_account_list_model_new ());
	ifaces[2] = TNY_LIST (tny_gtk_attach_list_model_new ());
	ifaces[3] = TNY_LIST (tny_gtk_folder_store_tree_model_new (NULL));
	ifaces[4] = TNY_LIST (tny_gtk_header_list_model_new ());

	return;
}

static void 
tny_list_test_teardown (void)
{
    int i =0;

    /* TODO Fix the code so that this sleep is not necessary */
    /* The problem is that a TnyCamelIMAPStoreAccount is unrefed before */
    /* it is unrefed in tny_camel_store_account_get_folders_async_thread */
    sleep (1);
    for (i=0; i < 5; i++)
    {
	 g_object_unref (G_OBJECT (ifaces[i]));
    }
}

static gint counter;

static void
tny_list_test_foreach (gpointer gptr_item, gpointer user_data)
{
	counter++;
}

static TnyAccountStore *astore = NULL;
static TnySessionCamel *session;

static void
setup_objs (int num, GObject **a, GObject **b, GObject **c, GObject **d)
{
    if (session == NULL)
    {
	astore = tny_test_account_store_new (FALSE, NULL);
	session = tny_session_camel_new (astore);
    }
    
    if (num == 0)
    {
    	*a = tny_test_object_new (g_strdup ("2"));
	*b = tny_test_object_new (g_strdup ("3"));
	*c = tny_test_object_new (g_strdup ("4"));
	*d = tny_test_object_new (g_strdup ("1"));
    }

    if (num == 1 || num == 3 || num == 4)
    {
	TnyAccount *a1,*b1,*c1,*d1;
	a1 = (TnyAccount*) tny_camel_imap_store_account_new ();
	b1 = (TnyAccount*) tny_camel_imap_store_account_new ();
	c1 = (TnyAccount*) tny_camel_imap_store_account_new ();
	d1 = (TnyAccount*) tny_camel_imap_store_account_new ();
	
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT (a1), session);
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT (b1), session);
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT (d1), session);
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT (c1), session);
	
	tny_account_set_url_string (a1, "imap://user@localhost");
	tny_account_set_url_string (b1, "imap://user@localhost");
	tny_account_set_url_string (c1, "imap://user@localhost");
	tny_account_set_url_string (d1, "imap://user@localhost");

	*a = (GObject*)a1; *b = (GObject*)b1; *c = (GObject*)c1; *d = (GObject*)d1;
    }

    if (num == 2)
    {
	*a = (GObject*) tny_camel_mime_part_new ();
	*b = (GObject*) tny_camel_mime_part_new ();
	*c = (GObject*) tny_camel_mime_part_new ();
	*d = (GObject*) tny_camel_mime_part_new ();
    }

    if (num == 5)
    {
/*	*a = (GObject*) tny_camel_header_new ();
	*b = (GObject*) tny_camel_header_new ();
	*c = (GObject*) tny_camel_header_new ();
	*d = (GObject*) tny_camel_header_new (); */
    }

    
}

START_TEST (tny_list_test_list)
{
	TnyList *iface = ifaces [0];
	TnyList *ref;
	TnyIterator *iterator;
	GObject *item;
	gint j;
	GObject *a, *b, *c, *d;
	setup_objs (0, &a, &b, &c, &d);
	
	tny_list_append (iface, a);
	g_object_unref (G_OBJECT (a));
	tny_list_append (iface, b);
	g_object_unref (G_OBJECT (b));
	tny_list_append (iface, c);
	g_object_unref (G_OBJECT (c));
	tny_list_prepend (iface, d);
	g_object_unref (G_OBJECT (d));
	
	counter=0;
	tny_list_foreach (iface, tny_list_test_foreach, NULL);
	
	str = g_strdup_printf ("Implementation: %s - Counter after foreach should be 4 but is %d\n", G_OBJECT_TYPE_NAME (iface), counter);
	fail_unless (counter == 4, str);
	g_free (str);
	
	str = g_strdup_printf ("Implementation: %s - Length should be 4 but is %d\n", G_OBJECT_TYPE_NAME (iface), tny_list_get_length (iface));
	fail_unless (tny_list_get_length (iface) == 4, str);
	g_free (str);

	iterator = tny_list_create_iterator (iface);
	str = g_strdup_printf ("Implementation: %s - get_list returns the wrong instance\n", G_OBJECT_TYPE_NAME (iface));
	ref = tny_iterator_get_list (iterator);
	fail_unless (ref == iface, str);
	g_free (str);
	g_object_unref (G_OBJECT (ref));
    
	tny_iterator_nth (iterator, 2);
	item = tny_iterator_get_current (iterator);
	
	str = g_strdup_printf ("Implementation: %s - Item should be \"3\"\n", G_OBJECT_TYPE_NAME (iface));
	fail_unless (item == b, str);
	g_free (str);
	g_object_unref (G_OBJECT(item));

	tny_iterator_next (iterator);
	item = tny_iterator_get_current (iterator);
	str = g_strdup_printf ("Implementation: %s - Item should be \"4\"\n", G_OBJECT_TYPE_NAME (iface));
	fail_unless (item == c, str);
	g_free (str);
	g_object_unref (G_OBJECT(item));

	tny_iterator_prev (iterator);
	item = tny_iterator_get_current (iterator);
	str = g_strdup_printf ("Implementation: %s - Item should be \"3\"\n", G_OBJECT_TYPE_NAME (iface));
	fail_unless (item == b, str);
	g_free (str);
	g_object_unref (G_OBJECT(item));

	tny_iterator_next (iterator);
	item = tny_iterator_get_current (iterator);
	str = g_strdup_printf ("Implementation: %s - Item should be \"4\"\n", G_OBJECT_TYPE_NAME (iface));
	fail_unless (item == c, str);
	g_free (str);
	g_object_unref (G_OBJECT(item));

	item = tny_iterator_get_current (iterator);
	str = g_strdup_printf ("Implementation %s - Item should be \"4\"\n", G_OBJECT_TYPE_NAME (iface));
	fail_unless (item == c, str);
	g_free (str);

	g_object_unref (G_OBJECT (iterator));
	tny_list_remove (iface, (GObject*)item);
	str = g_strdup_printf ("Implementation %s - Length should be 3 but is %d\n", G_OBJECT_TYPE_NAME (iface), tny_list_get_length (iface));
	fail_unless (tny_list_get_length (iface) == 3, str);
	g_free (str);

	g_object_unref (iterator);

	/* What's the initial state of an iterator? */
	iterator = tny_list_create_iterator (iface);
	item = tny_iterator_get_current (iterator);
	str = g_strdup_printf ("Implementation: %s - Item should be \"1\"\n", G_OBJECT_TYPE_NAME (iface));
	fail_unless (item == d, str);
	g_free (str);
	g_object_unref (G_OBJECT(item));
	g_object_unref (G_OBJECT (iterator));

	iterator = tny_list_create_iterator (iface);

	tny_iterator_first (iterator);
	item = tny_iterator_get_current (iterator);

	str = g_strdup_printf ("Implementation: %s - Item should be \"1\"\n", G_OBJECT_TYPE_NAME (iface));
	fail_unless (item == d, str);
	g_free (str);
	g_object_unref (G_OBJECT(item));
	
	for (j=0; j<3; j++)
	{
		str = g_strdup_printf ("Implementation %s - is_done should return FALSE\n", G_OBJECT_TYPE_NAME (iface));
		fail_unless (tny_iterator_is_done (iterator) == FALSE, str);
		g_free (str);
	  
		tny_iterator_next (iterator);
	}

	str = g_strdup_printf ("Implementation: %s - is_done should by now return TRUE\n", G_OBJECT_TYPE_NAME (iface));
	fail_unless (tny_iterator_is_done (iterator) == TRUE, str);
	g_free (str);

	g_object_unref (G_OBJECT (iterator));
}
END_TEST


Suite *
create_tny_list_suite (void)
{
     Suite *s = suite_create ("List");

     TCase *tc = tcase_create ("All lists");
     tcase_add_checked_fixture (tc, tny_list_test_setup, tny_list_test_teardown);
     tcase_add_loop_test (tc, tny_list_test_list, 0, 6);
     suite_add_tcase (s, tc);

     return s;
}
