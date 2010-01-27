/* libtinymailui-gtk - The Tiny Mail UI library for Gtk+
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

/**
 * TnyFolderStoreQuery:
 *
 * A query for filtering folders when getting them from a store 
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <tny-folder-store-query.h>
#include <tny-simple-list.h>

static GObjectClass *parent_class;
static GObjectClass *item_parent_class;

/* TNY TODO: Make this cope with AND constructs. Also take a look at 
 * tny-camel-common.c:_tny_folder_store_query_passes */

/**
 * tny_folder_store_query_new:
 *
 * Create a new #TnyFolderStoreQuery instance
 * 
 * returns: (caller-owns): a new #TnyFolderStoreQuery instance
 * since: 1.0
 * audience: application-developer
 **/
TnyFolderStoreQuery* 
tny_folder_store_query_new (void)
{
	TnyFolderStoreQuery *self = g_object_new (TNY_TYPE_FOLDER_STORE_QUERY, NULL);
	return self;
}

static void
tny_folder_store_query_item_finalize (GObject *object)
{
	TnyFolderStoreQueryItem *self = (TnyFolderStoreQueryItem*) object;

	if (self->regex) {
		regfree (self->regex);
		g_free (self->regex);
	}

	if (self->pattern)
		g_free (self->pattern);

	item_parent_class->finalize (object);
}

static void
tny_folder_store_query_finalize (GObject *object)
{
	TnyFolderStoreQuery *self = (TnyFolderStoreQuery*) object;
	g_object_unref (self->items);
	parent_class->finalize (object);
	return;
}

static void
tny_folder_store_query_item_class_init (TnyFolderStoreQueryItemClass *klass)
{
	GObjectClass *object_class;
	object_class = (GObjectClass *)klass;
	item_parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = tny_folder_store_query_item_finalize;
	return;
}

static void
tny_folder_store_query_class_init (TnyFolderStoreQueryClass *klass)
{
	GObjectClass *object_class;
	object_class = (GObjectClass *)klass;
	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = tny_folder_store_query_finalize;
	return;
}

static void
tny_folder_store_query_item_init (TnyFolderStoreQueryItem *self)
{
	self->options = 0;
	self->regex = NULL;
	self->pattern = NULL;

	return;
}

static void
tny_folder_store_query_init (TnyFolderStoreQuery *self)
{
	self->items = tny_simple_list_new ();
	return;
}


static gpointer
tny_folder_store_query_register_type (gpointer notused)
{
	GType object_type = 0;

	static const GTypeInfo object_info = 
		{
			sizeof (TnyFolderStoreQueryClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) tny_folder_store_query_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (TnyFolderStoreQuery),
			0,              /* n_preallocs */
			(GInstanceInitFunc) tny_folder_store_query_init,
			NULL
		};
	object_type = g_type_register_static (G_TYPE_OBJECT, 
					      "TnyFolderStoreQuery", &object_info, 0);

	return GUINT_TO_POINTER (object_type);
}

GType
tny_folder_store_query_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_folder_store_query_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}

static gpointer
tny_folder_store_query_item_register_type (gpointer notused)
{
	GType object_type = 0;
	
	static const GTypeInfo object_info = 
		{
			sizeof (TnyFolderStoreQueryItemClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) tny_folder_store_query_item_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (TnyFolderStoreQueryItem),
			0,              /* n_preallocs */
			(GInstanceInitFunc) tny_folder_store_query_item_init,
			NULL
		};
	object_type = g_type_register_static (G_TYPE_OBJECT, 
					      "TnyFolderStoreQueryItem", &object_info, 0);
	
	return GUINT_TO_POINTER (object_type);
}

/**
 * tny_folder_store_query_item_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType
tny_folder_store_query_item_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_folder_store_query_item_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}

static gchar*
get_regerror (int errcode, regex_t *compiled)
{
	size_t length = regerror (errcode, compiled, NULL, 0);
	gchar *buffer = (gchar*) g_malloc (length);
	(void) regerror (errcode, compiled, buffer, length);
	return buffer;
}


/**
 * tny_folder_store_query_add_item:
 * @query: a #TnyFolderStoreQuery
 * @pattern: (null-ok): a regular expression or NULL
 * @options: a #TnyFolderStoreQueryOption enum
 *
 * Add a query-item to @query.
 * 
 * If the options contain TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_NAME or 
 * TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_ID and @pattern is not NULL, then
 * @pattern will be used as regular expression for matching the property of
 * the folders. The tny_folder_get_name() and tny_folder_get_id() are used 
 * while matching in those cases.
 *
 * Example:
 * <informalexample><programlisting>
 * FolderStoreQuery *query = tny_folder_store_query_new ();
 * TnyList *folders = tny_simple_list_new ();
 * TnyFolderStore *store = ...; TnyIterator *iter; 
 * tny_folder_store_query_add_item (query, ".*GNOME.*", 
 *          TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_NAME);
 * tny_folder_store_get_folders (store, folders, query);
 * iter = tny_list_create_iterator (folders);
 * while (!tny_iterator_is_done (iter))
 * {
 *      TnyFolder *folder = TNY_FOLDER (tny_iterator_get_current (iter));
 *      g_print ("%s\n", tny_folder_get_name (folder));
 *      g_object_unref (folder);
 *      tny_iterator_next (iter);
 * }
 * g_object_unref (iter);
 * g_object_unref (folders); 
 * g_object_unref (query); 
 * </programlisting></informalexample>
 *
 * For the options TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED and 
 * TNY_FOLDER_STORE_QUERY_OPTION_UNSUBSCRIBED, the folder subscription status
 * is used.
 *
 * Example:
 * <informalexample><programlisting>
 * FolderStoreQuery *query = tny_folder_store_query_new ();
 * TnyList *folders = tny_simple_list_new ();
 * TnyFolderStore *store = ...; TnyIterator *iter; 
 * tny_folder_store_query_add_item (query, NULL, 
 *          TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED);
 * tny_folder_store_get_folders (store, folders, query);
 * iter = tny_list_create_iterator (folders);
 * while (!tny_iterator_is_done (iter))
 * {
 *      TnyFolder *folder = TNY_FOLDER (tny_iterator_get_current (iter));
 *      g_print ("%s\n", tny_folder_get_name (folder));
 *      g_object_unref (folder);
 *      tny_iterator_next (iter);
 * }
 * g_object_unref (iter);
 * g_object_unref (folders);
 * g_object_unref (query);
 * </programlisting></informalexample>
 *
 **/
