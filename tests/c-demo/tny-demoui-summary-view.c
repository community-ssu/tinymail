/* tinymail - Tiny Mail
 * Copyright (C) 2006-2008 Philip Van Hoof <pvanhoof@gnome.org>
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

#include <glib/gi18n-lib.h>

#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <tny-simple-list.h>
#include <tny-iterator.h>
#include <tny-platform-factory.h>

#include <tny-gtk-msg-view.h>

#if PLATFORM==1
#include <tny-gnome-platform-factory.h>
#include <tny-gnome-account-store.h>
#endif

#if PLATFORM==2
#include <tny-maemo-platform-factory.h>
#include <tny-maemo-account-store.h>
#endif

#if PLATFORM==3
#include <tny-gpe-platform-factory.h>
#include <tny-gpe-account-store.h>
#endif

#if PLATFORM==4
#include <tny-olpc-platform-factory.h>
#include <tny-olpc-account-store.h>
#endif

#include <tny-status.h>
#include <tny-account-store.h>
#include <tny-account.h>
#include <tny-store-account.h>
#include <tny-transport-account.h>
#include <tny-msg-view.h>
#include <tny-msg-window.h>
#include <tny-gtk-msg-window.h>
#include <tny-folder.h>
#include <tny-gtk-account-list-model.h>
#include <tny-gtk-folder-store-tree-model.h>
#include <tny-header.h>
#include <tny-gtk-header-list-model.h>
#include <tny-demoui-summary-view.h>
#include <tny-summary-view.h>
#include <tny-account-store-view.h>
#include <tny-merge-folder.h>

#include <tny-camel-send-queue.h>


#define GO_ONLINE_TXT _("Go online")
#define GO_OFFLINE_TXT _("Go offline")

static GObjectClass *parent_class = NULL;

#include <tny-camel-send-queue.h>
#include <tny-camel-transport-account.h>
#include <tny-folder-monitor.h>


typedef struct _TnyDemouiSummaryViewPriv TnyDemouiSummaryViewPriv;

struct _TnyDemouiSummaryViewPriv
{
	TnyAccountStore *account_store;
 	GtkComboBox *account_view;
	GtkTreeView *mailbox_view, *header_view;
	TnyMsgView *msg_view;
	GtkWidget *status, *progress, *online_button, *poke_button, 
		  *sync_button, *killacc_button;
	guint status_id;
	gulong mailbox_select_sid;
	GtkTreeSelection *mailbox_select;
	GtkTreeIter last_mailbox_correct_select;
 	gboolean last_mailbox_correct_select_set;
	guint connchanged_signal, online_button_signal;
	TnyList *current_accounts;
	TnyFolderMonitor *monitor; GMutex *monitor_lock;
	gboolean handle_recon;
	TnySendQueue *send_queue;
};

#define TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_DEMOUI_SUMMARY_VIEW, TnyDemouiSummaryViewPriv))


static gboolean
cleanup_statusbar (gpointer data)
{
	TnyDemouiSummaryViewPriv *priv = data;

	//gtk_widget_hide (GTK_WIDGET (priv->progress));
	gtk_statusbar_pop (GTK_STATUSBAR (priv->status), priv->status_id);

	return FALSE;
}


static void
status_update (GObject *sender, TnyStatus *status, gpointer user_data)
{
	TnySummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	gchar *new_what;

	if (!user_data)
		return;

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress), 
		tny_status_get_fraction (status));
	gtk_statusbar_pop (GTK_STATUSBAR (priv->status), priv->status_id);

	new_what = g_strdup_printf ("%s (%d/%d)", status->message, status->position, 
		status->of_total);

	gtk_statusbar_push (GTK_STATUSBAR (priv->status), priv->status_id, new_what);
	g_free (new_what);

	return;
}

static void
set_header_view_model (GtkTreeView *header_view, GtkTreeModel *model)
{
	GtkTreeModel *oldsortable = gtk_tree_view_get_model (GTK_TREE_VIEW (header_view));

	if (oldsortable && GTK_IS_TREE_MODEL_SORT (oldsortable))
	{ 
		GtkTreeModel *oldmodel = gtk_tree_model_sort_get_model 
					(GTK_TREE_MODEL_SORT (oldsortable));
		if (oldmodel)
			g_object_unref (G_OBJECT (oldmodel));
		g_object_unref (G_OBJECT (oldsortable));
	} 

	gtk_tree_view_set_model (header_view, model);

	return;
}

static GtkTreeModel *empty_model;

static void 
clear_header_view (TnyDemouiSummaryViewPriv *priv)
{
	g_mutex_lock (priv->monitor_lock);
	{

		if (priv->monitor)
		{
			tny_folder_monitor_stop (priv->monitor);
			g_object_unref (priv->monitor);
		}
		priv->monitor = NULL;
	}
	g_mutex_unlock (priv->monitor_lock);


	/* Clear the header_view by giving it an empty model */
	if (G_UNLIKELY (!empty_model))
		empty_model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	set_header_view_model (GTK_TREE_VIEW (priv->header_view), empty_model);

	tny_msg_view_clear (priv->msg_view);
}


static GtkWidget *rem_dialog = NULL;

typedef struct
{
	TnySummaryView *self;
	TnyHeader *header;
} OnResponseInfo;

static void
on_response (GtkDialog *dialog, gint arg1, gpointer user_data)
{
	OnResponseInfo *info = (OnResponseInfo *) user_data;
	TnySummaryView *self = info->self;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	TnyHeader *header = info->header;


	if (arg1 == GTK_RESPONSE_YES)
	{
		TnyFolder *outbox = tny_send_queue_get_outbox (priv->send_queue);
		tny_folder_remove_msg (outbox, header, NULL);
		tny_folder_sync (outbox, TRUE, NULL);
		g_object_unref (outbox);
	}

	g_object_unref (header);
	g_object_unref (self);

	gtk_widget_destroy (GTK_WIDGET (dialog));
	rem_dialog = NULL;

	g_slice_free (OnResponseInfo, info);
}

static void 
on_send_queue_error_happened (TnySendQueue *self, TnyHeader *header, TnyMsg *msg, GError *err, gpointer user_data)
{
	if (header)
	{
		gchar *subject;
		gchar *str;
		
		subject = tny_header_dup_subject (header);
		str = g_strdup_printf ("%s. Do you want to remove the message (%s)?",
			err->message, subject);
		g_free (subject);
		OnResponseInfo *info = g_slice_new (OnResponseInfo);

		if (!rem_dialog)
		{
			rem_dialog = gtk_message_dialog_new (NULL, 0,
				GTK_MESSAGE_ERROR, GTK_BUTTONS_YES_NO, str);
			info->self = g_object_ref (user_data);
			info->header = g_object_ref (header);
			g_signal_connect (G_OBJECT (rem_dialog), "response",
				G_CALLBACK (on_response), info);
			gtk_widget_show_all (rem_dialog);
		}
		g_free (str);
	}
	return;
}

static void
on_constatus_changed (TnyAccount *a, TnyConnectionStatus status, gpointer user_data)
{
	gchar *str = NULL;

	if (status == TNY_CONNECTION_STATUS_DISCONNECTED)
		str = "disconnected";

	if (status == TNY_CONNECTION_STATUS_CONNECTED)
		str = "connected";
	
	if (status == TNY_CONNECTION_STATUS_CONNECTED_BROKEN)
	{
		/* tny_camel_account_set_online (TNY_CAMEL_ACCOUNT (a), FALSE, NULL); */
		str = "con broken";
	}

	if (status == TNY_CONNECTION_STATUS_DISCONNECTED_BROKEN)
		str = "discon broken";

	g_print ("CON EVENT FOR %s: %s\n",
		tny_account_get_name (a), str);

}

