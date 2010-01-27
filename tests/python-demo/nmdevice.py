#! /usr/bin/env python 
import gobject
import tinymail

import dbus
from dbus.mainloop.glib import DBusGMainLoop

class NMDevice(gobject.GObject, tinymail.Device):
	"""
	An implementation of the tinymail device interface
	that uses Network manager.
	"""
	ACTIVE_NETWORK_CONNECTION = 3

	def __init__(self):
		gobject.GObject.__init__(self)
		self.__online = False
		self.__forced = False

		DBusGMainLoop(set_as_default=True)
		bus = dbus.SystemBus()
		nm_object = bus.get_object(
			'org.freedesktop.NetworkManager', 
			'/org/freedesktop/NetworkManager')
		nm_interface = dbus.Interface(
			nm_object, 
			dbus_interface='org.freedesktop.NetworkManager')

		self.__state = nm_interface.get_dbus_method('state')
		nm_interface.connect_to_signal('StateChange', self.__update_state)
		self.__state(reply_handler=self.__update_state,
				error_handler=self.__dbus_error)

	def do_is_online(self):
		return self.online

	def do_force_online(self):
		self.__forced = True
		self.online = True

	def do_force_offline(self):
		self.__forced = True
		self.online = False

	def reset(self):
		self.__forced = False
		self.__state(reply_handler=self.__update_state,
				error_handler=self.__dbus_error)

	def __update_state(self, state):
		if not self.__forced:
			self.online = (state == ACTIVE_NETWORK_CONNECTION)

	def __dbus_error(self, error):
		raise error

	def __get_online(self):
		return self.__online

	def __set_online(self, online):
		if (online != self.__online):
			self.emit("connection-changed", online)
			self.__online = online

	online = property(__get_online, __set_online)

gobject.type_register(NMDevice)
