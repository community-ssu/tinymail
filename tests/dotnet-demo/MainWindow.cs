using System;
using Gtk;

public partial class MainWindow: Gtk.Window
{	

	private Tny.Ui.MsgView msg_view;
	private Tny.Folder cur_folder = null;
	private Tny.AccountStore store = null;
	
	public MainWindow (): base (Gtk.WindowType.Toplevel)
	{
		Tny.Platform.Init.Initialize();
		Build ();
		PrepareUI ();
	}

#region UI stuff
	private void PrepareUI () 
	{
		this.msg_view = new Tny.Ui.GTK.MsgView();
		((Gtk.Widget) this.msg_view).Show ();
		this.msg_scrolledwindow.AddWithViewport ((Gtk.Widget) this.msg_view);

		Gtk.TreeViewColumn FolderColumn = new Gtk.TreeViewColumn ();
		Gtk.CellRendererText FolderNameCell = new Gtk.CellRendererText ();
		FolderColumn.PackStart (FolderNameCell, true);
		FolderColumn.AddAttribute (FolderNameCell, "text", (int) Tny.Ui.GTK.FolderStoreTreeModelColumn.NameColumn);		
		FolderColumn.Title = "Folder";
		this.folders_treeview.AppendColumn (FolderColumn);

		Gtk.TreeViewColumn FromColumn = new Gtk.TreeViewColumn ();
		Gtk.CellRendererText FromNameCell = new Gtk.CellRendererText ();
		FromColumn.PackStart (FromNameCell, true);
		FromColumn.AddAttribute (FromNameCell, "text", (int) Tny.Ui.GTK.HeaderListModelColumn.FromColumn);		
		FromColumn.Title = "From";
		this.headers_treeview.AppendColumn (FromColumn);
	
		Gtk.TreeViewColumn SubjectColumn = new Gtk.TreeViewColumn ();
		Gtk.CellRendererText SubjectNameCell = new Gtk.CellRendererText ();
		SubjectColumn.PackStart (SubjectNameCell, true);
		SubjectColumn.AddAttribute (SubjectNameCell, "text", (int) Tny.Ui.GTK.HeaderListModelColumn.SubjectColumn);		
		SubjectColumn.Title = "Subject";
		this.headers_treeview.AppendColumn (SubjectColumn);

		this.headers_treeview.Selection.Changed += OnMailSelected;
		this.folders_treeview.Selection.Changed += OnFolderChanged;
	}
#endregion	

	protected void OnDeleteEvent (object sender, DeleteEventArgs a)
	{
		Application.Quit ();
		a.RetVal = true;
	}

	private void GetHeadersCallback (Tny.Folder folder, bool cancel, Tny.List model, Tny.TnyException ex)
	{
		Console.WriteLine (ex.Message);

		if (model != null && !cancel)
			this.headers_treeview.Model = (Gtk.TreeModel) model;
	}
	
	private void StatusCallback (GLib.Object sender, Tny.Status status)
	{
		this.progressbar.Fraction = status.Fraction;
	}

	private void GetMsgCallBack (Tny.Folder folder, bool cancel, Tny.Msg msg, Tny.TnyException ex)
	{
		Console.WriteLine (ex.Message);

		if (msg != null && !cancel)
			this.msg_view.Msg = msg;
	}
	
			
	private void OnMailSelected (object o, EventArgs args)
	{
		Tny.Ui.GTK.HeaderListModel model = (o as Gtk.TreeSelection).TreeView.Model as Tny.Ui.GTK.HeaderListModel;
		Tny.Header header = model.GetHeader (o as Gtk.TreeSelection);
		
		if (header != null) {
			Console.WriteLine ("Message selected: " + header.From);	
			this.cur_folder.GetMsgAsync (header, GetMsgCallBack, StatusCallback);
		}	
	}
	
	private void OnFolderChanged (object o, EventArgs args)
	{
		Tny.Ui.GTK.FolderStoreTreeModel model = (o as Gtk.TreeSelection).TreeView.Model as Tny.Ui.GTK.FolderStoreTreeModel;
		Tny.Folder folder = model.GetFolder (o as Gtk.TreeSelection);
		
		if (folder != null) {
			Tny.Ui.GTK.HeaderListModel headers_model = new Tny.Ui.GTK.HeaderListModel();
			Console.WriteLine ("Folder selected: " + folder.Name);	
			this.cur_folder = folder;
			folder.GetHeadersAsync (headers_model, true, GetHeadersCallback, StatusCallback);
		}
	}

	
	private void OnConnectButtonClicked (object sender, System.EventArgs e)
	{
		// Tny.Ui.PlatformFactory factory = Tny.Platform.PlatformFactory.Instance;
		// this.store = factory.NewAccountStore ();

		this.store = new Tny.Platform.AccountStore ();
		Tny.Ui.GTK.FolderStoreTreeModel model = new Tny.Ui.GTK.FolderStoreTreeModel (new Tny.FolderStoreQuery ());
		store.GetAccounts (model, Tny.GetAccountsRequestType.StoreAccounts);		
		this.folders_treeview.Model = model;
	}
}
