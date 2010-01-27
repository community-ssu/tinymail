<?xml version="1.0"?>
<api version="1.0">
	<namespace name="Tny">
		<interface name="TnyAccountStoreView" type-name="TnyAccountStoreView" get-type="tny_account_store_view_get_type">
			<requires>
				<interface name="GObject"/>
			</requires>
			<method name="set_account_store" symbol="tny_account_store_view_set_account_store">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccountStoreView*"/>
					<parameter name="account_store" type="TnyAccountStore*"/>
				</parameters>
			</method>
			<vfunc name="set_account_store">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyAccountStoreView*"/>
					<parameter name="account_store" type="TnyAccountStore*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyHeaderView" type-name="TnyHeaderView" get-type="tny_header_view_get_type">
			<requires>
				<interface name="GObject"/>
			</requires>
			<method name="clear" symbol="tny_header_view_clear">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeaderView*"/>
				</parameters>
			</method>
			<method name="set_header" symbol="tny_header_view_set_header">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyHeaderView*"/>
					<parameter name="header" type="TnyHeader*"/>
				</parameters>
			</method>
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
		</interface>
		<interface name="TnyMimePartSaveStrategy" type-name="TnyMimePartSaveStrategy" get-type="tny_mime_part_save_strategy_get_type">
			<requires>
				<interface name="GObject"/>
			</requires>
			<method name="perform_save" symbol="tny_mime_part_save_strategy_perform_save">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartSaveStrategy*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</method>
			<vfunc name="perform_save">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartSaveStrategy*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyMimePartSaver" type-name="TnyMimePartSaver" get-type="tny_mime_part_saver_get_type">
			<method name="get_save_strategy" symbol="tny_mime_part_saver_get_save_strategy">
				<return-type type="TnyMimePartSaveStrategy*"/>
				<parameters>
					<parameter name="self" type="TnyMimePartSaver*"/>
				</parameters>
			</method>
			<method name="save" symbol="tny_mime_part_saver_save">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartSaver*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</method>
			<method name="set_save_strategy" symbol="tny_mime_part_saver_set_save_strategy">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartSaver*"/>
					<parameter name="strategy" type="TnyMimePartSaveStrategy*"/>
				</parameters>
			</method>
			<vfunc name="get_save_strategy">
				<return-type type="TnyMimePartSaveStrategy*"/>
				<parameters>
					<parameter name="self" type="TnyMimePartSaver*"/>
				</parameters>
			</vfunc>
			<vfunc name="save">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartSaver*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_save_strategy">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartSaver*"/>
					<parameter name="strategy" type="TnyMimePartSaveStrategy*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyMimePartView" type-name="TnyMimePartView" get-type="tny_mime_part_view_get_type">
			<method name="clear" symbol="tny_mime_part_view_clear">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
				</parameters>
			</method>
			<method name="get_part" symbol="tny_mime_part_view_get_part">
				<return-type type="TnyMimePart*"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
				</parameters>
			</method>
			<method name="set_part" symbol="tny_mime_part_view_set_part">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMimePartView*"/>
					<parameter name="mime_part" type="TnyMimePart*"/>
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
		</interface>
		<interface name="TnyMsgView" type-name="TnyMsgView" get-type="tny_msg_view_get_type">
			<requires>
				<interface name="TnyMimePartView"/>
			</requires>
			<method name="clear" symbol="tny_msg_view_clear">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
				</parameters>
			</method>
			<method name="create_mime_part_view_for" symbol="tny_msg_view_create_mime_part_view_for">
				<return-type type="TnyMimePartView*"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
					<parameter name="part" type="TnyMimePart*"/>
				</parameters>
			</method>
			<method name="create_new_inline_viewer" symbol="tny_msg_view_create_new_inline_viewer">
				<return-type type="TnyMsgView*"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
				</parameters>
			</method>
			<method name="get_msg" symbol="tny_msg_view_get_msg">
				<return-type type="TnyMsg*"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
				</parameters>
			</method>
			<method name="set_msg" symbol="tny_msg_view_set_msg">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
					<parameter name="msg" type="TnyMsg*"/>
				</parameters>
			</method>
			<method name="set_unavailable" symbol="tny_msg_view_set_unavailable">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
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
			<vfunc name="set_msg">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
					<parameter name="msg" type="TnyMsg*"/>
				</parameters>
			</vfunc>
			<vfunc name="set_unavailable">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="TnyMsgView*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnyMsgWindow" type-name="TnyMsgWindow" get-type="tny_msg_window_get_type">
			<requires>
				<interface name="TnyMimePartView"/>
				<interface name="TnyMsgView"/>
			</requires>
		</interface>
		<interface name="TnyPlatformFactory" type-name="TnyPlatformFactory" get-type="tny_platform_factory_get_type">
			<requires>
				<interface name="GObject"/>
			</requires>
			<method name="new_account_store" symbol="tny_platform_factory_new_account_store">
				<return-type type="TnyAccountStore*"/>
				<parameters>
					<parameter name="self" type="TnyPlatformFactory*"/>
				</parameters>
			</method>
			<method name="new_device" symbol="tny_platform_factory_new_device">
				<return-type type="TnyDevice*"/>
				<parameters>
					<parameter name="self" type="TnyPlatformFactory*"/>
				</parameters>
			</method>
			<method name="new_mime_part" symbol="tny_platform_factory_new_mime_part">
				<return-type type="TnyMimePart*"/>
				<parameters>
					<parameter name="self" type="TnyPlatformFactory*"/>
				</parameters>
			</method>
			<method name="new_msg" symbol="tny_platform_factory_new_msg">
				<return-type type="TnyMsg*"/>
				<parameters>
					<parameter name="self" type="TnyPlatformFactory*"/>
				</parameters>
			</method>
			<method name="new_msg_view" symbol="tny_platform_factory_new_msg_view">
				<return-type type="TnyMsgView*"/>
				<parameters>
					<parameter name="self" type="TnyPlatformFactory*"/>
				</parameters>
			</method>
			<method name="new_password_getter" symbol="tny_platform_factory_new_password_getter">
				<return-type type="TnyPasswordGetter*"/>
				<parameters>
					<parameter name="self" type="TnyPlatformFactory*"/>
				</parameters>
			</method>
			<vfunc name="new_account_store">
				<return-type type="TnyAccountStore*"/>
				<parameters>
					<parameter name="self" type="TnyPlatformFactory*"/>
				</parameters>
			</vfunc>
			<vfunc name="new_device">
				<return-type type="TnyDevice*"/>
				<parameters>
					<parameter name="self" type="TnyPlatformFactory*"/>
				</parameters>
			</vfunc>
			<vfunc name="new_mime_part">
				<return-type type="TnyMimePart*"/>
				<parameters>
					<parameter name="self" type="TnyPlatformFactory*"/>
				</parameters>
			</vfunc>
			<vfunc name="new_msg">
				<return-type type="TnyMsg*"/>
				<parameters>
					<parameter name="self" type="TnyPlatformFactory*"/>
				</parameters>
			</vfunc>
			<vfunc name="new_msg_view">
				<return-type type="TnyMsgView*"/>
				<parameters>
					<parameter name="self" type="TnyPlatformFactory*"/>
				</parameters>
			</vfunc>
			<vfunc name="new_password_getter">
				<return-type type="TnyPasswordGetter*"/>
				<parameters>
					<parameter name="self" type="TnyPlatformFactory*"/>
				</parameters>
			</vfunc>
		</interface>
		<interface name="TnySummaryView" type-name="TnySummaryView" get-type="tny_summary_view_get_type">
			<requires>
				<interface name="GObject"/>
			</requires>
		</interface>
	</namespace>
</api>
