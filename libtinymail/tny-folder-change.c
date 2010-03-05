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
 * TnyFolderChange:
 *
 * A delta of changes, used by #TnyFolderObserver 
 *
 * free-functions: g_object_unref
 **/

#include <config.h>

#include <tny-folder-change.h>
#include <tny-simple-list.h>
#include <tny-list.h>

static GObjectClass *parent_class = NULL;

typedef struct _TnyFolderChangePriv TnyFolderChangePriv;

struct _TnyFolderChangePriv
{
	TnyList *added, *removed;
	guint new_unread_count;
	guint new_all_count;
	GMutex *lock;
	TnyFolder *folder;
	gchar *oldname, *newname;
	TnyFolderChangeChanged changed;
	TnyMsg *msg; gboolean check_duplicates;
};

#define TNY_FOLDER_CHANGE_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_FOLDER_CHANGE, TnyFolderChangePriv))


/**
 * tny_folder_change_get_check_duplicates:
 * @self: a #TnyFolderChange
 *
 * Get whether or not the observer must check for duplicates in its
 * tny_folder_observer_update() implementation.
 *
 * returns: whether or not to check duplicates
 * since: 1.0
 * audience: tinymail-developer
 **/
gboolean 
tny_folder_change_get_check_duplicates (TnyFolderChange *self)
{
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);
	return priv->check_duplicates;
}


/**
 * tny_folder_change_set_check_duplicates:
 * @self: a #TnyFolderChange
 * @check_duplicates: whether or not to check for duplicates
 *
 * Set whether or not the observer must check for duplicates in its
 * tny_folder_observer_update() implementation. Default value is FALSE.
 *
 * since: 1.0
 * audience: tinymail-developer
 **/
void 
tny_folder_change_set_check_duplicates (TnyFolderChange *self, gboolean check_duplicates)
{
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);
	priv->check_duplicates = check_duplicates;
	return;
}

/**
 * tny_folder_change_set_received_msg:
 * @self: a #TnyFolderChange
 * @msg: a #TnyMsg
 *
 * Set the message that got received in @self. This is an internal function not
 * intended for application developers to alter. Default value is FALSE.
 *
 * since: 1.0
 * audience: tinymail-developer
 **/
void 
tny_folder_change_set_received_msg (TnyFolderChange *self, TnyMsg *msg)
{
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);

	g_mutex_lock (priv->lock);
	if (priv->msg)
		g_object_unref (priv->msg);
	priv->msg = TNY_MSG (g_object_ref (msg));
	priv->changed |= TNY_FOLDER_CHANGE_CHANGED_MSG_RECEIVED;
	g_mutex_unlock (priv->lock);
}

/**
 * tny_folder_change_get_received_msg:
 * @self: a #TnyFolderChange
 *
 * Get the message that got received, if the change includes receiving a message.
 * If no message was received, NULL will be returned. If not NULL, the returned
 * value must be unreferenced after use.
 *
 * returns: (null-ok) (caller-owns): received message or NULL
 * since: 1.0
 * audience: application-developer
 **/
TnyMsg *
tny_folder_change_get_received_msg (TnyFolderChange *self)
{
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);
	TnyMsg *msg = NULL;

	g_mutex_lock (priv->lock);
	if (priv->msg)
		msg = TNY_MSG (g_object_ref (priv->msg));
	g_mutex_unlock (priv->lock);

	return msg;
}

/**
 * tny_folder_change_get_changed:
 * @self: a #TnyFolderChange
 *
 * Get a flags field with all the changed things
 *
 * returns: the changed items as a flags field
 * since: 1.0
 * audience: application-developer
 **/
TnyFolderChangeChanged 
tny_folder_change_get_changed  (TnyFolderChange *self)
{
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);
	return priv->changed;
}

/**
 * tny_folder_change_set_new_all_count:
 * @self: a #TnyFolderChange
 * @new_all_count: all count
 *
 * Set the all count of @self. This is an internal function not intended for
 * application developers to alter.
 *
 * since: 1.0
 * audience: tinymail-developer
 **/
