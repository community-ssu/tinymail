<?xml version="1.0"?>
<api version="1.0">
	<namespace name="Tny">
		<function name="clear_status" symbol="tny_clear_status">
			<return-type type="void"/>
			<parameters>
				<parameter name="status" type="TnyStatus**"/>
			</parameters>
		</function>
		<function name="error_get_code" symbol="tny_error_get_code">
			<return-type type="gint"/>
			<parameters>
				<parameter name="err" type="GError*"/>
			</parameters>
		</function>
		<function name="error_get_message" symbol="tny_error_get_message">
			<return-type type="gchar*"/>
			<parameters>
				<parameter name="err" type="GError*"/>
			</parameters>
		</function>
		<function name="marshal_VOID__OBJECT_OBJECT_INT_INT" symbol="tny_marshal_VOID__OBJECT_OBJECT_INT_INT">
			<return-type type="void"/>
			<parameters>
				<parameter name="closure" type="GClosure*"/>
				<parameter name="return_value" type="GValue*"/>
				<parameter name="n_param_values" type="guint"/>
				<parameter name="param_values" type="GValue*"/>
				<parameter name="invocation_hint" type="gpointer"/>
				<parameter name="marshal_data" type="gpointer"/>
			</parameters>
		</function>
		<function name="marshal_VOID__OBJECT_OBJECT_POINTER" symbol="tny_marshal_VOID__OBJECT_OBJECT_POINTER">
			<return-type type="void"/>
			<parameters>
				<parameter name="closure" type="GClosure*"/>
				<parameter name="return_value" type="GValue*"/>
				<parameter name="n_param_values" type="guint"/>
				<parameter name="param_values" type="GValue*"/>
				<parameter name="invocation_hint" type="gpointer"/>
				<parameter name="marshal_data" type="gpointer"/>
			</parameters>
		</function>
		<function name="set_status" symbol="tny_set_status">
			<return-type type="void"/>
			<parameters>
				<parameter name="status" type="TnyStatus**"/>
				<parameter name="domain" type="GQuark"/>
				<parameter name="code" type="gint"/>
				<parameter name="position" type="guint"/>
				<parameter name="of_total" type="guint"/>
				<parameter name="format" type="gchar*"/>
			</parameters>
		</function>
		<callback name="TnyCopyFolderCallback">
			<return-type type="void"/>
			<parameters>
				<parameter name="self" type="TnyFolder*"/>
				<parameter name="cancelled" type="gboolean"/>
				<parameter name="into" type="TnyFolderStore*"/>
				<parameter name="new_folder" type="TnyFolder*"/>
				<parameter name="err" type="GError*"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<callback name="TnyCreateFolderCallback">
			<return-type type="void"/>
			<parameters>
				<parameter name="self" type="TnyFolderStore*"/>
				<parameter name="cancelled" type="gboolean"/>
				<parameter name="new_folder" type="TnyFolder*"/>
				<parameter name="err" type="GError*"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<callback name="TnyFolderCallback">
			<return-type type="void"/>
			<parameters>
				<parameter name="self" type="TnyFolder*"/>
				<parameter name="cancelled" type="gboolean"/>
				<parameter name="err" type="GError*"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<callback name="TnyForgetPassFunc">
			<return-type type="void"/>
			<parameters>
				<parameter name="self" type="TnyAccount*"/>
			</parameters>
		</callback>
		<callback name="TnyGetFoldersCallback">
			<return-type type="void"/>
			<parameters>
				<parameter name="self" type="TnyFolderStore*"/>
				<parameter name="cancelled" type="gboolean"/>
				<parameter name="list" type="TnyList*"/>
				<parameter name="err" type="GError*"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<callback name="TnyGetHeadersCallback">
			<return-type type="void"/>
			<parameters>
				<parameter name="self" type="TnyFolder*"/>
				<parameter name="cancelled" type="gboolean"/>
				<parameter name="headers" type="TnyList*"/>
				<parameter name="err" type="GError*"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<callback name="TnyGetMsgCallback">
			<return-type type="void"/>
			<parameters>
				<parameter name="folder" type="TnyFolder*"/>
				<parameter name="cancelled" type="gboolean"/>
				<parameter name="msg" type="TnyMsg*"/>
				<parameter name="err" type="GError*"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<callback name="TnyGetPassFunc">
			<return-type type="gchar*"/>
			<parameters>
				<parameter name="self" type="TnyAccount*"/>
				<parameter name="prompt" type="gchar*"/>
				<parameter name="cancel" type="gboolean*"/>
			</parameters>
		</callback>
		<callback name="TnyListMatcher">
			<return-type type="gboolean"/>
			<parameters>
				<parameter name="list" type="TnyList*"/>
				<parameter name="item" type="GObject*"/>
				<parameter name="match_data" type="gpointer"/>
			</parameters>
		</callback>
		<callback name="TnyMimePartCallback">
			<return-type type="void"/>
			<parameters>
				<parameter name="self" type="TnyMimePart*"/>
				<parameter name="cancelled" type="gboolean"/>
				<parameter name="stream" type="TnyStream*"/>
				<parameter name="err" type="GError*"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<callback name="TnySendQueueAddCallback">
			<return-type type="void"/>
			<parameters>
				<parameter name="self" type="TnySendQueue*"/>
				<parameter name="cancelled" type="gboolean"/>
				<parameter name="msg" type="TnyMsg*"/>
				<parameter name="err" type="GError*"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<callback name="TnyStatusCallback">
			<return-type type="void"/>
			<parameters>
				<parameter name="self" type="GObject*"/>
				<parameter name="status" type="TnyStatus*"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<callback name="TnyTransferMsgsCallback">
			<return-type type="void"/>
			<parameters>
				<parameter name="folder" type="TnyFolder*"/>
				<parameter name="cancelled" type="gboolean"/>
				<parameter name="err" type="GError*"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<struct name="TError">
		</struct>
		<struct name="TnyStatus">
			<method name="copy" symbol="tny_status_copy">
				<return-type type="TnyStatus*"/>
				<parameters>
					<parameter name="status" type="TnyStatus*"/>
				</parameters>
			</method>
			<method name="free" symbol="tny_status_free">
				<return-type type="void"/>
				<parameters>
					<parameter name="status" type="TnyStatus*"/>
				</parameters>
			</method>
			<method name="get_fraction" symbol="tny_status_get_fraction">
				<return-type type="gdouble"/>
				<parameters>
					<parameter name="status" type="TnyStatus*"/>
				</parameters>
			</method>
			<method name="matches" symbol="tny_status_matches">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="status" type="TnyStatus*"/>
					<parameter name="domain" type="GQuark"/>
					<parameter name="code" type="gint"/>
				</parameters>
			</method>
			<method name="new" symbol="tny_status_new">
				<return-type type="TnyStatus*"/>
				<parameters>
					<parameter name="domain" type="GQuark"/>
					<parameter name="code" type="gint"/>
					<parameter name="position" type="guint"/>
					<parameter name="of_total" type="guint"/>
					<parameter name="format" type="gchar*"/>
				</parameters>
			</method>
			<method name="new_literal" symbol="tny_status_new_literal">
				<return-type type="TnyStatus*"/>
				<parameters>
					<parameter name="domain" type="GQuark"/>
					<parameter name="code" type="gint"/>
					<parameter name="position" type="guint"/>
					<parameter name="of_total" type="guint"/>
					<parameter name="message" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_fraction" symbol="tny_status_set_fraction">
				<return-type type="void"/>
				<parameters>
					<parameter name="status" type="TnyStatus*"/>
					<parameter name="fraction" type="gdouble"/>
				</parameters>
			</method>
			<field name="domain" type="GQuark"/>
			<field name="code" type="gint"/>
			<field name="message" type="gchar*"/>
			<field name="position" type="guint"/>
			<field name="of_total" type="guint"/>
		</struct>
		<enum name="TnyAccountSignal" type-name="TnyAccountSignal" get-type="tny_account_signal_type_get_type">
			<member name="TNY_ACCOUNT_CONNECTION_STATUS_CHANGED" value="0"/>
			<member name="TNY_ACCOUNT_CHANGED" value="1"/>
			<member name="TNY_ACCOUNT_LAST_SIGNAL" value="2"/>
		</enum>
		<enum name="TnyAccountType" type-name="TnyAccountType" get-type="tny_account_type_get_type">
			<member name="TNY_ACCOUNT_TYPE_STORE" value="0"/>
			<member name="TNY_ACCOUNT_TYPE_TRANSPORT" value="1"/>
		</enum>
		<enum name="TnyAlertType" type-name="TnyAlertType" get-type="tny_alert_type_get_type">
			<member name="TNY_ALERT_TYPE_INFO" value="0"/>
			<member name="TNY_ALERT_TYPE_WARNING" value="1"/>
			<member name="TNY_ALERT_TYPE_ERROR" value="2"/>
		</enum>
		<enum name="TnyConnectionStatus" type-name="TnyConnectionStatus" get-type="tny_connection_status_get_type">
			<member name="TNY_CONNECTION_STATUS_DISCONNECTED" value="0"/>
			<member name="TNY_CONNECTION_STATUS_DISCONNECTED_BROKEN" value="1"/>
			<member name="TNY_CONNECTION_STATUS_CONNECTED_BROKEN" value="2"/>
			<member name="TNY_CONNECTION_STATUS_CONNECTED" value="3"/>
			<member name="TNY_CONNECTION_STATUS_RECONNECTING" value="4"/>
			<member name="TNY_CONNECTION_STATUS_INIT" value="5"/>
		</enum>
		<enum name="TnyError" type-name="TnyError" get-type="tny_error_get_type">
		</enum>
		<enum name="TnyErrorDomain" type-name="TnyErrorDomain" get-type="tny_error_domain_get_type">
		</enum>
		<enum name="TnyFolderSignal" type-name="TnyFolderSignal" get-type="tny_folder_signal_get_type">
			<member name="TNY_FOLDER_FOLDER_INSERTED" value="0"/>
			<member name="TNY_FOLDER_FOLDERS_RELOADED" value="1"/>
			<member name="TNY_FOLDER_LAST_SIGNAL" value="2"/>
		</enum>
		<enum name="TnyFolderType" type-name="TnyFolderType" get-type="tny_folder_type_get_type">
			<member name="TNY_FOLDER_TYPE_UNKNOWN" value="0"/>
			<member name="TNY_FOLDER_TYPE_NORMAL" value="1"/>
			<member name="TNY_FOLDER_TYPE_INBOX" value="2"/>
			<member name="TNY_FOLDER_TYPE_OUTBOX" value="3"/>
			<member name="TNY_FOLDER_TYPE_TRASH" value="4"/>
			<member name="TNY_FOLDER_TYPE_JUNK" value="5"/>
			<member name="TNY_FOLDER_TYPE_SENT" value="6"/>
			<member name="TNY_FOLDER_TYPE_ROOT" value="7"/>
			<member name="TNY_FOLDER_TYPE_NOTES" value="8"/>
			<member name="TNY_FOLDER_TYPE_DRAFTS" value="9"/>
			<member name="TNY_FOLDER_TYPE_CONTACTS" value="10"/>
			<member name="TNY_FOLDER_TYPE_CALENDAR" value="11"/>
			<member name="TNY_FOLDER_TYPE_ARCHIVE" value="12"/>
			<member name="TNY_FOLDER_TYPE_MERGE" value="13"/>
		</enum>
		<enum name="TnyGetAccountsRequestType" type-name="TnyGetAccountsRequestType" get-type="tny_get_accounts_request_type_get_type">
			<member name="TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS" value="0"/>
			<member name="TNY_ACCOUNT_STORE_STORE_ACCOUNTS" value="1"/>
			<member name="TNY_ACCOUNT_STORE_BOTH" value="2"/>
		</enum>
		<enum name="TnySendQueueCancelAction">
			<member name="TNY_SEND_QUEUE_CANCEL_ACTION_SUSPEND" value="0"/>
			<member name="TNY_SEND_QUEUE_CANCEL_ACTION_REMOVE" value="1"/>
		</enum>
		<enum name="TnyStatusCode" type-name="TnyStatusCode" get-type="tny_status_code_get_type">
			<member name="TNY_FOLDER_STATUS_CODE_REFRESH" value="1"/>
			<member name="TNY_FOLDER_STATUS_CODE_GET_MSG" value="2"/>
			<member name="TNY_GET_MSG_QUEUE_STATUS_GET_MSG" value="3"/>
			<member name="TNY_FOLDER_STATUS_CODE_XFER_MSGS" value="4"/>
			<member name="TNY_FOLDER_STATUS_CODE_COPY_FOLDER" value="5"/>
			<member name="TNY_GET_SUPPORTED_SECURE_AUTH_STATUS_GET_SECURE_AUTH" value="6"/>
			<member name="TNY_FOLDER_STATUS_CODE_SYNC" value="7"/>
		</enum>
		<enum name="TnyStatusDomain" type-name="TnyStatusDomain" get-type="tny_status_domain_get_type">
			<member name="TNY_FOLDER_STATUS" value="1"/>
			<member name="TNY_GET_MSG_QUEUE_STATUS" value="2"/>
			<member name="TNY_GET_SUPPORTED_SECURE_AUTH_STATUS" value="3"/>
		</enum>
		<flags name="TnyFolderCaps" type-name="TnyFolderCaps" get-type="tny_folder_caps_get_type">
			<member name="TNY_FOLDER_CAPS_WRITABLE" value="1"/>
			<member name="TNY_FOLDER_CAPS_PUSHEMAIL" value="2"/>
		</flags>
		<flags name="TnyFolderChangeChanged" type-name="TnyFolderChangeChanged" get-type="tny_folder_change_changed_get_type">
			<member name="TNY_FOLDER_CHANGE_CHANGED_ALL_COUNT" value="1"/>
			<member name="TNY_FOLDER_CHANGE_CHANGED_UNREAD_COUNT" value="2"/>
			<member name="TNY_FOLDER_CHANGE_CHANGED_ADDED_HEADERS" value="4"/>
			<member name="TNY_FOLDER_CHANGE_CHANGED_EXPUNGED_HEADERS" value="8"/>
			<member name="TNY_FOLDER_CHANGE_CHANGED_FOLDER_RENAME" value="16"/>
			<member name="TNY_FOLDER_CHANGE_CHANGED_MSG_RECEIVED" value="32"/>
		</flags>
		<flags name="TnyFolderStoreChangeChanged" type-name="TnyFolderStoreChangeChanged" get-type="tny_folder_store_change_changed_get_type">
			<member name="TNY_FOLDER_STORE_CHANGE_CHANGED_CREATED_FOLDERS" value="1"/>
			<member name="TNY_FOLDER_STORE_CHANGE_CHANGED_REMOVED_FOLDERS" value="2"/>
		</flags>
		<flags name="TnyFolderStoreQueryOption" type-name="TnyFolderStoreQueryOption" get-type="tny_folder_store_query_option_get_type">
			<member name="TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED" value="2"/>
			<member name="TNY_FOLDER_STORE_QUERY_OPTION_UNSUBSCRIBED" value="4"/>
			<member name="TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_NAME" value="8"/>
			<member name="TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_ID" value="16"/>
			<member name="TNY_FOLDER_STORE_QUERY_OPTION_PATTERN_IS_CASE_INSENSITIVE" value="32"/>
			<member name="TNY_FOLDER_STORE_QUERY_OPTION_PATTERN_IS_REGEX" value="64"/>
		</flags>
		<flags name="TnyHeaderFlags" type-name="TnyHeaderFlags" get-type="tny_header_flags_get_type">
			<member name="TNY_HEADER_FLAG_ANSWERED" value="1"/>
			<member name="TNY_HEADER_FLAG_DELETED" value="2"/>
			<member name="TNY_HEADER_FLAG_DRAFT" value="4"/>
			<member name="TNY_HEADER_FLAG_FLAGGED" value="8"/>
			<member name="TNY_HEADER_FLAG_SEEN" value="16"/>
			<member name="TNY_HEADER_FLAG_ATTACHMENTS" value="32"/>
			<member name="TNY_HEADER_FLAG_CACHED" value="64"/>
			<member name="TNY_HEADER_FLAG_PARTIAL" value="128"/>
			<member name="TNY_HEADER_FLAG_EXPUNGED" value="256"/>
			<member name="TNY_HEADER_FLAG_HIGH_PRIORITY" value="1024"/>
			<member name="TNY_HEADER_FLAG_NORMAL_PRIORITY" value="0"/>
			<member name="TNY_HEADER_FLAG_LOW_PRIORITY" value="512"/>
			<member name="TNY_HEADER_FLAG_SUSPENDED" value="2048"/>
		</flags>
		<object name="TnyCombinedAccount" parent="GObject" type-name="TnyCombinedAccount" get-type="tny_combined_account_get_type">
			<implements>
				<interface name="TnyFolderStore"/>
				<interface name="TnyAccount"/>
				<interface name="TnyTransportAccount"/>
				<interface name="TnyStoreAccount"/>
			</implements>
			<method name="get_store_account" symbol="tny_combined_account_get_store_account">
				<return-type type="TnyStoreAccount*"/>
				<parameters>
					<parameter name="self" type="TnyCombinedAccount*"/>
				</parameters>
			</method>
			<method name="get_transport_account" symbol="tny_combined_account_get_transport_account">
				<return-type type="TnyTransportAccount*"/>
				<parameters>
					<parameter name="self" type="TnyCombinedAccount*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_combined_account_new">
				<return-type type="TnyAccount*"/>
				<parameters>
					<parameter name="ta" type="TnyTransportAccount*"/>
					<parameter name="sa" type="TnyStoreAccount*"/>
				</parameters>
			</constructor>
		</object>
		<object name="TnyFolderChange" parent="GObject" type-name="TnyFolderChange" get-type="tny_folder_change_get_type">
			<method name="add_added_header" symbol="tny_folder_change_add_added_header">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
					<parameter name="header" type="TnyHeader*"/>
				</parameters>
			</method>
			<method name="add_expunged_header" symbol="tny_folder_change_add_expunged_header">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
					<parameter name="header" type="TnyHeader*"/>
				</parameters>
			</method>
			<method name="get_added_headers" symbol="tny_folder_change_get_added_headers">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
					<parameter name="headers" type="TnyList*"/>
				</parameters>
			</method>
			<method name="get_changed" symbol="tny_folder_change_get_changed">
				<return-type type="TnyFolderChangeChanged"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
				</parameters>
			</method>
			<method name="get_check_duplicates" symbol="tny_folder_change_get_check_duplicates">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
				</parameters>
			</method>
			<method name="get_expunged_headers" symbol="tny_folder_change_get_expunged_headers">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
					<parameter name="headers" type="TnyList*"/>
				</parameters>
			</method>
			<method name="get_folder" symbol="tny_folder_change_get_folder">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
				</parameters>
			</method>
			<method name="get_new_all_count" symbol="tny_folder_change_get_new_all_count">
				<return-type type="guint"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
				</parameters>
			</method>
			<method name="get_new_unread_count" symbol="tny_folder_change_get_new_unread_count">
				<return-type type="guint"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
				</parameters>
			</method>
			<method name="get_received_msg" symbol="tny_folder_change_get_received_msg">
				<return-type type="TnyMsg*"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
				</parameters>
			</method>
			<method name="get_rename" symbol="tny_folder_change_get_rename">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
					<parameter name="oldname" type="gchar**"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_folder_change_new">
				<return-type type="TnyFolderChange*"/>
				<parameters>
					<parameter name="folder" type="TnyFolder*"/>
				</parameters>
			</constructor>
			<method name="reset" symbol="tny_folder_change_reset">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
				</parameters>
			</method>
			<method name="set_check_duplicates" symbol="tny_folder_change_set_check_duplicates">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
					<parameter name="check_duplicates" type="gboolean"/>
				</parameters>
			</method>
			<method name="set_new_all_count" symbol="tny_folder_change_set_new_all_count">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
					<parameter name="new_all_count" type="guint"/>
				</parameters>
			</method>
			<method name="set_new_unread_count" symbol="tny_folder_change_set_new_unread_count">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
					<parameter name="new_unread_count" type="guint"/>
				</parameters>
			</method>
			<method name="set_received_msg" symbol="tny_folder_change_set_received_msg">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
					<parameter name="msg" type="TnyMsg*"/>
				</parameters>
			</method>
			<method name="set_rename" symbol="tny_folder_change_set_rename">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderChange*"/>
					<parameter name="newname" type="gchar*"/>
				</parameters>
			</method>
		</object>
		<object name="TnyFolderMonitor" parent="GObject" type-name="TnyFolderMonitor" get-type="tny_folder_monitor_get_type">
			<implements>
				<interface name="TnyFolderObserver"/>
			</implements>
			<method name="add_list" symbol="tny_folder_monitor_add_list">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderMonitor*"/>
					<parameter name="list" type="TnyList*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_folder_monitor_new">
				<return-type type="TnyFolderObserver*"/>
				<parameters>
					<parameter name="folder" type="TnyFolder*"/>
				</parameters>
			</constructor>
			<method name="poke_status" symbol="tny_folder_monitor_poke_status">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderMonitor*"/>
				</parameters>
			</method>
			<method name="remove_list" symbol="tny_folder_monitor_remove_list">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderMonitor*"/>
					<parameter name="list" type="TnyList*"/>
				</parameters>
			</method>
			<method name="start" symbol="tny_folder_monitor_start">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderMonitor*"/>
				</parameters>
			</method>
			<method name="stop" symbol="tny_folder_monitor_stop">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderMonitor*"/>
				</parameters>
			</method>
			<vfunc name="add_list">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderMonitor*"/>
					<parameter name="list" type="TnyList*"/>
				</parameters>
			</vfunc>
			<vfunc name="poke_status">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderMonitor*"/>
				</parameters>
			</vfunc>
			<vfunc name="remove_list">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderMonitor*"/>
					<parameter name="list" type="TnyList*"/>
				</parameters>
			</vfunc>
			<vfunc name="start">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderMonitor*"/>
				</parameters>
			</vfunc>
			<vfunc name="stop">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderMonitor*"/>
				</parameters>
			</vfunc>
			<vfunc name="update">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderObserver*"/>
					<parameter name="change" type="TnyFolderChange*"/>
				</parameters>
			</vfunc>
		</object>
		<object name="TnyFolderStats" parent="GObject" type-name="TnyFolderStats" get-type="tny_folder_stats_get_type">
			<method name="get_all_count" symbol="tny_folder_stats_get_all_count">
				<return-type type="guint"/>
				<parameters>
					<parameter name="self" type="TnyFolderStats*"/>
				</parameters>
			</method>
			<method name="get_folder" symbol="tny_folder_stats_get_folder">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnyFolderStats*"/>
				</parameters>
			</method>
			<method name="get_local_size" symbol="tny_folder_stats_get_local_size">
				<return-type type="gsize"/>
				<parameters>
					<parameter name="self" type="TnyFolderStats*"/>
				</parameters>
			</method>
			<method name="get_unread_count" symbol="tny_folder_stats_get_unread_count">
				<return-type type="guint"/>
				<parameters>
					<parameter name="self" type="TnyFolderStats*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_folder_stats_new">
				<return-type type="TnyFolderStats*"/>
				<parameters>
					<parameter name="folder" type="TnyFolder*"/>
				</parameters>
			</constructor>
			<method name="set_local_size" symbol="tny_folder_stats_set_local_size">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStats*"/>
					<parameter name="local_size" type="gsize"/>
				</parameters>
			</method>
		</object>
		<object name="TnyFolderStoreChange" parent="GObject" type-name="TnyFolderStoreChange" get-type="tny_folder_store_change_get_type">
			<method name="add_created_folder" symbol="tny_folder_store_change_add_created_folder">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStoreChange*"/>
					<parameter name="folder" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="add_removed_folder" symbol="tny_folder_store_change_add_removed_folder">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStoreChange*"/>
					<parameter name="folder" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="get_changed" symbol="tny_folder_store_change_get_changed">
				<return-type type="TnyFolderStoreChangeChanged"/>
				<parameters>
					<parameter name="self" type="TnyFolderStoreChange*"/>
				</parameters>
			</method>
			<method name="get_created_folders" symbol="tny_folder_store_change_get_created_folders">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStoreChange*"/>
					<parameter name="folders" type="TnyList*"/>
				</parameters>
			</method>
			<method name="get_folder_store" symbol="tny_folder_store_change_get_folder_store">
				<return-type type="TnyFolderStore*"/>
				<parameters>
					<parameter name="self" type="TnyFolderStoreChange*"/>
				</parameters>
			</method>
			<method name="get_removed_folders" symbol="tny_folder_store_change_get_removed_folders">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStoreChange*"/>
					<parameter name="folders" type="TnyList*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_folder_store_change_new">
				<return-type type="TnyFolderStoreChange*"/>
				<parameters>
					<parameter name="folderstore" type="TnyFolderStore*"/>
				</parameters>
			</constructor>
			<method name="reset" symbol="tny_folder_store_change_reset">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStoreChange*"/>
				</parameters>
			</method>
		</object>
		<object name="TnyFolderStoreQuery" parent="GObject" type-name="TnyFolderStoreQuery" get-type="tny_folder_store_query_get_type">
			<method name="add_item" symbol="tny_folder_store_query_add_item">
				<return-type type="void"/>
				<parameters>
					<parameter name="query" type="TnyFolderStoreQuery*"/>
					<parameter name="pattern" type="gchar*"/>
					<parameter name="options" type="TnyFolderStoreQueryOption"/>
				</parameters>
			</method>
			<method name="get_items" symbol="tny_folder_store_query_get_items">
				<return-type type="TnyList*"/>
				<parameters>
					<parameter name="query" type="TnyFolderStoreQuery*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_folder_store_query_new">
				<return-type type="TnyFolderStoreQuery*"/>
			</constructor>
			<field name="items" type="TnyList*"/>
		</object>
		<object name="TnyFolderStoreQueryItem" parent="GObject" type-name="TnyFolderStoreQueryItem" get-type="tny_folder_store_query_item_get_type">
			<method name="get_options" symbol="tny_folder_store_query_item_get_options">
				<return-type type="TnyFolderStoreQueryOption"/>
				<parameters>
					<parameter name="item" type="TnyFolderStoreQueryItem*"/>
				</parameters>
			</method>
			<method name="get_pattern" symbol="tny_folder_store_query_item_get_pattern">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="item" type="TnyFolderStoreQueryItem*"/>
				</parameters>
			</method>
			<method name="get_regex" symbol="tny_folder_store_query_item_get_regex">
				<return-type type="regex_t*"/>
				<parameters>
					<parameter name="item" type="TnyFolderStoreQueryItem*"/>
				</parameters>
			</method>
			<field name="options" type="TnyFolderStoreQueryOption"/>
			<field name="regex" type="regex_t*"/>
			<field name="pattern" type="gchar*"/>
		</object>
		<object name="TnyFsStream" parent="GObject" type-name="TnyFsStream" get-type="tny_fs_stream_get_type">
			<implements>
				<interface name="TnyStream"/>
				<interface name="TnySeekable"/>
			</implements>
			<constructor name="new" symbol="tny_fs_stream_new">
				<return-type type="TnyStream*"/>
				<parameters>
					<parameter name="fd" type="int"/>
				</parameters>
			</constructor>
			<method name="set_fd" symbol="tny_fs_stream_set_fd">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFsStream*"/>
					<parameter name="fd" type="int"/>
				</parameters>
			</method>
		</object>
		<object name="TnyMergeFolder" parent="GObject" type-name="TnyMergeFolder" get-type="tny_merge_folder_get_type">
			<implements>
				<interface name="TnyFolder"/>
				<interface name="TnyFolderObserver"/>
			</implements>
			<method name="add_folder" symbol="tny_merge_folder_add_folder">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMergeFolder*"/>
					<parameter name="folder" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="get_folders" symbol="tny_merge_folder_get_folders">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMergeFolder*"/>
					<parameter name="list" type="TnyList*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_merge_folder_new">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="folder_name" type="gchar*"/>
				</parameters>
			</constructor>
			<constructor name="new_with_ui_locker" symbol="tny_merge_folder_new_with_ui_locker">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="folder_name" type="gchar*"/>
					<parameter name="ui_locker" type="TnyLockable*"/>
				</parameters>
			</constructor>
			<method name="remove_folder" symbol="tny_merge_folder_remove_folder">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMergeFolder*"/>
					<parameter name="folder" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="set_folder_type" symbol="tny_merge_folder_set_folder_type">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMergeFolder*"/>
					<parameter name="folder_type" type="TnyFolderType"/>
				</parameters>
			</method>
			<method name="set_ui_locker" symbol="tny_merge_folder_set_ui_locker">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMergeFolder*"/>
					<parameter name="ui_locker" type="TnyLockable*"/>
				</parameters>
			</method>
		</object>
		<object name="TnyNoopLockable" parent="GObject" type-name="TnyNoopLockable" get-type="tny_noop_lockable_get_type">
			<implements>
				<interface name="TnyLockable"/>
			</implements>
			<constructor name="new" symbol="tny_noop_lockable_new">
				<return-type type="TnyLockable*"/>
			</constructor>
		</object>
		<object name="TnyPair" parent="GObject" type-name="TnyPair" get-type="tny_pair_get_type">
			<method name="get_name" symbol="tny_pair_get_name">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyPair*"/>
				</parameters>
			</method>
			<method name="get_value" symbol="tny_pair_get_value">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyPair*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_pair_new">
				<return-type type="TnyPair*"/>
				<parameters>
					<parameter name="name" type="gchar*"/>
					<parameter name="value" type="gchar*"/>
				</parameters>
			</constructor>
			<method name="set_name" symbol="tny_pair_set_name">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyPair*"/>
					<parameter name="name" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_value" symbol="tny_pair_set_value">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyPair*"/>
					<parameter name="value" type="gchar*"/>
				</parameters>
			</method>
		</object>
		<object name="TnySimpleList" parent="GObject" type-name="TnySimpleList" get-type="tny_simple_list_get_type">
			<implements>
				<interface name="TnyList"/>
			</implements>
			<constructor name="new" symbol="tny_simple_list_new">
				<return-type type="TnyList*"/>
			</constructor>
		</object>
		<interface name="TnyAccount" type-name="TnyAccount" get-type="tny_account_get_type">
			<requires>
				<interface name="GObject"/>
			</requires>
			<method name="cancel" symbol="tny_account_cancel">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="get_account_type" symbol="tny_account_get_account_type">
				<return-type type="TnyAccountType"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="get_connection_policy" symbol="tny_account_get_connection_policy">
				<return-type type="TnyConnectionPolicy*"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="get_connection_status" symbol="tny_account_get_connection_status">
				<return-type type="TnyConnectionStatus"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="get_forget_pass_func" symbol="tny_account_get_forget_pass_func">
				<return-type type="TnyForgetPassFunc"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="get_hostname" symbol="tny_account_get_hostname">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="get_id" symbol="tny_account_get_id">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="get_name" symbol="tny_account_get_name">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="get_pass_func" symbol="tny_account_get_pass_func">
				<return-type type="TnyGetPassFunc"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="get_port" symbol="tny_account_get_port">
				<return-type type="guint"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="get_proto" symbol="tny_account_get_proto">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="get_secure_auth_mech" symbol="tny_account_get_secure_auth_mech">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="get_url_string" symbol="tny_account_get_url_string">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="get_user" symbol="tny_account_get_user">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="is_ready" symbol="tny_account_is_ready">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="matches_url_string" symbol="tny_account_matches_url_string">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="url_string" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_connection_policy" symbol="tny_account_set_connection_policy">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="policy" type="TnyConnectionPolicy*"/>
				</parameters>
			</method>
			<method name="set_forget_pass_func" symbol="tny_account_set_forget_pass_func">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="forget_pass_func" type="TnyForgetPassFunc"/>
				</parameters>
			</method>
			<method name="set_hostname" symbol="tny_account_set_hostname">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="host" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_id" symbol="tny_account_set_id">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="id" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_name" symbol="tny_account_set_name">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="name" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_pass_func" symbol="tny_account_set_pass_func">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="get_pass_func" type="TnyGetPassFunc"/>
				</parameters>
			</method>
			<method name="set_port" symbol="tny_account_set_port">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="port" type="guint"/>
				</parameters>
			</method>
			<method name="set_proto" symbol="tny_account_set_proto">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="proto" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_secure_auth_mech" symbol="tny_account_set_secure_auth_mech">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="mech" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_url_string" symbol="tny_account_set_url_string">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="url_string" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_user" symbol="tny_account_set_user">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="user" type="gchar*"/>
				</parameters>
			</method>
			<method name="start_operation" symbol="tny_account_start_operation">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="domain" type="TnyStatusDomain"/>
					<parameter name="code" type="TnyStatusCode"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="status_user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="stop_operation" symbol="tny_account_stop_operation">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="cancelled" type="gboolean*"/>
				</parameters>
			</method>
			<signal name="changed" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</signal>
			<signal name="connection-status-changed" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="status" type="gint"/>
				</parameters>
			</signal>
			<vfunc name="cancel">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_account_type">
				<return-type type="TnyAccountType"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_connection_policy">
				<return-type type="TnyConnectionPolicy*"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_connection_status">
				<return-type type="TnyConnectionStatus"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_forget_pass_func">
				<return-type type="TnyForgetPassFunc"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_hostname">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_id">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_name">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_pass_func">
				<return-type type="TnyGetPassFunc"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_port">
				<return-type type="guint"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_proto">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_secure_auth_mech">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_url_string">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_user">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="is_ready">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="matches_url_string">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="url_string" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_connection_policy">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="policy" type="TnyConnectionPolicy*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_forget_pass_func">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="forget_pass_func" type="TnyForgetPassFunc"/>
				</parameters>
			</vfunc>
			<vfunc name="set_hostname">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="host" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_id">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="id" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_name">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="name" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_pass_func">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="get_pass_func" type="TnyGetPassFunc"/>
				</parameters>
			</vfunc>
			<vfunc name="set_port">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="port" type="guint"/>
				</parameters>
			</vfunc>
			<vfunc name="set_proto">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="proto" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_secure_auth_mech">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="mech" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_url_string">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="url_string" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_user">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="user" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="start_operation">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="domain" type="TnyStatusDomain"/>
					<parameter name="code" type="TnyStatusCode"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="status_user_data" type="gpointer"/>
				</parameters>
			</vfunc>
			<vfunc name="stop_operation">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="cancelled" type="gboolean*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyAccountStore" type-name="TnyAccountStore" get-type="tny_account_store_get_type">
			<requires>
				<interface name="GObject"/>
			</requires>
			<method name="alert" symbol="tny_account_store_alert">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyAccountStore*"/>
					<parameter name="account" type="TnyAccount*"/>
					<parameter name="type" type="TnyAlertType"/>
					<parameter name="question" type="gboolean"/>
					<parameter name="error" type="GError*"/>
				</parameters>
			</method>
			<method name="find_account" symbol="tny_account_store_find_account">
				<return-type type="TnyAccount*"/>
				<parameters>
					<parameter name="self" type="TnyAccountStore*"/>
					<parameter name="url_string" type="gchar*"/>
				</parameters>
			</method>
			<method name="get_accounts" symbol="tny_account_store_get_accounts">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccountStore*"/>
					<parameter name="list" type="TnyList*"/>
					<parameter name="types" type="TnyGetAccountsRequestType"/>
				</parameters>
			</method>
			<method name="get_cache_dir" symbol="tny_account_store_get_cache_dir">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyAccountStore*"/>
				</parameters>
			</method>
			<method name="get_device" symbol="tny_account_store_get_device">
				<return-type type="TnyDevice*"/>
				<parameters>
					<parameter name="self" type="TnyAccountStore*"/>
				</parameters>
			</method>
			<signal name="connecting-started" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccountStore*"/>
				</parameters>
			</signal>
			<vfunc name="alert">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyAccountStore*"/>
					<parameter name="account" type="TnyAccount*"/>
					<parameter name="type" type="TnyAlertType"/>
					<parameter name="question" type="gboolean"/>
					<parameter name="error" type="GError*"/>
				</parameters>
			</vfunc>
			<vfunc name="find_account">
				<return-type type="TnyAccount*"/>
				<parameters>
					<parameter name="self" type="TnyAccountStore*"/>
					<parameter name="url_string" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_accounts">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccountStore*"/>
					<parameter name="list" type="TnyList*"/>
					<parameter name="types" type="TnyGetAccountsRequestType"/>
				</parameters>
			</vfunc>
			<vfunc name="get_cache_dir">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyAccountStore*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_device">
				<return-type type="TnyDevice*"/>
				<parameters>
					<parameter name="self" type="TnyAccountStore*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyConnectionPolicy" type-name="TnyConnectionPolicy" get-type="tny_connection_policy_get_type">
			<method name="on_connect" symbol="tny_connection_policy_on_connect">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyConnectionPolicy*"/>
					<parameter name="account" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="on_connection_broken" symbol="tny_connection_policy_on_connection_broken">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyConnectionPolicy*"/>
					<parameter name="account" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="on_disconnect" symbol="tny_connection_policy_on_disconnect">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyConnectionPolicy*"/>
					<parameter name="account" type="TnyAccount*"/>
				</parameters>
			</method>
			<method name="set_current" symbol="tny_connection_policy_set_current">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyConnectionPolicy*"/>
					<parameter name="account" type="TnyAccount*"/>
					<parameter name="folder" type="TnyFolder*"/>
				</parameters>
			</method>
			<vfunc name="on_connect">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyConnectionPolicy*"/>
					<parameter name="account" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="on_connection_broken">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyConnectionPolicy*"/>
					<parameter name="account" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="on_disconnect">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyConnectionPolicy*"/>
					<parameter name="account" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_current">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyConnectionPolicy*"/>
					<parameter name="account" type="TnyAccount*"/>
					<parameter name="folder" type="TnyFolder*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyDevice" type-name="TnyDevice" get-type="tny_device_get_type">
			<requires>
				<interface name="GObject"/>
			</requires>
			<method name="force_offline" symbol="tny_device_force_offline">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyDevice*"/>
				</parameters>
			</method>
			<method name="force_online" symbol="tny_device_force_online">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyDevice*"/>
				</parameters>
			</method>
			<method name="is_online" symbol="tny_device_is_online">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyDevice*"/>
				</parameters>
			</method>
			<method name="reset" symbol="tny_device_reset">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyDevice*"/>
				</parameters>
			</method>
			<signal name="connection-changed" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyDevice*"/>
					<parameter name="online" type="gboolean"/>
				</parameters>
			</signal>
			<vfunc name="force_offline">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyDevice*"/>
				</parameters>
			</vfunc>
			<vfunc name="force_online">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyDevice*"/>
				</parameters>
			</vfunc>
			<vfunc name="is_online">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyDevice*"/>
				</parameters>
			</vfunc>
			<vfunc name="reset">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyDevice*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyFolder" type-name="TnyFolder" get-type="tny_folder_get_type">
			<requires>
				<interface name="GObject"/>
			</requires>
			<method name="add_msg" symbol="tny_folder_add_msg">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="msg" type="TnyMsg*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="add_msg_async" symbol="tny_folder_add_msg_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="msg" type="TnyMsg*"/>
					<parameter name="callback" type="TnyFolderCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="add_observer" symbol="tny_folder_add_observer">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="observer" type="TnyFolderObserver*"/>
				</parameters>
			</method>
			<method name="copy" symbol="tny_folder_copy">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="into" type="TnyFolderStore*"/>
					<parameter name="new_name" type="gchar*"/>
					<parameter name="del" type="gboolean"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="copy_async" symbol="tny_folder_copy_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="into" type="TnyFolderStore*"/>
					<parameter name="new_name" type="gchar*"/>
					<parameter name="del" type="gboolean"/>
					<parameter name="callback" type="TnyCopyFolderCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="find_msg" symbol="tny_folder_find_msg">
				<return-type type="TnyMsg*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="url_string" type="gchar*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="get_account" symbol="tny_folder_get_account">
				<return-type type="TnyAccount*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="get_all_count" symbol="tny_folder_get_all_count">
				<return-type type="guint"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="get_caps" symbol="tny_folder_get_caps">
				<return-type type="TnyFolderCaps"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="get_folder_store" symbol="tny_folder_get_folder_store">
				<return-type type="TnyFolderStore*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="get_folder_type" symbol="tny_folder_get_folder_type">
				<return-type type="TnyFolderType"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="get_headers" symbol="tny_folder_get_headers">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="headers" type="TnyList*"/>
					<parameter name="refresh" type="gboolean"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="get_headers_async" symbol="tny_folder_get_headers_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="headers" type="TnyList*"/>
					<parameter name="refresh" type="gboolean"/>
					<parameter name="callback" type="TnyGetHeadersCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="get_id" symbol="tny_folder_get_id">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="get_local_size" symbol="tny_folder_get_local_size">
				<return-type type="guint"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="get_msg" symbol="tny_folder_get_msg">
				<return-type type="TnyMsg*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="get_msg_async" symbol="tny_folder_get_msg_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="callback" type="TnyGetMsgCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="get_msg_receive_strategy" symbol="tny_folder_get_msg_receive_strategy">
				<return-type type="TnyMsgReceiveStrategy*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="get_msg_remove_strategy" symbol="tny_folder_get_msg_remove_strategy">
				<return-type type="TnyMsgRemoveStrategy*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="get_name" symbol="tny_folder_get_name">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="get_stats" symbol="tny_folder_get_stats">
				<return-type type="TnyFolderStats*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="get_unread_count" symbol="tny_folder_get_unread_count">
				<return-type type="guint"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="get_url_string" symbol="tny_folder_get_url_string">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="is_subscribed" symbol="tny_folder_is_subscribed">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="poke_status" symbol="tny_folder_poke_status">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="refresh" symbol="tny_folder_refresh">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="refresh_async" symbol="tny_folder_refresh_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="callback" type="TnyFolderCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="remove_msg" symbol="tny_folder_remove_msg">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="remove_msgs" symbol="tny_folder_remove_msgs">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="headers" type="TnyList*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="remove_msgs_async" symbol="tny_folder_remove_msgs_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="headers" type="TnyList*"/>
					<parameter name="callback" type="TnyFolderCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="remove_observer" symbol="tny_folder_remove_observer">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="observer" type="TnyFolderObserver*"/>
				</parameters>
			</method>
			<method name="set_msg_receive_strategy" symbol="tny_folder_set_msg_receive_strategy">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="st" type="TnyMsgReceiveStrategy*"/>
				</parameters>
			</method>
			<method name="set_msg_remove_strategy" symbol="tny_folder_set_msg_remove_strategy">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="st" type="TnyMsgRemoveStrategy*"/>
				</parameters>
			</method>
			<method name="sync" symbol="tny_folder_sync">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="expunge" type="gboolean"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="sync_async" symbol="tny_folder_sync_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="expunge" type="gboolean"/>
					<parameter name="callback" type="TnyFolderCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="transfer_msgs" symbol="tny_folder_transfer_msgs">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="header_list" type="TnyList*"/>
					<parameter name="folder_dst" type="TnyFolder*"/>
					<parameter name="delete_originals" type="gboolean"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="transfer_msgs_async" symbol="tny_folder_transfer_msgs_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="header_list" type="TnyList*"/>
					<parameter name="folder_dst" type="TnyFolder*"/>
					<parameter name="delete_originals" type="gboolean"/>
					<parameter name="callback" type="TnyTransferMsgsCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<vfunc name="add_msg">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="msg" type="TnyMsg*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="add_msg_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="msg" type="TnyMsg*"/>
					<parameter name="callback" type="TnyFolderCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</vfunc>
			<vfunc name="add_observer">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="observer" type="TnyFolderObserver*"/>
				</parameters>
			</vfunc>
			<vfunc name="copy">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="into" type="TnyFolderStore*"/>
					<parameter name="new_name" type="gchar*"/>
					<parameter name="del" type="gboolean"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="copy_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="into" type="TnyFolderStore*"/>
					<parameter name="new_name" type="gchar*"/>
					<parameter name="del" type="gboolean"/>
					<parameter name="callback" type="TnyCopyFolderCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</vfunc>
			<vfunc name="find_msg">
				<return-type type="TnyMsg*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="url_string" type="gchar*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="get_account">
				<return-type type="TnyAccount*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_all_count">
				<return-type type="guint"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_caps">
				<return-type type="TnyFolderCaps"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_folder_store">
				<return-type type="TnyFolderStore*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_folder_type">
				<return-type type="TnyFolderType"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_headers">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="headers" type="TnyList*"/>
					<parameter name="refresh" type="gboolean"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="get_headers_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="headers" type="TnyList*"/>
					<parameter name="refresh" type="gboolean"/>
					<parameter name="callback" type="TnyGetHeadersCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</vfunc>
			<vfunc name="get_id">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_local_size">
				<return-type type="guint"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_msg">
				<return-type type="TnyMsg*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="get_msg_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="callback" type="TnyGetMsgCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</vfunc>
			<vfunc name="get_msg_receive_strategy">
				<return-type type="TnyMsgReceiveStrategy*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_msg_remove_strategy">
				<return-type type="TnyMsgRemoveStrategy*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_name">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_stats">
				<return-type type="TnyFolderStats*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_unread_count">
				<return-type type="guint"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_url_string">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</vfunc>
			<vfunc name="is_subscribed">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</vfunc>
			<vfunc name="poke_status">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
				</parameters>
			</vfunc>
			<vfunc name="refresh">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="refresh_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="callback" type="TnyFolderCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</vfunc>
			<vfunc name="remove_msg">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="remove_msgs">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="headers" type="TnyList*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="remove_msgs_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="headers" type="TnyList*"/>
					<parameter name="callback" type="TnyFolderCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</vfunc>
			<vfunc name="remove_observer">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="observer" type="TnyFolderObserver*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_msg_receive_strategy">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="st" type="TnyMsgReceiveStrategy*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_msg_remove_strategy">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="st" type="TnyMsgRemoveStrategy*"/>
				</parameters>
			</vfunc>
			<vfunc name="sync">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="expunge" type="gboolean"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="sync_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="expunge" type="gboolean"/>
					<parameter name="callback" type="TnyFolderCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</vfunc>
			<vfunc name="transfer_msgs">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="header_list" type="TnyList*"/>
					<parameter name="folder_dst" type="TnyFolder*"/>
					<parameter name="delete_originals" type="gboolean"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="transfer_msgs_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolder*"/>
					<parameter name="header_list" type="TnyList*"/>
					<parameter name="folder_dst" type="TnyFolder*"/>
					<parameter name="delete_originals" type="gboolean"/>
					<parameter name="callback" type="TnyTransferMsgsCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyFolderObserver" type-name="TnyFolderObserver" get-type="tny_folder_observer_get_type">
			<method name="update" symbol="tny_folder_observer_update">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderObserver*"/>
					<parameter name="change" type="TnyFolderChange*"/>
				</parameters>
			</method>
			<vfunc name="update">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderObserver*"/>
					<parameter name="change" type="TnyFolderChange*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyFolderStore" type-name="TnyFolderStore" get-type="tny_folder_store_get_type">
			<method name="add_observer" symbol="tny_folder_store_add_observer">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="observer" type="TnyFolderStoreObserver*"/>
				</parameters>
			</method>
			<method name="create_folder" symbol="tny_folder_store_create_folder">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="name" type="gchar*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="create_folder_async" symbol="tny_folder_store_create_folder_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="name" type="gchar*"/>
					<parameter name="callback" type="TnyCreateFolderCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="get_folders" symbol="tny_folder_store_get_folders">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="list" type="TnyList*"/>
					<parameter name="query" type="TnyFolderStoreQuery*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="get_folders_async" symbol="tny_folder_store_get_folders_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="list" type="TnyList*"/>
					<parameter name="query" type="TnyFolderStoreQuery*"/>
					<parameter name="callback" type="TnyGetFoldersCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="remove_folder" symbol="tny_folder_store_remove_folder">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="folder" type="TnyFolder*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="remove_observer" symbol="tny_folder_store_remove_observer">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="observer" type="TnyFolderStoreObserver*"/>
				</parameters>
			</method>
			<vfunc name="add_observer">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="observer" type="TnyFolderStoreObserver*"/>
				</parameters>
			</vfunc>
			<vfunc name="create_folder">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="name" type="gchar*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="create_folder_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="name" type="gchar*"/>
					<parameter name="callback" type="TnyCreateFolderCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</vfunc>
			<vfunc name="get_folders">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="list" type="TnyList*"/>
					<parameter name="query" type="TnyFolderStoreQuery*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="get_folders_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="list" type="TnyList*"/>
					<parameter name="query" type="TnyFolderStoreQuery*"/>
					<parameter name="callback" type="TnyGetFoldersCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</vfunc>
			<vfunc name="remove_folder">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="folder" type="TnyFolder*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="remove_observer">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="observer" type="TnyFolderStoreObserver*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyFolderStoreObserver" type-name="TnyFolderStoreObserver" get-type="tny_folder_store_observer_get_type">
			<method name="update" symbol="tny_folder_store_observer_update">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStoreObserver*"/>
					<parameter name="change" type="TnyFolderStoreChange*"/>
				</parameters>
			</method>
			<vfunc name="update">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStoreObserver*"/>
					<parameter name="change" type="TnyFolderStoreChange*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyHeader" type-name="TnyHeader" get-type="tny_header_get_type">
			<requires>
				<interface name="GObject"/>
			</requires>
			<method name="dup_bcc" symbol="tny_header_dup_bcc">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</method>
			<method name="dup_cc" symbol="tny_header_dup_cc">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</method>
			<method name="dup_from" symbol="tny_header_dup_from">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</method>
			<method name="dup_message_id" symbol="tny_header_dup_message_id">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</method>
			<method name="dup_replyto" symbol="tny_header_dup_replyto">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</method>
			<method name="dup_subject" symbol="tny_header_dup_subject">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</method>
			<method name="dup_to" symbol="tny_header_dup_to">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</method>
			<method name="dup_uid" symbol="tny_header_dup_uid">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</method>
			<method name="get_date_received" symbol="tny_header_get_date_received">
				<return-type type="time_t"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</method>
			<method name="get_date_sent" symbol="tny_header_get_date_sent">
				<return-type type="time_t"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</method>
			<method name="get_flags" symbol="tny_header_get_flags">
				<return-type type="TnyHeaderFlags"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</method>
			<method name="get_folder" symbol="tny_header_get_folder">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</method>
			<method name="get_message_size" symbol="tny_header_get_message_size">
				<return-type type="guint"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</method>
			<method name="get_priority" symbol="tny_header_get_priority">
				<return-type type="TnyHeaderFlags"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</method>
			<method name="set_bcc" symbol="tny_header_set_bcc">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="bcc" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_cc" symbol="tny_header_set_cc">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="cc" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_flag" symbol="tny_header_set_flag">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="mask" type="TnyHeaderFlags"/>
				</parameters>
			</method>
			<method name="set_from" symbol="tny_header_set_from">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="from" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_priority" symbol="tny_header_set_priority">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="priority" type="TnyHeaderFlags"/>
				</parameters>
			</method>
			<method name="set_replyto" symbol="tny_header_set_replyto">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="to" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_subject" symbol="tny_header_set_subject">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="subject" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_to" symbol="tny_header_set_to">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="to" type="gchar*"/>
				</parameters>
			</method>
			<method name="unset_flag" symbol="tny_header_unset_flag">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="mask" type="TnyHeaderFlags"/>
				</parameters>
			</method>
			<vfunc name="dup_bcc">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</vfunc>
			<vfunc name="dup_cc">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</vfunc>
			<vfunc name="dup_from">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</vfunc>
			<vfunc name="dup_message_id">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</vfunc>
			<vfunc name="dup_replyto">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</vfunc>
			<vfunc name="dup_subject">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</vfunc>
			<vfunc name="dup_to">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</vfunc>
			<vfunc name="dup_uid">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_date_received">
				<return-type type="time_t"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_date_sent">
				<return-type type="time_t"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_flags">
				<return-type type="TnyHeaderFlags"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_folder">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_message_size">
				<return-type type="guint"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_bcc">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="bcc" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_cc">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="cc" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_flag">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="mask" type="TnyHeaderFlags"/>
				</parameters>
			</vfunc>
			<vfunc name="set_from">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="from" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_replyto">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="to" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_subject">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="subject" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_to">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="to" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="unset_flag">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeader*"/>
					<parameter name="mask" type="TnyHeaderFlags"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyIterator" type-name="TnyIterator" get-type="tny_iterator_get_type">
			<method name="first" symbol="tny_iterator_first">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyIterator*"/>
				</parameters>
			</method>
			<method name="get_current" symbol="tny_iterator_get_current">
				<return-type type="GObject*"/>
				<parameters>
					<parameter name="self" type="TnyIterator*"/>
				</parameters>
			</method>
			<method name="get_list" symbol="tny_iterator_get_list">
				<return-type type="TnyList*"/>
				<parameters>
					<parameter name="self" type="TnyIterator*"/>
				</parameters>
			</method>
			<method name="is_done" symbol="tny_iterator_is_done">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyIterator*"/>
				</parameters>
			</method>
			<method name="next" symbol="tny_iterator_next">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyIterator*"/>
				</parameters>
			</method>
			<method name="nth" symbol="tny_iterator_nth">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyIterator*"/>
					<parameter name="nth" type="guint"/>
				</parameters>
			</method>
			<method name="prev" symbol="tny_iterator_prev">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyIterator*"/>
				</parameters>
			</method>
			<vfunc name="first">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyIterator*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_current">
				<return-type type="GObject*"/>
				<parameters>
					<parameter name="self" type="TnyIterator*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_list">
				<return-type type="TnyList*"/>
				<parameters>
					<parameter name="self" type="TnyIterator*"/>
				</parameters>
			</vfunc>
			<vfunc name="is_done">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyIterator*"/>
				</parameters>
			</vfunc>
			<vfunc name="next">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyIterator*"/>
				</parameters>
			</vfunc>
			<vfunc name="nth">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyIterator*"/>
					<parameter name="nth" type="guint"/>
				</parameters>
			</vfunc>
			<vfunc name="prev">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyIterator*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyList" type-name="TnyList" get-type="tny_list_get_type">
			<method name="append" symbol="tny_list_append">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyList*"/>
					<parameter name="item" type="GObject*"/>
				</parameters>
			</method>
			<method name="copy" symbol="tny_list_copy">
				<return-type type="TnyList*"/>
				<parameters>
					<parameter name="self" type="TnyList*"/>
				</parameters>
			</method>
			<method name="create_iterator" symbol="tny_list_create_iterator">
				<return-type type="TnyIterator*"/>
				<parameters>
					<parameter name="self" type="TnyList*"/>
				</parameters>
			</method>
			<method name="foreach" symbol="tny_list_foreach">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyList*"/>
					<parameter name="func" type="GFunc"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="get_length" symbol="tny_list_get_length">
				<return-type type="guint"/>
				<parameters>
					<parameter name="self" type="TnyList*"/>
				</parameters>
			</method>
			<method name="prepend" symbol="tny_list_prepend">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyList*"/>
					<parameter name="item" type="GObject*"/>
				</parameters>
			</method>
			<method name="remove" symbol="tny_list_remove">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyList*"/>
					<parameter name="item" type="GObject*"/>
				</parameters>
			</method>
			<method name="remove_matches" symbol="tny_list_remove_matches">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyList*"/>
					<parameter name="matcher" type="TnyListMatcher"/>
					<parameter name="match_data" type="gpointer"/>
				</parameters>
			</method>
			<vfunc name="append">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyList*"/>
					<parameter name="item" type="GObject*"/>
				</parameters>
			</vfunc>
			<vfunc name="copy">
				<return-type type="TnyList*"/>
				<parameters>
					<parameter name="self" type="TnyList*"/>
				</parameters>
			</vfunc>
			<vfunc name="create_iterator">
				<return-type type="TnyIterator*"/>
				<parameters>
					<parameter name="self" type="TnyList*"/>
				</parameters>
			</vfunc>
			<vfunc name="foreach">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyList*"/>
					<parameter name="func" type="GFunc"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</vfunc>
			<vfunc name="get_length">
				<return-type type="guint"/>
				<parameters>
					<parameter name="self" type="TnyList*"/>
				</parameters>
			</vfunc>
			<vfunc name="prepend">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyList*"/>
					<parameter name="item" type="GObject*"/>
				</parameters>
			</vfunc>
			<vfunc name="remove">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyList*"/>
					<parameter name="item" type="GObject*"/>
				</parameters>
			</vfunc>
			<vfunc name="remove_matches">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyList*"/>
					<parameter name="matcher" type="TnyListMatcher"/>
					<parameter name="match_data" type="gpointer"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyLockable" type-name="TnyLockable" get-type="tny_lockable_get_type">
			<method name="lock" symbol="tny_lockable_lock">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyLockable*"/>
				</parameters>
			</method>
			<method name="unlock" symbol="tny_lockable_unlock">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyLockable*"/>
				</parameters>
			</method>
			<vfunc name="lock">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyLockable*"/>
				</parameters>
			</vfunc>
			<vfunc name="unlock">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyLockable*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyMimePart" type-name="TnyMimePart" get-type="tny_mime_part_get_type">
			<method name="add_part" symbol="tny_mime_part_add_part">
				<return-type type="gint"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</method>
			<method name="construct" symbol="tny_mime_part_construct">
				<return-type type="gint"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="stream" type="TnyStream*"/>
					<parameter name="mime_type" type="gchar*"/>
					<parameter name="transfer_encoding" type="gchar*"/>
				</parameters>
			</method>
			<method name="content_type_is" symbol="tny_mime_part_content_type_is">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="type" type="gchar*"/>
				</parameters>
			</method>
			<method name="decode_to_stream" symbol="tny_mime_part_decode_to_stream">
				<return-type type="gssize"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="stream" type="TnyStream*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="decode_to_stream_async" symbol="tny_mime_part_decode_to_stream_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="stream" type="TnyStream*"/>
					<parameter name="callback" type="TnyMimePartCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="del_part" symbol="tny_mime_part_del_part">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</method>
			<method name="get_content_id" symbol="tny_mime_part_get_content_id">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</method>
			<method name="get_content_location" symbol="tny_mime_part_get_content_location">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</method>
			<method name="get_content_type" symbol="tny_mime_part_get_content_type">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</method>
			<method name="get_decoded_stream" symbol="tny_mime_part_get_decoded_stream">
				<return-type type="TnyStream*"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</method>
			<method name="get_description" symbol="tny_mime_part_get_description">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</method>
			<method name="get_filename" symbol="tny_mime_part_get_filename">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</method>
			<method name="get_header_pairs" symbol="tny_mime_part_get_header_pairs">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="list" type="TnyList*"/>
				</parameters>
			</method>
			<method name="get_parts" symbol="tny_mime_part_get_parts">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="list" type="TnyList*"/>
				</parameters>
			</method>
			<method name="get_stream" symbol="tny_mime_part_get_stream">
				<return-type type="TnyStream*"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</method>
			<method name="get_transfer_encoding" symbol="tny_mime_part_get_transfer_encoding">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</method>
			<method name="is_attachment" symbol="tny_mime_part_is_attachment">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</method>
			<method name="is_purged" symbol="tny_mime_part_is_purged">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</method>
			<method name="set_content_id" symbol="tny_mime_part_set_content_id">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="content_id" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_content_location" symbol="tny_mime_part_set_content_location">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="content_location" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_content_type" symbol="tny_mime_part_set_content_type">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="contenttype" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_description" symbol="tny_mime_part_set_description">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="description" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_filename" symbol="tny_mime_part_set_filename">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="filename" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_header_pair" symbol="tny_mime_part_set_header_pair">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="name" type="gchar*"/>
					<parameter name="value" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_purged" symbol="tny_mime_part_set_purged">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</method>
			<method name="set_transfer_encoding" symbol="tny_mime_part_set_transfer_encoding">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="transfer_encoding" type="gchar*"/>
				</parameters>
			</method>
			<method name="write_to_stream" symbol="tny_mime_part_write_to_stream">
				<return-type type="gssize"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="stream" type="TnyStream*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<vfunc name="add_part">
				<return-type type="gint"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="construct">
				<return-type type="gint"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="stream" type="TnyStream*"/>
					<parameter name="mime_type" type="gchar*"/>
					<parameter name="transfer_encoding" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="content_type_is">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="content_type" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="decode_to_stream">
				<return-type type="gssize"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="stream" type="TnyStream*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="decode_to_stream_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="stream" type="TnyStream*"/>
					<parameter name="callback" type="TnyMimePartCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</vfunc>
			<vfunc name="del_part">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_content_id">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_content_location">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_content_type">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_decoded_stream">
				<return-type type="TnyStream*"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_description">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_filename">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_header_pairs">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="list" type="TnyList*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_parts">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="list" type="TnyList*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_stream">
				<return-type type="TnyStream*"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_transfer_encoding">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="is_attachment">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="is_purged">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_content_id">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="content_id" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_content_location">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="content_location" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_content_type">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="contenttype" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_description">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="description" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_filename">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="filename" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_header_pair">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="name" type="gchar*"/>
					<parameter name="value" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_purged">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_transfer_encoding">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="transfer_encoding" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="write_to_stream">
				<return-type type="gssize"/>
				<parameters>
					<parameter name="self" type="TnyMimePart*"/>
					<parameter name="stream" type="TnyStream*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyMsg" type-name="TnyMsg" get-type="tny_msg_get_type">
			<requires>
				<interface name="TnyMimePart"/>
				<interface name="GObject"/>
			</requires>
			<method name="get_folder" symbol="tny_msg_get_folder">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnyMsg*"/>
				</parameters>
			</method>
			<method name="get_header" symbol="tny_msg_get_header">
				<return-type type="TnyHeader*"/>
				<parameters>
					<parameter name="self" type="TnyMsg*"/>
				</parameters>
			</method>
			<method name="get_url_string" symbol="tny_msg_get_url_string">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyMsg*"/>
				</parameters>
			</method>
			<method name="rewrite_cache" symbol="tny_msg_rewrite_cache">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsg*"/>
				</parameters>
			</method>
			<method name="uncache_attachments" symbol="tny_msg_uncache_attachments">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsg*"/>
				</parameters>
			</method>
			<vfunc name="get_folder">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnyMsg*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_header">
				<return-type type="TnyHeader*"/>
				<parameters>
					<parameter name="self" type="TnyMsg*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_url_string">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyMsg*"/>
				</parameters>
			</vfunc>
			<vfunc name="rewrite_cache">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsg*"/>
				</parameters>
			</vfunc>
			<vfunc name="uncache_attachments">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsg*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyMsgReceiveStrategy" type-name="TnyMsgReceiveStrategy" get-type="tny_msg_receive_strategy_get_type">
			<method name="perform_get_msg" symbol="tny_msg_receive_strategy_perform_get_msg">
				<return-type type="TnyMsg*"/>
				<parameters>
					<parameter name="self" type="TnyMsgReceiveStrategy*"/>
					<parameter name="folder" type="TnyFolder*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<vfunc name="perform_get_msg">
				<return-type type="TnyMsg*"/>
				<parameters>
					<parameter name="self" type="TnyMsgReceiveStrategy*"/>
					<parameter name="folder" type="TnyFolder*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyMsgRemoveStrategy" type-name="TnyMsgRemoveStrategy" get-type="tny_msg_remove_strategy_get_type">
			<method name="perform_remove" symbol="tny_msg_remove_strategy_perform_remove">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsgRemoveStrategy*"/>
					<parameter name="folder" type="TnyFolder*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<vfunc name="perform_remove">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsgRemoveStrategy*"/>
					<parameter name="folder" type="TnyFolder*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyPasswordGetter" type-name="TnyPasswordGetter" get-type="tny_password_getter_get_type">
			<method name="forget_password" symbol="tny_password_getter_forget_password">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyPasswordGetter*"/>
					<parameter name="aid" type="gchar*"/>
				</parameters>
			</method>
			<method name="get_password" symbol="tny_password_getter_get_password">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyPasswordGetter*"/>
					<parameter name="aid" type="gchar*"/>
					<parameter name="prompt" type="gchar*"/>
					<parameter name="cancel" type="gboolean*"/>
				</parameters>
			</method>
			<vfunc name="forget_password">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyPasswordGetter*"/>
					<parameter name="aid" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_password">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyPasswordGetter*"/>
					<parameter name="aid" type="gchar*"/>
					<parameter name="prompt" type="gchar*"/>
					<parameter name="cancel" type="gboolean*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnySeekable" type-name="TnySeekable" get-type="tny_seekable_get_type">
			<method name="seek" symbol="tny_seekable_seek">
				<return-type type="off_t"/>
				<parameters>
					<parameter name="self" type="TnySeekable*"/>
					<parameter name="offset" type="off_t"/>
					<parameter name="policy" type="int"/>
				</parameters>
			</method>
			<method name="set_bounds" symbol="tny_seekable_set_bounds">
				<return-type type="gint"/>
				<parameters>
					<parameter name="self" type="TnySeekable*"/>
					<parameter name="start" type="off_t"/>
					<parameter name="end" type="off_t"/>
				</parameters>
			</method>
			<method name="tell" symbol="tny_seekable_tell">
				<return-type type="off_t"/>
				<parameters>
					<parameter name="self" type="TnySeekable*"/>
				</parameters>
			</method>
			<vfunc name="seek">
				<return-type type="off_t"/>
				<parameters>
					<parameter name="self" type="TnySeekable*"/>
					<parameter name="offset" type="off_t"/>
					<parameter name="policy" type="int"/>
				</parameters>
			</vfunc>
			<vfunc name="set_bounds">
				<return-type type="gint"/>
				<parameters>
					<parameter name="self" type="TnySeekable*"/>
					<parameter name="start" type="off_t"/>
					<parameter name="end" type="off_t"/>
				</parameters>
			</vfunc>
			<vfunc name="tell">
				<return-type type="off_t"/>
				<parameters>
					<parameter name="self" type="TnySeekable*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnySendQueue" type-name="TnySendQueue" get-type="tny_send_queue_get_type">
			<requires>
				<interface name="GObject"/>
			</requires>
			<method name="add" symbol="tny_send_queue_add">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnySendQueue*"/>
					<parameter name="msg" type="TnyMsg*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="add_async" symbol="tny_send_queue_add_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnySendQueue*"/>
					<parameter name="msg" type="TnyMsg*"/>
					<parameter name="callback" type="TnySendQueueAddCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="cancel" symbol="tny_send_queue_cancel">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnySendQueue*"/>
					<parameter name="cancel_action" type="TnySendQueueCancelAction"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="get_outbox" symbol="tny_send_queue_get_outbox">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnySendQueue*"/>
				</parameters>
			</method>
			<method name="get_sentbox" symbol="tny_send_queue_get_sentbox">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnySendQueue*"/>
				</parameters>
			</method>
			<signal name="error-happened" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnySendQueue*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="msg" type="TnyMsg*"/>
					<parameter name="err" type="gpointer"/>
				</parameters>
			</signal>
			<signal name="msg-sending" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnySendQueue*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="msg" type="TnyMsg*"/>
					<parameter name="nth" type="guint"/>
					<parameter name="total" type="guint"/>
				</parameters>
			</signal>
			<signal name="msg-sent" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnySendQueue*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="msg" type="TnyMsg*"/>
					<parameter name="nth" type="guint"/>
					<parameter name="total" type="guint"/>
				</parameters>
			</signal>
			<signal name="queue-start" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnySendQueue*"/>
				</parameters>
			</signal>
			<signal name="queue-stop" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnySendQueue*"/>
				</parameters>
			</signal>
			<vfunc name="add">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnySendQueue*"/>
					<parameter name="msg" type="TnyMsg*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="add_async">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnySendQueue*"/>
					<parameter name="msg" type="TnyMsg*"/>
					<parameter name="callback" type="TnySendQueueAddCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</vfunc>
			<vfunc name="cancel">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnySendQueue*"/>
					<parameter name="cancel_action" type="TnySendQueueCancelAction"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="get_outbox">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnySendQueue*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_sentbox">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnySendQueue*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyStoreAccount" type-name="TnyStoreAccount" get-type="tny_store_account_get_type">
			<requires>
				<interface name="TnyFolderStore"/>
				<interface name="TnyAccount"/>
				<interface name="GObject"/>
			</requires>
			<method name="delete_cache" symbol="tny_store_account_delete_cache">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyStoreAccount*"/>
				</parameters>
			</method>
			<method name="find_folder" symbol="tny_store_account_find_folder">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnyStoreAccount*"/>
					<parameter name="url_string" type="gchar*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="subscribe" symbol="tny_store_account_subscribe">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyStoreAccount*"/>
					<parameter name="folder" type="TnyFolder*"/>
				</parameters>
			</method>
			<method name="unsubscribe" symbol="tny_store_account_unsubscribe">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyStoreAccount*"/>
					<parameter name="folder" type="TnyFolder*"/>
				</parameters>
			</method>
			<signal name="subscription-changed" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyStoreAccount*"/>
					<parameter name="folder" type="TnyFolder*"/>
				</parameters>
			</signal>
			<vfunc name="delete_cache">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyStoreAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="find_folder">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnyStoreAccount*"/>
					<parameter name="url_string" type="gchar*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
			<vfunc name="subscribe">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyStoreAccount*"/>
					<parameter name="folder" type="TnyFolder*"/>
				</parameters>
			</vfunc>
			<vfunc name="unsubscribe">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyStoreAccount*"/>
					<parameter name="folder" type="TnyFolder*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyStream" type-name="TnyStream" get-type="tny_stream_get_type">
			<method name="close" symbol="tny_stream_close">
				<return-type type="gint"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
				</parameters>
			</method>
			<method name="flush" symbol="tny_stream_flush">
				<return-type type="gint"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
				</parameters>
			</method>
			<method name="is_eos" symbol="tny_stream_is_eos">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
				</parameters>
			</method>
			<method name="read" symbol="tny_stream_read">
				<return-type type="gssize"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
					<parameter name="buffer" type="char*"/>
					<parameter name="n" type="gsize"/>
				</parameters>
			</method>
			<method name="reset" symbol="tny_stream_reset">
				<return-type type="gint"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
				</parameters>
			</method>
			<method name="write" symbol="tny_stream_write">
				<return-type type="gssize"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
					<parameter name="buffer" type="char*"/>
					<parameter name="n" type="gsize"/>
				</parameters>
			</method>
			<method name="write_to_stream" symbol="tny_stream_write_to_stream">
				<return-type type="gssize"/>
				<parameters>
					<parameter name="self" type="TnyStream*"/>
					<parameter name="output" type="TnyStream*"/>
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
		</interface>
		<interface name="TnyTransportAccount" type-name="TnyTransportAccount" get-type="tny_transport_account_get_type">
			<requires>
				<interface name="TnyAccount"/>
				<interface name="GObject"/>
			</requires>
			<method name="send" symbol="tny_transport_account_send">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyTransportAccount*"/>
					<parameter name="msg" type="TnyMsg*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<vfunc name="send">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyTransportAccount*"/>
					<parameter name="msg" type="TnyMsg*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
		</interface>
		<constant name="TNY_HEADER_FLAG_PRIORITY_MASK" type="int" value="1536"/>
		<constant name="TNY_PRIORITY_LOWER_THAN_GTK_REDRAWS" type="int" value="30"/>
	</namespace>
</api>
