#!/usr/bin/python
"""fightersgraph - graph of enemy fighter force

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hsave, hdata, fighters
from extra_data import Fighters as extra
import matplotlib.pyplot as plt

if __name__ == '__main__':
	showtotal = '--nototal' not in sys.argv
	legend = '--nolegend' not in sys.argv
	save = hsave.Save.parse(sys.stdin)
	data = fighters.extract_fighters(save)
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	cols = ['0.6','0.4','r','y','m','g','y','b']
	dates = [datum['date'].ordinal() for datum in data]
	lbc = hdata.Events.find('id', 'L_BC')
	if showtotal: gt = plt.plot_date(dates, [e['total'] for e in data], fmt='ko-', tz=None, xdate=True, ydate=False, label='total', zorder=-2)
	for fi,f in enumerate(hdata.Fighters.data):
		gf = plt.plot_date([datum['date'].ordinal() for datum in data if hdata.inservice(datum['date'], f)], [e['fighters'][fi] for e in data if hdata.inservice(e['date'], f)], fmt='o-', color=extra[f['name']]['colour'], tz=None, xdate=True, ydate=False, label=f['name'], zorder=0)
		if f['radpri']:
			def i(d):
				return hdata.inservice(d, f) and d >= lbc['date']
			gr = plt.plot_date([datum['date'].ordinal() for datum in data if i(datum['date'])], [e['radar'][fi] for e in data if i(e['date'])], fmt='o-', color=extra[f['name']]['2colour'], tz=None, xdate=True, ydate=False, label=None, zorder=0)
	if legend: plt.legend(ncol=2, loc='upper left')
	plt.show()
