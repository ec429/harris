#!/usr/bin/python
"""cshr - cash rate (budget) tracking"""

import sys
import hhist

class BadTimeSeries(Exception): pass
class UnpairedEntry(BadTimeSeries): pass

def extract_cshr(ents):
	return {e['date']: e['data']['data']['cshr'] for e in ents if e['class'] == 'M' and e['data']['etyp'] == 'CA'}

if __name__ == '__main__':
	entries = hhist.import_from_save(sys.stdin)
	cshr = extract_cshr(entries)
	for c in sorted(cshr):
		print '%s: cshr=%d' % (c, cshr[c])
