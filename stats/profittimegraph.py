#!/usr/bin/python3
"""profittimegraph - graph of effectiveness per type, versus time

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
	x.add_option('-a', '--after', type='string')
	x.add_option('--opti', action='store_true')
	x.add_option('--dead', action='store_true')
	x.add_option('--legend', action='store_true', default=True)
	x.add_option('--nolegend', dest='legend', action='store_false')
	x.add_option('--min', type='int', default=10)
	opts, args = x.parse_args()
	if opts.opti and opts.dead:
		x.error("Can't have --opti and --dead!")
	return opts, args

if __name__ == '__main__':
	opts, args = parse_args(sys.argv)
	after = hhist.date.parse(opts.after) if opts.after else None
	save = hsave.Save.parse(sys.stdin)
	bombers = {b['id']:[b['type'], 0, True, True] for b in save.init.bombers}
	targets = [[t['dmg'], 0, dict((i,0) for i in xrange(save.ntypes))] for t in save.init.targets]
	classes = [[profit.tcls,profit.tcls], [profit.icls,profit.icls]]
	costs = {b['i']:b['cost'] for b in hdata.Bombers}
	days = sorted(hhist.group_by_date(save.history))
	data = []
	for d in days:
		profit.daily_profit(d, bombers, targets, classes, d[0]>=after if after else True, False)
		bombers = {i:bombers[i] for i in bombers if bombers[i][3]}
		results = {i: {k:v for k,v in bombers.iteritems() if v[0] == i} for i in xrange(save.ntypes)}
		results[None] = {k:v for k,v in bombers.iteritems()}
		full = {i: (len(results[i]), sum(costs[d[0]] for d in results[i].itervalues()), sum(v[1] for v in results[i].itervalues())) for i in results}
		deadresults = {i: {k:v for k,v in results[i].iteritems() if not v[2]} for i in results}
		dead = {i: (len(deadresults[i]), sum(costs[d[0]] for d in deadresults[i].itervalues()), sum(v[1] for v in deadresults[i].itervalues())) for i in results}
		if opts.opti:
			value = {i: full[i][2]/float(dead[i][1]) if dead[i][0] >= opts.min else None for i in results}
		elif opts.dead:
			value = {i: dead[i][2]/float(dead[i][1]) if dead[i][0] else None for i in results}
		else:
			value = {i: full[i][2]/float(full[i][1]) if full[i][0] else None for i in results}
		if after is None or d[0]>=after:
			data.append((d[0], value))
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	dates = zip(*data)[0]
	values = dict(data)
	gt = plt.plot_date([d.ordinal() for d in dates], [values[d][None] for d in dates], fmt='.-', color='g', tz=None, xdate=True, ydate=False, label='Overall', zorder=1)
	for bi,b in enumerate(hdata.Bombers):
		if not full[bi][0]: continue
		gb = plt.plot_date([d.ordinal() for d in dates if hdata.inservice(d, b)], [values[d][bi] for d in dates if hdata.inservice(d, b)], fmt='o-', mew=0, color=extra[b['name']]['colour'], tz=None, xdate=True, ydate=False, label=b['name'], zorder=0)
	ax.grid(b=True, axis='y')
	if opts.legend: plt.legend(ncol=2, loc='upper left')
	plt.show()
