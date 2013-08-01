#!/usr/bin/python

import sys
sys.path.append('stats/')
from hdata import Events

print """/*
	harris - a strategy game
	Copyright (C) 2012-2013 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	events.h: maps event names to ids
*/
"""

for i,e in enumerate(Events):
	print "#define EVENT_%s %d" % (e['id'], i)

print "#define NEVENTS %d" % len(Events)
print
print "const char *event_names[%d]={" % len(Events)
for e in Events:
	print '\t"%s",' % e['id']
print "};"
