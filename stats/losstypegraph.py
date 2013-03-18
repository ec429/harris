#!/usr/bin/python
"""losstypegraph - graph of loss rates by type

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hsave, hdata, losstype
import matplotlib.pyplot as plt

if __name__ == '__main__':
	if '--targ' in sys.argv:
		targ = int(sys.argv[sys.argv.index('--targ')+1])
	else: targ = None
	save = hsave.Save.parse(sys.stdin)
	loss, data = losstype.extract_losstype(save, targ=targ)
	bars = list(reversed(zip(hdata.Bombers, data)))
	fbars = [bar for bar in bars if bar[1][2] is not None]
	mr = float(max([bar[1][1] for bar in fbars]))
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	ax.vlines(loss, 0, len(fbars)+1)
	yl = xrange(1, len(fbars)+1)
	cols = list(reversed(['0.5','y','r','c','m','b','0.5','y','r','c','m','r','c']))
	fcols = [cols[i] for i,bar in enumerate(bars) if bar[1][2] is not None]
	gl = plt.barh(yl, [bar[1][2] for bar in fbars], height=[bar[1][1]/mr for bar in fbars], color=fcols, align='center')
	ax.set_yticks(yl)
	ax.set_yticklabels([bar[0]['name'] for bar in fbars])
	plt.show()
