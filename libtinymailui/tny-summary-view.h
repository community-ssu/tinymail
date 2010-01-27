#ifndef TNY_SUMMARY_VIEW_H
#define TNY_SUMMARY_VIEW_H

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

G_BEGIN_DECLS

#define TNY_TYPE_SUMMARY_VIEW             (tny_summary_view_get_type ())
#define TNY_SUMMARY_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_SUMMARY_VIEW, TnySummaryView))
#define TNY_IS_SUMMARY_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_SUMMARY_VIEW))
#define TNY_SUMMARY_VIEW_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_SUMMARY_VIEW, TnySummaryViewIface))

typedef struct _TnySummaryView TnySummaryView;
typedef struct _TnySummaryViewIface TnySummaryViewIface;


struct _TnySummaryViewIface
{
	GTypeInterface parent;
};

GType tny_summary_view_get_type (void);


G_END_DECLS

#endif
