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

#include <config.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <tny-tp-mail-notifier.h>


static GObjectClass *parent_class;


typedef struct _TnyTpMailNotifierPriv TnyTpMailNotifierPriv;

struct _TnyTpMailNotifierPriv
{
	int *thing;
};

#define TNY_TP_MAIL_NOTIFIER_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_TP_MAIL_NOTIFIER, TnyTpMailNotifierPriv))


static void
tny_tp_mail_notifier_finalize (GObject *object)
{
	/* TnyTpMailNotifierPriv *priv = TNY_TP_MAIL_NOTIFIER_GET_PRIVATE (object); */

	parent_class->finalize (object);

	return;
}


static void
tny_tp_mail_notifier_class_init (TnyTpMailNotifierClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *)klass;
	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = tny_tp_mail_notifier_finalize;
	g_type_class_add_private (object_class, sizeof (TnyTpMailNotifierPriv));

	return;
}

static void
tny_tp_mail_notifier_init (TnySimpleList *self)
{
	TnyTpMailNotifierPriv *priv = TNY_TP_MAIL_NOTIFIER_GET_PRIVATE (self);

	priv->thing = NULL;

	return;
}


/**
 * tny_tp_mail_notifier_new:
 * @folder: a #TnyFolder
 * 
 * Create an observable for GMail's Push E-mail
 *
 * Return value: A observable for GMail's Push E-mail
 **/
TnyTpMailNotifier*
tny_tp_mail_notifier_new (TnyFolder *folder)
{
	TnyTpMailNotifier *self = g_object_new (TNY_TYPE_TP_MAIL_NOTIFIER, NULL);

	return self;
}

static gpointer
tny_tp_mail_notifier_register_type (gpointer notused)
{
	GType object_type = 0;

	static const GTypeInfo object_info = 
		{
			sizeof (TnyTpMailNotifierClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) tny_tp_mail_notifier_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (TnyTpMailNotifier),
			0,              /* n_preallocs */
			(GInstanceInitFunc) tny_tp_mail_notifier_init,
			NULL
		};

	object_type = g_type_register_static (G_TYPE_OBJECT, 
					      "TnyTpMailNotifier", &object_info, 0);

	return GSIZE_TO_POINTER (object_type);
}

GType
tny_tp_mail_notifier_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_tp_mail_notifier_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
