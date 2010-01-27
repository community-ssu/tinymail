#! /usr/bin/env python 
from gobject import GObject
from tinymail import List, Iterator
from UserList import UserList

import tinymail
import gobject

class PyTnyList(GObject, UserList, List):
	"""
	An implementation of the Tinymail List interface
	based on a python list. 
	"""
	def __init__(self, sequence=[]):
		GObject.__init__(self)
		UserList.__init__(self, sequence)

	def do_get_length(self):
		return len(self)

	def do_prepend(self, obj):
		self.insert(0, obj)

	def do_append(self, obj):
		self.append(obj)

	def do_foreach(self, func, data):
		for item in self:
			func(item, data) 

	def do_create_iterator(self):
		return self.__ListIterator(tuple(self))

	def do_copy(self):
		return PyTnyList(self)

	class __ListIterator(GObject, Iterator):
		"""
		An implementation of the Tinymail iterator interface 
		"""
		def __init__(self, sequence):
			self.sequence = sequence
			self.position = 0
	
		def do_next(self):
			self.position += 1
	
		def do_prev(self):
			self.position -= 1
	
		def do_first(self):
			self.position = 0
	
		def do_nth(self, pos):
			self.position = pos
	
		def do_get_current(self):
			return self.sequence[self.position]
	
		def do_is_done(self):
			return not self.position < len(sequence)
	
		def do_get_list(self):
			return PyTnyList(self.sequence)

gobject.type_register(PyTnyList)

def print_pair(pair):
	print pair.get_name()
	print pair.get_value()

if __name__ == '__main__':
	a = PyTnyList()
	for i in range(10):
		List.append(a, tinymail.Pair(str(i), str(i)))
	print a.get_length()
	for i in range(10):
		a.prepend(tinymail.Pair(str(i), str(i)))
	print a.get_length()
	a.foreach(print_pair)
	b = a.copy()
	b.foreach(print_pair)
