#!/usr/bin/python2
"""valuegraph - graph of total value

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hdata, value
from extra_data import Bombers as extra
import matplotlib.pyplot as plt

if __name__ == '__main__':
	showtotal = '--nototal' not in sys.argv
	legend = '--nolegend' not in sys.argv
	projected = '--project' in sys.argv
	data = value.extract_value(sys.stdin)
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	dates = [datum['date'].ordinal() for datum in data]
	if projected:
		if showtotal: gt = plt.plot_date(dates, [(e['total']+e['cshr']*25)/1e3 for e in data], fmt='ko-', tz=None, xdate=True, ydate=False, label='total', zorder=-2)
		gp = plt.plot_date(dates, [(e['cshr']*25)/1e3 for e in data], fmt='gd-', tz=None, xdate=True, ydate=False, label='projected', zorder=2)
	else:
		if showtotal: gt = plt.plot_date(dates, [e['total']/1e3 for e in data], fmt='ko-', tz=None, xdate=True, ydate=False, label='total', zorder=-2)
	gc = plt.plot_date(dates, [e['cash']/1e3 for e in data], fmt='g+-', tz=None, xdate=True, ydate=False, label='cash', zorder=2)
	for bi,b in enumerate(hdata.Bombers.data):
		bvalue = [datum['bvalues'][bi]/1e3 for datum in data if hdata.inservice(datum['date'], b)]
		if not any(bvalue): continue
		bdate = [datum['date'].ordinal() for datum in data if hdata.inservice(datum['date'], b)]
		gb = plt.plot_date(bdate, bvalue, fmt='o-', mew=0, color=extra[b['name']]['colour'], tz=None, xdate=True, ydate=False, label=b['name'], zorder=0)
	if legend: plt.legend(ncol=2, loc='upper left')
	plt.show()