static void 
load_accounts (TnySummaryView *self, gboolean first_time)
{
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	TnyAccountStore *account_store = priv->account_store;
	GtkTreeModel *sortable, *maccounts, *mailbox_model;
	TnyFolderStoreQuery *query;
	TnyList *accounts;
	TnyList *saccounts;

	/* Show only subscribed folders */
	query = tny_folder_store_query_new ();
	tny_folder_store_query_add_item (query, NULL, 
					 TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED);

	/* TnyGtkFolderStoreTreeModel is also a TnyList (it simply implements both the
	   TnyList and the GtkTreeModel interfaces) */
	mailbox_model = tny_gtk_folder_store_tree_model_new (NULL);

	g_object_unref (query);

	accounts = TNY_LIST (mailbox_model);
	saccounts = tny_simple_list_new ();

	maccounts = tny_gtk_account_list_model_new ();

	clear_header_view (priv);

	if (priv->current_accounts)
	{
		g_object_unref (G_OBJECT (priv->current_accounts));
		priv->current_accounts = NULL;
	}

	/* This method uses the TnyFolderStoreTreeModel as a TnyList */
	tny_account_store_get_accounts (account_store, accounts,
		TNY_ACCOUNT_STORE_STORE_ACCOUNTS);

	priv->current_accounts = TNY_LIST (g_object_ref (G_OBJECT (accounts)));

	if (first_time)
	{
		TnyIterator *aiter = NULL;
		aiter = tny_list_create_iterator (accounts);
		while (!tny_iterator_is_done (aiter))
		{
			GObject *a = tny_iterator_get_current (aiter);
			
			g_signal_connect (a, "connection-status-changed",
				G_CALLBACK (on_constatus_changed), self);

			g_object_unref (a);
			tny_iterator_next (aiter);
		}

		g_object_unref (aiter);
	}

	tny_account_store_get_accounts (account_store, TNY_LIST (maccounts),
			TNY_ACCOUNT_STORE_STORE_ACCOUNTS);

	gtk_combo_box_set_model (priv->account_view, maccounts);

	tny_account_store_get_accounts (account_store, saccounts,
			TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS);

	if (tny_list_get_length (saccounts) > 0)
	{
		TnyIterator *iter = tny_list_create_iterator (saccounts);
		TnyTransportAccount *tacc = (TnyTransportAccount *)tny_iterator_get_current (iter);
		if (priv->send_queue)
			g_object_unref (priv->send_queue);
		priv->send_queue = tny_camel_send_queue_new ((TnyCamelTransportAccount *) tacc);
		g_signal_connect (G_OBJECT (priv->send_queue), "error-happened",
			G_CALLBACK (on_send_queue_error_happened), self);
		g_object_unref (tacc);
		g_object_unref (iter);
	}
	g_object_unref (saccounts);

	/* Here we use the TnyFolderStoreTreeModel as a GtkTreeModel */
	sortable = gtk_tree_model_sort_new_with_model (mailbox_model);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sortable),
				TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, 
				GTK_SORT_ASCENDING);

	/* Set the model of the mailbox_view */
	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->mailbox_view), 
		sortable);

	return;
}


static void 
online_button_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	TnySummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);

	if (!priv->handle_recon)
		return;

	if (priv->account_store)
	{
		TnyDevice *device = tny_account_store_get_device (priv->account_store);

		if (gtk_toggle_button_get_active (togglebutton))
			tny_device_force_online (device);
		else
			tny_device_force_offline (device);

		g_object_unref (G_OBJECT (device));
	}
}

static void
recurse_poke (TnyFolderStore *f_store)
{
	TnyList *folders = tny_simple_list_new ();
	TnyIterator *f_iter;

	tny_folder_store_get_folders (TNY_FOLDER_STORE (f_store), folders, NULL, TRUE, NULL);
	f_iter = tny_list_create_iterator (folders);
	while (!tny_iterator_is_done (f_iter))
	{
		TnyFolder *f_cur = TNY_FOLDER (tny_iterator_get_current (f_iter));

		tny_folder_poke_status (f_cur);

		if (TNY_IS_FOLDER_STORE (f_cur))
			recurse_poke (TNY_FOLDER_STORE (f_cur));

		g_object_unref (f_cur);
		tny_iterator_next (f_iter);
	}
	g_object_unref (f_iter);
	g_object_unref (folders);
}

static void 
poke_button_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	TnySummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	TnyList *accounts = tny_simple_list_new ();
	TnyIterator *a_iter;

	tny_account_store_get_accounts (priv->account_store, accounts, 
		TNY_ACCOUNT_STORE_STORE_ACCOUNTS);

	a_iter = tny_list_create_iterator (accounts);

	while (!tny_iterator_is_done (a_iter))
	{
		TnyAccount *a_cur = TNY_ACCOUNT (tny_iterator_get_current (a_iter));

		recurse_poke (TNY_FOLDER_STORE (a_cur));

		g_object_unref (a_cur);
		tny_iterator_next (a_iter);
	}

	g_object_unref (a_iter);
	g_object_unref (accounts);
}



static void 
killacc_button_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	TnySummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	TnyList *accounts = tny_simple_list_new ();
	TnyIterator *a_iter;
	GObject *a_cur;

	tny_account_store_get_accounts (priv->account_store, accounts, 
		TNY_ACCOUNT_STORE_STORE_ACCOUNTS);

	a_iter = tny_list_create_iterator (accounts);

	a_cur = tny_iterator_get_current (a_iter);

	g_object_unref (a_iter);
	g_object_unref (accounts);

	printf ("%d\n", a_cur->ref_count);

	/* This is indeed incorrect, don't do this (it's a test) */
	g_object_unref (a_cur);
	g_object_unref (a_cur);
	g_object_unref (a_cur);
	g_object_unref (a_cur);
	g_object_unref (a_cur);

	return;
}

static void
connection_changed (TnyDevice *device, gboolean online, gpointer user_data)
{
	TnySummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);

	priv->handle_recon = FALSE;

	if (online)
	{
		gtk_button_set_label  (GTK_BUTTON (priv->online_button), GO_OFFLINE_TXT);
		g_signal_handler_block (G_OBJECT (priv->online_button), priv->online_button_signal);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->online_button), TRUE);
		g_signal_handler_unblock (G_OBJECT (priv->online_button), priv->online_button_signal);
	} else {

		gtk_button_set_label  (GTK_BUTTON (priv->online_button), GO_ONLINE_TXT);
		g_signal_handler_block (G_OBJECT (priv->online_button), priv->online_button_signal);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->online_button), FALSE);
		g_signal_handler_unblock (G_OBJECT (priv->online_button), priv->online_button_signal);
	}

	priv->handle_recon = TRUE;

	return;
}

static void
tny_demoui_summary_view_set_account_store (TnyAccountStoreView *self, TnyAccountStore *account_store)
{
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	TnyDevice *device = tny_account_store_get_device (account_store);

	if (priv->account_store)
	{ 
		TnyDevice *odevice = tny_account_store_get_device (priv->account_store);

		if (g_signal_handler_is_connected (G_OBJECT (odevice), priv->connchanged_signal))
			g_signal_handler_disconnect (G_OBJECT (odevice), 
				priv->connchanged_signal);

		g_object_unref (priv->account_store);
		g_object_unref (odevice);
	}

	g_object_ref (G_OBJECT (account_store));

	priv->connchanged_signal =  g_signal_connect (
			G_OBJECT (device), "connection_changed",
			G_CALLBACK (connection_changed), self);	

	priv->account_store = account_store;

	load_accounts ((TnySummaryView *) self, TRUE);

	g_object_unref (G_OBJECT (device));

	return;
}

