#ifndef TNY_STREAM_CACHE_H
#define TNY_STREAM_CACHE_H

/* libtinymail- The Tiny Mail base library
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

#include <glib.h>
#include <glib-object.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <tny-stream.h>
#include <tny-shared.h>

G_BEGIN_DECLS

#define TNY_TYPE_STREAM_CACHE             (tny_stream_cache_get_type ())
#define TNY_STREAM_CACHE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_STREAM_CACHE, TnyStreamCache))
#define TNY_IS_STREAM_CACHE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_STREAM_CACHE))
#define TNY_STREAM_CACHE_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_STREAM_CACHE, TnyStreamCacheIface))

#ifndef TNY_SHARED_H
typedef struct _TnyStreamCache TnyStreamCache;
typedef struct _TnyStreamCacheIface TnyStreamCacheIface;
#endif

struct _TnyStreamCacheIface 
{
	GTypeInterface parent;
	
	TnyStream*       (*get_stream)   (TnyStreamCache *self, const char *id,
					  TnyStreamCacheOpenStreamFetcher fetcher, gpointer userdata);
	gint64           (*get_max_size) (TnyStreamCache *self);
	void             (*set_max_size) (TnyStreamCache *self, gint64 max_size);
	void             (*remove)       (TnyStreamCache *self, TnyStreamCacheRemoveFilter filter, gpointer data);
};

GType      tny_stream_cache_get_type (void);

TnyStream* tny_stream_cache_get_stream (TnyStreamCache *cache, const char *id,
					TnyStreamCacheOpenStreamFetcher fetcher, gpointer userdata);
void       tny_stream_cache_remove (TnyStreamCache *cache, TnyStreamCacheRemoveFilter filter, gpointer userdata);
void       tny_stream_cache_set_max_size (TnyStreamCache *cache, gint64 max_size);
gint64    tny_stream_cache_get_max_size (TnyStreamCache *cache);

G_END_DECLS

#endif