void 
tny_folder_change_set_new_all_count (TnyFolderChange *self, guint new_all_count)
{
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);

	g_mutex_lock (priv->lock);
	priv->changed |= TNY_FOLDER_CHANGE_CHANGED_ALL_COUNT;
	priv->new_all_count = new_all_count;
	g_mutex_unlock (priv->lock);

	return;
}

/**
 * tny_folder_change_set_new_unread_count:
 * @self: a #TnyFolderChange 
 * @new_unread_count: unread count
 *
 * Set the unread count. This is an internal function not intended for
 * application developers to alter.
 *
 * since: 1.0
 * audience: tinymail-developer
 **/
void 
tny_folder_change_set_new_unread_count (TnyFolderChange *self, guint new_unread_count)
{
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);

	g_mutex_lock (priv->lock);
	priv->changed |= TNY_FOLDER_CHANGE_CHANGED_UNREAD_COUNT;
	priv->new_unread_count = new_unread_count;
	g_mutex_unlock (priv->lock);

	return;
}


/**
 * tny_folder_change_get_new_unread_count:
 * @self: a #TnyFolderChange
 *
 * Get the unread count of @self.
 *
 * returns: unread count
 * since: 1.0
 * audience: application-developer
 **/
guint 
tny_folder_change_get_new_unread_count (TnyFolderChange *self)
{
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);
	return priv->new_unread_count;
}

/**
 * tny_folder_change_get_new_all_count:
 * @self: a #TnyFolderChange
 *
 * Get the all count of @self
 *
 * returns: all count
 * since: 1.0
 * audience: application-developer
 **/
guint 
tny_folder_change_get_new_all_count (TnyFolderChange *self)
{
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);
	return priv->new_all_count;
}

/**
 * tny_folder_change_add_added_header:
 * @self: a #TnyFolderChange
 * @header: a #TnyHeader
 *
 * Add @header to @self. This is an internal function not intended for
 * application developers to alter.
 *
 * since: 1.0
 * audience: tinymail-developer
 **/
void 
tny_folder_change_add_added_header (TnyFolderChange *self, TnyHeader *header)
{
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);

	g_mutex_lock (priv->lock);

	if (!priv->added)
		priv->added = tny_simple_list_new ();
	priv->changed |= TNY_FOLDER_CHANGE_CHANGED_ADDED_HEADERS;
	tny_list_prepend (priv->added, G_OBJECT (header));
	g_mutex_unlock (priv->lock);

	return;
}

/**
 * tny_folder_change_add_expunged_header:
 * @self: a #TnyFolderChange instance
 * @header: a #TnyHeader
 *
 * Add @header to @self. This is an internal function not intended for
 * application developers to alter.
 *
 * since: 1.0
 * audience: tinymail-developer
 **/
void 
tny_folder_change_add_expunged_header (TnyFolderChange *self, TnyHeader *header)
{
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);

	g_mutex_lock (priv->lock);

	if (!priv->removed)
		priv->removed = tny_simple_list_new ();
	priv->changed |= TNY_FOLDER_CHANGE_CHANGED_EXPUNGED_HEADERS;
	tny_list_prepend (priv->removed, G_OBJECT (header));
	g_mutex_unlock (priv->lock);

	return;
}