static void
on_header_view_key_press_event (GtkTreeView *header_view, GdkEventKey *event, gpointer user_data)
{
	TnyDemouiSummaryView *self = (TnyDemouiSummaryView *) user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);


	if (event->keyval == GDK_Home && priv->send_queue)
	{
		GtkTreeSelection *selection = gtk_tree_view_get_selection (header_view);
		GtkTreeModel *model;
		GtkTreeIter iter;

		if (gtk_tree_selection_get_selected (selection, &model, &iter))
		{
			TnyHeader *header;

			gtk_tree_model_get (model, &iter, 
				TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, 
				&header, -1);

			if (header)
			{
				GError *err = NULL;
				TnyMsg *msg = NULL;
				TnyFolder *folder;

				GtkWidget *entry1, *entry2;
				GtkWidget *dialog;

				folder = tny_header_get_folder (header);
				if (folder) {
					msg = tny_folder_get_msg (folder, header, &err);
					g_object_unref (folder);
				}

				if (!msg || err != NULL)
				{
					g_warning ("Can't forward message: %s\n", err->message);
					if (msg) 
						g_object_unref (msg);
					g_object_unref (header);
					return;
				}

				dialog = gtk_dialog_new_with_buttons (_("Forward a message"),
						  NULL, GTK_DIALOG_MODAL,
						  GTK_STOCK_OK,
						  GTK_RESPONSE_YES,
						  GTK_STOCK_CANCEL,
						  GTK_RESPONSE_REJECT,
						  NULL);

				entry1 = gtk_entry_new ();
				gtk_entry_set_text (GTK_ENTRY (entry1), _("To: "));
				gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), entry1);
				gtk_widget_show (entry1);

				entry2 = gtk_entry_new ();
				gtk_entry_set_text (GTK_ENTRY (entry2), _("From: "));
				gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), entry2);
				gtk_widget_show (entry2);

				if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
				{
					TnyHeader *nheader = tny_msg_get_header (msg);
					const gchar *to = gtk_entry_get_text (GTK_ENTRY (entry1));
					const gchar *from = gtk_entry_get_text (GTK_ENTRY (entry2));

					tny_header_set_to (nheader, to);
					tny_header_set_from (nheader, from);
					g_object_unref (nheader);

					tny_send_queue_add (priv->send_queue, msg, NULL);
				}

				gtk_widget_destroy (dialog);

				g_object_unref (msg);
				g_object_unref (G_OBJECT (header));
			}
		}
	}

	if (event->keyval == GDK_Delete)
	{
		GtkTreeSelection *selection = gtk_tree_view_get_selection (header_view);
		GtkTreeModel *model, *mymodel;
		GtkTreeIter iter;

		if (gtk_tree_selection_get_selected (selection, &model, &iter))
		{
			TnyHeader *header;

			gtk_tree_model_get (model, &iter, 
				TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, 
				&header, -1);

			if (header)
			{
				gchar *subject;
				GtkWidget *dialog;

				subject = tny_header_dup_subject (header);
				dialog = gtk_message_dialog_new (NULL, 
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, 
					_("This will remove the message with subject \"%s\""),
					subject);
				g_free (subject);

				if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
				{
					TnyFolder *folder;

					if (GTK_IS_TREE_MODEL_SORT (model))
					{
						mymodel = gtk_tree_model_sort_get_model 
							(GTK_TREE_MODEL_SORT (model));
					} else mymodel = model;

					tny_list_remove (TNY_LIST (mymodel), G_OBJECT (header));
					folder = tny_header_get_folder (header);
					tny_folder_remove_msg (folder, header, NULL);
					tny_folder_sync (folder, TRUE, NULL);
					g_object_unref (G_OBJECT (folder));
				}

				gtk_widget_destroy (dialog);
				g_object_unref (G_OBJECT (header));
			}
		}
		
	}



	if (event->keyval == GDK_End)
	{
		GtkTreeSelection *selection = gtk_tree_view_get_selection (header_view);
		GtkTreeModel *model;
		GtkTreeIter iter;

		if (gtk_tree_selection_get_selected (selection, &model, &iter))
		{
			TnyHeader *header;

			gtk_tree_model_get (model, &iter, 
				TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, 
				&header, -1);

			if (header)
			{
				gchar *subject;
				GtkWidget *dialog;

				subject = tny_header_dup_subject (header);
				dialog = gtk_message_dialog_new (NULL, 
								 GTK_DIALOG_MODAL,
								 GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, 
								 _("This will uncache the message with subject \"%s\""),
								 subject);
				g_free (subject);

				if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
				{
					TnyFolder *folder;
					TnyMsg *msg;

					folder = tny_header_get_folder (header);
					msg = tny_folder_get_msg (folder, header, NULL);
					if (msg) {
						tny_msg_uncache_attachments (msg);
						g_object_unref (msg);
					}
					g_object_unref (folder);
				}

				gtk_widget_destroy (dialog);
				g_object_unref (G_OBJECT (header));
			}
		}
		
	}

	return;
}


typedef struct
{
	TnyDemouiSummaryView *self;
	TnyHeader *header;
	TnyMsgView *msg_view;
} OnGetMsgInfo;

static void
status_update_on_get_msg (GObject *sender, TnyStatus *status, gpointer user_data)
{
	OnGetMsgInfo *info = user_data;
	status_update (sender, status, info->self);
	return;
}

static void
on_get_msg (TnyFolder *folder, gboolean cancelled, TnyMsg *msg, GError *merr, gpointer user_data)
{
	OnGetMsgInfo *info = user_data;
	TnyDemouiSummaryView *self = info->self;
	TnyHeader *header = info->header;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);

	g_idle_add (cleanup_statusbar, priv);

	if (cancelled) {
		g_object_unref (self);
		g_object_unref (header);
		g_slice_free (OnGetMsgInfo, info);
		return;
	}

	if (msg) {
		TnyHeaderFlags flags = tny_header_get_flags (header);
		if (!(flags & TNY_HEADER_FLAG_SEEN))
			tny_header_set_flag (header, TNY_HEADER_FLAG_SEEN);
		tny_msg_view_set_msg (info->msg_view, msg);
	} else 
		tny_msg_view_set_unavailable (info->msg_view);

	if (merr != NULL)
	{
		GtkWidget *edialog;

		edialog = gtk_message_dialog_new (
				  GTK_WINDOW (gtk_widget_get_parent (GTK_WIDGET (self))),
				  GTK_DIALOG_DESTROY_WITH_PARENT,
				  GTK_MESSAGE_ERROR,
				  GTK_BUTTONS_CLOSE,
				  merr->message);
		g_signal_connect_swapped (edialog, "response",
			G_CALLBACK (gtk_widget_destroy), edialog);
		gtk_widget_show_all (edialog);
	}

	g_object_unref (self);
	g_object_unref (info->msg_view);
	g_object_unref (header);
	g_slice_free (OnGetMsgInfo, info);

	return;
}

static void
on_header_view_tree_selection_changed (GtkTreeSelection *selection, 
		gpointer user_data)
{
	TnyDemouiSummaryView *self  = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (G_LIKELY (gtk_tree_selection_get_selected (selection, &model, &iter)))
	{
		TnyHeader *header;

		gtk_tree_model_get (model, &iter, 
			TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, 
			&header, -1);
	
		if (G_LIKELY (header))
		{
			TnyFolder *folder;
			folder = tny_header_get_folder (header);
			if (folder)
			{
				OnGetMsgInfo *info = g_slice_new (OnGetMsgInfo);
				info->self = TNY_DEMOUI_SUMMARY_VIEW (g_object_ref (self));
				info->header = TNY_HEADER (g_object_ref (header));
				info->msg_view = TNY_MSG_VIEW (g_object_ref (priv->msg_view));
				gtk_widget_show (GTK_WIDGET (priv->progress));
				tny_folder_get_msg_async (folder, header, 
					on_get_msg, status_update_on_get_msg, info);
				g_object_unref (G_OBJECT (folder));
			}

			g_object_unref (G_OBJECT (header));
		
		} else {
			tny_msg_view_set_unavailable (priv->msg_view);
		}
	}

	return;
}


