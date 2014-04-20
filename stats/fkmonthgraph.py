#!/usr/bin/python
"""fkmonthgraph - graph of enemy fighter kills & losses, by month

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hsave, hdata, fighterkill
from extra_data import Fighters as extra
import matplotlib.pyplot as plt

if __name__ == '__main__':
	showtotal = '--nototal' not in sys.argv
	legend = '--nolegend' not in sys.argv
	save = hsave.Save.parse(sys.stdin)
	data = fighterkill.extract_kills(save)
	monthly = {}
	month = min(data.keys())
	last = max(data.keys())
	while month <= last:
		next = month.nextmonth()
		monthly[month] = {'total':{'kills':0, 'losses':0}, 'kills':[0 for i,f in enumerate(hdata.Fighters)], 'losses':[0 for i,f in enumerate(hdata.Fighters)]}
		d = month.copy()
		while d < next:
			if d in data:
				monthly[month]['total']['kills'] += data[d]['total']['kills']
				monthly[month]['total']['losses'] += data[d]['total']['losses']
				for i,f in enumerate(hdata.Fighters):
					monthly[month]['kills'][i] += data[d]['kills'][i]
					monthly[month]['losses'][i] += data[d]['losses'][i]
			d = d.next()
		month = next
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	dates = sorted(monthly.keys())
	for fi,f in enumerate(hdata.Fighters):
		def ins(m):
			return hdata.inservice(m, f) or hdata.inservice(m.nextmonth(), f)
		gp = plt.plot_date([d.ordinal() for d in dates if ins(d)], [monthly[d]['kills'][fi] for d in dates if ins(d)], fmt='o-', mew=0, color=extra[f['name']]['colour'], tz=None, xdate=True, ydate=False, label=f['name'], zorder=0)
		gl = plt.plot_date([d.ordinal() for d in dates if ins(d)], [-monthly[d]['losses'][fi] for d in dates if ins(d)], fmt='o-', mew=0, color=extra[f['name']]['colour'], tz=None, xdate=True, ydate=False, label=None, zorder=0)
	gt = plt.plot_date([d.ordinal() for d in dates], [monthly[d]['total']['kills'] for d in dates], fmt='k+-', tz=None, xdate=True, ydate=False, label='total', zorder=-2)
	gb = plt.plot_date([d.ordinal() for d in dates], [-monthly[d]['total']['losses'] for d in dates], fmt='k+-', tz=None, xdate=True, ydate=False, label=None, zorder=-2)
	ax.grid(b=True, axis='y')
	plt.axhline(y=0, xmin=0, xmax=1, c='k', zorder=-1)
	if legend: plt.legend(ncol=2, loc='upper left')
	plt.show()
