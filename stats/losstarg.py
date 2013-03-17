#!/usr/bin/python
"""losstarg - loss rates by target"""

import sys, operator
import hhist, hsave

def extract_losstarg(save, typ=None):
	rcount = [0 for i in xrange(save.ntargets)]
	lcount = list(rcount)
	days = sorted(hhist.group_by_date(save.history))
	for d in days:
		raiding = {}
		for h in d[1]:
			if h['class'] == 'A':
				if h['data']['type']['fb'] == 'B':
					if typ is not None and h['data']['type']['ti'] != typ: continue
					if h['data']['etyp'] == 'RA':
						rcount[h['data']['data']['target']] += 1
						raiding[h['data']['acid']] = h['data']['data']['target']
					elif h['data']['etyp'] == 'CR':
						lcount[raiding[h['data']['acid']]] += 1
	return (sum(lcount)*100/float(sum(rcount)) if sum(rcount) else None, [(l, r, l*100/float(r) if r else None) for l, r in zip(lcount, rcount)])

if __name__ == '__main__':
	save = hsave.Save.parse(sys.stdin)
	losstarg = extract_losstarg(save)
	print losstarg
