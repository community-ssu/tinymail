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
 * TnyFolder:
 * 
 * A folder, or a mailbox depending on what you prefer to call it
 *
 * free-function: g_object_unref
 */

#include <config.h>

#ifdef DBC
#include <string.h>
#endif

#include <tny-folder-store.h>
#include <tny-header.h>
#include <tny-list.h>
#include <tny-folder-stats.h>
#include <tny-msg-receive-strategy.h>
#include <tny-folder-observer.h>

#include <tny-folder.h>
guint tny_folder_signals [TNY_FOLDER_LAST_SIGNAL];



/**
 * tny_folder_get_caps:
 * @self: a #TnyFolder
 * 
 * Get a few relevant capabilities of @self.
 * 
 * returns: The capabilities of the folder
 * since: 1.0
 * audience: application-developer
 **/
TnyFolderCaps 
tny_folder_get_caps (TnyFolder *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_caps!= NULL);
#endif

	return TNY_FOLDER_GET_IFACE (self)->get_caps(self);
}

/**
 * tny_folder_get_url_string:
 * @self: a #TnyFolder
 * 
 * Get the url_string of @self or NULL if it's impossible to determine the url 
 * string of @self. If not NULL, the returned value must be freed after use.
 *
 * The url string is specified in RFC 1808 and / or RFC 4467 and looks like 
 * this: imap://server/INBOX/folder. Note that it doesn't necessarily contain 
 * the password of the IMAP account but can contain it.
 * 
 * returns: (null-ok) (caller-owns): The url string or NULL.
 * since: 1.0
 * audience: application-developer
 **/
gchar* 
tny_folder_get_url_string (TnyFolder *self)
{
	gchar *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_url_string!= NULL);
#endif

	retval = TNY_FOLDER_GET_IFACE (self)->get_url_string(self);

#ifdef DBC /* ensure */
	if (retval) {
		g_assert (strlen (retval) > 0);
		g_assert (strstr (retval, ":/") != NULL);
	}
#endif

	return retval;
}

/**
 * tny_folder_get_stats:
 * @self: a #TnyFolder
 *
 * Get some statistics of the folder @self. The returned statistics object will
 * not change after you got it. If you need an updated version, you need to call
 * this method again. You must unreference the return value after use.
 *
 * This method is a rather heavy operation that might consume a lot of memory
 * to achieve its return value. There are more finegrained APIs available in
 * #TnyFolder that will get you the same counts in a less expensive way. With
 * this method you get a combined statistic, however.
 *
 * returns: (caller-owns): some statistics of the folder
 * since: 1.0
 * audience: application-developer
 **/
TnyFolderStats* 
tny_folder_get_stats (TnyFolder *self)
{
	TnyFolderStats *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_stats!= NULL);
#endif

	retval = TNY_FOLDER_GET_IFACE (self)->get_stats(self);

#ifdef DBC /* ensure */
#endif

	return retval;
}

/**
 * tny_folder_add_observer:
 * @self: a #TnyFolder
 * @observer: a #TnyFolderObserver
 *
 * Add @observer to the list of event observers. Among the events that could
 * happen are caused by for example a tny_folder_poke_status() and spontaneous
 * change notifications like Push E-mail events.
 *
 * A weak reference is added to @observer. When @observer finalises will @self
 * automatically update itself by unregistering @observer. It's recommended to
 * use tny_folder_remove_observer() yourself to unregister @observer as observer
 * of @self.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_add_observer (TnyFolder *self, TnyFolderObserver *observer)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (observer);
	g_assert (TNY_IS_FOLDER_OBSERVER (observer));
	g_assert (TNY_FOLDER_GET_IFACE (self)->add_observer!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->add_observer(self, observer);
	return;
}


/**
 * tny_folder_remove_observer:
 * @self: a #TnyFolder
 * @observer: a #TnyFolderObserver
 *
 * Remove @observer from the list of event observers of @self.
 *
 * The weak reference added by tny_folder_add_observer() to @observer, is 
 * removed.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_remove_observer (TnyFolder *self, TnyFolderObserver *observer)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (observer);
	g_assert (TNY_IS_FOLDER_OBSERVER (observer));
	g_assert (TNY_FOLDER_GET_IFACE (self)->remove_observer!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->remove_observer(self, observer);
	return;

}

/**
 * tny_folder_poke_status:
 * @self: a #TnyFolder
 *
 * Poke for the status, this will invoke an event to the event observers of 
 * @self. The change will always at least contain the unread and total folder
 * count, also if the unread and total didn't change at all. This makes it
 * possible for a model or view that shows folders to get the initial folder 
 * unread and total counts (the #TnyGtkFolderStoreListModel uses this method 
 * internally, for example).
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_poke_status (TnyFolder *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->poke_status!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->poke_status(self);
	return;
}


/**
 * tny_folder_get_msg_receive_strategy:
 * @self: a #TnyFolder
 *
 * Get the strategy for receiving a message. The return value of this method
 * must be unreferenced after use.
 *
 * returns: (caller-owns): the strategy for receiving a message
 * since: 1.0
 * audience: application-developer
 **/
TnyMsgReceiveStrategy* 
tny_folder_get_msg_receive_strategy (TnyFolder *self)
{
	TnyMsgReceiveStrategy *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_msg_receive_strategy!= NULL);
#endif

	retval = TNY_FOLDER_GET_IFACE (self)->get_msg_receive_strategy(self);

#ifdef DBC /* ensure */
	g_assert (TNY_IS_MSG_RECEIVE_STRATEGY (retval));
#endif

	return retval;
}

/**
 * tny_folder_set_msg_receive_strategy:
 * @self: a #TnyFolder
 * @st: a #TnyMsgReceiveStrategy
 *
 * Set the strategy for receiving a message. Some common strategies are:
 *
 * - #TnyCamelPartialMsgReceiveStrategy for a strategy that tries to only
 * receive the text parts of an E-mail.
 * - #TnyCamelFullMsgReceiveStrategy (current default) for a strategy that 
 * requests the entire message
 * - #TnyCamelBsMsgReceiveStrategy for a strategy that receives a BODYSTRUCTURE
 * and then per MIME part that is needed, requests the needed MIME part
 *
 * For more information take a look at tny_msg_receive_strategy_peform_get_msg()
 * of #TnyMsgReceiveStrategy.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_set_msg_receive_strategy (TnyFolder *self, TnyMsgReceiveStrategy *st)
{
#ifdef DBC /* require */
	TnyMsgReceiveStrategy *test;
	g_assert (TNY_IS_FOLDER (self));
	g_assert (st);
	g_assert (TNY_IS_MSG_RECEIVE_STRATEGY (st));
	g_assert (TNY_FOLDER_GET_IFACE (self)->set_msg_receive_strategy!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->set_msg_receive_strategy(self, st);

#ifdef DBC /* ensure */
	test = tny_folder_get_msg_receive_strategy (self);
	g_assert (test);
	g_assert (TNY_IS_MSG_RECEIVE_STRATEGY (test));
	g_assert (test == st);
	g_object_unref (G_OBJECT (test));
#endif

	return;
}



