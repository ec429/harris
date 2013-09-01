#!/usr/bin/python
"""hdata - Harris game data

The hdata module provides access to the Harris game data from the files
in 'dat/'.
"""

import sys
from hhist import date

class BadDataLine(Exception): pass
class ExcessValues(BadDataLine): pass

class Counter(object):
	def __init__(self, value):
		self.value = value
	def next(self, text):
		self.value += 1
		return self.value - 1

def inservice(date, b):
	"""Test whether a given Bomber or Fighter type is in service at a given date"""
	entry = b['entry']
	exit = b['exit']
	if entry and entry > date: return False
	if exit and exit < date: return False
	return True

class Table(object):
	def __init__(self, keys):
		counter = Counter(0)
		self.keys = [('i', counter.next)] + keys
		self.data = []
	def read(self, f):
		for line in f:
			line = line.rstrip('\n')
			if not line.strip(): continue
			if line.lstrip().startswith('#'): continue
			self.data.append(self.parse(line))
	def parse(self, line):
		values = [''] + line.split(':', len(self.keys))
		if len(values) > len(self.keys):
			raise ExcessValues(values, self.keys)
		return {key: parse(value) for ((key, parse), value) in zip(self.keys, values)}
	def __unicode__(self):
		widths = {k:max(len(str(k[0])), max((len(unicode(row[k[0]])) for row in self.data))) for k in self.keys}
		return '\n'.join((' '.join((str(k[0]).ljust(widths[k]) for k in self.keys)), '\n'.join((' '.join((unicode(row[k[0]]).ljust(widths[k]) for k in self.keys)) for row in self.data))))
	__str__ = __unicode__
	def __repr__(self):
		return repr(self.data)
	def __len__(self):
		return len(self.data)
	def __getitem__(self, index):
		return self.data[index]
	def find(self, key, value):
		for row in self.data:
			if row[key] == value:
				return row
		return None

def parse_string(text):
	return unicode(text.strip(), encoding='utf8')
parse_int = int
def parse_date(text):
	if text.strip():
		return date.parse(text)
	else:
		return None
def parse_flags(text):
	return [flag for flag in text.split(',') if flag]

# Bombers: MANUFACTURER:NAME:COST:SPEED:CEILING:CAPACITY:SVP:DEFENCE:FAILURE:ACCURACY:RANGE:BLAT:BLONG:DD-MM-YYYY:DD-MM-YYYY:NAVAIDS,FLAGS
Bombers = Table([('manf', parse_string),
				 ('name', parse_string), 
				 ('cost', parse_int),
				 ('speed', parse_int),
				 ('ceil', parse_int),
				 ('cap', parse_int),
				 ('svp', parse_int),
				 ('defn', parse_int),
				 ('fail', parse_int),
				 ('accu', parse_int),
				 ('range', parse_int),
				 ('blat', parse_int),
				 ('blong', parse_int),
				 ('entry', parse_date),
				 ('exit', parse_date),
				 ('flags', parse_flags), # Flags includes NAVAIDS
				 ])
Bombers.read(open('dat/bombers'))

# Events: ID:DATE
Events = Table([('id', parse_string), ('date', parse_date)])
Events.read(open('dat/events'))

# Fighters: MANUFACTURER:NAME:COST:SPEED:ARMAMENT:MNV:DD-MM-YYYY:DD-MM-YYYY:FLAGS
Fighters = Table([('manf', parse_string),
				  ('name', parse_string),
				  ('cost', parse_int),
				  ('speed', parse_int),
				  ('arm', parse_int),
				  ('mnv', parse_int),
				  ('entry', parse_date),
				  ('exit', parse_date),
				  ('flags', parse_flags),
				  ])
Fighters.read(open('dat/fighters'))

# Flak: STRENGTH:LAT:LONG:ENTRY:RADAR:EXIT
Flak = Table([('str', parse_int),
			  ('lat', parse_int),
			  ('long', parse_int),
			  ('entry', parse_date),
			  ('radar', parse_date),
			  ('exit', parse_date),
			  ])
Flak.read(open('dat/flak'))

# Ftrbases: LAT:LONG:ENTRY:EXIT
Ftrbases = Table([('lat', parse_int),
				  ('long', parse_int),
				  ('entry', parse_date),
				  ('exit', parse_date),
				  ])
Ftrbases.read(open('dat/ftrbases'))

# Targets: NAME:PROD:FLAK:ESIZ:LAT:LONG:DD-MM-YYYY:DD-MM-YYYY:CLASS[,Flags]
Targets = Table([('name', parse_string),
				 ('prod', parse_int),
				 ('flak', parse_int),
				 ('esiz', parse_int),
				 ('lat', parse_int),
				 ('long', parse_int),
				 ('entry', parse_date),
				 ('exit', parse_date),
				 ('flags', parse_flags), # Flags includes CLASS
				 ])
Targets.read(open('dat/targets'))

if __name__ == "__main__":
	if '--bombers' in sys.argv or '--all' in sys.argv:
		print '%d bombers' % len(Bombers)
		print Bombers
	if '--events' in sys.argv or '--all' in sys.argv:
		print '%d events' % len(Events)
		print Events
	if '--fighters' in sys.argv or '--all' in sys.argv:
		print '%d fighters' % len(Fighters)
		print Fighters
	if '--flaksites' in sys.argv or '--all' in sys.argv:
		print '%d flaksites' % len(Flak)
		print Flak
	if '--ftrbases' in sys.argv or '--all' in sys.argv:
		print '%d ftrbases' % len(Ftrbases)
		print Ftrbases
	if '--targets' in sys.argv or '--all' in sys.argv:
		print '%d targets' % len(Targets)
		print unicode(Targets)
