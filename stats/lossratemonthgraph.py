#!/usr/bin/python3
"""lossratemonthgraph - graph of lossrate per type, by month

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hsave, hdata, hhist
from extra_data import Bombers as extra
import matplotlib.pyplot as plt
import optparse

def parse_args(argv):
	x = optparse.OptionParser()
	x.add_option('--legend', action='store_true', default=True)
	x.add_option('--nolegend', dest='legend', action='store_false')
	opts, args = x.parse_args()
	return opts, args

if __name__ == '__main__':
	opts, args = parse_args(sys.argv)
	save = hsave.Save.parse(sys.stdin)
	data = []
	month = save.history[0]['date']
	history = sorted(hhist.group_by_date(save.history))
	i = 0
	while month <= save.history[-1]['date']:
		next = month.nextmonth()
		d = month.copy()
		types = {i:[0,0] for i in xrange(save.ntypes)}
		while d < next:
			if i >= len(history):
				d = d.next()
				continue
			if history[i][0] > d:
				d = d.next()
				continue
			if history[i][0] < d:
				raise hhist.OutOfOrder(d, history[i][0])
			raiding = {}
			for h in history[i][1]:
				if h['class'] == 'A':
					if h['data']['type']['fb'] == 'B':
						if h['data']['etyp'] == 'RA':
							types[h['data']['type']['ti']][0] += 1
						elif h['data']['etyp'] == 'CR':
							types[h['data']['type']['ti']][1] += 1
			d = d.next()
			i += 1
		types[None] = [sum(t[0] for t in types.values()), sum(t[1] for t in types.values())]
		value = {i: (types[i][1]*100.0/float(types[i][0])) if types[i][0] else None for i in types}
		data.append((month, value))
		month = next
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	dates = zip(*data)[0]
	values = dict(data)
	for bi,b in enumerate(hdata.Bombers):
		bdate = [d.ordinal() for d in dates if values[d][bi] is not None]
		if not bdate: continue
		gb = plt.plot_date(bdate, [values[d][bi] for d in dates if values[d][bi] is not None], fmt='o-', mew=0, color=extra[b['name']]['colour'], tz=None, xdate=True, ydate=False, label=b['name'], zorder=0)
	tdate = [d.ordinal() for d in dates if values[d][None] is not None]
	if tdate:
		gt = plt.plot_date(tdate, [values[d][None] for d in dates if values[d][None] is not None], fmt='.-', color='g', tz=None, xdate=True, ydate=False, label='Overall', zorder=1)
	ax.grid(b=True, axis='y')
	if opts.legend: plt.legend(ncol=2, loc='upper left')
	plt.show()
