#!/usr/bin/python
"""profittimegraph - graph of effectiveness per type, versus time

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hsave, hdata, hhist, profit
import matplotlib.pyplot as plt

if __name__ == '__main__':
	save = hsave.Save.parse(sys.stdin)
	bombers = {b['id']:[b['type'], 0, True] for b in save.init.bombers}
	targets = [t['dmg'] for t in save.init.targets]
	costs = {b['i']:b['cost'] for b in hdata.Bombers}
	days = sorted(hhist.group_by_date(save.history))
	data = []
	for d in days:
		profit.daily_profit(d, bombers, targets)
		results = {i: {k:v for k,v in bombers.iteritems() if v[0] == i} for i in xrange(save.ntypes)}
		results[None] = {k:v for k,v in bombers.iteritems()}
		full = {i: (len(results[i]), sum(v[1] for v in results[i].itervalues())) for i in results}
		deadresults = {i: {k:v for k,v in results[i].iteritems() if not v[2]} for i in results}
		dead = {i: (len(deadresults[i]), sum(costs[d[0]] for d in deadresults[i].itervalues()), sum(v[1] for v in deadresults[i].itervalues())) for i in results}
		opti = {i: full[i][1]/float(dead[i][1]) if dead[i][0] > 10 else None for i in results}
		data.append((d[0], opti))
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	cols = ['0.5','y','r','c','m','0.5','y','b','r','0.5','c']
	dates = zip(*data)[0]
	optis = dict(data)
	gt = plt.plot_date([d.ordinal() for d in dates], [optis[d][None] for d in dates], fmt='.-', color='g', tz=None, xdate=True, ydate=False, label='Overall', zorder=1)
	for bi,b in enumerate(hdata.Bombers):
		gb = plt.plot_date([d.ordinal() for d in dates if hdata.inservice(d, b)], [optis[d][bi] for d in dates if hdata.inservice(d, b)], fmt='o-', color=cols[bi], tz=None, xdate=True, ydate=False, label=b['name'], zorder=0)
	ax.grid(b=True, axis='y')
	plt.legend(ncol=2, loc='upper left')
	plt.show()
