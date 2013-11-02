#!/usr/bin/python
"""losstarg - loss rates by target"""

import sys, operator
import hhist, hsave
import optparse

def parse_args(argv):
	x = optparse.OptionParser()
	x.add_option('-a', '--after', type='string')
	x.add_option('-b', '--before', type='string')
	x.add_option('--type', type='int')
	opts, args = x.parse_args()
	opts.after = hhist.date.parse(opts.after) if opts.after else None
	opts.before = hhist.date.parse(opts.before) if opts.before else None
	return opts, args

def extract_losstarg(save, opts):
	rcount = [0 for i in xrange(save.ntargets)]
	lcount = list(rcount)
	days = sorted(hhist.group_by_date(save.history))
	for d in days:
		if opts.after and d[0]<opts.after: continue
		if opts.before and d[0]>=opts.before: continue
		raiding = {}
		for h in d[1]:
			if h['class'] == 'A':
				if h['data']['type']['fb'] == 'B':
					if opts.type is not None and h['data']['type']['ti'] != opts.type: continue
					if h['data']['etyp'] == 'RA':
						rcount[h['data']['data']['target']] += 1
						raiding[h['data']['acid']] = h['data']['data']['target']
					elif h['data']['etyp'] == 'CR':
						lcount[raiding[h['data']['acid']]] += 1
	return (sum(lcount)*100/float(sum(rcount)) if sum(rcount) else None, [(l, r, l*100/float(r) if r else None) for l, r in zip(lcount, rcount)])

if __name__ == '__main__':
	opts, args = losstarg.parse_args(sys.argv)
	save = hsave.Save.parse(sys.stdin)
	losstarg = extract_losstarg(save, opts)
	print losstarg
