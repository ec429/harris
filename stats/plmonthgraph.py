#!/usr/bin/python3
"""plmonthgraph - graph of production & losses, by month

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hhist, hsave, hdata, prodloss
from extra_data import Bombers as extra
import matplotlib.pyplot as plt

if __name__ == '__main__':
	legend = '--nolegend' not in sys.argv
	save = hsave.Save.parse(sys.stdin)
	entries = save.history
	data = prodloss.extract_prodloss(entries) # gives {date: [[prod, loss] for type]}
	data = {k:v for k,v in data.items() if k >= hhist.date(3, 9, 1939)}
	monthly = {}
	month = min(data.keys())
	last = max(data.keys())
	coff = len(hdata.Bombers.data)
	while month <= last:
		next = month.nextmonth()
		monthly[month] = [[0, 0] for i in range(coff+32)]
		d = month.copy()
		while d < next:
			if d in data:
				for i in range(coff+32):
					monthly[month][i][0] += data[d][i][0]
					monthly[month][i][1] += data[d][i][1]
			d = d.next()
		month = next
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	dates = sorted(monthly.keys())
	total = {m: [sum(monthly[m][i][0] for i in range(coff+32)), sum(monthly[m][i][1] for i in range(coff+32))] for m in monthly}
	top = max(v[0] for v in total.values())
	bottom = min(v[1] for v in total.values())
	plt.axis(ymax=max(top, -bottom), ymin=min(-top, bottom))
	for bi,b in enumerate(hdata.Bombers):
		def ins(m):
			return hdata.inservice(m, b) or hdata.inservice(m.nextmonth(), b)
		bprod = [monthly[d][bi][0] for d in dates if ins(d)]
		bloss = [monthly[d][bi][1] for d in dates if ins(d)]
		if not any(bprod+bloss): continue
		bdate = [d.ordinal() for d in dates if ins(d)]
		gp = plt.plot_date(bdate, bprod, fmt='o-', mew=0, color=extra[b['name']]['colour'], tz=None, xdate=True, ydate=False, label=b['name'], zorder=0)
		gl = plt.plot_date(bdate, bloss, fmt='o-', mew=0, color=extra[b['name']]['colour'], tz=None, xdate=True, ydate=False, label=None, zorder=0)
	for bi in range(32):
		name = 'Type %d'%(bi+coff)
		if bi+coff in save.designs:
			name = save.designs[bi+coff]['name']
		bprod = [monthly[d][bi+coff][0] for d in dates]
		bloss = [monthly[d][bi+coff][1] for d in dates]
		if not any(bprod+bloss): continue
		bdate = [d.ordinal() for d in dates]
		gp = plt.plot_date(bdate, bprod, fmt='o-', mew=0, color='C%d'%bi, tz=None, xdate=True, ydate=False, label=name, zorder=0)
		gl = plt.plot_date(bdate, bloss, fmt='o-', mew=0, color='C%d'%bi, tz=None, xdate=True, ydate=False, label=None, zorder=0)
	gt = plt.plot_date([d.ordinal() for d in dates], [total[d][0] for d in dates], fmt='k+-', tz=None, xdate=True, ydate=False, label='total', zorder=-2)
	gb = plt.plot_date([d.ordinal() for d in dates], [total[d][1] for d in dates], fmt='k+-', tz=None, xdate=True, ydate=False, label=None, zorder=-2)
	ax.grid(visible=True, axis='y')
	plt.axhline(y=0, xmin=0, xmax=1, c='k', zorder=-1)
	if legend: plt.legend(ncol=2, loc='upper left')
	plt.show()
