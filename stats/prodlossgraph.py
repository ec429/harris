#!/usr/bin/python2
"""prodlossgraph - graph of production & losses

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hhist, hdata, prodloss
from extra_data import Bombers as extra
import matplotlib.pyplot as plt

if __name__ == '__main__':
	legend = '--nolegend' not in sys.argv
	entries = hhist.import_from_save(sys.stdin)
	data = prodloss.extract_prodloss(entries)
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	dates = [key.ordinal() for key in sorted(data)]
	total = [[sum(d[0] for d in data[key]), sum(d[1] for d in data[key])] for key in sorted(data)]
	top = max(zip(*total)[0])
	bottom = min(zip(*total)[1])
	plt.axis(ymax=max(top, -bottom), ymin=min(-top, bottom))
	gt = plt.plot_date(dates, zip(*total)[0], fmt='k+-', tz=None, xdate=True, ydate=False, label='total', zorder=-2)
	gb = plt.plot_date(dates, zip(*total)[1], fmt='k+-', tz=None, xdate=True, ydate=False, label=None, zorder=-2)
	for bi,b in enumerate(hdata.Bombers.data):
		bprod = [data[key][bi][0] for key in sorted(data) if hdata.inservice(key, b)]
		bloss = [data[key][bi][1] for key in sorted(data) if hdata.inservice(key, b)]
		if not any(bprod+bloss): continue
		bdate = [key.ordinal() for key in sorted(data) if hdata.inservice(key, b)]
		gp = plt.plot_date(bdate, bprod, fmt='o-', mew=0, color=extra[b['name']]['colour'], tz=None, xdate=True, ydate=False, label=b['name'], zorder=0)
		gl = plt.plot_date(bdate, bloss, fmt='o-', mew=0, color=extra[b['name']]['colour'], tz=None, xdate=True, ydate=False, label=None, zorder=0)
	plt.axhline(y=0, xmin=0, xmax=1, c='k', zorder=-1)
	if legend: plt.legend(ncol=2)
	plt.show()