static void
refresh_current_folder (TnyFolder *folder, gboolean cancelled, GError *err, gpointer user_data)
{
	TnySummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	GtkTreeModel *select_model;

	if (!cancelled)
	{
		GtkSelectionMode mode;

		g_idle_add (cleanup_statusbar, priv);

		mode = gtk_tree_selection_get_mode (priv->mailbox_select);

		if (mode == GTK_SELECTION_SINGLE)
		{
			gtk_tree_selection_get_selected (priv->mailbox_select, &select_model, 
				&priv->last_mailbox_correct_select);
			priv->last_mailbox_correct_select_set = TRUE;
		}

		/* gtk_widget_set_sensitive (GTK_WIDGET (priv->header_view), TRUE); */

	} else {
		/* Restore selection */
		g_signal_handler_block (G_OBJECT (priv->mailbox_select), 
			priv->mailbox_select_sid);
		gtk_tree_selection_select_iter (priv->mailbox_select, 
			&priv->last_mailbox_correct_select);
		g_signal_handler_unblock (G_OBJECT (priv->mailbox_select), 
			priv->mailbox_select_sid);
	}

	return;
}


static void 
set_folder_cb (TnyFolder *self, gboolean cancelled, TnyList *headers, GError *err, gpointer user_data)
{
	if (err) {
		TnySummaryView *self = user_data;
		GtkWidget *edialog;
		edialog = gtk_message_dialog_new (
				  GTK_WINDOW (gtk_widget_get_parent (GTK_WIDGET (self))),
				  GTK_DIALOG_DESTROY_WITH_PARENT,
				  GTK_MESSAGE_ERROR,
				  GTK_BUTTONS_CLOSE,
				  err->message);
		g_signal_connect_swapped (edialog, "response",
			G_CALLBACK (gtk_widget_destroy), edialog);
		gtk_widget_show_all (edialog);
	}
}

static void
on_mailbox_view_tree_selection_changed (GtkTreeSelection *selection, 
		gpointer user_data)
{
	TnySummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);

	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeModel *hmodel;
	GtkTreeView *header_view = GTK_TREE_VIEW (priv->header_view);
	GtkTreeModel *sortable;
	GtkSelectionMode mode;

	mode = gtk_tree_selection_get_mode (selection);

	if (mode == GTK_SELECTION_SINGLE)
	{
	  if (gtk_tree_selection_get_selected (selection, &model, &iter))
	  {
		TnyFolder *folder;
		gint type;

		gtk_tree_model_get (model, &iter, 
			TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, 
			&type, -1);

		if (type == TNY_FOLDER_TYPE_ROOT) 
		{ 
			/* If an "account name"-row was clicked */
			if (priv->last_mailbox_correct_select_set) 
			{
				g_signal_handler_block (G_OBJECT (priv->mailbox_select), priv->mailbox_select_sid);
				gtk_tree_selection_select_iter (priv->mailbox_select, &priv->last_mailbox_correct_select);
				g_signal_handler_unblock (G_OBJECT (priv->mailbox_select), priv->mailbox_select_sid);
			}

			priv->last_mailbox_correct_select_set = FALSE;

		} else {

			priv->last_mailbox_correct_select_set = FALSE;

			gtk_tree_model_get (model, &iter, 
				TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, 
				&folder, -1);

			gtk_widget_show (GTK_WIDGET (priv->progress));
			/* gtk_widget_set_sensitive (GTK_WIDGET (priv->header_view), FALSE); */

			hmodel = tny_gtk_header_list_model_new ();
			tny_gtk_header_list_model_set_folder (TNY_GTK_HEADER_LIST_MODEL (hmodel), 
				folder, FALSE, set_folder_cb, status_update, self);

			g_mutex_lock (priv->monitor_lock);
			{
				if (priv->monitor) {
					tny_folder_monitor_stop (priv->monitor);
					g_object_unref (priv->monitor);
				}
				priv->monitor = TNY_FOLDER_MONITOR (tny_folder_monitor_new (folder));
				tny_folder_monitor_add_list (priv->monitor, TNY_LIST (hmodel));
				tny_folder_monitor_start (priv->monitor);
			}
			g_mutex_unlock (priv->monitor_lock);

			sortable = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (hmodel));

			gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortable),
				TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_COLUMN,
				tny_gtk_header_list_model_received_date_sort_func, 
				NULL, NULL);

			gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortable),
				TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_COLUMN,
				tny_gtk_header_list_model_sent_date_sort_func, 
				NULL, NULL);

			set_header_view_model (header_view, sortable);

#ifndef NONASYNC_TEST
			tny_folder_refresh_async (folder, 
				refresh_current_folder, 
				status_update, self);
#else
			tny_folder_refresh (folder, NULL);
			refresh_current_folder (folder, FALSE, NULL, self);
#endif
			g_object_unref (folder);
		}
	  }
	} else {
		GList *list;
		TnyFolder *merge = tny_merge_folder_new ("Merged");
		GtkTreeView *header_view = GTK_TREE_VIEW (priv->header_view);

		list = gtk_tree_selection_get_selected_rows (priv->mailbox_select, &model);

		while (list)
		{
			GtkTreePath *path = list->data;
			GtkTreeIter iter;
			TnyFolder *folder;

			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_model_get (model, &iter, 
				TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, 
				&folder, -1);

			if (folder && TNY_IS_FOLDER (folder))
			{
				tny_merge_folder_add_folder (TNY_MERGE_FOLDER (merge), folder);
				g_object_unref (folder);
			}

			gtk_tree_path_free (path);
			list = g_list_next (list);
		}

		hmodel = tny_gtk_header_list_model_new ();
		tny_gtk_header_list_model_set_folder (TNY_GTK_HEADER_LIST_MODEL (hmodel), 
			merge, FALSE, NULL, status_update, self);

		g_mutex_lock (priv->monitor_lock);
		{
			if (priv->monitor) {
				tny_folder_monitor_stop (priv->monitor);
				g_object_unref (priv->monitor);
			}
			priv->monitor = TNY_FOLDER_MONITOR (tny_folder_monitor_new (merge));
			tny_folder_monitor_add_list (priv->monitor, TNY_LIST (hmodel));
			tny_folder_monitor_start (priv->monitor);
		}
		g_mutex_unlock (priv->monitor_lock);

		sortable = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (hmodel));

		gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortable),
			TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_COLUMN,
			tny_gtk_header_list_model_received_date_sort_func, 
			NULL, NULL);

		gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortable),
			TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_COLUMN,
			tny_gtk_header_list_model_sent_date_sort_func, 
			NULL, NULL);

		set_header_view_model (header_view, sortable);

		tny_folder_refresh_async (merge, 
			refresh_current_folder, 
			status_update, self);

		g_list_free (list);
	}

	return;
}


static void
on_header_view_tree_row_activated (GtkTreeView *treeview, GtkTreePath *path,
			GtkTreeViewColumn *col,  gpointer user_data)
{
	TnySummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model(treeview);

	if (G_LIKELY (gtk_tree_model_get_iter(model, &iter, path)))
	{
		TnyHeader *header;

		gtk_tree_model_get (model, &iter, 
			TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, 
			&header, -1);
		
		if (G_LIKELY (header))
		{
			TnyFolder *folder = NULL;
			TnyPlatformFactory *platfact = NULL;


#if PLATFORM==1
			platfact = tny_gnome_platform_factory_get_instance ();    
#endif

#if PLATFORM==2
			platfact = tny_maemo_platform_factory_get_instance ();    
#endif

#if PLATFORM==3
			platfact = tny_gpe_platform_factory_get_instance ();
#endif

#if PLATFORM==4
			platfact = tny_olpc_platform_factory_get_instance ();    
#endif

			folder = tny_header_get_folder (TNY_HEADER (header));

			if (G_LIKELY (folder))
			{
				OnGetMsgInfo *info = g_slice_new (OnGetMsgInfo);
				info->msg_view = TNY_MSG_VIEW (tny_gtk_msg_window_new (tny_platform_factory_new_msg_view (platfact)));

				if (TNY_IS_GTK_MSG_VIEW (info->msg_view))
					tny_gtk_msg_view_set_status_callback (TNY_GTK_MSG_VIEW (info->msg_view), status_update, self);

				g_object_ref (info->msg_view);
				gtk_widget_show (GTK_WIDGET (info->msg_view));
				info->self = TNY_DEMOUI_SUMMARY_VIEW (g_object_ref (self));
				info->header = TNY_HEADER (g_object_ref (header));
				gtk_widget_show (GTK_WIDGET (priv->progress));
				tny_folder_get_msg_async (folder, header, 
					on_get_msg, status_update_on_get_msg, info);
				g_object_unref (G_OBJECT (folder));
			}
			g_object_unref (G_OBJECT (header));
		}
	}
}