/**
 * tny_folder_change_get_added_headers:
 * @self: a #TnyFolderChange
 * @headers: a #TnyList where the added headers will be prepended to
 *
 * Get the added headers in @self.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_change_get_added_headers (TnyFolderChange *self, TnyList *headers)
{
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);
	TnyIterator *iter;

	g_assert (TNY_IS_LIST (headers));

	g_mutex_lock (priv->lock);

	if (!priv->added)
	{
		g_mutex_unlock (priv->lock);
		return;
	}

	iter = tny_list_create_iterator (priv->added);

	while (!tny_iterator_is_done (iter))
	{
		GObject *header = tny_iterator_get_current (iter);
		tny_list_prepend (headers, header);
		g_object_unref (header);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);

	g_mutex_unlock (priv->lock);

	return;
}


/**
 * tny_folder_change_get_expunged_headers:
 * @self: a #TnyFolderChange
 * @headers: a #TnyList where the removed headers will be prepended to
 *
 * Get the removed headers in @self.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_change_get_expunged_headers (TnyFolderChange *self, TnyList *headers)
{
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);
	TnyIterator *iter;

	g_assert (TNY_IS_LIST (headers));

	g_mutex_lock (priv->lock);

	if (!priv->removed)
	{
		g_mutex_unlock (priv->lock);
		return;
	}

	iter = tny_list_create_iterator (priv->removed);

	while (!tny_iterator_is_done (iter))
	{
		GObject *header = tny_iterator_get_current (iter);
		tny_list_prepend (headers, header);
		g_object_unref (header);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);

	g_mutex_unlock (priv->lock);

	return;
}

/**
 * tny_folder_change_reset:
 * @self: a #TnyFolderChange
 *
 * Reset the state of @self. This is an internal function not intended for
 * application developers to alter.
 *
 * since: 1.0
 * audience: tinymail-developer
 **/
void 
tny_folder_change_reset (TnyFolderChange *self)
{
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);

	g_mutex_lock (priv->lock);

	priv->changed = 0;
	priv->new_unread_count = 0;
	priv->new_all_count = 0;
	if (priv->added)
		g_object_unref (G_OBJECT (priv->added));
	if (priv->removed)
		g_object_unref (G_OBJECT (priv->removed));
	priv->added = NULL;
	priv->removed = NULL;
	if (priv->msg)
		g_object_unref (priv->msg);
	priv->msg = NULL;
	g_mutex_unlock (priv->lock);
}

/**
 * tny_folder_change_get_rename:
 * @self: a #TnyFolderChange instance
 * @oldname: (null-ok) (out): a byref string or NULL
 *
 * Get the new name of the folder in case of a rename. This will return NULL
 * if no rename happened. You can pass a pointer if you need the old name of
 * the folder too, else pass NULL.
 *
 * You must not free the returned value nor the @oldname pointer.
 *
 * return: (null-ok): The new folder name or NULL
 * since: 1.0
 * audience: application-developer
 **/
const gchar *
tny_folder_change_get_rename (TnyFolderChange *self, const gchar **oldname)
{
	const gchar *retval = NULL;
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);

	g_mutex_lock (priv->lock);

	if (priv->changed & TNY_FOLDER_CHANGE_CHANGED_FOLDER_RENAME)
	{
		retval = priv->newname;
		if (oldname)
			*oldname = priv->oldname;
	}

	g_mutex_unlock (priv->lock);

	return retval;
}

/**
 * tny_folder_change_set_rename:
 * @self: a #TnyFolderChange instance
 * @newname: the new name of the folder
 *
 * Mark the change in such a way that the user can know that a rename has 
 * happened. This is an internal function not intended for application 
 * developers to alter.
 *
 * since: 1.0
 * audience: tinymail-developer
 **/
void 
tny_folder_change_set_rename (TnyFolderChange *self, const gchar *newname)
{
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);

	g_mutex_lock (priv->lock);
	priv->changed |= TNY_FOLDER_CHANGE_CHANGED_FOLDER_RENAME;
	if (priv->newname)
		g_free (priv->newname);
	priv->newname = g_strdup (newname);
	g_mutex_unlock (priv->lock);
}


/**
 * tny_folder_change_new:
 * @folder: a #TnyFolder
 *
 * Creates a changeset for @folder
 *
 * returns: (caller-owns): a new #TnyFolderChange instance
 * since: 1.0
 * audience: application-developer
 **/
TnyFolderChange*
tny_folder_change_new (TnyFolder *folder)
{
	TnyFolderChange *self = g_object_new (TNY_TYPE_FOLDER_CHANGE, NULL);
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);

	priv->folder = TNY_FOLDER (g_object_ref (G_OBJECT (folder)));
	priv->oldname = g_strdup (tny_folder_get_name (folder));

	return self;
}

/**
 * tny_folder_change_get_folder:
 * @self: a #TnyFolderChange instance
 *
 * Get the folder of @self. The return value of this method must be unreferenced 
 * after use.
 *
 * returns: (caller-owns): the #TnyFolder of this changeset
 * since: 1.0
 * audience: application-developer
 **/
