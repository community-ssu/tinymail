#! /usr/bin/env python 
import gobject
import tinymail

import os

from gobject import GObject

from xml.dom import minidom
from xml.dom import getDOMImplementation
from xml.parsers.expat import ExpatError

from tinymail import Account, AccountStore
from tinymail.camel import *

from nmdevice import NMDevice

accountFields = [
	'name',
	'secure_auth_mech',
	'proto',
	'user',
	'hostname',
	'port']

ACCOUNT_ELEMENT = 'account'

class XmlAccountStore(gobject.GObject, AccountStore):
	"""
	An implementation of the account store interface
	that uses XML to store the user accounts.
	"""

	def __init__(self, filename):
		GObject.__init__(self)
		self.__accounts = []
		self.next_id = 0
		self.cacheName = filename
		self.__device = NMDevice()

		self.__session = SessionCamel(self)
		self.__session.set_initialized()

		self.__load_cache()
	
	def do_get_accounts(self, list, types):
		store = False
		transport = False
		if types == tinymail.ACCOUNT_STORE_STORE_ACCOUNTS:
			store = True
		elif types == tinymail.ACCOUNT_STORE_TRANSPORT_ACCOUNTS:
			transport = True
		else:
			store = True
			transport = True
		for account in self.__accounts:
			if gobject.type_is_a(account.__gtype__, tinymail.StoreAccount) and store:
				list.append(account)
			elif gobject.type_is_a(account._gtype__, tinymail.TransportAccount) and transport:
				list.append(account)

	def do_get_cache_dir(self):
		return self.cacheName

	def do_get_device(self):
		return self.__device

	def do_find_account(selfi, urlString):
		for account in self.__accounts:
			if account.matches_url_string(urlString):
				return account

	def new_account(self, *args, **kwargs):
		account = None
		if 'proto' in kwargs:
			proto = kwargs['proto']
		else:
			proto = 'imap'
		if proto == 'imap':
			account = CamelIMAPStoreAccount() 
		elif proto == 'nntp':
			pass
		elif proto == 'pop':
			pass
		elif proto == 'smtp':
			pass

		if account:
			for key, element in kwargs.items():
				setMethod = getattr(account, 'set_%s' % key)
				setMethod(element)
			account.set_id('%s;%d' % (kwargs['name'], self.next_id))
			account.set_session(self.__session)
			self.next_id += 1
			self.__accounts.append(account)

	def __load_cache(self):
		try:
			self.xmlCache = minidom.parse(self.cacheName)
			top = self.xmlCache.documentElement
			accountElements = top.getElementsByTagName(ACCOUNT_ELEMENT)
			for element in accountElements:
				accountData = self.__load_account(element)
				self.new_account(self, **accountData)
		except (IOError, ExpatError):
			xmlImp = getDOMImplementation()
			self.xmlCache = xmlImp.createDocument(None, "Tinymail_Account_Cache", None)

	def write_cache(self):
		file = open(self.cacheName, 'w')
		top = self.xmlCache.documentElement
		for account in self.__accounts:
			self.__store_account(account, self.xmlCache, top)
		file.write(self.xmlCache.toxml())

	def __store_account(self, account, doc, node):
		accElement = doc.createElement(ACCOUNT_ELEMENT)
		for field in accountFields:
			element = doc.createElement(field)
			getMethod = getattr(account, "get_%s" % field)
			if getMethod:
				text = doc.createTextNode(str(getMethod()))
				element.appendChild(text)
			accElement.appendChild(element)
		node.appendChild(accElement)

	def __load_account(self, node):
		accountData = {}
		for field in accountFields:
			if field == 'port':
				elements = node.getElementsByTagName(field)
				if elements[0].firstChild:
					accountData[field] = int(elements[0].firstChild.data)
			else:
				elements = node.getElementsByTagName(field)
				if elements[0].firstChild:
					accountData[field] = elements[0].firstChild.data
		return accountData

gobject.type_register(XmlAccountStore)

