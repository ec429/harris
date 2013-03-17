#!/usr/bin/python
"""losstype - loss rates by type"""

import sys, operator
import hhist, hsave

def extract_losstype(save, targ=None):
	rcount = [0 for i in xrange(save.ntypes)]
	lcount = list(rcount)
	days = sorted(hhist.group_by_date(save.history))
	for d in days:
		raiding = {}
		for h in d[1]:
			if h['class'] == 'A':
				if h['data']['type']['fb'] == 'B':
					if h['data']['etyp'] == 'RA':
						raiding[h['data']['acid']] = h['data']['data']['target']
						if targ is not None and h['data']['data']['target'] != targ: continue
						rcount[h['data']['type']['ti']] += 1
					elif h['data']['etyp'] == 'CR':
						if targ is not None and raiding[h['data']['acid']] != targ: continue
						lcount[h['data']['type']['ti']] += 1
	return (sum(lcount)*100/float(sum(rcount)) if sum(rcount) else None, [(l, r, l*100/float(r) if r else None) for l, r in zip(lcount, rcount)])

if __name__ == '__main__':
	save = hsave.Save.parse(sys.stdin)
	losstype = extract_losstype(save)
	print losstype
