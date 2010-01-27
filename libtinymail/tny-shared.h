#ifndef TNY_SHARED_H
#define TNY_SHARED_H

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

#include <glib.h>
#include <glib-object.h>

#ifdef DEBUG
#define tny_debug g_print
#else
#define tny_debug(o, ...)	
#endif

G_BEGIN_DECLS

/* GTK+ uses G_PRIORITY_HIGH_IDLE + 10 for resizing operations,
 * and G_PRIORITY_HIGH_IDLE + 20 for redrawing operations;
 * this makes sure that status callbacks happen after redraws, so we don't
 * get a lot of notifications but very little visual feedback */
#define TNY_PRIORITY_LOWER_THAN_GTK_REDRAWS G_PRIORITY_HIGH_IDLE + 30

typedef struct _TnyStatus TnyStatus;
typedef struct _TnyAccountStore TnyAccountStore;
typedef struct _TnyAccountStoreIface TnyAccountStoreIface;
typedef struct _TnyList TnyList;
typedef struct _TnyIterator TnyIterator;
typedef struct _TnyListIface TnyListIface;
typedef struct _TnyIteratorIface TnyIteratorIface;
typedef struct _TnyMsg TnyMsg;
typedef struct _TnyMsgIface TnyMsgIface;
typedef struct _TnyFolder TnyFolder;
typedef struct _TnyFolderIface TnyFolderIface;
typedef struct _TnyHeader TnyHeader;
typedef struct _TnyHeaderIface TnyHeaderIface;
typedef struct _TnyMimePart TnyMimePart;
typedef struct _TnyMimePartIface TnyMimePartIface;
typedef struct _TnyAccount TnyAccount;
typedef struct _TnyAccountIface TnyAccountIface;
typedef struct _TnyDevice TnyDevice;
typedef struct _TnyDeviceIface TnyDeviceIface;
typedef struct _TnyStoreAccount TnyStoreAccount;
typedef struct _TnyStoreAccountIface TnyStoreAccountIface;
typedef struct _TnyTransportAccount TnyTransportAccount;
typedef struct _TnyTransportAccountIface TnyTransportAccountIface;
typedef struct _TnyStream TnyStream;
typedef struct _TnyStreamIface TnyStreamIface;
typedef struct _TnySimpleList TnySimpleList;
typedef struct _TnySimpleListClass TnySimpleListClass;
typedef struct _TnyFsStream TnyFsStream;
typedef struct _TnyFsStreamClass TnyFsStreamClass;
typedef struct _TnyFolderStore TnyFolderStore;
typedef struct _TnyFolderStoreIface TnyFolderStoreIface;
typedef struct _TnyFolderStoreQuery TnyFolderStoreQuery;
typedef struct _TnyFolderStoreQueryClass TnyFolderStoreQueryClass;
typedef struct _TnyFolderStoreQueryItem TnyFolderStoreQueryItem;
typedef struct _TnyFolderStoreQueryItemClass TnyFolderStoreQueryItemClass;
typedef struct _TnyMsgRemoveStrategy TnyMsgRemoveStrategy;
typedef struct _TnyMsgRemoveStrategyIface TnyMsgRemoveStrategyIface;
typedef struct _TnySendQueue TnySendQueue;
typedef struct _TnySendQueueIface TnySendQueueIface;
typedef struct _TnyMsgReceiveStrategy TnyMsgReceiveStrategy;
typedef struct _TnyMsgReceiveStrategyIface TnyMsgReceiveStrategyIface;
typedef struct _TnyPair TnyPair;
typedef struct _TnyPairClass TnyPairClass;
typedef struct _TnyLockable TnyLockable;
typedef struct _TnyLockableIface TnyLockableIface;
typedef struct _TnyNoopLockable TnyNoopLockable;
typedef struct _TnyNoopLockableClass TnyNoopLockableClass;
typedef struct _TnyFolderObserver TnyFolderObserver;
typedef struct _TnyFolderObserverIface TnyFolderObserverIface;
typedef struct _TnyFolderChange TnyFolderChange;
typedef struct _TnyFolderChangeClass TnyFolderChangeClass;
typedef struct _TnyFolderMonitor TnyFolderMonitor;
typedef struct _TnyFolderMonitorClass TnyFolderMonitorClass;
typedef struct _TnyFolderStoreChange TnyFolderStoreChange;
typedef struct _TnyFolderStoreChangeClass TnyFolderStoreChangeClass;
typedef struct _TnyFolderStoreObserver TnyFolderStoreObserver;
typedef struct _TnyFolderStoreObserverIface TnyFolderStoreObserverIface;
typedef struct _TnyFolderStats TnyFolderStats;
typedef struct _TnyFolderStatsClass TnyFolderStatsClass;
typedef struct _TnyPasswordGetter TnyPasswordGetter;
typedef struct _TnyPasswordGetterIface TnyPasswordGetterIface;
typedef struct _TnyCombinedAccount TnyCombinedAccount;
typedef struct _TnyCombinedAccountClass TnyCombinedAccountClass;
typedef struct _TnyConnectionPolicy TnyConnectionPolicy;
typedef struct _TnyConnectionPolicyIface TnyConnectionPolicyIface;
typedef struct _TnySeekable TnySeekable;
typedef struct _TnySeekableIface TnySeekableIface;
typedef struct _TnyStreamCache TnyStreamCache;
typedef struct _TnyStreamCacheIface TnyStreamCacheIface;
typedef struct _TnyFsStreamCache TnyFsStreamCache;
typedef struct _TnyFsStreamCacheClass TnyFsStreamCacheClass;
typedef struct _TnyCachedFile TnyCachedFile;
typedef struct _TnyCachedFileClass TnyCachedFileClass;
typedef struct _TnyCachedFileStream TnyCachedFileStream;
typedef struct _TnyCachedFileStreamClass TnyCachedFileStreamClass;