static void
on_rename (TnyFolder *self, gboolean cancelled, TnyFolderStore *into, TnyFolder *new_folder, GError *err, gpointer user_data)
{
	GObject *mself = user_data;

	if (err != NULL)
	{
		GtkWidget *edialog;
		edialog = gtk_message_dialog_new (
			  GTK_WINDOW (gtk_widget_get_parent (GTK_WIDGET (mself))),
			  GTK_DIALOG_DESTROY_WITH_PARENT,
			  GTK_MESSAGE_ERROR,
			  GTK_BUTTONS_CLOSE,
			  err->message);
		g_signal_connect_swapped (edialog, "response",
			G_CALLBACK (gtk_widget_destroy), edialog);
		gtk_widget_show_all (edialog);
	} else {
	}

	g_object_unref (mself);
}

static void
fetch_folder_store_from_path (const gchar *path, TnyFolder *origin, gchar **name, TnyFolderStore **into)
{
	TnyFolderStore *origin_store;
	*name = NULL;
	*into = NULL;

	if (path == NULL || path[0] == '\0') {
		return;
	}

	if (path[0] == '/') {
		origin_store = TNY_FOLDER_STORE (tny_folder_get_account (origin));
		path++;
	} else {
		origin_store = tny_folder_get_folder_store (origin);
	}

	while (*path != '\0') {
		char *slash_pos;
		if (strncmp (path, "../", 3)== 0) {
			if (!TNY_IS_ACCOUNT (origin_store)) {
				TnyFolderStore *tmp;
				tmp = tny_folder_get_folder_store (TNY_FOLDER (origin_store));
				if (tmp) {
					g_object_unref (origin_store);
					origin_store = tmp;
				}
			}
			path += 3;
		} else if ((slash_pos = strstr (path, "/")) != NULL) {
			TnyFolderStoreQuery *query;
			TnyList *list;
			gchar *folder_name;
			
			query = tny_folder_store_query_new ();
			folder_name = g_strndup (path, slash_pos - path);
			tny_folder_store_query_add_item (query, folder_name, TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_NAME);
			g_free (folder_name);
			list = tny_simple_list_new ();
			tny_folder_store_get_folders (origin_store, list, query, TRUE, NULL);
			g_object_unref (query);
			if (tny_list_get_length (list) == 1) {
				TnyIterator *iter;
				TnyFolderStore *tmp;
				iter = tny_list_create_iterator (list);
				tmp = TNY_FOLDER_STORE (tny_iterator_get_current (iter));
				g_object_unref (origin_store);
				origin_store = tmp;
				g_object_unref (iter);
			}
			g_object_unref (list);
			path = slash_pos + 1;
		} else {
			*name = g_strdup (path);
			*into = origin_store;
			break;
		}
	}
}

static void 
on_rename_folder_activate (GtkMenuItem *mitem, gpointer user_data)
{
	TnyDemouiSummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected (priv->mailbox_select, &model, &iter))
	{
		gint type;

		gtk_tree_model_get (model, &iter, 
			TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, 
			&type, -1);

		if (type != TNY_FOLDER_TYPE_ROOT) 
		{ 
			TnyFolder *folder;
			GtkWidget *dialog, *entry;
			gint result;

			gtk_tree_model_get (model, &iter, 
				TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, 
				&folder, -1);

			dialog = gtk_dialog_new_with_buttons (_("Rename or copy a folder (first OK=rename)"),
							  GTK_WINDOW (gtk_widget_get_parent (GTK_WIDGET (self))),
							  GTK_DIALOG_MODAL,
							  GTK_STOCK_OK,
							  GTK_RESPONSE_ACCEPT,
							  GTK_STOCK_COPY,
							  GTK_RESPONSE_YES,
							  GTK_STOCK_CANCEL,
							  GTK_RESPONSE_REJECT,
							  NULL);

			entry = gtk_entry_new ();
			gtk_entry_set_text (GTK_ENTRY (entry), tny_folder_get_name (folder));
			gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), entry);
			gtk_widget_show (entry);

			result = gtk_dialog_run (GTK_DIALOG (dialog));

			switch (result)
			{
				case GTK_RESPONSE_YES:
				case GTK_RESPONSE_ACCEPT: 
				{
					gboolean move = (result == GTK_RESPONSE_ACCEPT);
					GError *err = NULL;
					gchar *entry_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
					gchar *new_name;
					TnyFolderStore *into;

					clear_header_view (priv);

					
					fetch_folder_store_from_path (entry_value, folder, &new_name, &into);

					gtk_widget_destroy (dialog);
					dialog = NULL;

					if (new_name != NULL) {

						tny_folder_copy_async (folder, into, new_name, move, 
								       on_rename, NULL, g_object_ref (self));

						g_free (new_name);
					}

					if (into)
						g_object_unref (into);


				}
				break;

				default:
				break;
			}

			if (dialog)
				gtk_widget_destroy (dialog);
			g_object_unref (G_OBJECT (folder));
		}

	}

}

static void 
on_delete_folder_activate (GtkMenuItem *mitem, gpointer user_data)
{
	TnyDemouiSummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected (priv->mailbox_select, &model, &iter))
	{
		gint type;

		gtk_tree_model_get (model, &iter, 
			TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, 
			&type, -1);

		if (type != TNY_FOLDER_TYPE_ROOT) 
		{ 
			TnyFolder *folder;
			GtkWidget *dialog, *label;
			gint result;

			gtk_tree_model_get (model, &iter, 
				TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, 
				&folder, -1);

			dialog = gtk_dialog_new_with_buttons (_("Delete a folder"),
												  GTK_WINDOW (gtk_widget_get_parent (GTK_WIDGET (self))),
												  GTK_DIALOG_MODAL,
												  GTK_STOCK_OK,
												  GTK_RESPONSE_ACCEPT,
												  GTK_STOCK_CANCEL,
												  GTK_RESPONSE_REJECT,
												  NULL);

			label = gtk_label_new (_("Are you sure you want to delete this folder?"));
			gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
			gtk_widget_show (label);

			result = gtk_dialog_run (GTK_DIALOG (dialog));

			switch (result)
			{
				case GTK_RESPONSE_ACCEPT: 
				{
					GError *err = NULL;
					TnyFolderStore *folderstore = tny_folder_get_folder_store (folder);

					tny_folder_store_remove_folder (folderstore, folder, &err);

					if (err != NULL)
					{
						GtkWidget *edialog;

						gtk_widget_destroy (dialog);
						dialog = NULL;

						edialog = gtk_message_dialog_new (
										  GTK_WINDOW (gtk_widget_get_parent (GTK_WIDGET (self))),
										  GTK_DIALOG_DESTROY_WITH_PARENT,
										  GTK_MESSAGE_ERROR,
										  GTK_BUTTONS_CLOSE,
										  err->message);
						g_signal_connect_swapped (edialog, "response",
							G_CALLBACK (gtk_widget_destroy), edialog);
						gtk_widget_show_all (edialog);
						g_error_free (err);
					}

					g_object_unref (G_OBJECT (folderstore));
				}
				break;

				default:
				break;
			}

			if (dialog)
				gtk_widget_destroy (dialog);
			g_object_unref (G_OBJECT (folder));
		}

	}
}


