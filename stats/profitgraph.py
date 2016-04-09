#!/usr/bin/python
"""profitgraph - graph of effectiveness by type

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hsave, hdata, hhist, profit
import matplotlib.pyplot as plt
import optparse

def parse_args(argv):
	x = optparse.OptionParser()
	x.add_option('-a', '--after', type='string')
	x.add_option('-b', '--before', type='string')
	x.add_option('--targ', type='string')
	opts, args = x.parse_args()
	if opts.targ:
		try:
			opts.targ=[t['name'] for t in hdata.Targets].index(opts.targ)
		except ValueError:
			try:
				opts.targ=int(opts.targ)
			except ValueError:
				x.error("No such targ", opts.targ)
	return opts, args

if __name__ == '__main__':
	opts, args = parse_args(sys.argv)
	before = hhist.date.parse(opts.before) if opts.before else None
	after = hhist.date.parse(opts.after) if opts.after else None
	save = hsave.Save.parse(sys.stdin)
	data = profit.extract_profit(save, before, after, targ_id=opts.targ)
	bars = reversed(data.items())
	fbars = [bar for bar in bars if bar[1]['full'][0]]
	mr = float(max([bar[1]['full'][0] for bar in fbars]))/0.75
	fig = plt.figure()
	ax = fig.add_subplot(1,1,1)
	yl = xrange(1, len(fbars)+1)
	plt.barh(yl, [hdata.Bombers[bar[0]]['cost'] for bar in fbars], height=0.8, color='0.5', align='center', label='Cost')
	plt.barh(yl, [bar[1]['fullr'] for bar in fbars], height=[max(bar[1]['full'][0]/mr, 0.1) for bar in fbars], color='b', align='center', label='All')
	plt.barh(yl, [bar[1]['opti'] for bar in fbars], height=[max(bar[1]['dead'][0]/mr, 0.06) for bar in fbars], color='g', align='center', label='Optimistic')
	plt.barh(yl, [bar[1]['deadr'] for bar in fbars], height=[max(bar[1]['dead'][0]/mr, 0.06) for bar in fbars], color='r', align='center', label='Dead only')
	ax.set_yticks(yl)
	ax.set_yticklabels([hdata.Bombers[bar[0]]['name'] for bar in fbars])
	plt.xlabel('Value in pounds')
	plt.ylabel('Type')
	plt.title('Profitability by bomber type')
	plt.legend(loc='upper right')
	plt.show()