/**
 * tny_folder_copy:
 * @self: a #TnyFolder
 * @into: a #TnyFolderStore
 * @new_name: the new name in @into
 * @del: whether or not to delete the original location
 * @err: (null-ok): a #GError object or NULL
 *
 * Copies @self to @into giving the new folder the name @new_name. Returns the
 * newly created folder in @into, which will carry the name @new_name. For some
 * remote services this functionality is implemented as a rename operation.
 *
 * It will do both copying and in case of @del being TRUE, removing too, 
 * recursively. If you don't want it to be recursive, you should in stead create
 * the folder @new_name in @into manually using tny_folder_store_create_folder()
 * and then use the tny_folder_transfer_msgs() API.
 *
 * This method will  result in create events being thrown to the observers of
 * each subfolder of @self that is copied or moved to @new_name in creation
 * order (parents first, childs after that). 
 *
 * In case of @del being TRUE it will on top of that and before those create
 * events also result in remove events being thrown to the observers of each
 * subfolder of @self in deletion order (the exact reversed order: childs 
 * first, parents after that)
 *
 * Standard folder models like the #TnyGtkFolderStoreListModel know about this
 * behavior and will act by updating themselves automatically.
 * 
 * When you are moving @self to @into with @del set to TRUE (moving @self), you
 * must make sure that @self nor any of its sub folders are used anymore. For
 * example if you have gotten headers using tny_folder_get_headers(), you must
 * get rid of those first. In case you used a default component like the 
 * #TnyGtkHeaderListModel or any other #TnyList for storing the headers, you can 
 * easily get rid if your headers by setting the model of the GtkTreeView to
 * an empty one or unreferencing the list.
 * 
 * If @new_name already exists in @into and @err is not NULL, then an error will
 * be set in @err and no further action will be performed.
 * 
 * Deprecated: 1.0: Use tny_folder_copy_async in stead
 * returns: (null-ok) (caller-owns): a new folder instance to whom was copied or NULL
 * since: 1.0
 * audience: application-developer
 **/
TnyFolder* 
tny_folder_copy (TnyFolder *self, TnyFolderStore *into, const gchar *new_name, gboolean del, GError **err)
{
	TnyFolder *retval;

#ifdef DBC /* require */
	TnyFolderStore *test;
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_IS_FOLDER_STORE (into));
	g_assert (new_name);
	g_assert (strlen (new_name) > 0);
	g_assert (TNY_FOLDER_GET_IFACE (self)->copy!= NULL);
#endif

	retval = TNY_FOLDER_GET_IFACE (self)->copy(self, into, new_name, del, err);

#ifdef DBC /* ensure */
	if (retval) {
		g_assert (!strcmp (tny_folder_get_name (retval), new_name));
		test = tny_folder_get_folder_store (retval);
		g_assert (test);
		g_assert (TNY_IS_FOLDER_STORE (test));
		g_assert (test == into);
		g_object_unref (G_OBJECT (test));
	}
#endif

	return retval;
}


/** 
 * TnyCopyFolderCallback:
 * @self: a #TnyFolder that caused the callback
 * @cancelled: if the operation got cancelled
 * @into: (null-ok): where @self got copied to
 * @new_folder: (null-ok): the new folder in @into
 * @err: (null-ok): if an error occurred
 * @user_data: (null-ok):  user data that was passed to the callbacks
 *
 * A folder callback for when a copy of a folder was requested. If allocated,
 * you must cleanup @user_data at the end of your implementation of the callback.
 * All other fields in the parameters of the callback are read-only.
 *
 * When cancelled, @cancelled will be TRUE while @err might nonetheless be NULL.
 * If @err is not NULL, an error occurred that you should handle gracefully.
 * The parameter @into is the #TnyFolderStore where you copied @self to. The
 * parameter @new_folder is the folder instance in @into that got created by
 * copying @self into it.
 *
 * Note that you should not use @self without care. It's possible that when you
 * requested tny_folder_copy_async(), you specified to delete the original folder.
 * Therefore will @self point to an instance that is about to be removed as an
 * instance locally, and of which the remote folder has already been removed in
 * case the copy was indeed successful and the request included deleting the
 * original. In such a case is @self as a folder not to be trusted any longer.
 * In this case wont @self be NULL, but any operation that you'll do on it will
 * yield unexpected and more importantly unspecified results. Please do not 
 * ignore this warning.
 *
 * since: 1.0
 * audience: application-developer
 **/


