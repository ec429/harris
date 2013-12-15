#!/usr/bin/python
"""gprodgraph - graph of German production

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hsave, hdata, gprod
from extra_data import Industries as extra
import matplotlib.pyplot as plt

if __name__ == '__main__':
	showtotal = '--nototal' not in sys.argv
	legend = '--nolegend' not in sys.argv
	save = hsave.Save.parse(sys.stdin)
	data = gprod.extract_gprod(save)
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	dates = [datum['date'].ordinal() for datum in data]
	if showtotal: gt = plt.plot_date(dates, [e['total']/1e3 for e in data], fmt='ko-', tz=None, xdate=True, ydate=False, label='total', zorder=-2)
	for i in xrange(save.iclasses):
		gb = plt.plot_date([datum['date'].ordinal() for datum in data], [e[i]/1e3 for e in data], fmt='o-', mew=0, color=extra[i]['colour'], tz=None, xdate=True, ydate=False, label=extra[i]['name'], zorder=0)
	if legend: plt.legend(ncol=2, loc='upper left')
	plt.show()
