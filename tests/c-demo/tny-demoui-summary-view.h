#ifndef TNY_DEMOUI_SUMMARY_VIEW_H
#define TNY_DEMOUI_SUMMARY_VIEW_H

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

#include <gtk/gtk.h>
#include <glib-object.h>
#include <tny-shared.h>
#include <tny-summary-view.h>

G_BEGIN_DECLS

#define TNY_TYPE_DEMOUI_SUMMARY_VIEW             (tny_demoui_summary_view_get_type ())
#define TNY_DEMOUI_SUMMARY_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_DEMOUI_SUMMARY_VIEW, TnyDemouiSummaryView))
#define TNY_DEMOUI_SUMMARY_VIEW_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_DEMOUI_SUMMARY_VIEW, TnyDemuiSummaryViewClass))
#define TNY_IS_DEMOUI_SUMMARY_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_DEMOUI_SUMMARY_VIEW))
#define TNY_IS_DEMOUI_SUMMARY_VIEW_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_DEMOUI_SUMMARY_VIEW))
#define TNY_DEMOUI_SUMMARY_VIEW_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_DEMOUI_SUMMARY_VIEW, TnyDemouiSummaryViewClass))

typedef struct _TnyDemouiSummaryView TnyDemouiSummaryView;
typedef struct _TnyDemouiSummaryViewClass TnyDemouiSummaryViewClass;

struct _TnyDemouiSummaryView
{
	GtkVBox parent;

};

struct _TnyDemouiSummaryViewClass
{
	GtkVBoxClass parent_class;
};

GType tny_demoui_summary_view_get_type (void);
TnySummaryView* tny_demoui_summary_view_new (void);


G_END_DECLS

#endif
