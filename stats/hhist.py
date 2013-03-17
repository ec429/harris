#!/usr/bin/python
"""hhist - Harris history parsing

The hhist module provides support for parsing Harris history events and
lists thereof.
"""

import sys

class BadHistLine(Exception): pass
class NoSuchEvent(BadHistLine): pass
class NoSuchAcType(BadHistLine): pass
class NoSuchDmgType(BadHistLine): pass
class ExcessData(BadHistLine): pass
class OutOfOrder(Exception): pass

class date(object):
	def __init__(self, day, month, year):
		self.day = day
		self.month = month
		self.year = year
	@classmethod
	def parse(cls, text):
		day, month, year = map(int, text.split('-', 2))
		return cls(day, month, year)
	def __str__(self):
		return '%02d-%02d-%04d' % (self.day, self.month, self.year)
	__repr__ = __str__
	def __cmp__(self, other):
		if self.year != other.year: return self.year - other.year
		if self.month != other.month: return self.month - other.month
		return self.day - other.day
	def copy(self):
		return date(self.day, self.month, self.year)
	def next(self):
		monthdays=[31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]
		next = self.copy()
		next.day += 1
		assert next.year % 100 # if we encounter century years, we have bigger problems than just leap years
		ly = 1 if next.month == 2 and not next.year % 4 else 0
		if next.day > monthdays[next.month-1]+ly:
			next.day = 1
			next.month += 1
			if next.month > 12:
				next.month = 1
				next.year += 1
		return next

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
	def obs_parse(text):
		if text: raise ExcessData('A', 'CR', text)
		return {}
	parsers = {'CT':ct_parse, 'NA':nav_parse, 'PF':pff_parse, 'RA':raid_parse, 'HI':hit_parse, 'DM':dmg_parse, 'FA':fail_parse, 'CR':crash_parse, 'OB':obs_parse}
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
	sdate, time, ec, data = line.split(' ', 3)
	odate = date.parse(sdate)
	hour, minute = time.split(':', 1)
	rest = parsers.get(ec, lambda x: {})(data)
	return {'date':odate,
			'time':{'hour':hour, 'minute':minute},
			'class':ec,
			'text':data,
			'data':rest}

def group_by_date(entries):
	date = entries[0]['date']
	res = [(date, [])]
	for ent in entries:
		while date < ent['date']:
			date = date.next()
			res.append((date, []))
		if ent['date'] != date:
			raise OutOfOrder(date, ent)
		res[-1][1].append(ent)
	return(res)

def import_from_save(f):
	start = False
	entries = []
	for line in f:
		if line[-1:] == '\n':
			line = line[:-1]
		if start:
			entries.append(raw_parse(line))
		elif line.startswith('History:'):
			start = True
	return entries

if __name__ == '__main__':
	entries = import_from_save(sys.stdin)
	print 'Imported %d entries' % len(entries)
