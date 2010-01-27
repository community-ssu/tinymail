#ifndef TNY_TP_MAIL_NOTIFIER_H
#define TNY_TP_MAIL_NOTIFIER_H

/* libtinymail - The Tiny Mail library
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
#include <tny-shared.h>
#include <tny-folder.h>

G_BEGIN_DECLS

#define TNY_TYPE_TP_MAIL_NOTIFIER             (tny_tp_mail_notifier_get_type ())
#define TNY_TP_MAIL_NOTIFIER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_TP_MAIL_NOTIFIER, TnyTpMailNotifier))
#define TNY_TP_MAIL_NOTIFIER_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_TP_MAIL_NOTIFIER, TnyTpMailNotifierClass))
#define TNY_IS_TP_MAIL_NOTIFIER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_TP_MAIL_NOTIFIER))
#define TNY_IS_TP_MAIL_NOTIFIER_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_TP_MAIL_NOTIFIER))
#define TNY_TP_MAIL_NOTIFIER_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_TP_MAIL_NOTIFIER, TnyTpMailNotifierClass))

typedef struct _TnyTpMailNotifier TnyTpMailNotifier;
typedef struct _TnyTpMailNotifierClass TnyTpMailNotifierClass;

struct _TnyTpMailNotifier 
{
	GObject parent;
};

struct _TnyTpMailNotifierClass 
{
	GObjectClass parent;
};

GType tny_tp_mail_notifier_get_type (void);
TnyTpMailNotifier* tny_tp_mail_notifier_new (TnyFolder *folder);

G_END_DECLS

#endif
