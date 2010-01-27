<?xml version="1.0"?>
<api version="1.0">
	<namespace name="Tny">
		<enum name="TnyGtkAccountListModelColumn" type-name="TnyGtkAccountListModelColumn" get-type="tny_gtk_account_list_model_column_get_type">
			<member name="TNY_GTK_ACCOUNT_LIST_MODEL_NAME_COLUMN" value="0"/>
			<member name="TNY_GTK_ACCOUNT_LIST_MODEL_INSTANCE_COLUMN" value="1"/>
			<member name="TNY_GTK_ACCOUNT_LIST_MODEL_N_COLUMNS" value="2"/>
		</enum>
		<enum name="TnyGtkAttachListModelColumn" type-name="TnyGtkAttachListModelColumn" get-type="tny_gtk_attach_list_model_column_get_type">
			<member name="TNY_GTK_ATTACH_LIST_MODEL_PIXBUF_COLUMN" value="0"/>
			<member name="TNY_GTK_ATTACH_LIST_MODEL_FILENAME_COLUMN" value="1"/>
			<member name="TNY_GTK_ATTACH_LIST_MODEL_INSTANCE_COLUMN" value="2"/>
			<member name="TNY_GTK_ATTACH_LIST_MODEL_N_COLUMNS" value="3"/>
		</enum>
		<enum name="TnyGtkFolderStoreTreeModelColumn" type-name="TnyGtkFolderStoreTreeModelColumn" get-type="tny_gtk_folder_store_tree_model_column_get_type">
			<member name="TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN" value="0"/>
			<member name="TNY_GTK_FOLDER_STORE_TREE_MODEL_UNREAD_COLUMN" value="1"/>
			<member name="TNY_GTK_FOLDER_STORE_TREE_MODEL_ALL_COLUMN" value="2"/>
			<member name="TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN" value="3"/>
			<member name="TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN" value="4"/>
			<member name="TNY_GTK_FOLDER_STORE_TREE_MODEL_N_COLUMNS" value="5"/>
		</enum>
		<enum name="TnyGtkHeaderListModelColumn" type-name="TnyGtkHeaderListModelColumn" get-type="tny_gtk_header_list_model_column_get_type">
			<member name="TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN" value="0"/>
			<member name="TNY_GTK_HEADER_LIST_MODEL_TO_COLUMN" value="1"/>
			<member name="TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN" value="2"/>
			<member name="TNY_GTK_HEADER_LIST_MODEL_CC_COLUMN" value="3"/>
			<member name="TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_COLUMN" value="4"/>
			<member name="TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN" value="5"/>
			<member name="TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN" value="6"/>
			<member name="TNY_GTK_HEADER_LIST_MODEL_MESSAGE_SIZE_COLUMN" value="8"/>
			<member name="TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_COLUMN" value="7"/>
			<member name="TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN" value="9"/>
			<member name="TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN" value="10"/>
			<member name="TNY_GTK_HEADER_LIST_MODEL_N_COLUMNS" value="11"/>
		</enum>
		<object name="TnyGtkAccountListModel" parent="GtkListStore" type-name="TnyGtkAccountListModel" get-type="tny_gtk_account_list_model_get_type">
			<implements>
				<interface name="GtkBuildable"/>
				<interface name="GtkTreeModel"/>
				<interface name="GtkTreeDragSource"/>
				<interface name="GtkTreeDragDest"/>
				<interface name="GtkTreeSortable"/>
				<interface name="TnyList"/>
			</implements>
			<constructor name="new" symbol="tny_gtk_account_list_model_new">
				<return-type type="GtkTreeModel*"/>
			</constructor>
			<field name="iterator_lock" type="GMutex*"/>
			<field name="first" type="GList*"/>
			<field name="first_needs_unref" type="gboolean"/>
		</object>
		<object name="TnyGtkAttachListModel" parent="GtkListStore" type-name="TnyGtkAttachListModel" get-type="tny_gtk_attach_list_model_get_type">
			<implements>
				<interface name="GtkBuildable"/>
				<interface name="GtkTreeModel"/>
				<interface name="GtkTreeDragSource"/>
				<interface name="GtkTreeDragDest"/>
				<interface name="GtkTreeSortable"/>
				<interface name="TnyList"/>
			</implements>
			<constructor name="new" symbol="tny_gtk_attach_list_model_new">
				<return-type type="GtkTreeModel*"/>
			</constructor>
			<field name="first" type="GList*"/>
			<field name="iterator_lock" type="GMutex*"/>
			<field name="first_needs_unref" type="gboolean"/>
		</object>
		<object name="TnyGtkAttachmentMimePartView" parent="GObject" type-name="TnyGtkAttachmentMimePartView" get-type="tny_gtk_attachment_mime_part_view_get_type">
			<implements>
				<interface name="TnyMimePartView"/>
			</implements>
			<constructor name="new" symbol="tny_gtk_attachment_mime_part_view_new">
				<return-type type="TnyMimePartView*"/>
				<parameters>
					<parameter name="iview" type="TnyGtkAttachListModel*"/>
				</parameters>
			</constructor>
			<vfunc name="clear">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_part">
				<return-type type="TnyMimePart*"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_part">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
		</object>
		<object name="TnyGtkExpanderMimePartView" parent="GtkExpander" type-name="TnyGtkExpanderMimePartView" get-type="tny_gtk_expander_mime_part_view_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
				<interface name="TnyMimePartView"/>
			</implements>
			<constructor name="new" symbol="tny_gtk_expander_mime_part_view_new">
				<return-type type="TnyMimePartView*"/>
				<parameters>
					<parameter name="view" type="TnyMimePartView*"/>
				</parameters>
			</constructor>
			<method name="set_view" symbol="tny_gtk_expander_mime_part_view_set_view">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyGtkExpanderMimePartView*"/>
					<parameter name="view" type="TnyMimePartView*"/>
				</parameters>
			</method>
			<vfunc name="clear">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_part">
				<return-type type="TnyMimePart*"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_part">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
		</object>
		<object name="TnyGtkFolderStoreTreeModel" parent="GtkTreeStore" type-name="TnyGtkFolderStoreTreeModel" get-type="tny_gtk_folder_store_tree_model_get_type">
			<implements>
				<interface name="GtkBuildable"/>
				<interface name="GtkTreeModel"/>
				<interface name="GtkTreeDragSource"/>
				<interface name="GtkTreeDragDest"/>
				<interface name="GtkTreeSortable"/>
				<interface name="TnyList"/>
				<interface name="TnyFolderStoreObserver"/>
				<interface name="TnyFolderObserver"/>
			</implements>
			<method name="append" symbol="tny_gtk_folder_store_tree_model_append">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyGtkFolderStoreTreeModel*"/>
					<parameter name="item" type="TnyFolderStore*"/>
					<parameter name="root_name" type="gchar*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_gtk_folder_store_tree_model_new">
				<return-type type="GtkTreeModel*"/>
				<parameters>
					<parameter name="query" type="TnyFolderStoreQuery*"/>
				</parameters>
			</constructor>
			<method name="prepend" symbol="tny_gtk_folder_store_tree_model_prepend">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyGtkFolderStoreTreeModel*"/>
					<parameter name="item" type="TnyFolderStore*"/>
					<parameter name="root_name" type="gchar*"/>
				</parameters>
			</method>
			<field name="first" type="GList*"/>
			<field name="store_obs" type="GList*"/>
			<field name="fol_obs" type="GList*"/>
			<field name="iterator_lock" type="GMutex*"/>
			<field name="query" type="TnyFolderStoreQuery*"/>
			<field name="first_needs_unref" type="gboolean"/>
			<field name="signals" type="GPtrArray*"/>
		</object>
		<object name="TnyGtkHeaderListModel" parent="GObject" type-name="TnyGtkHeaderListModel" get-type="tny_gtk_header_list_model_get_type">
			<implements>
				<interface name="GtkTreeModel"/>
				<interface name="TnyList"/>
			</implements>
			<method name="get_no_duplicates" symbol="tny_gtk_header_list_model_get_no_duplicates">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyGtkHeaderListModel*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_gtk_header_list_model_new">
				<return-type type="GtkTreeModel*"/>
			</constructor>
			<method name="received_date_sort_func" symbol="tny_gtk_header_list_model_received_date_sort_func">
				<return-type type="gint"/>
				<parameters>
					<parameter name="model" type="GtkTreeModel*"/>
					<parameter name="a" type="GtkTreeIter*"/>
					<parameter name="b" type="GtkTreeIter*"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="sent_date_sort_func" symbol="tny_gtk_header_list_model_sent_date_sort_func">
				<return-type type="gint"/>
				<parameters>
					<parameter name="model" type="GtkTreeModel*"/>
					<parameter name="a" type="GtkTreeIter*"/>
					<parameter name="b" type="GtkTreeIter*"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="set_folder" symbol="tny_gtk_header_list_model_set_folder">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyGtkHeaderListModel*"/>
					<parameter name="folder" type="TnyFolder*"/>
					<parameter name="refresh" type="gboolean"/>
					<parameter name="callback" type="TnyGetHeadersCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="set_no_duplicates" symbol="tny_gtk_header_list_model_set_no_duplicates">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyGtkHeaderListModel*"/>
					<parameter name="setting" type="gboolean"/>
				</parameters>
			</method>
		</object>
		<object name="TnyGtkHeaderView" parent="GtkTable" type-name="TnyGtkHeaderView" get-type="tny_gtk_header_view_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
				<interface name="TnyHeaderView"/>
			</implements>
			<constructor name="new" symbol="tny_gtk_header_view_new">
				<return-type type="TnyHeaderView*"/>
			</constructor>
			<vfunc name="clear">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeaderView*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_header">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeaderView*"/>
					<parameter name="header" type="TnyHeader*"/>
				</parameters>
			</vfunc>
		</object>
		<object name="TnyGtkImageMimePartView" parent="GtkImage" type-name="TnyGtkImageMimePartView" get-type="tny_gtk_image_mime_part_view_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
				<interface name="TnyMimePartView"/>
			</implements>
			<constructor name="new" symbol="tny_gtk_image_mime_part_view_new">
				<return-type type="TnyMimePartView*"/>
				<parameters>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="status_user_data" type="gpointer"/>
				</parameters>
			</constructor>
			<vfunc name="clear">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_part">
				<return-type type="TnyMimePart*"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_part">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
		</object>
		<object name="TnyGtkLockable" parent="GObject" type-name="TnyGtkLockable" get-type="tny_gtk_lockable_get_type">
			<implements>
				<interface name="TnyLockable"/>
			</implements>
			<constructor name="new" symbol="tny_gtk_lockable_new">
				<return-type type="TnyLockable*"/>
			</constructor>
		</object>
		<object name="TnyGtkMimePartSaveStrategy" parent="GObject" type-name="TnyGtkMimePartSaveStrategy" get-type="tny_gtk_mime_part_save_strategy_get_type">
			<implements>
				<interface name="TnyMimePartSaveStrategy"/>
			</implements>
			<constructor name="new" symbol="tny_gtk_mime_part_save_strategy_new">
				<return-type type="TnyMimePartSaveStrategy*"/>
				<parameters>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="status_user_data" type="gpointer"/>
				</parameters>
			</constructor>
			<vfunc name="perform_save">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartSaveStrategy*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
		</object>
		<object name="TnyGtkMsgView" parent="GtkBin" type-name="TnyGtkMsgView" get-type="tny_gtk_msg_view_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
				<interface name="TnyMimePartView"/>
				<interface name="TnyMsgView"/>
			</implements>
			<method name="get_display_attachments" symbol="tny_gtk_msg_view_get_display_attachments">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyGtkMsgView*"/>
				</parameters>
			</method>
			<method name="get_display_html" symbol="tny_gtk_msg_view_get_display_html">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyGtkMsgView*"/>
				</parameters>
			</method>
			<method name="get_display_plain" symbol="tny_gtk_msg_view_get_display_plain">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyGtkMsgView*"/>
				</parameters>
			</method>
			<method name="get_display_rfc822" symbol="tny_gtk_msg_view_get_display_rfc822">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyGtkMsgView*"/>
				</parameters>
			</method>
			<method name="get_status_callback" symbol="tny_gtk_msg_view_get_status_callback">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyGtkMsgView*"/>
					<parameter name="status_callback" type="TnyStatusCallback*"/>
					<parameter name="status_user_data" type="gpointer*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_gtk_msg_view_new">
				<return-type type="TnyMsgView*"/>
			</constructor>
			<method name="set_display_attachments" symbol="tny_gtk_msg_view_set_display_attachments">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyGtkMsgView*"/>
					<parameter name="setting" type="gboolean"/>
				</parameters>
			</method>
			<method name="set_display_html" symbol="tny_gtk_msg_view_set_display_html">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyGtkMsgView*"/>
					<parameter name="setting" type="gboolean"/>
				</parameters>
			</method>
			<method name="set_display_plain" symbol="tny_gtk_msg_view_set_display_plain">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyGtkMsgView*"/>
					<parameter name="setting" type="gboolean"/>
				</parameters>
			</method>
			<method name="set_display_rfc822" symbol="tny_gtk_msg_view_set_display_rfc822">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyGtkMsgView*"/>
					<parameter name="setting" type="gboolean"/>
				</parameters>
			</method>
			<method name="set_parented" symbol="tny_gtk_msg_view_set_parented">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyGtkMsgView*"/>
					<parameter name="parented" type="gboolean"/>
				</parameters>
			</method>
			<method name="set_status_callback" symbol="tny_gtk_msg_view_set_status_callback">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyGtkMsgView*"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="status_user_data" type="gpointer"/>
				</parameters>
			</method>
			<vfunc name="clear">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
				</parameters>
			</vfunc>
			<vfunc name="create_header_view">
				<return-type type="TnyHeaderView*"/>
				<parameters>
					<parameter name="self" type="TnyGtkMsgView*"/>
				</parameters>
			</vfunc>
			<vfunc name="create_mime_part_view_for">
				<return-type type="TnyMimePartView*"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="create_new_inline_viewer">
				<return-type type="TnyMsgView*"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_msg">
				<return-type type="TnyMsg*"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_part">
				<return-type type="TnyMimePart*"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_msg">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
					<parameter name="msg" type="TnyMsg*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_part">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_unavailable">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
				</parameters>
			</vfunc>
			<field name="viewers" type="GtkContainer*"/>
		</object>
		<object name="TnyGtkMsgWindow" parent="GtkWindow" type-name="TnyGtkMsgWindow" get-type="tny_gtk_msg_window_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
				<interface name="TnyMimePartView"/>
				<interface name="TnyMsgView"/>
				<interface name="TnyMsgWindow"/>
			</implements>
			<constructor name="new" symbol="tny_gtk_msg_window_new">
				<return-type type="TnyMsgWindow*"/>
				<parameters>
					<parameter name="msgview" type="TnyMsgView*"/>
				</parameters>
			</constructor>
			<method name="set_view" symbol="tny_gtk_msg_window_set_view">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyGtkMsgWindow*"/>
					<parameter name="view" type="TnyMsgView*"/>
				</parameters>
			</method>
			<vfunc name="clear">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
				</parameters>
			</vfunc>
			<vfunc name="create_mime_part_view_for">
				<return-type type="TnyMimePartView*"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="create_new_inline_viewer">
				<return-type type="TnyMsgView*"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_msg">
				<return-type type="TnyMsg*"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_part">
				<return-type type="TnyMimePart*"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_msg">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
					<parameter name="msg" type="TnyMsg*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_part">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_unavailable">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
				</parameters>
			</vfunc>
		</object>
		<object name="TnyGtkPasswordDialog" parent="GObject" type-name="TnyGtkPasswordDialog" get-type="tny_gtk_password_dialog_get_type">
			<implements>
				<interface name="TnyPasswordGetter"/>
			</implements>
			<constructor name="new" symbol="tny_gtk_password_dialog_new">
				<return-type type="TnyPasswordGetter*"/>
			</constructor>
		</object>
		<object name="TnyGtkPixbufStream" parent="GObject" type-name="TnyGtkPixbufStream" get-type="tny_gtk_pixbuf_stream_get_type">
			<implements>
				<interface name="TnyStream"/>
			</implements>
			<method name="get_pixbuf" symbol="tny_gtk_pixbuf_stream_get_pixbuf">
				<return-type type="GdkPixbuf*"/>
				<parameters>
					<parameter name="self" type="TnyGtkPixbufStream*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_gtk_pixbuf_stream_new">
				<return-type type="TnyStream*"/>
				<parameters>
					<parameter name="mime_type" type="gchar*"/>
				</parameters>
			</constructor>
			<vfunc name="close">
				<return-type type="gint"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
				</parameters>
			</vfunc>
			<vfunc name="flush">
				<return-type type="gint"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
				</parameters>
			</vfunc>
			<vfunc name="is_eos">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
				</parameters>
			</vfunc>
			<vfunc name="read">
				<return-type type="gssize"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
					<parameter name="buffer" type="char*"/>
					<parameter name="n" type="gsize"/>
				</parameters>
			</vfunc>
			<vfunc name="reset">
				<return-type type="gint"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
				</parameters>
			</vfunc>
			<vfunc name="write">
				<return-type type="gssize"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
					<parameter name="buffer" type="char*"/>
					<parameter name="n" type="gsize"/>
				</parameters>
			</vfunc>
			<vfunc name="write_to_stream">
				<return-type type="gssize"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
					<parameter name="output" type="TnyStream*"/>
				</parameters>
			</vfunc>
		</object>
		<object name="TnyGtkTextBufferStream" parent="GObject" type-name="TnyGtkTextBufferStream" get-type="tny_gtk_text_buffer_stream_get_type">
			<implements>
				<interface name="TnyStream"/>
			</implements>
			<constructor name="new" symbol="tny_gtk_text_buffer_stream_new">
				<return-type type="TnyStream*"/>
				<parameters>
					<parameter name="buffer" type="GtkTextBuffer*"/>
				</parameters>
			</constructor>
			<method name="set_text_buffer" symbol="tny_gtk_text_buffer_stream_set_text_buffer">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyGtkTextBufferStream*"/>
					<parameter name="buffer" type="GtkTextBuffer*"/>
				</parameters>
			</method>
			<vfunc name="close">
				<return-type type="gint"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
				</parameters>
			</vfunc>
			<vfunc name="flush">
				<return-type type="gint"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
				</parameters>
			</vfunc>
			<vfunc name="is_eos">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
				</parameters>
			</vfunc>
			<vfunc name="read">
				<return-type type="gssize"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
					<parameter name="buffer" type="char*"/>
					<parameter name="n" type="gsize"/>
				</parameters>
			</vfunc>
			<vfunc name="reset">
				<return-type type="gint"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
				</parameters>
			</vfunc>
			<vfunc name="write">
				<return-type type="gssize"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
					<parameter name="buffer" type="char*"/>
					<parameter name="n" type="gsize"/>
				</parameters>
			</vfunc>
			<vfunc name="write_to_stream">
				<return-type type="gssize"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
					<parameter name="output" type="TnyStream*"/>
				</parameters>
			</vfunc>
		</object>
		<object name="TnyGtkTextMimePartView" parent="GtkTextView" type-name="TnyGtkTextMimePartView" get-type="tny_gtk_text_mime_part_view_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
				<interface name="TnyMimePartView"/>
			</implements>
			<constructor name="new" symbol="tny_gtk_text_mime_part_view_new">
				<return-type type="TnyMimePartView*"/>
				<parameters>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="status_user_data" type="gpointer"/>
				</parameters>
			</constructor>
			<vfunc name="clear">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_part">
				<return-type type="TnyMimePart*"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_part">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
		</object>
	</namespace>
</api>