/**
 * tny_folder_copy_async:
 * @self: a #TnyFolder
 * @into: a #TnyFolderStore
 * @new_name: the new name in @into
 * @del: whether or not to delete the original location
 * @callback: (null-ok): a #TnyCopyFolderCallback or NULL
 * @status_callback: (null-ok): a #TnyStatusCallback or NULL
 * @user_data: (null-ok): user data that will be passed to the callbacks
 *
 * See tny_folder_copy(). This is the asynchronous version of the same function.
 *
 * Example:
 * <informalexample><programlisting>
 * static void
 * status_update_cb (GObject *sender, TnyStatus *status, gpointer user_data)
 * {
 *     g_print (".");
 * }
 * static void
 * folder_copy_cb (TnyFolder *folder, TnyFolderStore *into, const gchar *new_name, gboolean cancelled, GError **err, gpointer user_data)
 * {
 *     if (!cancelled) {
 *         TnyList *headers = tny_simple_list_new ();
 *         TnyIterator *iter;
 *         g_print ("done\nHeaders copied into %s are:", 
 *                tny_folder_get_name (into));
 *         tny_folder_get_headers (into, headers, FALSE);
 *         iter = tny_list_create_iterator (headers);
 *         while (!tny_iterator_is_done (iter))
 *         {
 *             TnyHeader *header = tny_iterator_current (iter);
 *             g_print ("\t%s\n", tny_header_get_subject (header));
 *             g_object_unref (header);
 *             tny_iterator_next (iter);
 *         }
 *         g_object_unref (iter);
 *         g_object_unref (headers);
 *     }
 * }
 * TnyFolder *folder = ...
 * TnyFolderStore *into = ...
 * gchar *new_name = ...
 * g_print ("Getting headers ");
 * tny_folder_copy_async (folder, into, new_name, 
 *          folder_copy_cb, 
 *          status_update_cb, NULL); 
 * </programlisting></informalexample>
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_copy_async (TnyFolder *self, TnyFolderStore *into, const gchar *new_name, gboolean del, TnyCopyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{

#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_IS_FOLDER_STORE (into));
	g_assert (new_name);
	g_assert (strlen (new_name) > 0);
	g_assert (TNY_FOLDER_GET_IFACE (self)->copy_async!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->copy_async(self, into, new_name, del, callback, status_callback, user_data);

#ifdef DBC /* ensure */
#endif

	return;
}

/**
 * tny_folder_get_msg_remove_strategy:
 * @self: a #TnyFolder
 *
 * Get the strategy for removing a message. The return value of this method
 * must be unreferenced after use.
 *
 * returns: (caller-owns): the strategy for removing a message
 * since: 1.0
 * audience: application-developer
 **/
TnyMsgRemoveStrategy* 
tny_folder_get_msg_remove_strategy (TnyFolder *self)
{
	TnyMsgRemoveStrategy *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_msg_remove_strategy!= NULL);
#endif

	retval = TNY_FOLDER_GET_IFACE (self)->get_msg_remove_strategy(self);

#ifdef DBC /* ensure */
	g_assert (TNY_IS_MSG_REMOVE_STRATEGY (retval));
#endif

	return retval;
}

/**
 * tny_folder_set_msg_remove_strategy:
 * @self: a #TnyFolder
 * @st: a #TnyMsgRemoveStrategy
 *
 * Set the strategy for removing a message
 *
 * For more information take a look at tny_msg_remove_strategy_peform_remove()
 * of #TnyMsgRemoveStrategy.
 * 
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_set_msg_remove_strategy (TnyFolder *self, TnyMsgRemoveStrategy *st)
{
#ifdef DBC /* require */
	TnyMsgRemoveStrategy *test;
	g_assert (TNY_IS_FOLDER (self));
	g_assert (st);
	g_assert (TNY_IS_MSG_REMOVE_STRATEGY (st));
	g_assert (TNY_FOLDER_GET_IFACE (self)->set_msg_remove_strategy!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->set_msg_remove_strategy(self, st);

#ifdef DBC /* ensure */
	test = tny_folder_get_msg_remove_strategy (self);
	g_assert (test);
	g_assert (TNY_IS_MSG_REMOVE_STRATEGY (test));
	g_assert (test == st);
	g_object_unref (G_OBJECT (test));
#endif

	return;
}

/**
 * tny_folder_sync:
 * @self: a #TnyFolder
 * @expunge: also expunge deleted messages
 * @err: (null-ok): a #GError or NULL
 *
 * Persist changes made to a folder to its backing store, expunging deleted 
 * messages (the ones marked with TNY_HEADER_FLAG_DELETED) as well if @expunge
 * is TRUE.
 *
 * Example:
 * <informalexample><programlisting>
 * TnyHeader *header = ...
 * TnyFolder *folder = tny_header_get_folder (header);
 * tny_folder_remove_msg (folder, header, NULL);
 * tny_list_remove (TNY_LIST (mymodel), G_OBJECT (header));
 * g_object_unref (header);
 * tny_folder_sync (folder, TRUE, NULL);
 * g_object_unref (folder);
 * </programlisting></informalexample>
 *
 * Deprecated: 1.0: Use tny_folder_sync_async in stead
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_sync (TnyFolder *self, gboolean expunge, GError **err)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->sync!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->sync(self, expunge, err);
	return;
}


/**
 * tny_folder_sync_async:
 * @self: a #TnyFolder
 * @expunge: also expunge deleted messages
 * @callback: (null-ok): a #TnySyncFolderCallback or NULL
 * @status_callback: (null-ok): a #TnyStatusCallback or NULL
 * @user_data: (null-ok): user data that will be passed to the callbacks
 *
 * The authors of Tinymail know that sync async sounds paradoxical. Though if
 * you think about it, it makes perfect sense: you synchronize the content in
 * an asynchronous way. Nothing paradoxical about it, right?
 *
 * Persist changes made to a folder to its backing store, expunging deleted 
 * messages (the ones marked with TNY_HEADER_FLAG_DELETED) as well if @expunge
 * is TRUE.
 *
 * This is a cancelable operation which means that if another cancelable 
 * operation executes, this operation will be aborted. Being aborted means that
 * the callback will still be called, but with cancelled=TRUE.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_sync_async (TnyFolder *self, gboolean expunge, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->sync_async!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->sync_async(self, expunge, callback, status_callback, user_data);
	return;
}


/** 
 * TnyFolderCallback:
 * @self: a #TnyFolder that caused the callback
 * @cancelled: if the operation got cancelled
 * @err: (null-ok): if an error occurred
 * @user_data: (null-ok):  user data that was passed to the callbacks
 *
 * A generic folder callback. If allocated, you must cleanup @user_data at the
 * end of your implementation of the callback. All other fields in the parameters
 * of the callback are read-only.
 *
 * since: 1.0
 * audience: application-developer
 **/

