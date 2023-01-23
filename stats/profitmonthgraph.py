#!/usr/bin/python3
"""profitmonthgraph - graph of effectiveness per type, by month

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
	x.add_option('--opti', action='store_true')
	x.add_option('--dead', action='store_true')
	x.add_option('--legend', action='store_true', default=True)
	x.add_option('--nolegend', dest='legend', action='store_false')
	opts, args = x.parse_args()
	if opts.opti and opts.dead:
		x.error("Can't have --opti and --dead!")
	return opts, args

if __name__ == '__main__':
	opts, args = parse_args(sys.argv)
	save = hsave.Save.parse(sys.stdin)
	costs = {b['i']:b['cost'] for b in hdata.Bombers}
	data = []
	month = save.history[0]['date']
	bombers = {b['id']:[b['type'], 0, True, True] for b in save.init.bombers}
	targets = [[t['dmg'], 0, dict((i,0) for i in xrange(save.ntypes))] for t in save.init.targets]
	classes = [[profit.tcls,profit.tcls], [profit.icls,profit.icls]]
	history = sorted(hhist.group_by_date(save.history))
	i = 0
	while month <= save.history[-1]['date']:
		next = month.nextmonth()
		d = month.copy()
		while d < next:
			if i >= len(history):
				d = d.next()
				continue
			if history[i][0] > d:
				d = d.next()
				continue
			if history[i][0] < d:
				raise hhist.OutOfOrder(d, history[i][0])
			profit.daily_profit(history[i], bombers, targets, classes, True, False)
			bombers = {i:bombers[i] for i in bombers if bombers[i][3]}
			d = d.next()
			i += 1
		results = {i: {k:v for k,v in bombers.iteritems() if v[0] == i} for i in xrange(save.ntypes)}
		full = {i: (len(results[i]), sum(v[1] for v in results[i].itervalues())) for i in results}
		deadresults = {i: {k:v for k,v in results[i].iteritems() if not v[2]} for i in results}
		dead = {i: (len(deadresults[i]), sum(v[1] for v in deadresults[i].itervalues())) for i in results}
		p = {i: {'full':full[i], 'fullr':full[i][1]/full[i][0] if full[i][0] else 0,
				'dead':dead[i], 'deadr':dead[i][1]/dead[i][0] if dead[i][0] else 0,
				'opti':full[i][1]/dead[i][0] if dead[i][0] else 0}
			for i in results}
		if opts.opti:
			value = {i: (p[i]['opti']/float(costs[i])) if p[i]['dead'][0] else None for i in xrange(save.ntypes)}
		elif opts.dead:
			value = {i: (p[i]['deadr']/float(costs[i])) if p[i]['dead'][0] else None for i in xrange(save.ntypes)}
		else:
			value = {i: (p[i]['fullr']/float(costs[i])) if p[i]['full'][0] else None for i in xrange(save.ntypes)}
		data.append((month, value))
		bombers = {i:[bombers[i][0], 0, True, True] for i in bombers if bombers[i][2]}
		month = next
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	dates = zip(*data)[0]
	values = dict(data)
	for bi,b in enumerate(hdata.Bombers):
		bdate = [d.ordinal() for d in dates if values[d][bi] is not None]
		if not bdate: continue
		gb = plt.plot_date(bdate, [values[d][bi] for d in dates if values[d][bi] is not None], fmt='o-', mew=0, color=extra[b['name']]['colour'], tz=None, xdate=True, ydate=False, label=b['name'], zorder=0)
	ax.grid(b=True, axis='y')
	if opts.legend: plt.legend(ncol=2, loc='upper left')
	plt.show()