static void 
on_uncache_account_activate (GtkMenuItem *mitem, gpointer user_data)
{
	TnyDemouiSummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected (priv->mailbox_select, &model, &iter))
	{
		gint type;

		gtk_tree_model_get (model, &iter, 
			TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, 
			&type, -1);

		if (type != TNY_FOLDER_TYPE_ROOT) 
		{ 
			TnyFolder *folder;
			TnyStoreAccount *acc;
			GtkWidget *dialog, *label;
			gint result;
			gchar *str = NULL;

			gtk_tree_model_get (model, &iter, 
				TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, 
				&folder, -1);

			dialog = gtk_dialog_new_with_buttons (_("Uncache an account"),
				  GTK_WINDOW (gtk_widget_get_parent (GTK_WIDGET (self))),
				  GTK_DIALOG_MODAL,
				  GTK_STOCK_OK,
				  GTK_RESPONSE_ACCEPT,
				  GTK_STOCK_CANCEL,
				  GTK_RESPONSE_REJECT,
				  NULL);

			acc = TNY_STORE_ACCOUNT (tny_folder_get_account (folder));

			str = g_strdup_printf (_("Are you sure you want to uncache the account %s?"),
				tny_account_get_name (TNY_ACCOUNT (acc)));
			label = gtk_label_new (str);
			g_free (str);

			gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
			gtk_widget_show (label);

			result = gtk_dialog_run (GTK_DIALOG (dialog));

			switch (result)
			{
				case GTK_RESPONSE_ACCEPT: 
					tny_store_account_delete_cache (acc);
				break;

				default:
				break;
			}

			g_object_unref (acc);

			if (dialog)
				gtk_widget_destroy (dialog);
			g_object_unref (folder);
		}

	}
}

static void
sync_button_clicked (GtkWidget *button, gpointer user_data)
{
	TnyDemouiSummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected (priv->mailbox_select, &model, &iter))
	{
		gint type;

		gtk_tree_model_get (model, &iter, 
			TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, 
			&type, -1);

		if (type != TNY_FOLDER_TYPE_ROOT) 
		{ 
			TnyFolder *folder;
			gtk_tree_model_get (model, &iter, 
				TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, 
				&folder, -1);

			if (TNY_IS_FOLDER (folder))
				tny_folder_sync_async (folder, TRUE, NULL, NULL, NULL);

			g_object_unref (folder);
		}

	}
}



static void
cancel_button_clicked (GtkWidget *button, gpointer user_data)
{
	TnyDemouiSummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected (priv->mailbox_select, &model, &iter))
	{
		gint type;
		TnyAccount *account = NULL;

		gtk_tree_model_get (model, &iter, 
			TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, 
			&type, -1);

		if (type != TNY_FOLDER_TYPE_ROOT) 
		{ 
			TnyFolder *folder;

			gtk_tree_model_get (model, &iter, 
				TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, 
				&folder, -1);

			if (TNY_IS_FOLDER (folder))
				account = tny_folder_get_account (folder);

			g_object_unref (folder);

		} else {
			gtk_tree_model_get (model, &iter, 
				TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, 
				&account, -1);
		}

		if (account) {
			tny_account_cancel (account);
			g_object_unref (account);
		}
	}
}

static void
create_cb (TnyFolderStore *store, gboolean cancelled, TnyFolder *new_folder, GError *err, gpointer user_data)
{
	TnyDemouiSummaryView *self = user_data;
	if (err != NULL) {
		GtkWidget *edialog;
		edialog = gtk_message_dialog_new (
						  GTK_WINDOW (gtk_widget_get_parent (GTK_WIDGET (self))),
						  GTK_DIALOG_DESTROY_WITH_PARENT,
						  GTK_MESSAGE_ERROR,
						  GTK_BUTTONS_CLOSE,
						  err->message);
		g_signal_connect_swapped (edialog, "response",
			G_CALLBACK (gtk_widget_destroy), edialog);
		gtk_widget_show_all (edialog);
	}

	g_object_unref (self);
}
static void 
on_create_folder_activate (GtkMenuItem *mitem, gpointer user_data)
{
	TnyDemouiSummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected (priv->mailbox_select, &model, &iter))
	{
		gint type;

		gtk_tree_model_get (model, &iter, 
			TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, 
			&type, -1);

		if (type != TNY_FOLDER_TYPE_ROOT) 
		{ 
			TnyFolderStore *folderstore;
			GtkWidget *dialog, *entry;
			gint result;

			gtk_tree_model_get (model, &iter, 
				TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, 
				&folderstore, -1);

			dialog = gtk_dialog_new_with_buttons (_("Create a folder"),
					  GTK_WINDOW (gtk_widget_get_parent (GTK_WIDGET (self))),
					  GTK_DIALOG_MODAL,
					  GTK_STOCK_OK,
					  GTK_RESPONSE_ACCEPT,
					  GTK_STOCK_CANCEL,
					  GTK_RESPONSE_REJECT,
					  NULL);

			entry = gtk_entry_new ();
			gtk_entry_set_text (GTK_ENTRY (entry), _("New folder"));
			gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), entry);
			gtk_widget_show (entry);

			result = gtk_dialog_run (GTK_DIALOG (dialog));

			switch (result)
			{
				case GTK_RESPONSE_ACCEPT: 
				{
					GError *err = NULL;
					const gchar *newname = gtk_entry_get_text (GTK_ENTRY (entry));

					tny_folder_store_create_folder_async (folderstore, newname, 
						create_cb, status_update, 
						g_object_ref (self));

				}
				break;

				default:
				break;
			}

			if (dialog)
				gtk_widget_destroy (dialog);
			g_object_unref (folderstore);
		}

	}
}


static void 
on_merge_view_activate (GtkMenuItem *mitem, gpointer user_data)
{
	TnyDemouiSummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	GtkSelectionMode mode;

	mode = gtk_tree_selection_get_mode (priv->mailbox_select);

	if (mode == GTK_SELECTION_SINGLE)
		gtk_tree_selection_set_mode (priv->mailbox_select, GTK_SELECTION_MULTIPLE);
	else
		gtk_tree_selection_set_mode (priv->mailbox_select, GTK_SELECTION_SINGLE);

}


typedef struct {
	TnyHeader *header;
	TnyFolder *to_folder;
} OnMoveToFolderInfo;

static void 
on_move_to_folder_activate (GtkMenuItem *mitem, gpointer user_data)
{
	OnMoveToFolderInfo *info = user_data;
	TnyFolder *folder = tny_header_get_folder (info->header);

	if (folder) 
	{
		gchar *subject;
		TnyList *headers = tny_simple_list_new ();

		subject = tny_header_dup_subject (info->header);

		g_print ("Transfering: %s to %s\n",
			subject, 
			tny_folder_get_name (info->to_folder));
		g_free (subject);

		tny_list_prepend (headers, (GObject *) info->header);
		tny_folder_transfer_msgs_async (folder, headers, info->to_folder, 
			TRUE, NULL, NULL, user_data);
		g_object_unref (headers);
	}

	return;
}

static void
recursive_all_folders (GtkWidget *my_widget, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data, GtkWidget *menu)
{
	TnyDemouiSummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);

	do {
		GtkTreeIter niter;
		GObject *citem;
		gchar *line;

		gtk_tree_model_get (model, iter, 
			TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, 
			&citem, TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, &line, -1);

		if (TNY_IS_FOLDER (citem))
		{
			GtkTreeIter hiter;
			GtkTreeModel *hmodel = gtk_tree_view_get_model (priv->header_view);
			GtkTreeSelection *sel = gtk_tree_view_get_selection (priv->header_view);

			if (gtk_tree_selection_get_selected (sel, &hmodel, &hiter))
			{
				/* Small leak, indeed */
				OnMoveToFolderInfo *info = g_new0 (OnMoveToFolderInfo, 1);
				GtkWidget *menuitem = NULL;

				info->to_folder = (TnyFolder *) citem;
				gtk_tree_model_get (hmodel, &hiter, 
					TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, 
					&info->header, -1);

				menuitem = gtk_menu_item_new_with_label (tny_folder_get_name ((TnyFolder *)citem));

				g_signal_connect (G_OBJECT (menuitem), "activate",
					G_CALLBACK (on_move_to_folder_activate), info);

				gtk_widget_show (menuitem);
				gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
			}
		}

		g_object_unref (citem);

		if (gtk_tree_model_iter_children (model, &niter, iter)) {
			recursive_all_folders (my_widget, model, &niter, user_data, menu);
		}

	} while (gtk_tree_model_iter_next (model, iter));
}