/**
 * tny_folder_add_msg_async:
 * @self: a #TnyFolder
 * @msg: a #TnyMsg
 * @callback: (null-ok): a #TnyFolderCallback or NULL
 * @status_callback: (null-ok): a #TnyStatusCallback or NULL
 * @user_data: (null-ok): user data that will be passed to the callbacks
 *
 * Add a message to a @self. It's recommended to destroy @msg afterwards because 
 * after receiving the same message from the folder again, the instance wont be
 * the same and a property like the tny_msg_get_id() will have changed.
 * 
 * Folder observers of @self will get a header-added event caused by this
 * action.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_add_msg_async (TnyFolder *self, TnyMsg *msg, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (msg);
	g_assert (TNY_IS_MSG (msg));
	g_assert (TNY_FOLDER_GET_IFACE (self)->add_msg_async!= NULL);
#endif
	TNY_FOLDER_GET_IFACE (self)->add_msg_async(self, msg, callback, status_callback, user_data);
	return;
}


/**
 * tny_folder_add_msg:
 * @self: a #TnyFolder
 * @msg: a #TnyMsg
 * @err: (null-ok): a #GError or NULL
 *
 * Add a message to a @self. It's recommended to destroy @msg afterwards because 
 * after receiving the same message from the folder again, the instance wont be
 * the same and a property like the tny_msg_get_id() will have changed.
 * 
 * Folder observers of @self will get a header-added event caused by this
 * action.
 *
 * Deprecated: 1.0: Use tny_folder_add_msg_async in stead
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_add_msg (TnyFolder *self, TnyMsg *msg, GError **err)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (msg);
	g_assert (TNY_IS_MSG (msg));
	g_assert (TNY_FOLDER_GET_IFACE (self)->add_msg!= NULL);
#endif
	TNY_FOLDER_GET_IFACE (self)->add_msg(self, msg, err);
	return;
}

/**
 * tny_folder_remove_msg:
 * @self: a #TnyFolder
 * @header: a #TnyHeader of the message to remove
 * @err: (null-ok): a #GError or NULL
 *
 * Remove a message from a folder. It will use the #TnyMsgRemoveStrategy of 
 * @self to perform the removal itself. For more details, check out the 
 * documentation of the used #TnyMsgRemoveStrategy type. The default 
 * implementation for libtinymail-camel is the #TnyCamelMsgRemoveStrategy.
 *
 * Folder observers of @self will get a header-removed event caused by this
 * action.
 *
 * Example:
 * <informalexample><programlisting>
 * static void
 * on_header_view_key_press_event (GtkTreeView *header_view, GdkEventKey *event, gpointer user_data)
 * {
 *   if (event->keyval == GDK_Delete) {
 *       GtkTreeSelection *selection;
 *       GtkTreeModel *model;
 *       GtkTreeIter iter;
 *       selection = gtk_tree_view_get_selection (header_view);
 *       if (gtk_tree_selection_get_selected (selection, &amp;model, &amp;iter))
 *       {
 *          TnyHeader *header;
 *          gtk_tree_model_get (model, &amp;iter, 
 *                TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, 
 *                &amp;header, -1);
 *          TnyFolder *folder = tny_header_get_folder (header);
 *          tny_folder_remove_msg (folder, header, NULL);
 *          tny_list_remove (TNY_LIST (model), G_OBJECT (header));
 *          g_object_unref (folder);
 *          g_object_unref (header);
 *       }
 *   }
 * }
 * </programlisting></informalexample>
 *
 * Deprecated: 1.0: Use tny_folder_remove_msgs_async in stead
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_remove_msg (TnyFolder *self, TnyHeader *header, GError **err)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (header);
	g_assert (TNY_IS_HEADER (header));
	g_assert (TNY_FOLDER_GET_IFACE (self)->remove_msg!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->remove_msg(self, header, err);
	return;
}

/**
 * tny_folder_remove_msgs:
 * @self: a #TnyFolder
 * @headers: a #TnyList with #TnyHeader items of the messages to remove
 * @err: (null-ok): a #GError or NULL
 *
 * Remove messages from a folder. It will use the #TnyMsgRemoveStrategy of 
 * @self to perform the removal itself. For more details, check out the 
 * documentation of the used #TnyMsgRemoveStrategy type. The default 
 * implementation for libtinymail-camel is the #TnyCamelMsgRemoveStrategy.
 *
 * Folder observers of @self will get only one header-removed trigger caused
 * by this action (with all the removed headers in the #TnyFolderChange).
 *
 * Deprecated: 1.0: Use tny_folder_remove_msgs_async in stead
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_remove_msgs (TnyFolder *self, TnyList *headers, GError **err)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (headers);
	g_assert (TNY_IS_LIST (headers));
	g_assert (TNY_FOLDER_GET_IFACE (self)->remove_msgs!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->remove_msgs(self, headers, err);
	return;
}



/**
 * tny_folder_remove_msgs_async:
 * @self: a #TnyFolder
 * @headers: a #TnyList with #TnyHeader items of the messages to remove
 * @callback: (null-ok): a #TnyFolderCallback or NULL
 * @status_callback: (null-ok): a #TnyStatusCallback or NULL
 * @user_data: (null-ok): user data that will be passed to the callbacks
 *
 * Remove messages from a folder. It will use the #TnyMsgRemoveStrategy of 
 * @self to perform the removal itself. For more details, check out the 
 * documentation of the used #TnyMsgRemoveStrategy type. The default 
 * implementation for libtinymail-camel is the #TnyCamelMsgRemoveStrategy.
 *
 * Folder observers of @self will get only one header-removed trigger caused
 * by this action (with all the removed headers in the #TnyFolderChange).
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_remove_msgs_async (TnyFolder *self, TnyList *headers, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (headers);
	g_assert (TNY_IS_LIST (headers));
	g_assert (TNY_FOLDER_GET_IFACE (self)->remove_msgs_async!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->remove_msgs_async(self, headers, callback, status_callback, user_data);
	return;
}

/**
 * tny_folder_refresh_async:
 * @self: a #TnyFolder
 * @callback: (null-ok): a #TnyFolderCallback or NULL
 * @status_callback: (null-ok): a #TnyStatusCallback or NULL
 * @user_data: (null-ok): user data that will be passed to the callbacks
 *
 * Refresh @self and callback when finished. This gets the summary information
 * from the E-Mail service, writes it to the the on-disk cache and/or updates it.
 *
 * After this method, when @callback takes place, tny_folder_get_all_count()
 * and tny_folder_get_unread_count() are guaranteed to be correct. The API
 * tny_folder_get_headers() is guaranteed to put an accurate list of headers
 * in the provided #TnyList.
 *
 * While the refresh takes place, it's possible that event observers of @self
 * will get notified of new header additions. #TnyFolderMonitor copes with those
 * by publishing them to #TnyList instances, like a #TnyGtkHeaderListModel.
 *
 * status_callback() will be called during this operation. It is guaranteed that the refresh 
 * messages happen but it's not guaranteed that only refresh messages happen.
 * 
 * Example:
 * <informalexample><programlisting>
 * static void
 * status_update_cb (GObject *sender, TnyStatus *status, gpointer user_data)
 * {
 *     g_print (".");
 * }
 * static void
 * folder_refresh_cb (TnyFolder *folder, gboolean cancelled, GError **err, gpointer user_data)
 * {
 *     if (!cancelled)
 *     {
 *         TnyList *headers = tny_simple_list_new ();
 *         TnyIterator *iter;
 *         g_print ("done\nHeaders for %s are:", 
 *                tny_folder_get_name (folder));
 *         tny_folder_get_headers (folder, headers, FALSE);
 *         iter = tny_list_create_iterator (headers);
 *         while (!tny_iterator_is_done (iter))
 *         {
 *             TnyHeader *header = tny_iterator_current (iter);
 *             g_print ("\t%s\n", tny_header_get_subject (header));
 *             g_object_unref (header);
 *             tny_iterator_next (iter);
 *         }
 *         g_object_unref (iter);
 *         g_object_unref (headers);
 *     }
 * }
 * TnyFolder *folder = ...
 * g_print ("Getting headers ");
 * tny_folder_refresh_async (folder, 
 *          folder_refresh_cb, 
 *          status_update_cb, NULL); 
 * </programlisting></informalexample>
 *
 * This is a cancelable operation which means that if another cancelable 
 * operation executes, this operation will be aborted. Being aborted means that
 * the callback will still be called, but with cancelled=TRUE.
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_folder_refresh_async (TnyFolder *self, TnyFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->refresh_async!= NULL);
#endif


	TNY_FOLDER_GET_IFACE (self)->refresh_async(self, callback, status_callback, user_data);
	return;
}



/**
 * tny_folder_refresh:
 * @self: a #TnyFolder
 * @err: (null-ok): a #GError or NULL
 *
 * Refresh @self. This gets the summary information from the E-Mail service, 
 * writes it to the the on-disk cache and/or updates it.
 *
 * After this method tny_folder_get_all_count() and tny_folder_get_unread_count()
 * are guaranteed to be correct. The API tny_folder_get_headers() is guaranteed to
 * put an accurate list of headers in the provided #TnyList.
 *
 * While the refresh takes place, it's possible that event observers of @self
 * will get notified of new header additions. #TnyFolderMonitor copes with those
 * by publishing them to #TnyList instances, like a #TnyGtkHeaderListModel.
 *
 * This function can take a very long time. You are advised to consider using
 * tny_folder_refresh_async() in stead.
 *
 * Deprecated: 1.0: Use tny_folder_refresh_async in stead
 * since: 1.0
 * audience: application-developer
 **/
