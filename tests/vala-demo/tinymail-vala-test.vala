using GLib;
using Gtk;
using Tny;

public class Tny.ValaMailWindow : Window {
	private GtkMsgView msgview;
	private TreeView headerlist;
	private ProgressBar progressbar;

	public ValaMailWindow () {
		title = "Vala Mail Demo";
	}

	construct {
		CellRenderer renderer;
		TreeViewColumn column;

		// initialize Tinymail
		var platfact = GnomePlatformFactory.get_instance ();
		var account_store = platfact.new_account_store ();
		var device = account_store.get_device ();
		device.force_online ();

		var hpaned = new HPaned ();
		add (hpaned);

		// create scrollable folder tree
		var foldertree_scrolled = new ScrolledWindow (null, null) {
			hscrollbar_policy = PolicyType.AUTOMATIC,
			vscrollbar_policy = PolicyType.AUTOMATIC
		};
		hpaned.pack1 (foldertree_scrolled, false, true);
		var foldertree = new TreeView ();
		foldertree_scrolled.add (foldertree);

		renderer = new CellRendererText ();
		column = new TreeViewColumn.with_attributes ("Folder", renderer, "text", GtkFolderStoreTreeModelColumn.NAME_COLUMN);
		column.fixed_width = 100;
		column.sizing = TreeViewColumnSizing.FIXED;
		foldertree.append_column (column);

		// add all subscribed folders to tree
		var query = new FolderStoreQuery ();
		query.add_item ("", FolderStoreQueryOption.SUBSCRIBED);

		var accounts = new GtkFolderStoreTreeModel (query);
		account_store.get_accounts (accounts, GetAccountsRequestType.STORE_ACCOUNTS);
		foldertree.model = accounts;

		var vpaned = new VPaned ();
		hpaned.pack2 (vpaned, true, true);

		var vbox = new VBox (false, 0);
		vpaned.pack1 (vbox, true, true);

		// create scrollable header list
		var headerlist_scrolled = new ScrolledWindow (null, null) {
			hscrollbar_policy = PolicyType.AUTOMATIC,
			vscrollbar_policy = PolicyType.AUTOMATIC
		};
		vbox.pack_start (headerlist_scrolled, true, true, 0);
		headerlist = new TreeView ();
		headerlist_scrolled.add (headerlist);

		progressbar = new ProgressBar ();
		vbox.pack_start (progressbar, false, false, 0);

		renderer = new CellRendererText ();
		column = new TreeViewColumn.with_attributes ("From", renderer, "text", GtkHeaderListModelColumn.FROM_COLUMN);
		column.fixed_width = 100;
		column.sizing = TreeViewColumnSizing.FIXED;
		headerlist.append_column (column);

		renderer = new CellRendererText ();
		column = new TreeViewColumn.with_attributes ("Subject", renderer, "text", GtkHeaderListModelColumn.SUBJECT_COLUMN);
		column.fixed_width = 200;
		column.sizing = TreeViewColumnSizing.FIXED;
		headerlist.append_column (column);

		renderer = new CellRendererText ();
		column = new TreeViewColumn.with_attributes ("Received", renderer, "text", GtkHeaderListModelColumn.DATE_RECEIVED_COLUMN);
		column.fixed_width = 100;
		column.sizing = TreeViewColumnSizing.FIXED;
		headerlist.append_column (column);

		msgview = new GtkMsgView ();
		vpaned.pack2 (msgview, true, true);

		destroy += Gtk.main_quit;
		foldertree.get_selection ().changed += on_folder_selected;
		headerlist.get_selection ().changed += on_message_selected;

	}

	private void on_folder_selected (TreeSelection treeselection) {
		weak TreeModel model;
		TreeIter iter;
		treeselection.get_selected (out model, out iter);
		FolderType folder_type;
		GLib.Object folder_instance;
		model.get (iter, GtkFolderStoreTreeModelColumn.TYPE_COLUMN, out folder_type, GtkFolderStoreTreeModelColumn.INSTANCE_COLUMN, out folder_instance);
		if (folder_type != FolderType.ROOT) {
			var folder = (Folder) folder_instance;
			progressbar.show ();
			folder.refresh_async (on_refresh_folder, on_status);
		}
	}

	private void on_message_selected (TreeSelection treeselection) {
		weak TreeModel model;
		TreeIter iter;
		if (treeselection.get_selected (out model, out iter)) {
			Header header;
			model.get (iter, GtkHeaderListModelColumn.INSTANCE_COLUMN, out header);
			var folder = header.get_folder ();
			var msg = folder.get_msg (header);
			msgview.set_msg (msg);
		}
	}

	private void on_refresh_folder (Folder folder, bool cancelled, GLib.Error err) {
		var listm = new GtkHeaderListModel ();
		listm.set_folder (folder, false, null, null);
		headerlist.model = listm;
		progressbar.hide ();
	}

	private void on_status (GLib.Object obj, Tny.Status status) {
		progressbar.pulse ();
	}

	static void main (string[] args) {
		Gtk.init (ref args);

		var win = new ValaMailWindow ();
		win.show_all ();

		Gtk.main ();
	}
}

