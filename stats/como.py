#!/usr/bin/python2
"""como - confid & morale tracking"""

import sys
import os.path
import subprocess
import hhist, hsave

this_script_path = os.path.abspath(os.path.dirname(sys.argv[0]))

class BadTimeSeries(Exception): pass
class UnpairedEntry(BadTimeSeries): pass

def extract_confid(ents):
	return ((e['date'], e['confid']) for e in ents if e['etyp'] == 'CO')

def extract_morale(ents):
	return ((e['date'], e['morale']) for e in ents if e['etyp'] == 'MO')

def extract_como(f):
	records = subprocess.check_output(os.path.join(this_script_path, 'como'), stdin=f)
	entries = []
	for record in records.splitlines():
		date, etyp, value = record.split(" ")
		date = hhist.date.parse(date)
		value = hsave.readfloat(value)
		if etyp == 'CO':
			vname = 'confid'
		elif etyp == 'MO':
			vname = 'morale'
		else:
			raise Exception("Bad etyp", etyp, "in record", record)
		entries.append({'date':date, 'etyp':etyp, vname:value})
	co = extract_confid(entries)
	mo = extract_morale(entries)
	res = []
	last = None
	for c,m in zip(co, mo):
		if c[0]!=m[0]:
			raise UnpairedEntry(c[0], m[0])
		while last and (c[0] != last['date'].next()):
			last['date'] = last['date'].next()
			res.append(dict(last))
		last = {'date':c[0], 'confid':c[1], 'morale':m[1]}
		res.append(dict(last))
	return(res)

if __name__ == '__main__':
	como = extract_como(sys.stdin)
	for cm in como:
		ostr = '%s: confid=%d morale=%d' % (cm['date'], cm['confid'], cm['morale'])
		print(ostr)