void
tny_folder_refresh (TnyFolder *self, GError **err)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->refresh!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->refresh(self, err);
	return;
}


/**
 * tny_folder_is_subscribed:
 * @self: a #TnyFolder
 * 
 * Get the subscription status of @self.
 * 
 * returns: subscription state
 * since: 1.0
 * audience: application-developer
 **/
gboolean
tny_folder_is_subscribed (TnyFolder *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->is_subscribed!= NULL);
#endif

	return TNY_FOLDER_GET_IFACE (self)->is_subscribed(self);
}

/**
 * tny_folder_get_unread_count:
 * @self: a #TnyFolder
 * 
 * Get the amount of unread messages in this folder. The value is only
 * guaranteed to be correct after tny_folder_refresh() or during the
 * callback of a tny_folder_refresh_async().
 * 
 * returns: amount of unread messages
 * since: 1.0
 * audience: application-developer
 **/
guint
tny_folder_get_unread_count (TnyFolder *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_unread_count!= NULL);
#endif

	return TNY_FOLDER_GET_IFACE (self)->get_unread_count(self);
}


/**
 * tny_folder_get_local_size:
 * @self: a #TnyFolder
 * 
 * Get the local size of this folder in bytes. Local size means the amount of
 * disk space that this folder consumes locally in the caches.
 * 
 * returns: get the local size of this folder
 * since: 1.0
 * audience: application-developer
 **/
guint
tny_folder_get_local_size (TnyFolder *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_local_size!= NULL);
#endif

	return TNY_FOLDER_GET_IFACE (self)->get_local_size(self);
}

/**
 * tny_folder_get_all_count:
 * @self: a #TnyFolder
 * 
 * Get the amount of messages in this folder. The value is only guaranteed to
 * be correct after tny_folder_refresh() or during the callback of a 
 * tny_folder_refresh_async().
 * 
 * returns: amount of messages
 * since: 1.0
 * audience: application-developer
 **/
guint
tny_folder_get_all_count (TnyFolder *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_all_count!= NULL);
#endif

	return TNY_FOLDER_GET_IFACE (self)->get_all_count(self);
}

/**
 * tny_folder_get_account:
 * @self: a #TnyFolder
 * 
 * Get a the account of this folder or NULL. If not NULL, you must unreference
 * the return value after use.
 * 
 * returns: (null-ok) (caller-owns): the account of this folder or NULL
 * since: 1.0
 * audience: application-developer
 **/
TnyAccount* 
tny_folder_get_account (TnyFolder *self)
{
	TnyAccount *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_account!= NULL);
#endif

	retval = TNY_FOLDER_GET_IFACE (self)->get_account(self);


#ifdef DBC /* ensure */
	if (retval)
		g_assert (TNY_IS_ACCOUNT (retval));
#endif

	return retval;
}

