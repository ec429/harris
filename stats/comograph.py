#!/usr/bin/python2
"""comograph - graph of confid & morale

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import como
import matplotlib.pyplot as plt

if __name__ == '__main__':
	legend = '--nolegend' not in sys.argv
	data = como.extract_como(sys.stdin)
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	plt.axis(ymin=0, ymax=100)
	dates = [datum['date'].ordinal() for datum in data]
	confid = [datum['confid'] for datum in data]
	morale = [datum['morale'] for datum in data]
	gc = plt.plot_date(dates, confid, fmt='bo-', tz=None, xdate=True, ydate=False, label='Confid', zorder=0)
	gm = plt.plot_date(dates, morale, fmt='r+-', tz=None, xdate=True, ydate=False, label='Morale', zorder=0)
	for y in xrange(10, 91, 10):
		plt.axhline(y=y, xmin=0, xmax=1, c='k', zorder=-1)
	if legend: plt.legend()
	plt.show()
