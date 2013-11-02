#!/usr/bin/python
"""losstarggraph - graph of loss rates by target

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hsave, hdata, losstarg
import matplotlib.pyplot as plt

if __name__ == '__main__':
	opts, args = losstarg.parse_args(sys.argv)
	save = hsave.Save.parse(sys.stdin)
	loss, data = losstarg.extract_losstarg(save, opts)
	bars = reversed(zip(hdata.Targets, data))
	fbars = [bar for bar in bars if bar[1][2] is not None]
	mr = float(max([bar[1][1] for bar in fbars]))
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	ax.vlines(loss, 0, len(fbars)+1)
	yl = xrange(1, len(fbars)+1)
	gl = plt.barh(yl, [bar[1][2] for bar in fbars], height=[max(bar[1][1]/mr, 0.2) for bar in fbars], color='b', align='center', linewidth=0)
	ax.set_yticks(yl)
	ax.set_yticklabels([bar[0]['name'] for bar in fbars])
	plt.show()