/**
 * tny_folder_transfer_msgs:
 * @self: a #TnyFolder
 * @header_list: a #TnyList of #TnyHeader instances in @self to move
 * @folder_dst: a destination #TnyFolder
 * @delete_originals: TRUE moves msgs, FALSE copies
 * @err: (null-ok): a #GError or NULL
 * 
 * Transfers messages of which the headers are in @header_list from @self to 
 * @folder_dst. They could be moved or just copied depending on the value of 
 * the @delete_originals argument.
 *
 * You must not use header instances that you got from tny_msg_get_header()
 * with this API. You must only use instances that you got from 
 * tny_folder_get_headers().
 *
 * Deprecated: 1.0: Use tny_folder_transfer_msgs_async in stead
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_transfer_msgs (TnyFolder *self, TnyList *headers, TnyFolder *folder_dst, gboolean delete_originals, GError **err)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (headers);
	g_assert (TNY_IS_LIST (headers));
	g_assert (folder_dst);
	g_assert  (TNY_IS_FOLDER (folder_dst));
	g_assert (TNY_FOLDER_GET_IFACE (self)->transfer_msgs!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->transfer_msgs(self, headers, folder_dst, delete_originals, err);
	return;
}

/**
 * tny_folder_transfer_msgs_async:
 * @self: a #TnyFolder
 * @header_list: a #TnyList of #TnyHeader instances in @self to move
 * @folder_dst: a destination #TnyFolder
 * @delete_originals: TRUE moves msgs, FALSE copies
 * @callback: (null-ok): a #TnyTransferMsgsCallback or NULL
 * @status_callback: (null-ok): a #TnyStatusCallback or NULL
 * @user_data: (null-ok): user data that will be passed to the callbacks
 * 
 * Transfers messages of which the headers are in @header_list from @self to 
 * @folder_dst. They could be moved or just copied depending on the value of 
 * the @delete_originals argument.
 *
 * You must not use header instances that you got from tny_msg_get_header()
 * with this API. You must only use instances that you got from 
 * tny_folder_get_headers().
 *
 * This is a cancelable operation which means that if another cancelable 
 * operation executes, this operation will be aborted. Being aborted means that
 * the callback will still be called, but with cancelled=TRUE.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_transfer_msgs_async (TnyFolder *self, TnyList *header_list, TnyFolder *folder_dst, gboolean delete_originals, TnyTransferMsgsCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (header_list);
	g_assert (TNY_IS_LIST (header_list));
	g_assert (folder_dst);
	g_assert (TNY_IS_FOLDER (folder_dst));
	g_assert (TNY_FOLDER_GET_IFACE (self)->transfer_msgs_async!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->transfer_msgs_async(self, header_list, folder_dst, delete_originals, callback, status_callback, user_data);
	return;
}


/**
 * tny_folder_get_msg:
 * @self: a #TnyFolder
 * @header: a #TnyHeader the message to get
 * @err: (null-ok): a #GError or NULL
 * 
 * Get a message in @self identified by @header. If not NULL, you must 
 * unreference the return value after use.
 * 
 * Example:
 * <informalexample><programlisting>
 * TnyMsgView *message_view = tny_platform_factory_new_msg_view (platfact);
 * TnyFolder *folder = ...
 * TnyHeader *header = ...
 * TnyMsg *message = tny_folder_get_msg (folder, header, NULL);
 * tny_msg_view_set_msg (message_view, message);
 * g_object_unref (message);
 * </programlisting></informalexample>
 *
 * Note that once you received the message this way, the entire message instance
 * is detached from @self. This means that if you get a new header using the
 * tny_msg_get_header() API, that this header can't be used with folder API like
 * tny_folder_transfer_msgs() and tny_folder_transfer_msgs_async(). You can only
 * use header instances that you got with tny_folder_get_headers() for this and
 * other #TnyFolder methods. The message's instance nor its header will receive 
 * updates from the observable folder.
 * 
 * Deprecated: 1.0: Use tny_folder_get_msg_async in stead
 * returns: (null-ok) (caller-owns): The message instance or NULL
 * since: 1.0
 * audience: application-developer
 **/
TnyMsg*
tny_folder_get_msg (TnyFolder *self, TnyHeader *header, GError **err)
{
	TnyMsg *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (header);
	g_assert (TNY_IS_HEADER (header));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_msg!= NULL);
#endif

	retval = TNY_FOLDER_GET_IFACE (self)->get_msg(self, header, err);

#ifdef DBC /* ensure */
	if (retval)
		g_assert (TNY_IS_MSG (retval));
#endif

	return retval;
}



/**
 * tny_folder_find_msg:
 * @self: a #TnyFolder
 * @url_string: the url string
 * @err: (null-ok): a #GError or NULL
 * 
 * Get the message in @self identified by @url_string. If not NULL, you must 
 * unreference the return value after use. See tny_folder_get_url_string() for
 * details of the @url-string syntax.
 * 
 * Example:
 * <informalexample><programlisting>
 * TnyMsgView *message_view = tny_platform_factory_new_msg_view (platfact);
 * TnyFolder *folder = ...
 * TnyMsg *message = tny_folder_find_msg (folder, "imap://account/INBOX/100", NULL);
 * tny_msg_view_set_msg (message_view, message);
 * g_object_unref (message);
 * </programlisting></informalexample>
 *
 * Deprecated: 1.0: Use tny_folder_find_msg_async in stead
 * returns: (null-ok) (caller-owns): The message instance or NULL
 * since: 1.0
 * audience: application-developer
 **/
TnyMsg*
tny_folder_find_msg (TnyFolder *self, const gchar *url_string, GError **err)
{
	TnyMsg *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (url_string);
	g_assert (strlen (url_string) > 0);
	g_assert (strstr (url_string, ":") != NULL);
	g_assert (TNY_FOLDER_GET_IFACE (self)->find_msg!= NULL);
#endif

	retval = TNY_FOLDER_GET_IFACE (self)->find_msg(self, url_string, err);

#ifdef DBC /* ensure */
	if (retval)
		g_assert (TNY_IS_MSG (retval));
#endif

	return retval;
}


