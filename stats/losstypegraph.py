#!/usr/bin/python3
"""losstypegraph - graph of loss rates by type

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hdata, hhist, losstype
from extra_data import Bombers as extra
import optparse
import matplotlib.pyplot as plt

def parse_args(argv):
	x = optparse.OptionParser()
	x.add_option('-a', '--after', type='string')
	x.add_option('-b', '--before', type='string')
	x.add_option('-s', '--stratify', action='store_true')
	return x.parse_args()

if __name__ == '__main__':
	opts, args = parse_args(sys.argv)
	before = hhist.date.parse(opts.before) if opts.before else None
	after = hhist.date.parse(opts.after) if opts.after else None
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	if opts.stratify:
		loss, data = losstype.stratified_losstype(sys.stdin, after, before)
		bars = reversed(zip(hdata.Bombers, data))
		fbars = [bar for bar in bars if bar[1][2] is not None]
		ax.vlines(loss[3], 0, len(fbars)+1, color='0.75')
		x = [bar[1][3] for bar in fbars]
	else:
		loss, data = losstype.extract_losstype(sys.stdin, after, before)
		bars = reversed(zip(hdata.Bombers, data))
		fbars = [bar for bar in bars if bar[1][2] is not None]
		x = [bar[1][2] for bar in fbars]
	mr = float(max([bar[1][1] for bar in fbars]))
	ax.vlines(loss[2], 0, len(fbars)+1)
	yl = xrange(1, len(fbars)+1)
	gl = plt.barh(yl, x, height=[max(bar[1][1]/mr, 0.03) for bar in fbars], color=[extra[bar[0]['name']]['colour'] for bar in fbars], align='center', linewidth=0)
	ax.set_yticks(yl)
	ax.set_yticklabels([bar[0]['name'] for bar in fbars])
	plt.show()