void 
tny_folder_store_query_add_item (TnyFolderStoreQuery *query, const gchar *pattern, TnyFolderStoreQueryOption options)
{
	gint er=0;
	gboolean addit=TRUE;
	regex_t *regex = NULL;
	gboolean has_regex = FALSE;

	if (pattern && (options & TNY_FOLDER_STORE_QUERY_OPTION_PATTERN_IS_REGEX)) 
	{
		regex = g_new0 (regex_t, 1);
		er = regcomp (regex, (const char*)pattern, 0);
		if (er != 0) {
			gchar *erstr = get_regerror (er, regex);
			g_warning (erstr);
			g_free (erstr);
			regfree (regex);
			addit = FALSE;
			regex = NULL;
		} else
			has_regex = TRUE;
	}

	if (addit)
	{
		TnyFolderStoreQueryItem *add = g_object_new (TNY_TYPE_FOLDER_STORE_QUERY_ITEM, NULL);

		add->options = options;
		if (pattern)
			add->pattern = g_strdup (pattern);
		else
			add->pattern = FALSE;

		if (has_regex)
			add->regex = regex;
		else 
			add->regex = NULL;

		tny_list_prepend (query->items, (GObject *) add);
		g_object_unref (add);
	}

	return;
}

/**
 * tny_folder_store_query_get_items:
 * @query: a #TnyFolderStoreQuery
 *
 * Get a list of query items in @query. The return value must be unreferenced
 * after use.
 *
 * returns: (caller-owns): a list of query items
 * since: 1.0
 * audience: tinymail-developer
 **/
TnyList*
tny_folder_store_query_get_items (TnyFolderStoreQuery *query)
{
	return TNY_LIST (g_object_ref (query->items));
}


/**
 * tny_folder_store_query_item_get_options:
 * @item: a #TnyFolderStoreQueryItem
 *
 * Get the options of @item as a #TnyFolderStoreQueryOption enum.
 *
 * returns: the options of a query item
 * since: 1.0
 * audience: tinymail-developer
 **/
TnyFolderStoreQueryOption 
tny_folder_store_query_item_get_options (TnyFolderStoreQueryItem *item)
{
	return item->options;
}


/**
 * tny_folder_store_query_item_get_regex:
 * @item: a #TnyFolderStoreQueryItem
 *
 * Get the compiled regular expression of @item. You must not free the returned
 * value.
 *
 * returns: (null-ok): the compiled regular expression of a query item
 * since: 1.0
 * audience: tinymail-developer
 **/
const regex_t*
tny_folder_store_query_item_get_regex (TnyFolderStoreQueryItem *item)
{
	return (const regex_t*) item->regex;
}


/**
 * tny_folder_store_query_item_get_pattern:
 * @item: a #TnyFolderStoreQueryItem
 *
 * Get the regular expression's pattern of @item. You must not free the returned
 * value.
 *
 * returns: (null-ok): the pattern of the regular expression of a query item
 * since: 1.0
 * audience: tinymail-developer
 **/
const gchar* 
tny_folder_store_query_item_get_pattern (TnyFolderStoreQueryItem *item)
{
	return (const gchar*) item->pattern;
}

static gpointer
tny_folder_store_query_option_register_type (gpointer notused)
{
	GType etype = 0;
	static const GFlagsValue values[] = {
		{ TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED, "TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED", "subscribed" },
		{ TNY_FOLDER_STORE_QUERY_OPTION_UNSUBSCRIBED, "TNY_FOLDER_STORE_QUERY_OPTION_UNSUBSCRIBED", "unsubscribed" },
		{ TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_NAME, "TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_NAME", "match_on_name" },
		{ TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_ID, "TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_ID", "match_on_id" },
		{ TNY_FOLDER_STORE_QUERY_OPTION_PATTERN_IS_CASE_INSENSITIVE, "TNY_FOLDER_STORE_QUERY_OPTION_PATTERN_IS_CASE_INSENSITIVE", "pattern_is_case_insensitive" },
		{ TNY_FOLDER_STORE_QUERY_OPTION_PATTERN_IS_REGEX, "TNY_FOLDER_STORE_QUERY_OPTION_PATTERN_IS_REGEX", "pattern_is_regex" },
		{ 0, NULL, NULL }
	};
	etype = g_flags_register_static ("TnyFolderStoreQueryOption", values);
	return GUINT_TO_POINTER (etype);
}

/**
 * tny_folder_store_query_option_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType
tny_folder_store_query_option_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_folder_store_query_option_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
