#!/usr/bin/python
"""hhist - Harris history parsing

The hhist module provides support for parsing Harris history events and
lists thereof.
"""

import sys, pprint

class BadHistLine(Exception): pass
class NoSuchEvent(BadHistLine): pass
class NoSuchAcType(BadHistLine): pass
class NoSuchDmgType(BadHistLine): pass
class ExcessData(BadHistLine): pass

def ac_parse(text):
	def ct_parse(text):
		if text: raise ExcessData('A', 'CT', text)
		return {}
	def nav_parse(text):
		if ' ' in text: raise ExcessData('A', 'NA', text.split(' ', 1)[1])
		return {'navaid':int(text)}
	def pff_parse(text):
		if text: raise ExcessData('A', 'PF', text)
		return {}
	def raid_parse(text):
		if ' ' in text: raise ExcessData('A', 'RA', text.split(' ', 1)[1])
		return {'target':int(text)}
	def hit_parse(text):
		target, bombs = text.split(' ', 1)
		if ' ' in bombs: raise ExcessData('A', 'HI', bombs.split(' ', 1)[1])
		return {'target':int(target), 'bombs':int(bombs)}
	def dmg_parse(text):
		delta, current, styp, src = text.split(' ', 3)
		if ' ' in src: raise ExcessData('A', 'DM', src.split(' ', 1)[1])
		ddmg = float.fromhex(delta)
		cdmg = float.fromhex(current)
		if styp == 'AC':
			return {'ddmg':ddmg, 'cdmg':cdmg, 'styp':styp, 'ac':int(src, 16)}
		elif styp == 'FK':
			return {'ddmg':ddmg, 'cdmg':cdmg, 'styp':styp, 'flak':int(src)}
		elif styp == 'TF':
			return {'ddmg':ddmg, 'cdmg':cdmg, 'styp':styp, 'targ':int(src)}
		else:
			raise NoSuchDmgType('A', 'DM', styp)
	def fail_parse(text):
		if ' ' in text: raise ExcessData('A', 'FA', text.split(' ', 1)[1])
		return {'failed':int(text)}
	def crash_parse(text):
		if text: raise ExcessData('A', 'CR', text)
		return {}
	parsers = {'CT':ct_parse, 'NA':nav_parse, 'PF':pff_parse, 'RA':raid_parse, 'HI':hit_parse, 'DM':dmg_parse, 'FA':fail_parse, 'CR':crash_parse}
	parts = text.split(' ', 3)
	acid, atyp, etyp = parts[0:3]
	if len(parts) > 3:
		rest = parts[3]
	else:
		rest = ''
	if atyp[0] not in 'BF':
		raise NoSuchAcType('A', atyp)
	fb = atyp[0]
	ti = int(atyp[1:])
	return {'acid':int(acid, 16),
			'type':{'fb':fb, 'ti':ti},
			'etyp':etyp,
			'text':rest,
			'data':parsers.get(etyp, lambda x: {})(rest)}

def init_parse(text):
	event, data = text.split(' ', 1)
	if event != 'INIT':
		raise NoSuchEvent('I', event)
	return {'event':event, 'filename':data}

def misc_parse(text):
	def cash_parse(text):
		delta, current = text.split(' ', 1)
		if ' ' in current: raise ExcessData('M', 'CA', current.split(' ', 1)[1])
		return {'cshr':int(delta), 'cash':int(current)}
	def confid_parse(text):
		if ' ' in text: raise ExcessData('M', 'CO', text.split(' ', 1)[1])
		return {'confid':float.fromhex(text)}
	def morale_parse(text):
		if ' ' in text: raise ExcessData('M', 'MO', text.split(' ', 1)[1])
		return {'morale':float.fromhex(text)}
	parsers = {'CA':cash_parse, 'CO':confid_parse, 'MO':morale_parse}
	etyp, rest = text.split(' ', 1)
	return {'etyp':etyp,
			'text':rest,
			'data':parsers.get(etyp, lambda x: {})(rest)}

def targ_parse(text):
	def dmg_parse(text):
		delta, current = text.split(' ', 1)
		if ' ' in current: raise ExcessData('T', 'DM', current.split(' ', 1)[1])
		return {'ddmg':float.fromhex(delta), 'cdmg':float.fromhex(current)}
	def flak_parse(text):
		delta, current = text.split(' ', 1)
		if ' ' in current: raise ExcessData('T', 'FK', current.split(' ', 1)[1])
		return {'dflk':float.fromhex(delta), 'cflk':float.fromhex(current)}
	def ship_parse(text):
		if text: raise ExcessData('T', 'SH', text)
		return {}
	parsers = {'DM':dmg_parse, 'FK':flak_parse, 'SH':ship_parse}
	parts = text.split(' ', 2)
	target, etyp = parts[0:2]
	if len(parts) > 2:
		rest = parts[2]
	else:
		rest = ''
	return {'target':target,
			'etyp':etyp,
			'text':rest,
			'data':parsers.get(etyp, lambda x: {})(rest)}

parsers = {'A':ac_parse, 'I':init_parse, 'M':misc_parse, 'T':targ_parse};

def raw_parse(line):
	date, time, ec, data = line.split(' ', 3)
	day, month, year = date.split('-', 2)
	hour, minute = time.split(':', 1)
	rest = parsers.get(ec, lambda x: {})(data)
	return {'date':{'day':day, 'month':month, 'year':year},
			'time':{'hour':hour, 'minute':minute},
			'class':ec,
			'text':data,
			'data':rest}

if __name__ == '__main__':
	start = False
	entries = []
	for line in sys.stdin:
		if line[-1:] == '\n':
			line = line[:-1]
		if start:
			entries.append(raw_parse(line))
		elif line.startswith('History:'):
			start = True
	print 'Imported %d entries' % len(entries)
