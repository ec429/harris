#!/usr/bin/python2
"""cshrgraph - graph of cash rate (budget)

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import cshr
import matplotlib.pyplot as plt

if __name__ == '__main__':
	legend = '--nolegend' not in sys.argv
	data = cshr.extract_cshr(sys.stdin)
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	dates = [key.ordinal() for key in sorted(data)]
	cshr = [data[key] for key in sorted(data)]
	plt.axis(ymin=0, ymax=max(cshr))
	gc = plt.plot_date(dates, cshr, fmt='g+-', tz=None, xdate=True, ydate=False, label='Budget', zorder=0)
	if legend: plt.legend()
	plt.show()
