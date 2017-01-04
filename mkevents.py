#!/usr/bin/python2

import sys
sys.path.append('stats/')
from hdata import Events

assert len(sys.argv) == 2, sys.argv

if sys.argv[1] == 'h':
	print """/*
	harris - a strategy game
	Copyright (C) 2012-2013 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details

	events: maps event names to ids
*/

#ifndef HAVE_EVENTS_H
#define HAVE_EVENTS_H
"""

	for i,e in enumerate(Events):
		print "#define EVENT_%s %d" % (e['id'], i)

	print "#define NEVENTS %d" % len(Events)
	print
	print "extern const char *event_names[%d];" % len(Events)

	print
	print "#endif /* HAVE_EVENTS_H */"
else:
	assert sys.argv[1] == 'c', sys.argv
	print """/*
	harris - a strategy game
	Copyright (C) 2012-2013 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details

	events: maps event names to ids
*/

#include "events.h"
"""

	print "const char *event_names[%d]={" % len(Events)
	for e in Events:
		print '\t"%s",' % e['id']
	print "};"
