#!/usr/bin/python
"""comograph - graph of confid & morale

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hhist, como
import matplotlib.pyplot as plt
import datetime

def todt(date):
	return datetime.date(date.year, date.month, date.day)

if __name__ == '__main__':
	entries = hhist.import_from_save(sys.stdin)
	data = como.extract_como(entries)
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	plt.axis(ymin=0, ymax=100)
	dates = [todt(datum['date']).toordinal() for datum in data]
	confid = [datum['confid'] for datum in data]
	morale = [datum['morale'] for datum in data]
	gc = plt.plot_date(dates, confid, fmt='bo-', tz=None, xdate=True, ydate=False, label='Confid')
	gm = plt.plot_date(dates, morale, fmt='r+-', tz=None, xdate=True, ydate=False, label='Morale')
	for y in xrange(10, 91, 10):
		plt.axhline(y=y, xmin=0, xmax=1, c='k', zorder=-1)
	plt.legend()
	plt.show()