TnyFolder* 
tny_folder_change_get_folder (TnyFolderChange *self)
{
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);
	TnyFolder *retval = NULL;

	g_mutex_lock (priv->lock);
	if (priv->folder)
		retval = TNY_FOLDER (g_object_ref (G_OBJECT (priv->folder)));
	g_mutex_unlock (priv->lock);

	return retval;
}


static void
tny_folder_change_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyFolderChange *self = (TnyFolderChange *)instance;
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);

	priv->lock = g_mutex_new ();

	g_mutex_lock (priv->lock);

	priv->check_duplicates = FALSE;
	priv->msg = NULL;
	priv->changed = 0;
	priv->new_unread_count = 0;
	priv->new_all_count = 0;
	priv->added = NULL;
	priv->removed = NULL;
	priv->folder = NULL;
	priv->oldname = NULL;
	priv->newname = NULL;

	g_mutex_unlock (priv->lock);

	return;
}

static void
tny_folder_change_finalize (GObject *object)
{
	TnyFolderChange *self = (TnyFolderChange *)object;	
	TnyFolderChangePriv *priv = TNY_FOLDER_CHANGE_GET_PRIVATE (self);

	g_mutex_lock (priv->lock);

	if (priv->added)
		g_object_unref (G_OBJECT (priv->added));
	if (priv->removed)
		g_object_unref (G_OBJECT (priv->removed));
	priv->added = NULL;
	priv->removed = NULL;
	if (priv->msg)
		g_object_unref (priv->msg);
	if (priv->oldname)
		g_free (priv->oldname);
	if (priv->newname)
		g_free (priv->newname);
	if (priv->folder)
		g_object_unref (G_OBJECT (priv->folder));

	g_mutex_unlock (priv->lock);

	g_mutex_free (priv->lock);

	(*parent_class->finalize) (object);

	return;
}


static void 
tny_folder_change_class_init (TnyFolderChangeClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	object_class->finalize = tny_folder_change_finalize;
	g_type_class_add_private (object_class, sizeof (TnyFolderChangePriv));

	return;
}

static gpointer
tny_folder_change_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyFolderChangeClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_folder_change_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyFolderChange),
			0,      /* n_preallocs */
			tny_folder_change_instance_init,   /* instance_init */
			NULL
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyFolderChange",
				       &info, 0);
	return GSIZE_TO_POINTER (type);
}

GType 
tny_folder_change_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_folder_change_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}

static gpointer
tny_folder_change_changed_register_type (gpointer notused)
{
  GType etype = 0;
  static const GFlagsValue values[] = {
	  { TNY_FOLDER_CHANGE_CHANGED_ALL_COUNT, "TNY_FOLDER_CHANGE_CHANGED_ALL_COUNT", "all-count" },
	  { TNY_FOLDER_CHANGE_CHANGED_UNREAD_COUNT, "TNY_FOLDER_CHANGE_CHANGED_UNREAD_COUNT", "unread-count" },
	  { TNY_FOLDER_CHANGE_CHANGED_ADDED_HEADERS, "TNY_FOLDER_CHANGE_CHANGED_ADDED_HEADERS","added-headers" },
	  { TNY_FOLDER_CHANGE_CHANGED_EXPUNGED_HEADERS, "TNY_FOLDER_CHANGE_CHANGED_EXPUNGED_HEADERS", "expunged-headers" },
	  { TNY_FOLDER_CHANGE_CHANGED_FOLDER_RENAME, "TNY_FOLDER_CHANGE_CHANGED_FOLDER_RENAME", "rename" },
	  { TNY_FOLDER_CHANGE_CHANGED_MSG_RECEIVED, "TNY_FOLDER_CHANGE_CHANGED_MSG_RECEIVED", "received" },
	  { 0, NULL, NULL }
  };
  etype = g_flags_register_static ("TnyFolderChangeChanged", values);
  return GSIZE_TO_POINTER (etype);
}

/**
 * tny_folder_change_changed_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType
tny_folder_change_changed_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_folder_change_changed_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
