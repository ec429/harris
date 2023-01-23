#!/usr/bin/python3
"""losstarg - loss rates by target"""

import sys, operator
import hhist, hdata, losstype
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

def extract_losstarg(f, opts):
	records = losstype.extract_raids(f)
	rcount = [0 for i in hdata.Targets]
	lcount = list(rcount)
	for d in records:
		if opts.after and d<opts.after: continue
		if opts.before and d>=opts.before: continue
		for key in records[d]:
			typ, targ = key
			r, l = records[d][key]
			if opts.type is not None and typ != opts.type: continue
			rcount[targ] += r
			lcount[targ] += l
	return (sum(lcount)*100/float(sum(rcount)) if sum(rcount) else None, [(l, r, l*100/float(r) if r else None) for l, r in zip(lcount, rcount)])

if __name__ == '__main__':
	opts, args = parse_args(sys.argv)
	losstarg = extract_losstarg(sys.stdin, opts)
	print losstarg