static void
header_view_do_popup_menu (GtkWidget *my_widget, GdkEventButton *event, gpointer user_data)
{
	TnyDemouiSummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);

	GtkWidget *menu;
	int button, event_time;
	GtkTreeView *view = priv->mailbox_view;
	GtkTreeModel *model = gtk_tree_view_get_model (view);

	menu = gtk_menu_new ();

	if (model) {
		GtkTreeIter iter;
		gtk_tree_model_get_iter_first (model, &iter);
		recursive_all_folders (my_widget, model, &iter, user_data, menu);
	}

	if (event)
	{
		button = event->button;
		event_time = event->time;
	}
	else
	{
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_attach_to_widget (GTK_MENU (menu), my_widget, NULL);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 
					button, event_time);
}

static gboolean 
on_header_view_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
	{
		header_view_do_popup_menu (widget, event, user_data);
		return TRUE;
	}

	return FALSE;
}

static gboolean
header_view_popup_menu_event (GtkWidget *widget, gpointer user_data)
{
	header_view_do_popup_menu (widget, NULL, user_data);
	return TRUE;
}


static void
mailbox_view_do_popup_menu (GtkWidget *my_widget, GdkEventButton *event, gpointer user_data)
{
	TnyDemouiSummaryView *self = user_data;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	GtkWidget *menu;
	int button, event_time;
	GtkSelectionMode mode;
	GtkWidget *mrename, *mdelete, *mcreate, *mmerge, *muncache;

	menu = gtk_menu_new ();

	mrename = gtk_menu_item_new_with_label (_("Rename or Copy folder"));
	mcreate = gtk_menu_item_new_with_label (_("Create folder"));
	mdelete = gtk_menu_item_new_with_label (_("Delete folder"));
	muncache = gtk_menu_item_new_with_label (_("Uncache account of this folder"));

	mode = gtk_tree_selection_get_mode (priv->mailbox_select);

	if (mode == GTK_SELECTION_SINGLE)
		mmerge = gtk_menu_item_new_with_label (_("Merge view of selected mode"));
	else
		mmerge = gtk_menu_item_new_with_label (_("Select one folder mode"));

	g_signal_connect (G_OBJECT (muncache), "activate",
		G_CALLBACK (on_uncache_account_activate), user_data);
	g_signal_connect (G_OBJECT (mrename), "activate",
		G_CALLBACK (on_rename_folder_activate), user_data);
	g_signal_connect (G_OBJECT (mcreate), "activate",
		G_CALLBACK (on_create_folder_activate), user_data);
	g_signal_connect (G_OBJECT (mdelete), "activate",
		G_CALLBACK (on_delete_folder_activate), user_data);
	g_signal_connect (G_OBJECT (mmerge), "activate",
		G_CALLBACK (on_merge_view_activate), user_data);

	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), mrename);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), mcreate);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), mdelete);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), muncache);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), mmerge);
	gtk_widget_show (mrename);
	gtk_widget_show (mcreate);
	gtk_widget_show (mdelete);
	gtk_widget_show (muncache);
	gtk_widget_show (mmerge);

	if (event)
	{
		button = event->button;
		event_time = event->time;
	}
	else
	{
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_attach_to_widget (GTK_MENU (menu), my_widget, NULL);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 
					button, event_time);
}

static gboolean 
on_mailbox_view_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
	{
		mailbox_view_do_popup_menu (widget, event, user_data);
		return TRUE;
	}

	return FALSE;
}

static gboolean
mailbox_view_popup_menu_event (GtkWidget *widget, gpointer user_data)
{
	mailbox_view_do_popup_menu (widget, NULL, user_data);
	return FALSE;
}

/**
 * tny_demoui_summary_view_new:
 * 
 *
 * Return value: A new #TnySummaryView instance implemented for Gtk+
 **/
TnySummaryView*
tny_demoui_summary_view_new (void)
{
	TnyDemouiSummaryView *self = g_object_new (TNY_TYPE_DEMOUI_SUMMARY_VIEW, NULL);

	return TNY_SUMMARY_VIEW (self);
}

static void
tny_demoui_summary_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyDemouiSummaryView *self = (TnyDemouiSummaryView *)instance;
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);
	TnyPlatformFactory *platfact;
	GtkVBox *vbox = GTK_VBOX (self);
	GtkWidget *mailbox_sw, *widget, *cancel_button;
	GtkWidget *header_sw;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	GtkWidget *hpaned1;
	GtkWidget *vpaned1;
	GtkBox *vpbox;

	/* TODO: Persist application UI status (of the panes) */

	priv->handle_recon = TRUE;
	priv->monitor_lock = g_mutex_new ();
	priv->monitor = NULL;

	priv->send_queue = NULL;
	priv->last_mailbox_correct_select_set = FALSE;
	priv->online_button = gtk_toggle_button_new_with_label (GO_ONLINE_TXT);
	priv->poke_button = gtk_button_new_with_label ("Poke status");
	priv->sync_button = gtk_button_new_with_label ("Sync");
	priv->killacc_button = gtk_button_new_with_label ("Kill account");

	cancel_button = gtk_button_new_with_label ("Cancel");


	priv->current_accounts = NULL;

	priv->online_button_signal = g_signal_connect (G_OBJECT (priv->online_button), "toggled", 
		G_CALLBACK (online_button_toggled), self);

	g_signal_connect (G_OBJECT (priv->poke_button), "clicked", 
		G_CALLBACK (poke_button_toggled), self);

	g_signal_connect (G_OBJECT (cancel_button), "clicked", 
		G_CALLBACK (cancel_button_clicked), self);

	g_signal_connect (G_OBJECT (priv->sync_button), "clicked", 
		G_CALLBACK (sync_button_clicked), self);

	g_signal_connect (G_OBJECT (priv->killacc_button), "clicked", 
		G_CALLBACK (killacc_button_toggled), self);

#if PLATFORM==1
	platfact = tny_gnome_platform_factory_get_instance ();
#endif

#if PLATFORM==2
	platfact = tny_maemo_platform_factory_get_instance ();
#endif

#if PLATFORM==3
	platfact = tny_gpe_platform_factory_get_instance ();
#endif

#if PLATFORM==4
	platfact = tny_olpc_platform_factory_get_instance ();
