#!/usr/bin/python
"""fighterkillgraph - graph of enemy fighter kills & losses

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hsave, hdata, fighterkill
import matplotlib.pyplot as plt

if __name__ == '__main__':
	showtotal = '--nototal' not in sys.argv
	save = hsave.Save.parse(sys.stdin)
	data = fighterkill.extract_kills(save)
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	cols = ['0.6','0.4','r','y','m','g','y','b']
	dates = [datum['date'].ordinal() for datum in data]
	if showtotal:
		gtk = plt.plot_date(dates, [e['total'][0] for e in data], fmt='ko-', tz=None, xdate=True, ydate=False, label='total', zorder=-2)
		gtl = plt.plot_date(dates, [-e['total'][1] for e in data], fmt='ko-', tz=None, xdate=True, ydate=False, label=None, zorder=-2)
	for fi,f in enumerate(hdata.Fighters.data):
		gfk = plt.plot_date([datum['date'].ordinal() for datum in data if hdata.inservice(datum['date'], f)], [e['kills'][fi] for e in data if hdata.inservice(e['date'], f)], fmt='o-', color=cols[fi], tz=None, xdate=True, ydate=False, label=f['name'], zorder=0)
		gfl = plt.plot_date([datum['date'].ordinal() for datum in data if hdata.inservice(datum['date'], f)], [-e['losses'][fi] for e in data if hdata.inservice(e['date'], f)], fmt='o-', color=cols[fi], tz=None, xdate=True, ydate=False, label=None, zorder=0)
	plt.legend(ncol=2, loc='upper left')
	plt.show()
