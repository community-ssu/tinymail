/* libtinymailui - The Tiny Mail UI library
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
 * TnyHeaderView:
 * 
 * A view for a #TnyHeader
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <tny-header-view.h>

/**
 * tny_header_view_clear:
 * @self: A #TnyHeaderView
 *
 * Clear @self, show nothing
 *
 * since: 1.0
 * audience: application-developer, type-implementer 
 **/
void
tny_header_view_clear (TnyHeaderView *self)
{
#ifdef DEBUG
	if (!TNY_HEADER_VIEW_GET_IFACE (self)->clear)
		g_critical ("You must implement tny_header_view_clear\n");
#endif

	TNY_HEADER_VIEW_GET_IFACE (self)->clear(self);
	return;    
}


/**
 * tny_header_view_set_header:
 * @self: A #TnyHeaderView
 * @header: A #TnyHeader
 *
 * Set @self to display @header
 * 
 * Note that the #TnyHeaderView type is often used in a composition with a 
 * #TnyMsgView type (the #TnyMsgView implementation contains  or aggregates a 
 * #TnyHeaderView).
 *
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
void
tny_header_view_set_header (TnyHeaderView *self, TnyHeader *header)
{
#ifdef DEBUG
	if (!TNY_HEADER_VIEW_GET_IFACE (self)->set_header)
		g_critical ("You must implement tny_header_view_set_header\n");
#endif

	TNY_HEADER_VIEW_GET_IFACE (self)->set_header(self, header);
	return;
}

static void
tny_header_view_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_header_view_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyHeaderViewIface),
		  tny_header_view_base_init,   /* base_init */
		  NULL,   /* base_finalize */
		  NULL,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  0,
		  0,      /* n_preallocs */
		  NULL    /* instance_init */
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyHeaderView", &info, 0);

	g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);

	return GUINT_TO_POINTER (type);
}

GType
tny_header_view_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_header_view_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
