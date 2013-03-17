#!/usr/bin/python
"""losstarggraph - graph of loss rates by target

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hsave, hdata, losstarg
import matplotlib.pyplot as plt

if __name__ == '__main__':
	if '--type' in sys.argv:
		typ = int(sys.argv[sys.argv.index('--type')+1])
	else: typ = None
	save = hsave.Save.parse(sys.stdin)
	loss, data = losstarg.extract_losstarg(save, typ=typ)
	bars = reversed(zip(hdata.Targets, data))
	fbars = [bar for bar in bars if bar[1][2] is not None]
	mr = float(max([bar[1][1] for bar in fbars]))
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	ax.vlines(loss, 0, len(fbars)+1)
	yl = xrange(1, len(fbars)+1)
	gl = plt.barh(yl, [bar[1][2] for bar in fbars], height=[bar[1][1]/mr for bar in fbars], color='b', align='center')
	ax.set_yticks(yl)
	ax.set_yticklabels([bar[0]['name'] for bar in fbars])
	plt.show()
