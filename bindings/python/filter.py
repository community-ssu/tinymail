import sys
import re
import getopt
import os

def walk_dir (dirn):
	for root, dirs, files in os.walk(dirn):
		for filen in files:
			q = fregex.match (filen)
			if q != None:
	                	mf = open (os.path.join(root, filen), 'r')
        	        	for line in mf:
                			j = hregex.match(line)
                			if j != None:
                        			ifaces.append (j.groups ()[0])
		for mdirn in dirs:
			walk_dir (mdirn)
filename = None
topsrcdir = None
f = None

try:
	opts, args = getopt.getopt (sys.argv[1:], "he:t:", ["help", "extra=", "topsrcdir="])
except getopt.error, msg:
	sys.stdout.write (msg)
	sys.stdout.write ("for help use --help")
	sys.exit(2)

for o, a in opts:
	if o in ("-h", "--help"):
		sys.stdout.write ("--extra=FILENAME\n")
		sys.stdout.write ("--topsrcdir=DIR\n")
		sys.stdout.write ("--help\n")
		sys.exit(0)
	if o in ("-e", "--extra"):
		filename = a
	if o in ("-t", "--topsrcdir"):
		topsrcdir = a;

things = []
extra = []
ifaces = []

fregex = re.compile('.*\.h')
hregex = re.compile("^\s*struct _Tny(.*)Iface.*$")
eregex = re.compile('\(define-(.*)\s$')
tregex = re.compile('\(define-object (.*)\s$')
opened=True

if filename != None:
	f = open (filename, 'r')
	for line in f:
		e = eregex.match(line)
		if e != None:
			things.append (e.groups ()[0])
		extra.append (line)

if topsrcdir != None:
	walk_dir (topsrcdir)

#for line in ifaces:
#	sys.stdout.write (line + "\n")
#sys.exit (0)

for line in sys.stdin:
	t = tregex.match(line)
	if t != None:
		mytype = t.groups ()[0]
		if mytype in ifaces:
			line = "(define-interface " + mytype + "\n"
	
	m = eregex.match(line)
	if m != None:
		thing = m.groups ()[0]
		if thing in things:
			opened=False
	else:
		opened=True

	if opened:
		sys.stdout.write (line)
	else:
		if line == ')\n':
			opened=True

for line in extra:
	sys.stdout.write (line)

	
