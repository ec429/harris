#!/usr/bin/python

import sys
sys.path.append('stats/')
from hdata import Events

assert len(sys.argv) == 2, sys.argv

if sys.argv[1] == 'h':
	print("""/*
	harris - a strategy game
	Copyright (C) 2012-2013 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details

	events: maps event names to ids
*/

#ifndef HAVE_EVENTS_H
#define HAVE_EVENTS_H
""")

	for i,e in enumerate(Events):
		ostr = "#define EVENT_%s %d" % (e['id'], i)
		print(ostr)

	ostr = "#define NEVENTS %d" % len(Events)
	print(ostr)
	print("")
	ostr = "extern const char *event_names[%d];" % len(Events)
	print(ostr)
	print("")
	ostr = "#endif /* HAVE_EVENTS_H */"
	print(ostr)
else:
	assert sys.argv[1] == 'c', sys.argv
	print("""/*
	harris - a strategy game
	Copyright (C) 2012-2013 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details

	events: maps event names to ids
*/

#include "events.h"
""")

	ostr = "const char *event_names[%d]={" % len(Events)
	print(ostr)
	for e in Events:
		ostr = '\t"%s",' % e['id']
		print(ostr)
	print("};")