#endif


	hpaned1 = gtk_hpaned_new ();
	gtk_widget_show (hpaned1);
	priv->status = GTK_WIDGET (gtk_statusbar_new ());
	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (priv->status), FALSE);
	priv->progress = gtk_progress_bar_new ();
	priv->status_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->status), "default");

	gtk_box_pack_start (GTK_BOX (priv->status), priv->progress, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->status), priv->online_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->status), priv->poke_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->status), priv->sync_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->status), priv->killacc_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->status), cancel_button, FALSE, FALSE, 0);

	gtk_widget_show (cancel_button);
	gtk_widget_show (priv->online_button);
	gtk_widget_show (priv->poke_button);
	gtk_widget_show (priv->killacc_button);
	gtk_widget_show (priv->sync_button);
	gtk_widget_show (priv->status);
	gtk_widget_show (GTK_WIDGET (vbox));

	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hpaned1), TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (priv->status), FALSE, TRUE, 0);

	vpaned1 = gtk_vpaned_new ();
	gtk_widget_show (vpaned1);

	priv->msg_view = tny_platform_factory_new_msg_view (platfact);

	tny_gtk_msg_view_set_status_callback (TNY_GTK_MSG_VIEW (priv->msg_view), status_update, self);

	gtk_widget_show (GTK_WIDGET (priv->msg_view));

	widget = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget), 
				GTK_SHADOW_NONE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (widget), 
			GTK_WIDGET (priv->msg_view));

	gtk_widget_show (widget);

	gtk_paned_pack2 (GTK_PANED (vpaned1), widget, TRUE, TRUE);

	priv->account_view = GTK_COMBO_BOX (gtk_combo_box_new ());
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start((GtkCellLayout *)priv->account_view, renderer, TRUE);
	gtk_cell_layout_set_attributes((GtkCellLayout *)priv->account_view, renderer, "text", 0, NULL);

	mailbox_sw = gtk_scrolled_window_new (NULL, NULL);
	vpbox = GTK_BOX (gtk_vbox_new (FALSE, 0));
	gtk_box_pack_start (GTK_BOX (vpbox), GTK_WIDGET (priv->account_view), FALSE, FALSE, 0);    
	gtk_box_pack_start (GTK_BOX (vpbox), mailbox_sw, TRUE, TRUE, 0);
	gtk_widget_show (GTK_WIDGET (vpbox));    
	gtk_widget_show (GTK_WIDGET (priv->account_view));

	gtk_paned_pack1 (GTK_PANED (hpaned1), GTK_WIDGET (vpbox), TRUE, TRUE);
	gtk_paned_pack2 (GTK_PANED (hpaned1), vpaned1, TRUE, TRUE);
	gtk_widget_show (GTK_WIDGET (mailbox_sw));

	header_sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_paned_pack1 (GTK_PANED (vpaned1), header_sw, TRUE, TRUE);
	gtk_widget_show (GTK_WIDGET (header_sw));

	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (header_sw), 
			GTK_SHADOW_ETCHED_IN);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (header_sw),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (mailbox_sw),
		GTK_SHADOW_ETCHED_IN);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (mailbox_sw),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	
	priv->header_view = GTK_TREE_VIEW (gtk_tree_view_new ());
	gtk_widget_show (GTK_WIDGET (priv->header_view));

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (priv->header_view), TRUE);
	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW(priv->header_view), TRUE);

	priv->mailbox_view = GTK_TREE_VIEW (gtk_tree_view_new ());
	gtk_widget_show (GTK_WIDGET (priv->mailbox_view));

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (priv->mailbox_view), TRUE);
	
	gtk_container_add (GTK_CONTAINER (header_sw), GTK_WIDGET (priv->header_view));
	gtk_container_add (GTK_CONTAINER (mailbox_sw), GTK_WIDGET (priv->mailbox_view));


	/* TODO: Persist application UI status */
	/* mailbox_view columns */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Folder"), renderer,
			"text", TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id (column, TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN);

	gtk_tree_view_append_column (GTK_TREE_VIEW(priv->mailbox_view), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Unread"), renderer,
		"text", TNY_GTK_FOLDER_STORE_TREE_MODEL_UNREAD_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id (column, TNY_GTK_FOLDER_STORE_TREE_MODEL_UNREAD_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW(priv->mailbox_view), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Total"), renderer,
		"text", TNY_GTK_FOLDER_STORE_TREE_MODEL_ALL_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id (column, TNY_GTK_FOLDER_STORE_TREE_MODEL_ALL_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW(priv->mailbox_view), column);

	/* TODO: Persist application UI status */
	/* header_view columns */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("From"), renderer,
		"text", TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id (column, TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 230);
	gtk_tree_view_append_column (GTK_TREE_VIEW(priv->header_view), column);

	if (G_UNLIKELY (FALSE))
	{ /* Unlikely ;-) */
		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes (_("To"), renderer,
			"text", TNY_GTK_HEADER_LIST_MODEL_TO_COLUMN, NULL);
		gtk_tree_view_column_set_sort_column_id (column, TNY_GTK_HEADER_LIST_MODEL_TO_COLUMN);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_fixed_width (column, 100);
		gtk_tree_view_append_column (GTK_TREE_VIEW(priv->header_view), column);
	}

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Subject"), renderer,
		"text", TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id (column, TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 200);
	gtk_tree_view_append_column (GTK_TREE_VIEW(priv->header_view), column);


	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Date sent"), renderer,
		"text", TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id (column, TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 100);
	gtk_tree_view_append_column (GTK_TREE_VIEW(priv->header_view), column);


	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Flags"), renderer,
		"text", TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id (column, TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 100);
	gtk_tree_view_append_column (GTK_TREE_VIEW(priv->header_view), column);


	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Size"), renderer,
		"text", TNY_GTK_HEADER_LIST_MODEL_MESSAGE_SIZE_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id (column, TNY_GTK_HEADER_LIST_MODEL_MESSAGE_SIZE_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 100);
	gtk_tree_view_append_column (GTK_TREE_VIEW(priv->header_view), column);

	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->mailbox_view));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	priv->mailbox_select = select;
	priv->mailbox_select_sid = g_signal_connect (G_OBJECT (select), "changed",
		G_CALLBACK (on_mailbox_view_tree_selection_changed), self);


	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->header_view));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT (priv->header_view), "row-activated", 
		G_CALLBACK (on_header_view_tree_row_activated), self);

	g_signal_connect(G_OBJECT (priv->header_view), "key-press-event", 
		G_CALLBACK (on_header_view_key_press_event), self);

	g_signal_connect (G_OBJECT (select), "changed",
		G_CALLBACK (on_header_view_tree_selection_changed), self);


	g_signal_connect (G_OBJECT (priv->header_view), "button-press-event",
		G_CALLBACK (on_header_view_button_press_event), self);
	g_signal_connect (G_OBJECT (priv->header_view), "popup-menu",
		G_CALLBACK (header_view_popup_menu_event), self);

	g_signal_connect (G_OBJECT (priv->mailbox_view), "button-press-event",
		G_CALLBACK (on_mailbox_view_button_press_event), self);
	g_signal_connect (G_OBJECT (priv->mailbox_view), "popup-menu",
		G_CALLBACK (mailbox_view_popup_menu_event), self);

	gtk_widget_hide (priv->progress);

	return;
}

static void
tny_demoui_summary_view_finalize (GObject *object)
{
	TnyDemouiSummaryView *self = (TnyDemouiSummaryView *)object;	
	TnyDemouiSummaryViewPriv *priv = TNY_DEMOUI_SUMMARY_VIEW_GET_PRIVATE (self);

	if (priv->current_accounts)
		g_object_unref (priv->current_accounts);

	if (priv->account_store)
		g_object_unref (priv->account_store);

	g_mutex_lock (priv->monitor_lock);
	if (priv->monitor)
	{
		tny_folder_monitor_stop (priv->monitor);
		g_object_unref (priv->monitor);
		priv->monitor = NULL;
	}
	g_mutex_unlock (priv->monitor_lock);

	g_mutex_free (priv->monitor_lock);

	(*parent_class->finalize) (object);

	return;
}

static void
tny_summary_view_init (gpointer g, gpointer iface_data)
{
	return;
}

static void
tny_account_store_view_init (gpointer g, gpointer iface_data)
{
	TnyAccountStoreViewIface *klass = (TnyAccountStoreViewIface *)g;

	klass->set_account_store= tny_demoui_summary_view_set_account_store;

	return;
}

static void 
tny_demoui_summary_view_class_init (TnyDemouiSummaryViewClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_demoui_summary_view_finalize;

	g_type_class_add_private (object_class, sizeof (TnyDemouiSummaryViewPriv));

	return;
}

GType 
tny_demoui_summary_view_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
		  sizeof (TnyDemouiSummaryViewClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_demoui_summary_view_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyDemouiSummaryView),
		  0,      /* n_preallocs */
		  tny_demoui_summary_view_instance_init    /* instance_init */
		};

		static const GInterfaceInfo tny_summary_view_info = 
		{
		  (GInterfaceInitFunc) tny_summary_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		static const GInterfaceInfo tny_account_store_view_info = 
		{
		  (GInterfaceInitFunc) tny_account_store_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		type = g_type_register_static (GTK_TYPE_VBOX,
			"TnyDemouiSummaryView",
			&info, 0);

		g_type_add_interface_static (type, TNY_TYPE_SUMMARY_VIEW, 
			&tny_summary_view_info);

		g_type_add_interface_static (type, TNY_TYPE_ACCOUNT_STORE_VIEW, 
			&tny_account_store_view_info);

	}

	return type;
}
