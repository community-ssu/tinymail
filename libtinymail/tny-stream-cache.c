/* libtinymail - The Tiny Mail base library
 * Copyright (C) 2008 Jose Dapena Paz <jdapena@igalia.com>
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
 * TnyStreamCache:
 *
 * A file-oriented cache for streams.
 *
 * free-function: g_object_unref
 **/

#include <config.h>
#include <tny-stream-cache.h>

/**
 * tny_stream_cache_get_stream:
 * @self: a #TnyStreamCache
 * @id: a stringthe unique id used to identify the stream
 * @fetcher: a #TnyStreamCacheOpenStreamFetcher, function to obtain a
 *  TnyStream for fetching the file in case it's not in cache.
 * @destroy_userdata: a #GDestroyNotify, that frees @userdata once we used it
 * @userdata a #gpointer
 * 
 * Obtain a #TnyStream with the file referred with @id from cache. If it's
 * not in cache, it sets up a stream resource to fetch the file from internet
 * and store in cache while sending information to the user.
 *
 * Returns: a #TnyStream
 * since: 1.0
 * audience: application-developer
 */
TnyStream *
tny_stream_cache_get_stream (TnyStreamCache *self, const gchar *id,
			     TnyStreamCacheOpenStreamFetcher fetcher, gpointer userdata)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_STREAM_CACHE (self));
	g_assert (id);
	g_assert (TNY_STREAM_CACHE_GET_IFACE (self)->get_stream!= NULL);
#endif

	return TNY_STREAM_CACHE_GET_IFACE (self)->get_stream(self, id, fetcher, userdata);
}

/**
 * tny_stream_cache_set_max_size:
 * @self: a #TnyStreamCache
 * @max_size: a #gint64
 *
 * sets the new maximum size the cache can host.
 *
 * since: 1.0
 * audience: application-developer
 */
void
tny_stream_cache_set_max_size (TnyStreamCache *self, gint64 max_size)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_STREAM_CACHE (self));
	g_assert (TNY_STREAM_CACHE_GET_IFACE (self)->set_max_size != NULL);
#endif

	TNY_STREAM_CACHE_GET_IFACE (self)->set_max_size(self, max_size);

	return;
}

/**
 * tny_stream_cache_get_max_size:
 * @self: a #TnyStreamCache
 *
 * obtain the maximum size the cache can host.
 *
 * Returns: a #gint64
 *
 * since: 1.0
 * audience: application-developer
 */
gint64
tny_stream_cache_get_max_size (TnyStreamCache *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_STREAM_CACHE (self));
	g_assert (TNY_STREAM_CACHE_GET_IFACE (self)->get_max_size != NULL);
#endif

	return TNY_STREAM_CACHE_GET_IFACE (self)->get_max_size(self);
}

/**
 * tny_stream_cache_remove:
 * @self: a #TnyStreamCache
 * @filter: a #TnyStreamCacheRemoveFilter
 * @userdata: a #gpointer
 * 
 * Removes all the cached files matching filter.
 *
 * since: 1.0
 * audience: application-developer
 */
void
tny_stream_cache_remove (TnyStreamCache *self, TnyStreamCacheRemoveFilter filter, gpointer userdata)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_STREAM_CACHE (self));
	g_assert (filter);
	g_assert (TNY_STREAM_CACHE_GET_IFACE (self)->remove!= NULL);
#endif

	TNY_STREAM_CACHE_GET_IFACE (self)->remove(self, filter, userdata);
	return;
}


static void
tny_stream_cache_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_stream_cache_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyStreamCacheIface),
			tny_stream_cache_base_init,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL,   /* instance_init */
			NULL
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyCacheStream", &info, 0);
	
	return GUINT_TO_POINTER (type);
}

GType
tny_stream_cache_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_stream_cache_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