/**
 * tny_folder_get_msg_async:
 * @self: a #TnyFolder
 * @header: a #TnyHeader of the message to get
 * @callback: (null-ok): a #TnyGetMsgCallback or NULL
 * @status_callback: (null-ok): a #TnyStatusCallback or NULL
 * @user_data: (null-ok): user data that will be passed to the callbacks
 *
 * Get a message in @self identified by @header asynchronously.
 *
 * Example:
 * <informalexample><programlisting>
 * static void 
 * status_cb (GObject *sender, TnyStatus *status, gpointer user_data)
 * {
 *       printf (".");
 * }
 * static void
 * folder_get_msg_cb (TnyFolder *folder, gboolean cancelled, TnyMsg *msg, GError **err, gpointer user_data)
 * {
 *       TnyMsgView *message_view = user_data;
 *       if (!err && msg && !cancelled)
 *           tny_msg_view_set_msg (message_view, msg);
 * }
 * TnyMsgView *message_view = tny_platform_factory_new_msg_view (platfact);
 * TnyFolder *folder = ...; TnyHeader *header = ...;
 * tny_folder_get_msg_async (folder, header,
 *          folder_get_msg_cb, status_cb, message_view); 
 * </programlisting></informalexample>
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_folder_get_msg_async (TnyFolder *self, TnyHeader *header, TnyGetMsgCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (header);
	g_assert (TNY_IS_HEADER (header));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_msg_async!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->get_msg_async(self, header, callback, status_callback, user_data);

	return;
}

/**
 * tny_folder_find_msg_async:
 * @self: a #TnyFolder
 * @url_string: the url string
 * @callback: (null-ok): a #TnyGetMsgCallback or NULL
 * @status_callback: (null-ok): a #TnyStatusCallback or NULL
 * @user_data: (null-ok): user data that will be passed to the callbacks
 *
 * Get a message in @self identified by @url_string asynchronously.
 *
 * Example:
 * <informalexample><programlisting>
 * static void 
 * status_cb (GObject *sender, TnyStatus *status, gpointer user_data)
 * {
 *       printf (".");
 * }
 * static void
 * folder_find_msg_cb (TnyFolder *folder, gboolean cancelled, TnyMsg *msg, GError **err, gpointer user_data)
 * {
 *       TnyMsgView *message_view = user_data;
 *       if (!err && msg && !cancelled)
 *           tny_msg_view_set_msg (message_view, msg);
 * }
 * TnyMsgView *message_view = tny_platform_factory_new_msg_view (platfact);
 * TnyFolder *folder = ...; gchar *url_string = ...;
 * tny_folder_find_msg_async (folder, msg_url,
 *          folder_get_msg_cb, status_cb, message_view); 
 * </programlisting></informalexample>
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_folder_find_msg_async (TnyFolder *self, const gchar *url_string, TnyGetMsgCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (url_string);
	g_assert (strlen (url_string) > 0);
	g_assert (strstr (url_string, ":") != NULL);
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_msg_async!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->find_msg_async(self, url_string, callback, status_callback, user_data);

	return;
}

/**
 * tny_folder_get_headers:
 * @self: a #TnyFolder 
 * @headers: A #TnyList where the headers will be prepended to
 * @refresh: synchronize with the service first
 * @err: (null-ok): a #GError or NULL
 * 
 * Get the list of message header instances that are in @self. Also read
 * about tny_folder_refresh() and tny_folder_refresh_async().
 *
 * API warning: it's possible that between the @refresh and @err, a pointer to
 * a query object will be placed. This will introduce both an API and ABI 
 * breakage.
 *
 * Example:
 * <informalexample><programlisting>
 * TnyList *headers = tny_simple_list_new ();
 * TnyFolder *folder = ...;
 * TnyIterator *iter; 
 * tny_folder_get_headers (folder, headers, TRUE, NULL);
 * iter = tny_list_create_iterator (headers);
 * while (!tny_iterator_is_done (iter))
 * {
 *     TnyHeader *header = tny_iterator_get_current (iter);
 *     g_print ("%s\n", tny_header_get_subject (header));
 *     g_object_unref (header);
 *     tny_iterator_next (iter);
 * }
 * g_object_unref (iter);
 * g_object_unref (headers); 
 * </programlisting></informalexample>
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_folder_get_headers (TnyFolder *self, TnyList *headers, gboolean refresh, GError **err)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (headers);
	g_assert (TNY_IS_LIST (headers));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_headers!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->get_headers(self, headers, refresh, err);
	return;
}

/** 
 * TnyGetHeadersCallback:
 * @self: a #TnyFolder that caused the callback
 * @cancelled: if the operation got cancelled
 * @headers: (null-ok): a #TnyList with fetched #TnyHeader instances or NULL
 * @err: (null-ok): if an error occurred
 * @user_data: (null-ok):  user data that was passed to the callbacks
 *
 * A folder callback for when headers where requested. If allocated, you must
 * cleanup @user_data at the end of your implementation of the callback. All
 * other fields in the parameters of the callback are read-only.
 *
 * When cancelled, @cancelled will be TRUE while @err might nonetheless be NULL.
 * If @err is not NULL, an error occurred that you should handle gracefully.
 * The @headers parameter might be NULL in case of error or cancellation.
 *
 * since: 1.0
 * audience: application-developer
 **/

/**
 * tny_folder_get_headers_async:
 * @self: a TnyFolder
 * @headers: A #TnyList where the headers will be prepended to
 * @refresh: synchronize with the service first
 * @callback: (null-ok): a #TnyGetHeadersCallback or NULL
 * @status_callback: (null-ok): a #TnyStatusCallback or NULL
 * @user_data: (null-ok): user data that will be passed to the callbacks
 * 
 * Get the list of message header instances that are in @self. Also read
 * about tny_folder_refresh() and tny_folder_refresh_async().
 *
 * API warning: it's possible that between the @refresh and @callback, a pointer
 * to a query object will be placed. This will introduce both an API and ABI 
 * breakage.
 *
 * Only one instance of @tny_folder_get_headers_async for folder @self can run
 * at the same time. If you call for another, the first will be aborted. This
 * means that it's callback will be called with cancelled=TRUE.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_folder_get_headers_async (TnyFolder *self, TnyList *headers, gboolean refresh, TnyGetHeadersCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (headers);
	g_assert (TNY_IS_LIST (headers));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_headers_async!= NULL);
#endif

	TNY_FOLDER_GET_IFACE (self)->get_headers_async(self, headers, refresh, callback, status_callback, user_data);
	return;
}


/**
 * tny_folder_get_id:
 * @self: a TnyFolder
 * 
 * Get an unique id of @self (unique per account). The id will usually be a  
 * string separated with slashes or backslashes like (but not guaranteed)
 * "INBOX/parent-folder/folder" depending on the service type (this example is
 * for IMAP using the libtinymail-camel implementation).
 *
 * The ID is guaranteed to be unique per account. You must not free the result
 * of this method.
 * 
 * returns: unique id
 * since: 1.0
 * audience: application-developer
 **/
const gchar*
tny_folder_get_id (TnyFolder *self)
{
	const gchar *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_id!= NULL);
#endif

	retval = TNY_FOLDER_GET_IFACE (self)->get_id(self);

#ifdef DBC /* ensure */
	g_assert (retval);
#endif

	return retval;
}

