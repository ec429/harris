#!/usr/bin/python3
"""gprodgraph - graph of German production

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hdata, gprod
from extra_data import Industries as extra
import matplotlib.pyplot as plt

if __name__ == '__main__':
	showtotal = '--nototal' not in sys.argv
	legend = '--nolegend' not in sys.argv
	delta = '--dprod' in sys.argv
	what = 'dprod' if delta else 'gprod'
	data = gprod.extract_gprod(sys.stdin)
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	data = [e for e in data if what in e]
	dates = [datum['date'].ordinal() for datum in data]
	if showtotal: gt = plt.plot_date(dates, [e[what]['total']/1e3 for e in data], fmt='ko-', tz=None, xdate=True, ydate=False, label='total', zorder=-2)
	for i in data[0]['gprod']:
		if i == 'total': continue
		gb = plt.plot_date([datum['date'].ordinal() for datum in data], [e[what][i]/1e3 for e in data], fmt='o-', mew=0, color=extra[i]['colour'], tz=None, xdate=True, ydate=False, label=extra[i]['name'], zorder=0)
	if legend: plt.legend(ncol=2, loc='upper left')
	ax.set_ybound(0)
	plt.show()
