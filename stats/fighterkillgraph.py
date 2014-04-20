#!/usr/bin/python
"""fighterkillgraph - graph of enemy fighter kills & losses

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hsave, hdata, fighterkill
from extra_data import Fighters as extra
import matplotlib.pyplot as plt

if __name__ == '__main__':
	showtotal = '--nototal' not in sys.argv
	legend = '--nolegend' not in sys.argv
	save = hsave.Save.parse(sys.stdin)
	data = fighterkill.extract_kills(save)
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	if showtotal:
		gtk = plt.plot_date([d.ordinal() for d in data.keys()], [e['total']['kills'] for e in data.values()], fmt='k+-', tz=None, xdate=True, ydate=False, label='total', zorder=-2)
		gtl = plt.plot_date([d.ordinal() for d in data.keys()], [-e['total']['losses'] for e in data.values()], fmt='k+-', tz=None, xdate=True, ydate=False, label=None, zorder=-2)
	for fi,f in enumerate(hdata.Fighters.data):
		gfk = plt.plot_date([d.ordinal() for d in data.keys() if hdata.inservice(d, f)], [e['kills'][fi] for d,e in data.items() if hdata.inservice(d, f)], fmt='o-', mew=0, color=extra[f['name']]['colour'], tz=None, xdate=True, ydate=False, label=f['name'], zorder=0)
		gfl = plt.plot_date([d.ordinal() for d in data.keys() if hdata.inservice(d, f)], [-e['losses'][fi] for d,e in data.items() if hdata.inservice(d, f)], fmt='o-', mew=0, color=extra[f['name']]['colour'], tz=None, xdate=True, ydate=False, label=None, zorder=0)
	if legend: plt.legend(ncol=2, loc='upper left')
	plt.show()
