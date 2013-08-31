#!/usr/bin/python
"""profitgraph - graph of effectiveness by type

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import sys
import hsave, hdata, profit
import matplotlib.pyplot as plt

if __name__ == '__main__':
	save = hsave.Save.parse(sys.stdin)
	data = profit.extract_profit(save)
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
