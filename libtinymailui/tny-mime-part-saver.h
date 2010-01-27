#ifndef TNY_MIME_PART_SAVER_H
#define TNY_MIME_PART_SAVER_H

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

#include <tny-mime-part.h>
#include <tny-mime-part-save-strategy.h>

G_BEGIN_DECLS

#define TNY_TYPE_MIME_PART_SAVER             (tny_mime_part_saver_get_type ())
#define TNY_MIME_PART_SAVER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_MIME_PART_SAVER, TnyMimePartSaver))
#define TNY_IS_MIME_PART_SAVER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_MIME_PART_SAVER))
#define TNY_MIME_PART_SAVER_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_MIME_PART_SAVER, TnyMimePartSaverIface))

typedef struct _TnyMimePartSaver TnyMimePartSaver;
typedef struct _TnyMimePartSaverIface TnyMimePartSaverIface;

struct _TnyMimePartSaverIface 
{
	GTypeInterface parent;

	TnyMimePartSaveStrategy* (*get_save_strategy) (TnyMimePartSaver *self);
	void (*set_save_strategy) (TnyMimePartSaver *self, TnyMimePartSaveStrategy *strategy);
	void (*save) (TnyMimePartSaver *self, TnyMimePart *part);	
};

GType tny_mime_part_saver_get_type(void);

TnyMimePartSaveStrategy* tny_mime_part_saver_get_save_strategy (TnyMimePartSaver *self);
void tny_mime_part_saver_set_save_strategy (TnyMimePartSaver *self, TnyMimePartSaveStrategy *strategy);
void tny_mime_part_saver_save (TnyMimePartSaver *self, TnyMimePart *part);


G_END_DECLS

#endif
