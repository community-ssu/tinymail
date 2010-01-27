/* libtinymail - The Tiny Mail base library
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
 * TnyPair:
 *
 * a key - value pair
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <string.h>

#include <tny-pair.h>

static GObjectClass *parent_class = NULL;

typedef struct _TnyPairPriv TnyPairPriv;

struct _TnyPairPriv
{
	gchar *name, *value;
};

#define TNY_PAIR_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_PAIR, TnyPairPriv))


/**
 * tny_pair_set_name:
 * @self: a #TnyPair
 * @name: the name to set
 *
 * Sets the name of the pair
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_pair_set_name (TnyPair *self, const gchar *name)
{
	TnyPairPriv *priv = TNY_PAIR_GET_PRIVATE (self);
	if (priv->name)
		g_free (priv->name);
	priv->name = g_strdup ((gchar *) name);
	return;
}

/**
 * tny_pair_get_name:
 * @self: a #TnyPair
 * 
 * Get the name of the pair, the returned value must not be freed
 *
 * returns: the name of the pair
 * since: 1.0
 * audience: application-developer
 **/
const gchar* 
tny_pair_get_name (TnyPair *self)
{
	TnyPairPriv *priv = TNY_PAIR_GET_PRIVATE (self);
	return (const gchar *) priv->name;
}

/**
 * tny_pair_set_value:
 * @self: a #TnyPair
 * @value: the value to set
 *
 * Sets the value of the pair
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_pair_set_value (TnyPair *self, const gchar *value)
{
	TnyPairPriv *priv = TNY_PAIR_GET_PRIVATE (self);
	if (priv->value)
		g_free (priv->value);
	priv->value = g_strdup ((gchar *) value);
	return;
}


/**
 * tny_pair_get_value:
 * @self: a #TnyPair
 *
 * Get the value of the pair, the returned value must not be freed
 *
 * returns: the value of the pair
 * since: 1.0
 * audience: application-developer
 **/
const gchar* 
tny_pair_get_value (TnyPair *self)
{
	TnyPairPriv *priv = TNY_PAIR_GET_PRIVATE (self);
	return (const gchar *) priv->value;
}

/**
 * tny_pair_new:
 * @name: the name of the pair
 * @value: the value of the pair
 *
 * Creates an instance of a type that holds a name and a value pair
 *
 * returns: (caller-owns): a new #TnyPair instance
 * since: 1.0
 * audience: application-developer
 **/
TnyPair*
tny_pair_new (const gchar *name, const gchar *value)
{
	TnyPair *self = g_object_new (TNY_TYPE_PAIR, NULL);

	tny_pair_set_name (self, name);
	tny_pair_set_value (self, value);

	return self;
}

static void
tny_pair_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyPair *self = (TnyPair *)instance;
	TnyPairPriv *priv = TNY_PAIR_GET_PRIVATE (self);

	priv->name = NULL;
	priv->value = NULL;

	return;
}

static void
tny_pair_finalize (GObject *object)
{
	TnyPair *self = (TnyPair *)object;	
	TnyPairPriv *priv = TNY_PAIR_GET_PRIVATE (self);

	if (priv->name)
		g_free (priv->name);

	if (priv->value)
		g_free (priv->value);

	(*parent_class->finalize) (object);

	return;
}


static void 
tny_pair_class_init (TnyPairClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	object_class->finalize = tny_pair_finalize;
	g_type_class_add_private (object_class, sizeof (TnyPairPriv));

	return;
}

static gpointer
tny_pair_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyPairClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_pair_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyPair),
			0,      /* n_preallocs */
			tny_pair_instance_init,   /* instance_init */
			NULL
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyPair",
				       &info, 0);
	return GUINT_TO_POINTER (type);
}

GType 
tny_pair_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_pair_register_type, NULL);
	return  GPOINTER_TO_UINT (once.retval);
}