/** 
 * TnyStatusCallback:
 * @self: The sender of the status
 * @status: a #TnyStatus object
 * @user_data: user data passed by the caller
 * 
 * A method that gets called whenever there's a status event
 **/
typedef void (*TnyStatusCallback) (GObject *self, TnyStatus *status, gpointer user_data);

typedef gchar* (*TnyGetPassFunc) (TnyAccount *self, const gchar *prompt, gboolean *cancel);
typedef void (*TnyForgetPassFunc) (TnyAccount *self);

typedef void (*TnyMimePartCallback) (TnyMimePart *self, gboolean cancelled, TnyStream *stream, GError *err, gpointer user_data);

typedef void (*TnyGetFoldersCallback) (TnyFolderStore *self, gboolean cancelled, TnyList *list, GError *err, gpointer user_data);
typedef void (*TnyCreateFolderCallback) (TnyFolderStore *self, gboolean cancelled, TnyFolder *new_folder, GError *err, gpointer user_data);

typedef void (*TnyFolderCallback) (TnyFolder *self, gboolean cancelled, GError *err, gpointer user_data);
typedef void (*TnyGetHeadersCallback) (TnyFolder *self, gboolean cancelled, TnyList *headers, GError *err, gpointer user_data);

typedef TnyStream* (*TnyStreamCacheOpenStreamFetcher) (TnyStreamCache *self, gint64 *expected_size, gpointer userdata);
typedef gboolean (*TnyStreamCacheRemoveFilter) (TnyStreamCache *self, const gchar *id, gpointer userdata);
typedef void (*TnyFolderStoreCallback) (TnyFolderStore *self, gboolean cancelled, GError *err, gpointer user_data);

/** 
 * TnyGetMsgCallback:
 * @folder: a #TnyFolder that caused the callback
 * @cancelled: if the operation got cancelled
 * @msg: (null-ok): a #TnyMsg with the fetched message or NULL
 * @err: (null-ok): if an error occurred
 * @user_data: (null-ok):  user data that was passed to the callbacks
 *
 * A folder callback for when a message fetch was requested. If allocated, you
 * must cleanup @user_data at the end of your implementation of the callback. All 
 * other fields in the parameters of the callback are read-only.
 *
 * When cancelled, @cancelled will be TRUE while @err might nonetheless be NULL.
 * If @err is not NULL, an error occurred that you should handle gracefully.
 * The @msg parameter might be NULL in case of error or cancellation.
 *
 * since: 1.0
 * audience: application-developer
 **/
typedef void (*TnyGetMsgCallback) (TnyFolder *folder, gboolean cancelled, TnyMsg *msg, GError *err, gpointer user_data);
/** 
 * TnyTransferMsgsCallback:
 * @folder: a #TnyFolder that caused the callback
 * @cancelled: if the operation got cancelled
 * @err: (null-ok): if an error occurred
 * @user_data: (null-ok):  user data that was passed to the callbacks
 *
 * A folder callback for when a transfer of messages was requested. If allocated,
 * you must cleanup @user_data at the end of your implementation of the callback.
 * All other fields in the parameters of the callback are read-only.
 *
 * When cancelled, @cancelled will be TRUE while @err might nonetheless be NULL.
 * If @err is not NULL, an error occurred that you should handle gracefully.
 *
 * since: 1.0
 * audience: application-developer
 **/
typedef void (*TnyTransferMsgsCallback) (TnyFolder *folder, gboolean cancelled, GError *err, gpointer user_data);
typedef void (*TnyCopyFolderCallback) (TnyFolder *self, gboolean cancelled, TnyFolderStore *into, TnyFolder *new_folder, GError *err, gpointer user_data);

typedef void (*TnySendQueueAddCallback) (TnySendQueue *self, gboolean cancelled, TnyMsg *msg, GError *err, gpointer user_data);

G_END_DECLS

#endif
