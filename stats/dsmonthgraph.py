#!/usr/bin/python3
"""dsmonthgraph - graph of bomber damage by source, by month

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hhist
import matplotlib.pyplot as plt

if __name__ == '__main__':
	showtotal = '--nototal' not in sys.argv
	legend = '--nolegend' not in sys.argv
	monthly = {}
	entries = hhist.import_from_save(sys.stdin)
	for ent in entries:
		if ent['class'] == 'A' and ent['data']['etyp'] == 'DM':
			d = ent['date']
			month = hhist.date(1, d.month, d.year)
			monthly.setdefault(month, []).append(ent['data']['data'])
	ftr = {}
	flk = {}
	tf = {}
	for month,rows in monthly.items():
		ftr[month] = 0.0
		flk[month] = 0.0
		tf[month] = 0.0
		for ent in rows:
			if ent['styp'] == 'AC':
				ftr[month] += ent['ddmg']
			elif ent['styp'] == 'FK':
				flk[month] += ent['ddmg']
			elif ent['styp'] == 'TF':
				tf[month] += ent['ddmg']
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	dates = sorted(monthly.keys())
	plt.plot_date([d.ordinal() for d in dates], [ftr[d] for d in dates], fmt='o-', tz=None, xdate=True, ydate=False, label='Fighters', zorder=0)
	plt.plot_date([d.ordinal() for d in dates], [flk[d] for d in dates], fmt='o-', tz=None, xdate=True, ydate=False, label='Sited Flak', zorder=0)
	plt.plot_date([d.ordinal() for d in dates], [tf[d] for d in dates], fmt='o-', tz=None, xdate=True, ydate=False, label='Target Flak', zorder=0)
	if showtotal:
		plt.plot_date([d.ordinal() for d in dates], [ftr[d] + flk[d] + tf[d] for d in dates], fmt='k+-', tz=None, xdate=True, ydate=False, label='total', zorder=-2)
	ax.grid(visible=True, axis='y')
	if legend: plt.legend(ncol=2, loc='upper left')
	plt.show()
