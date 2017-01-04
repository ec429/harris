#!/usr/bin/python2
"""totalraidsgraph - graph of number of bombers deployed per night

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hsave, hdata, totalraids
import matplotlib.pyplot as plt

if __name__ == '__main__':
	legend = '--nolegend' not in sys.argv
	save = hsave.Save.parse(sys.stdin)
	data = totalraids.extract_totals(save, None, None)
	pff = hdata.Events.find('id', 'PFF')
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	dates = [key.ordinal() for key in sorted(data)]
	tmain = [data[key][0] for key in sorted(data)]
	tpff = [data[key][1] for key in sorted(data)]
	ax.bar(dates, tmain, bottom=tpff, align='center', color='r', label='Main Force', linewidth=0, width=1)
	ax.bar(dates, tpff, align='center', color='b', label='PFF', linewidth=0, width=1)
	ax.xaxis_date()
	if legend: plt.legend(ncol=2, loc=2)
	plt.show()
