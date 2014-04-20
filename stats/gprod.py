#!/usr/bin/python
"""gprod - German production tracking"""

import sys
import hhist, hdata, hsave

def extract_gprod(save):
	days = sorted(hhist.group_by_date(save.history))
	res = [{'date':save.init.date}]
	res[0]['gprod'] = dict(save.init.gprod)
	res[0]['gprod']['total'] = sum(save.init.gprod.values())
	for d in days:
		gprod = {i:0 for i in xrange(save.iclasses)}
		dprod = {i:0 for i in xrange(save.iclasses)}
		for h in d[1]:
			if h['class'] == 'M':
				if h['data']['etyp'] == 'GP':
					gprod[h['data']['data']['iclass']] = h['data']['data']['gprod']
					dprod[h['data']['data']['iclass']] = h['data']['data']['dprod']
		grow = {'total': sum(gprod.values())}
		drow = {'total': sum(dprod.values())}
		grow.update(gprod)
		drow.update(dprod)
		row = {'date':d[0].next(), 'gprod':grow, 'dprod':drow}
		res.append(row)
	return res

if __name__ == '__main__':
	save = hsave.Save.parse(sys.stdin)
	gprod = extract_gprod(save)
	print gprod
