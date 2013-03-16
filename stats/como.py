#!/usr/bin/python
"""como - confid & morale tracking"""

import sys
import hhist

class BadTimeSeries(Exception): pass
class UnpairedEntry(BadTimeSeries): pass

def extract_confid(ents):
	return ((e['date'], e['data']['data']['confid']) for e in ents if e['class'] == 'M' and e['data']['etyp'] == 'CO')

def extract_morale(ents):
	return ((e['date'], e['data']['data']['morale']) for e in ents if e['class'] == 'M' and e['data']['etyp'] == 'MO')

def extract_como(ents):
	co = extract_confid(ents)
	mo = extract_morale(ents)
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
	entries = hhist.import_from_save(sys.stdin)
	como = extract_como(entries)
	for cm in como:
		print '%s: confid=%d morale=%d' % (cm['date'], cm['confid'], cm['morale'])