/**
 * tny_folder_get_name:
 * @self: a #TnyFolder
 * 
 * Get the displayable name of @self. You should not free the result of 
 * this method.
 * 
 * returns: folder name
 * since: 1.0
 * audience: application-developer
 **/
const gchar*
tny_folder_get_name (TnyFolder *self)
{
	const gchar *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_name!= NULL);
#endif

	retval = TNY_FOLDER_GET_IFACE (self)->get_name(self);

#ifdef DBC /* ensure */
	g_assert (retval);
#endif

	return retval;
}



/**
 * tny_folder_get_folder_type:
 * @self: a #TnyFolder
 * 
 * Get the type of @self (Inbox, Outbox etc.). Most implementations decide the
 * type by comparing the name of the folder with some hardcoded values. Some
 * implementations (such as camel) might not provide precise information before
 * a connection has been made. For instance, the return value might be 
 * TNY_FOLDER_TYPE_NORMAL rather than TNY_FOLDER_TYPE_SENT.
 * 
 * returns: folder type
 * since: 1.0
 * audience: application-developer
 **/
TnyFolderType 
tny_folder_get_folder_type  (TnyFolder *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_folder_type!= NULL);
#endif

	return TNY_FOLDER_GET_IFACE (self)->get_folder_type(self);
}


/**
 * tny_folder_get_folder_store:
 * @self: a #TnyFolder
 * 
 * Get a the parent #TnyFolderStore of this folder. You must unreference the
 * return value after use if not NULL. Note that not every folder strictly has
 * to be inside a folder store. This API will in that case return NULL.
 * 
 * returns: (null-ok) (caller-owns): the folder store of this folder or NULL
 * since: 1.0
 * complexity: low
 * audience: application-developer
 **/
TnyFolderStore* 
tny_folder_get_folder_store (TnyFolder *self)
{
	TnyFolderStore* retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_FOLDER (self));
	g_assert (TNY_FOLDER_GET_IFACE (self)->get_folder_store!= NULL);
#endif

	retval = TNY_FOLDER_GET_IFACE (self)->get_folder_store(self);

#ifdef DBC /* require */
	if (retval)
		g_assert (TNY_IS_FOLDER_STORE (retval));
#endif

	return retval;
}


static void
tny_folder_base_init (gpointer g_class)
{
	static gboolean tny_folder_initialized = FALSE;

	if (!tny_folder_initialized) 
		tny_folder_initialized = TRUE;
}

static gpointer
tny_folder_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyFolderIface),
			tny_folder_base_init,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL,   /* instance_init */
			NULL
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyFolder", &info, 0);

	g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);

	return GSIZE_TO_POINTER (type);
}

GType
tny_folder_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_folder_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}

static gpointer
tny_folder_type_register_type (gpointer notused)
{
	GType etype = 0;
	static const GEnumValue values[] = {
		{ TNY_FOLDER_TYPE_UNKNOWN, "TNY_FOLDER_TYPE_UNKNOWN", "unknown" },
		{ TNY_FOLDER_TYPE_NORMAL, "TNY_FOLDER_TYPE_NORMAL", "normal" },
		{ TNY_FOLDER_TYPE_INBOX, "TNY_FOLDER_TYPE_INBOX", "inbox" },
		{ TNY_FOLDER_TYPE_OUTBOX, "TNY_FOLDER_TYPE_OUTBOX", "outbox" },
		{ TNY_FOLDER_TYPE_TRASH, "TNY_FOLDER_TYPE_TRASH", "trash" },
		{ TNY_FOLDER_TYPE_JUNK, "TNY_FOLDER_TYPE_JUNK", "junk" },
		{ TNY_FOLDER_TYPE_SENT, "TNY_FOLDER_TYPE_SENT", "sent" },
		{ TNY_FOLDER_TYPE_ROOT, "TNY_FOLDER_TYPE_ROOT", "root" },
		{ TNY_FOLDER_TYPE_NOTES, "TNY_FOLDER_TYPE_NOTES", "notes" },
		{ TNY_FOLDER_TYPE_DRAFTS, "TNY_FOLDER_TYPE_DRAFTS", "drafts" },
		{ TNY_FOLDER_TYPE_CONTACTS, "TNY_FOLDER_TYPE_CONTACTS", "contacts" },
		{ TNY_FOLDER_TYPE_CALENDAR, "TNY_FOLDER_TYPE_CALENDAR", "calendar" },
		{ TNY_FOLDER_TYPE_ARCHIVE, "TNY_FOLDER_TYPE_ARCHIVE", "archive" },
		{ TNY_FOLDER_TYPE_MERGE, "TNY_FOLDER_TYPE_MERGE", "merge" },
		{ 0, NULL, NULL }
	};
	etype = g_enum_register_static ("TnyFolderType", values);
	return GSIZE_TO_POINTER (etype);
}

/**
 * tny_folder_type_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType
tny_folder_type_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_folder_type_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}

static gpointer
tny_folder_caps_register_type (gpointer notused)
{
	GType etype = 0;
	static const GFlagsValue values[] = {
		{ TNY_FOLDER_CAPS_WRITABLE, "TNY_FOLDER_CAPS_WRITABLE", "writable" },
		{ TNY_FOLDER_CAPS_PUSHEMAIL, "TNY_FOLDER_CAPS_PUSHEMAIL", "pushemail" },
		{ 0, NULL, NULL }
	};
	etype = g_flags_register_static ("TnyFolderCaps", values);
	return GSIZE_TO_POINTER (etype);
}

/**
 * tny_folder_caps_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType
tny_folder_caps_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_folder_caps_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}

static gpointer
tny_folder_signal_register_type (gpointer notused)
{
	GType etype = 0;
	static const GEnumValue values[] = {
		{ TNY_FOLDER_FOLDER_INSERTED, "TNY_FOLDER_FOLDER_INSERTED", "inserted" },
		{ TNY_FOLDER_FOLDERS_RELOADED, "TNY_FOLDER_FOLDERS_RELOADED", "reloaded" },
		{ TNY_FOLDER_LAST_SIGNAL, "TNY_FOLDER_LAST_SIGNAL", "last-signal" },
		{ 0, NULL, NULL }
	};
	etype = g_enum_register_static ("TnyFolderSignal", values);
	return GSIZE_TO_POINTER (etype);
}

/**
 * tny_folder_signal_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType
tny_folder_signal_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_folder_signal_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
