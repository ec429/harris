#!/usr/bin/python
"""profitmonthgraph - graph of effectiveness per type, by month

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hsave, hdata, hhist, profit
import matplotlib.pyplot as plt
import optparse

def parse_args(argv):
	x = optparse.OptionParser()
	x.add_option('--opti', action='store_true')
	x.add_option('--dead', action='store_true')
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
	while month <= save.history[-1]['date']:
		next = month.nextmonth()
		p = profit.extract_profit(save, next, month)
		if opts.opti:
			value = {i: (p[i]['opti']/float(costs[i])) if p[i]['dead'][0] else None for i in xrange(save.ntypes)}
		elif opts.dead:
			value = {i: (p[i]['deadr']/float(costs[i])) if p[i]['dead'][0] else None for i in xrange(save.ntypes)}
		else:
			value = {i: (p[i]['fullr']/float(costs[i])) if p[i]['dead'][0] else None for i in xrange(save.ntypes)}
		data.append((month, value))
		month = next
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	cols = ['0.5','y','r','c','m','0.5','y','b','r','0.5','c']
	dates = zip(*data)[0]
	values = dict(data)
	for bi,b in enumerate(hdata.Bombers):
		gb = plt.plot_date([d.ordinal() for d in dates if hdata.inservice(d, b) and values[d][bi] is not None], [values[d][bi] for d in dates if hdata.inservice(d, b) and values[d][bi] is not None], fmt='o-', color=cols[bi], tz=None, xdate=True, ydate=False, label=b['name'], zorder=0)
	ax.grid(b=True, axis='y')
	plt.legend(ncol=2, loc='upper left')
	plt.show()
