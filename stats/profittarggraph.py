#!/usr/bin/python
"""profittarggraph - graph of effectiveness by target

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hsave, hdata, hhist, profit
from extra_data import Bombers as extra
import matplotlib.pyplot as plt
import optparse

def parse_args(argv):
	x = optparse.OptionParser()
	x.add_option('--legend', action='store_true', default=True)
	x.add_option('--nolegend', dest='legend', action='store_false')
	x.add_option('-a', '--after', type='string')
	x.add_option('-b', '--before', type='string')
	opts, args = x.parse_args()
	return opts, args

if __name__ == '__main__':
	opts, args = parse_args(sys.argv)
	before = hhist.date.parse(opts.before) if opts.before else None
	after = hhist.date.parse(opts.after) if opts.after else None
	save = hsave.Save.parse(sys.stdin)
	prof = profit.extract_targ_profit(save, before, after)
	sort = [prof[i] for i in xrange(len(prof))]
	totals = {k: sum(d[k] for d in sort) for k in ['gain', 'cost']}
	data = [(d['cost'], d['gain'], d['gain'] * 1.0 / d['cost'] if d['cost'] else None) for d in sort]
	mean = totals['gain'] * 1.0 / totals['cost'] if totals['cost'] else None
	bars = reversed(zip(hdata.Targets, data))
	fbars = [bar for bar in bars if bar[1][0] or bar[1][1]]
	mr = float(max([bar[1][0] for bar in fbars]))
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	if mean:
		ax.vlines(mean, 0, len(fbars)+1)
	yl = xrange(1, len(fbars)+1)
	gl = plt.barh(yl, [bar[1][2] or 0.01 for bar in fbars], height=[max(bar[1][0]/mr, 0.1) if bar[1][0] else max(bar[1][1]/mr, 0.1) for bar in fbars], color=[('b' if bar[1][0] else 'g') if bar[1][1] else 'r' for bar in fbars], align='center', linewidth=0)
	ax.vlines(0, 0, len(fbars)+1)
	ax.set_yticks(yl)
	ax.set_yticklabels([bar[0]['name'] for bar in fbars])
	plt.show()
