#!/usr/bin/python
"""cshrgraph - graph of cash rate (budget)

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hhist, cshr
import matplotlib.pyplot as plt
import datetime

def todt(date):
	return datetime.date(date.year, date.month, date.day)

if __name__ == '__main__':
	entries = hhist.import_from_save(sys.stdin)
	data = cshr.extract_cshr(entries)
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	dates = [todt(key).toordinal() for key in sorted(data)]
	cshr = [data[key] for key in sorted(data)]
	plt.axis(ymin=0, ymax=max(cshr))
	gc = plt.plot_date(dates, cshr, fmt='g+-', tz=None, xdate=True, ydate=False, label='Budget', zorder=0)
	plt.legend()
	plt.show()
