#ifndef TNY_HEADER_VIEW_H
#define TNY_HEADER_VIEW_H

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

#include <glib.h>
#include <glib-object.h>
#include <tny-shared.h>
#include <tny-header.h>

G_BEGIN_DECLS

#define TNY_TYPE_HEADER_VIEW             (tny_header_view_get_type ())
#define TNY_HEADER_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_HEADER_VIEW, TnyHeaderView))
#define TNY_IS_HEADER_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_HEADER_VIEW))
#define TNY_HEADER_VIEW_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_HEADER_VIEW, TnyHeaderViewIface))

typedef struct _TnyHeaderView TnyHeaderView;
typedef struct _TnyHeaderViewIface TnyHeaderViewIface;


struct _TnyHeaderViewIface
{
	GTypeInterface parent;

	void (*set_header) (TnyHeaderView *self, TnyHeader *header);
	void (*clear) (TnyHeaderView *self);   
};

GType tny_header_view_get_type (void);

void  tny_header_view_set_header (TnyHeaderView *self, TnyHeader *header);
void  tny_header_view_clear (TnyHeaderView *self);


G_END_DECLS

#endif
