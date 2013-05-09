#!/usr/bin/python
"""fightersgraph - graph of enemy fighter force

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hsave, hdata, fighters
import matplotlib.pyplot as plt
import datetime

def todt(date):
	return datetime.date(date.year, date.month, date.day)

if __name__ == '__main__':
	showtotal = '--nototal' not in sys.argv
	save = hsave.Save.parse(sys.stdin)
	data = fighters.extract_fighters(save)
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	cols = ['0.6','0.4','r','y','m','g','y','b']
	dates = [todt(datum['date']).toordinal() for datum in data]
	if showtotal: gt = plt.plot_date(dates, [e['total'] for e in data], fmt='ko-', tz=None, xdate=True, ydate=False, label='total', zorder=2)
	for fi,f in enumerate(hdata.Fighters.data):
		gf = plt.plot_date([todt(datum['date']).toordinal() for datum in data if hdata.inservice(datum['date'], f)], [e['fighters'][fi] for e in data if hdata.inservice(e['date'], f)], fmt='o-', color=cols[fi], tz=None, xdate=True, ydate=False, label=f['name'], zorder=0)
	plt.legend(ncol=2, loc='upper left')
	plt.show()
