<?xml version="1.0"?>
<api version="1.0">
	<namespace name="Tny">
		<callback name="TnyCamelGetSupportedSecureAuthCallback">
			<return-type type="void"/>
			<parameters>
				<parameter name="self" type="TnyCamelAccount*"/>
				<parameter name="cancelled" type="gboolean"/>
				<parameter name="auth_types" type="TnyList*"/>
				<parameter name="err" type="GError*"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<callback name="TnyCamelSetOnlineCallback">
			<return-type type="void"/>
			<parameters>
				<parameter name="account" type="TnyCamelAccount*"/>
				<parameter name="canceled" type="gboolean"/>
				<parameter name="err" type="GError*"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<struct name="TnySessionCamel">
			<method name="new" symbol="tny_session_camel_new">
				<return-type type="TnySessionCamel*"/>
				<parameters>
					<parameter name="account_store" type="TnyAccountStore*"/>
				</parameters>
			</method>
			<method name="set_account_store" symbol="tny_session_camel_set_account_store">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnySessionCamel*"/>
					<parameter name="account_store" type="TnyAccountStore*"/>
				</parameters>
			</method>
			<method name="set_device" symbol="tny_session_camel_set_device">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnySessionCamel*"/>
					<parameter name="device" type="TnyDevice*"/>
				</parameters>
			</method>
			<method name="set_initialized" symbol="tny_session_camel_set_initialized">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnySessionCamel*"/>
				</parameters>
			</method>
			<method name="set_ui_locker" symbol="tny_session_camel_set_ui_locker">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnySessionCamel*"/>
					<parameter name="ui_lock" type="TnyLockable*"/>
				</parameters>
			</method>
			<field name="parent_object" type="CamelSession"/>
			<field name="priv" type="TnySessionCamelPriv*"/>
		</struct>
		<struct name="TnySessionCamelClass">
			<field name="parent_class" type="CamelSessionClass"/>
		</struct>
		<struct name="TnySessionCamelPriv">
		</struct>
		<struct name="TnyStreamCamel">
			<method name="new" symbol="tny_stream_camel_new">
				<return-type type="CamelStream*"/>
				<parameters>
					<parameter name="stream" type="TnyStream*"/>
				</parameters>
			</method>
			<method name="set_stream" symbol="tny_stream_camel_set_stream">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyStreamCamel*"/>
					<parameter name="stream" type="TnyStream*"/>
				</parameters>
			</method>
			<method name="write_to_stream" symbol="tny_stream_camel_write_to_stream">
				<return-type type="gssize"/>
				<parameters>
					<parameter name="self" type="TnyStreamCamel*"/>
					<parameter name="output" type="TnyStream*"/>
				</parameters>
			</method>
			<field name="parent" type="CamelSeekableSubstream"/>
			<field name="stream" type="TnyStream*"/>
		</struct>
		<struct name="TnyStreamCamelClass">
			<field name="parent" type="CamelSeekableSubstreamClass"/>
		</struct>
		<object name="TnyCamelAccount" parent="GObject" type-name="TnyCamelAccount" get-type="tny_camel_account_get_type">
			<implements>
				<interface name="TnyAccount"/>
			</implements>
			<method name="add_option" symbol="tny_camel_account_add_option">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelAccount*"/>
					<parameter name="option" type="TnyPair*"/>
				</parameters>
			</method>
			<method name="clear_options" symbol="tny_camel_account_clear_options">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelAccount*"/>
				</parameters>
			</method>
			<method name="get_options" symbol="tny_camel_account_get_options">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelAccount*"/>
					<parameter name="options" type="TnyList*"/>
				</parameters>
			</method>
			<method name="get_supported_secure_authentication" symbol="tny_camel_account_get_supported_secure_authentication">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelAccount*"/>
					<parameter name="callback" type="TnyCamelGetSupportedSecureAuthCallback"/>
					<parameter name="status_callback" type="TnyStatusCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="set_online" symbol="tny_camel_account_set_online">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelAccount*"/>
					<parameter name="online" type="gboolean"/>
					<parameter name="callback" type="TnyCamelSetOnlineCallback"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="set_session" symbol="tny_camel_account_set_session">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelAccount*"/>
					<parameter name="session" type="TnySessionCamel*"/>
				</parameters>
			</method>
			<signal name="set-online-happened" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelAccount*"/>
					<parameter name="online" type="gboolean"/>
				</parameters>
			</signal>
			<vfunc name="add_option">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelAccount*"/>
					<parameter name="option" type="TnyPair*"/>
				</parameters>
			</vfunc>
			<vfunc name="cancel">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="clear_options">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="get_account_type">
				<return-type type="TnyAccountType"/>
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
			<vfunc name="get_options">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelAccount*"/>
					<parameter name="options" type="TnyList*"/>
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
			<vfunc name="matches_url_string">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="url_string" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="prepare">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelAccount*"/>
					<parameter name="recon_if" type="gboolean"/>
					<parameter name="reservice" type="gboolean"/>
				</parameters>
			</vfunc>
			<vfunc name="set_forget_pass_func">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="get_forget_pass_func" type="TnyForgetPassFunc"/>
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
			<vfunc name="set_online">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelAccount*"/>
					<parameter name="online" type="gboolean"/>
					<parameter name="callback" type="TnyCamelSetOnlineCallback"/>
					<parameter name="user_data" type="gpointer"/>
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
					<parameter name="name" type="gchar*"/>
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
					<parameter name="canceled" type="gboolean*"/>
				</parameters>
			</vfunc>
			<vfunc name="try_connect">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccount*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
		</object>
		<object name="TnyCamelBsMimePart" parent="GObject" type-name="TnyCamelBsMimePart" get-type="tny_camel_bs_mime_part_get_type">
			<implements>
				<interface name="TnyMimePart"/>
			</implements>
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
		</object>
		<object name="TnyCamelBsMsg" parent="TnyCamelBsMimePart" type-name="TnyCamelBsMsg" get-type="tny_camel_bs_msg_get_type">
			<implements>
				<interface name="TnyMimePart"/>
				<interface name="TnyMsg"/>
			</implements>
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
		</object>
		<object name="TnyCamelBsMsgReceiveStrategy" parent="GObject" type-name="TnyCamelBsMsgReceiveStrategy" get-type="tny_camel_bs_msg_receive_strategy_get_type">
			<implements>
				<interface name="TnyMsgReceiveStrategy"/>
			</implements>
			<constructor name="new" symbol="tny_camel_bs_msg_receive_strategy_new">
				<return-type type="TnyMsgReceiveStrategy*"/>
			</constructor>
			<method name="start_receiving_part" symbol="tny_camel_bs_msg_receive_strategy_start_receiving_part">
				<return-type type="TnyStream*"/>
				<parameters>
					<parameter name="self" type="TnyCamelBsMsgReceiveStrategy*"/>
					<parameter name="folder" type="TnyFolder*"/>
					<parameter name="part" type="TnyCamelBsMimePart*"/>
					<parameter name="binary" type="gboolean*"/>
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
		</object>
		<object name="TnyCamelDefaultConnectionPolicy" parent="GObject" type-name="TnyCamelDefaultConnectionPolicy" get-type="tny_camel_default_connection_policy_get_type">
			<implements>
				<interface name="TnyConnectionPolicy"/>
			</implements>
			<constructor name="new" symbol="tny_camel_default_connection_policy_new">
				<return-type type="TnyConnectionPolicy*"/>
			</constructor>
		</object>
		<object name="TnyCamelFolder" parent="GObject" type-name="TnyCamelFolder" get-type="tny_camel_folder_get_type">
			<implements>
				<interface name="TnyFolderStore"/>
				<interface name="TnyFolder"/>
			</implements>
			<method name="get_full_name" symbol="tny_camel_folder_get_full_name">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="self" type="TnyCamelFolder*"/>
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
			<vfunc name="add_store_observer">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="observer" type="TnyFolderStoreObserver*"/>
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
			<vfunc name="remove_folder">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="folder" type="TnyFolder*"/>
					<parameter name="err" type="GError**"/>
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
			<vfunc name="remove_store_observer">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyFolderStore*"/>
					<parameter name="observer" type="TnyFolderStoreObserver*"/>
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
					<parameter name="headers" type="TnyList*"/>
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
		</object>
		<object name="TnyCamelFullMsgReceiveStrategy" parent="GObject" type-name="TnyCamelFullMsgReceiveStrategy" get-type="tny_camel_full_msg_receive_strategy_get_type">
			<implements>
				<interface name="TnyMsgReceiveStrategy"/>
			</implements>
			<constructor name="new" symbol="tny_camel_full_msg_receive_strategy_new">
				<return-type type="TnyMsgReceiveStrategy*"/>
			</constructor>
			<vfunc name="perform_get_msg">
				<return-type type="TnyMsg*"/>
				<parameters>
					<parameter name="self" type="TnyMsgReceiveStrategy*"/>
					<parameter name="folder" type="TnyFolder*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
		</object>
		<object name="TnyCamelHeader" parent="GObject" type-name="TnyCamelHeader" get-type="tny_camel_header_get_type">
			<implements>
				<interface name="TnyHeader"/>
			</implements>
		</object>
		<object name="TnyCamelIMAPFolder" parent="TnyCamelFolder" type-name="TnyCamelIMAPFolder" get-type="tny_camel_imap_folder_get_type">
			<implements>
				<interface name="TnyFolderStore"/>
				<interface name="TnyFolder"/>
			</implements>
		</object>
		<object name="TnyCamelIMAPStoreAccount" parent="TnyCamelStoreAccount" type-name="TnyCamelIMAPStoreAccount" get-type="tny_camel_imap_store_account_get_type">
			<implements>
				<interface name="TnyFolderStore"/>
				<interface name="TnyAccount"/>
				<interface name="TnyStoreAccount"/>
			</implements>
			<constructor name="new" symbol="tny_camel_imap_store_account_new">
				<return-type type="TnyStoreAccount*"/>
			</constructor>
		</object>
		<object name="TnyCamelMemStream" parent="TnyCamelStream" type-name="TnyCamelMemStream" get-type="tny_camel_mem_stream_get_type">
			<implements>
				<interface name="TnyStream"/>
				<interface name="TnySeekable"/>
			</implements>
			<constructor name="new" symbol="tny_camel_mem_stream_new">
				<return-type type="TnyStream*"/>
			</constructor>
			<constructor name="new_with_buffer" symbol="tny_camel_mem_stream_new_with_buffer">
				<return-type type="TnyStream*"/>
				<parameters>
					<parameter name="buffer" type="char*"/>
					<parameter name="len" type="size_t"/>
				</parameters>
			</constructor>
		</object>
		<object name="TnyCamelMimePart" parent="GObject" type-name="TnyCamelMimePart" get-type="tny_camel_mime_part_get_type">
			<implements>
				<interface name="TnyMimePart"/>
			</implements>
			<method name="get_part" symbol="tny_camel_mime_part_get_part">
				<return-type type="CamelMimePart*"/>
				<parameters>
					<parameter name="self" type="TnyCamelMimePart*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_camel_mime_part_new">
				<return-type type="TnyMimePart*"/>
			</constructor>
			<constructor name="new_with_part" symbol="tny_camel_mime_part_new_with_part">
				<return-type type="TnyMimePart*"/>
				<parameters>
					<parameter name="part" type="CamelMimePart*"/>
				</parameters>
			</constructor>
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
		</object>
		<object name="TnyCamelMsg" parent="TnyCamelMimePart" type-name="TnyCamelMsg" get-type="tny_camel_msg_get_type">
			<implements>
				<interface name="TnyMimePart"/>
				<interface name="TnyMsg"/>
			</implements>
			<constructor name="new" symbol="tny_camel_msg_new">
				<return-type type="TnyMsg*"/>
			</constructor>
			<constructor name="new_with_part" symbol="tny_camel_msg_new_with_part">
				<return-type type="TnyMsg*"/>
				<parameters>
					<parameter name="part" type="CamelMimePart*"/>
				</parameters>
			</constructor>
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
		</object>
		<object name="TnyCamelMsgRemoveStrategy" parent="GObject" type-name="TnyCamelMsgRemoveStrategy" get-type="tny_camel_msg_remove_strategy_get_type">
			<implements>
				<interface name="TnyMsgRemoveStrategy"/>
			</implements>
			<constructor name="new" symbol="tny_camel_msg_remove_strategy_new">
				<return-type type="TnyMsgRemoveStrategy*"/>
			</constructor>
			<vfunc name="perform_remove">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsgRemoveStrategy*"/>
					<parameter name="folder" type="TnyFolder*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
		</object>
		<object name="TnyCamelNNTPFolder" parent="TnyCamelFolder" type-name="TnyCamelNNTPFolder" get-type="tny_camel_nntp_folder_get_type">
			<implements>
				<interface name="TnyFolderStore"/>
				<interface name="TnyFolder"/>
			</implements>
		</object>
		<object name="TnyCamelNNTPStoreAccount" parent="TnyCamelStoreAccount" type-name="TnyCamelNNTPStoreAccount" get-type="tny_camel_nntp_store_account_get_type">
			<implements>
				<interface name="TnyFolderStore"/>
				<interface name="TnyAccount"/>
				<interface name="TnyStoreAccount"/>
			</implements>
			<constructor name="new" symbol="tny_camel_nntp_store_account_new">
				<return-type type="TnyStoreAccount*"/>
			</constructor>
		</object>
		<object name="TnyCamelPOPFolder" parent="TnyCamelFolder" type-name="TnyCamelPOPFolder" get-type="tny_camel_pop_folder_get_type">
			<implements>
				<interface name="TnyFolderStore"/>
				<interface name="TnyFolder"/>
			</implements>
		</object>
		<object name="TnyCamelPOPStoreAccount" parent="TnyCamelStoreAccount" type-name="TnyCamelPOPStoreAccount" get-type="tny_camel_pop_store_account_get_type">
			<implements>
				<interface name="TnyFolderStore"/>
				<interface name="TnyAccount"/>
				<interface name="TnyStoreAccount"/>
			</implements>
			<constructor name="new" symbol="tny_camel_pop_store_account_new">
				<return-type type="TnyStoreAccount*"/>
			</constructor>
			<method name="reconnect" symbol="tny_camel_pop_store_account_reconnect">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelPOPStoreAccount*"/>
				</parameters>
			</method>
			<method name="set_leave_messages_on_server" symbol="tny_camel_pop_store_account_set_leave_messages_on_server">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelPOPStoreAccount*"/>
					<parameter name="enabled" type="gboolean"/>
				</parameters>
			</method>
		</object>
		<object name="TnyCamelPartialMsgReceiveStrategy" parent="GObject" type-name="TnyCamelPartialMsgReceiveStrategy" get-type="tny_camel_partial_msg_receive_strategy_get_type">
			<implements>
				<interface name="TnyMsgReceiveStrategy"/>
			</implements>
			<constructor name="new" symbol="tny_camel_partial_msg_receive_strategy_new">
				<return-type type="TnyMsgReceiveStrategy*"/>
			</constructor>
			<vfunc name="perform_get_msg">
				<return-type type="TnyMsg*"/>
				<parameters>
					<parameter name="self" type="TnyMsgReceiveStrategy*"/>
					<parameter name="folder" type="TnyFolder*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
		</object>
		<object name="TnyCamelPopRemoteMsgRemoveStrategy" parent="GObject" type-name="TnyCamelPopRemoteMsgRemoveStrategy" get-type="tny_camel_pop_remote_msg_remove_strategy_get_type">
			<implements>
				<interface name="TnyMsgRemoveStrategy"/>
			</implements>
			<constructor name="new" symbol="tny_camel_pop_remote_msg_remove_strategy_new">
				<return-type type="TnyMsgRemoveStrategy*"/>
			</constructor>
			<vfunc name="perform_remove">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsgRemoveStrategy*"/>
					<parameter name="folder" type="TnyFolder*"/>
					<parameter name="header" type="TnyHeader*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
		</object>
		<object name="TnyCamelRecoverConnectionPolicy" parent="GObject" type-name="TnyCamelRecoverConnectionPolicy" get-type="tny_camel_recover_connection_policy_get_type">
			<implements>
				<interface name="TnyConnectionPolicy"/>
			</implements>
			<constructor name="new" symbol="tny_camel_recover_connection_policy_new">
				<return-type type="TnyConnectionPolicy*"/>
			</constructor>
			<method name="set_reconnect_delay" symbol="tny_camel_recover_connection_policy_set_reconnect_delay">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelRecoverConnectionPolicy*"/>
					<parameter name="milliseconds" type="gint"/>
				</parameters>
			</method>
			<method name="set_recover_active_folder" symbol="tny_camel_recover_connection_policy_set_recover_active_folder">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelRecoverConnectionPolicy*"/>
					<parameter name="setting" type="gboolean"/>
				</parameters>
			</method>
		</object>
		<object name="TnyCamelSendQueue" parent="GObject" type-name="TnyCamelSendQueue" get-type="tny_camel_send_queue_get_type">
			<implements>
				<interface name="TnyFolderObserver"/>
				<interface name="TnySendQueue"/>
			</implements>
			<method name="flush" symbol="tny_camel_send_queue_flush">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelSendQueue*"/>
				</parameters>
			</method>
			<method name="get_transport_account" symbol="tny_camel_send_queue_get_transport_account">
				<return-type type="TnyCamelTransportAccount*"/>
				<parameters>
					<parameter name="self" type="TnyCamelSendQueue*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_camel_send_queue_new">
				<return-type type="TnySendQueue*"/>
				<parameters>
					<parameter name="trans_account" type="TnyCamelTransportAccount*"/>
				</parameters>
			</constructor>
			<method name="set_transport_account" symbol="tny_camel_send_queue_set_transport_account">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyCamelSendQueue*"/>
					<parameter name="trans_account" type="TnyCamelTransportAccount*"/>
				</parameters>
			</method>
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
		</object>
		<object name="TnyCamelStoreAccount" parent="TnyCamelAccount" type-name="TnyCamelStoreAccount" get-type="tny_camel_store_account_get_type">
			<implements>
				<interface name="TnyFolderStore"/>
				<interface name="TnyAccount"/>
				<interface name="TnyStoreAccount"/>
			</implements>
			<method name="factor_folder" symbol="tny_camel_store_account_factor_folder">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnyCamelStoreAccount*"/>
					<parameter name="full_name" type="gchar*"/>
					<parameter name="was_new" type="gboolean*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_camel_store_account_new">
				<return-type type="TnyStoreAccount*"/>
			</constructor>
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
			<vfunc name="delete_cache">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyStoreAccount*"/>
				</parameters>
			</vfunc>
			<vfunc name="factor_folder">
				<return-type type="TnyFolder*"/>
				<parameters>
					<parameter name="self" type="TnyCamelStoreAccount*"/>
					<parameter name="full_name" type="gchar*"/>
					<parameter name="was_new" type="gboolean*"/>
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
		</object>
		<object name="TnyCamelStream" parent="GObject" type-name="TnyCamelStream" get-type="tny_camel_stream_get_type">
			<implements>
				<interface name="TnyStream"/>
				<interface name="TnySeekable"/>
			</implements>
			<method name="get_stream" symbol="tny_camel_stream_get_stream">
				<return-type type="CamelStream*"/>
				<parameters>
					<parameter name="self" type="TnyCamelStream*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_camel_stream_new">
				<return-type type="TnyStream*"/>
				<parameters>
					<parameter name="stream" type="CamelStream*"/>
				</parameters>
			</constructor>
		</object>
		<object name="TnyCamelTransportAccount" parent="TnyCamelAccount" type-name="TnyCamelTransportAccount" get-type="tny_camel_transport_account_get_type">
			<implements>
				<interface name="TnyAccount"/>
				<interface name="TnyTransportAccount"/>
			</implements>
			<constructor name="new" symbol="tny_camel_transport_account_new">
				<return-type type="TnyTransportAccount*"/>
			</constructor>
			<vfunc name="send">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyTransportAccount*"/>
					<parameter name="msg" type="TnyMsg*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</vfunc>
		</object>
	</namespace>
</api>
