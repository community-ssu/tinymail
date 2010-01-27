<?xml version="1.0"?>
<api version="1.0">
	<namespace name="Tny">
		<object name="TnyGnomeAccountStore" parent="GObject" type-name="TnyGnomeAccountStore" get-type="tny_gnome_account_store_get_type">
			<implements>
				<interface name="TnyAccountStore"/>
			</implements>
			<method name="get_session" symbol="tny_gnome_account_store_get_session">
				<return-type type="TnySessionCamel*"/>
				<parameters>
					<parameter name="self" type="TnyGnomeAccountStore*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="tny_gnome_account_store_new">
				<return-type type="TnyAccountStore*"/>
			</constructor>
		</object>
		<object name="TnyGnomeDevice" parent="GObject" type-name="TnyGnomeDevice" get-type="tny_gnome_device_get_type">
			<implements>
				<interface name="TnyDevice"/>
			</implements>
			<constructor name="new" symbol="tny_gnome_device_new">
				<return-type type="TnyDevice*"/>
			</constructor>
		</object>
		<object name="TnyGnomePlatformFactory" parent="GObject" type-name="TnyGnomePlatformFactory" get-type="tny_gnome_platform_factory_get_type">
			<implements>
				<interface name="TnyPlatformFactory"/>
			</implements>
			<method name="get_instance" symbol="tny_gnome_platform_factory_get_instance">
				<return-type type="TnyPlatformFactory*"/>
			</method>
		</object>
	</namespace>
</api>
