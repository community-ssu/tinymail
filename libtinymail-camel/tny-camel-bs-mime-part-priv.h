#ifndef TNY_CAMEL_BS_MIME_PART_PRIV_H
#define TNY_CAMEL_BS_MIME_PART_PRIV_H

/* libtinymail-camel - The Tiny Mail base library for Camel
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

#include "bs/bodystruct.h"

#include <tny-camel-bs-msg-receive-strategy.h>

typedef struct _TnyCamelBsMimePartPriv TnyCamelBsMimePartPriv;

struct _TnyCamelBsMimePartPriv
{
	GMutex *part_lock;
	bodystruct_t *bodystructure;
	gchar *cached_content_type;
	gchar *cached_filename;
	gchar *uid;
	TnyCamelBsMimePart *parent;
	TnyFolder *folder;
	TnyCamelBsMsgReceiveStrategy *strat;
};

#define TNY_CAMEL_BS_MIME_PART_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_CAMEL_BS_MIME_PART, TnyCamelBsMimePartPriv))

TnyMimePart* _tny_camel_bs_mime_part_new (bodystruct_t *bodystructure, const gchar *uid, TnyCamelBsMimePart *parent);

void _tny_camel_bs_mime_part_set_folder (TnyCamelBsMimePart *self, TnyFolder* folder);
void _tny_camel_bs_mime_part_set_strat (TnyCamelBsMimePart *self, TnyCamelBsMsgReceiveStrategy* strat);

#endif
