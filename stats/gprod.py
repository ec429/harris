#!/usr/bin/python
"""gprod - German production tracking"""

import sys
import hhist, hdata, hsave

def extract_gprod(save):
	days = sorted(hhist.group_by_date(save.history))
	res = [{'date':save.init.date}]
	res[0].update(save.init.gprod)
	res[0]['total'] = sum(save.init.gprod.values())
	for d in days:
		gprod = {i:0 for i in xrange(save.iclasses)}
		for h in d[1]:
			if h['class'] == 'M':
				if h['data']['etyp'] == 'GP':
					gprod[h['data']['data']['iclass']] = h['data']['data']['gprod']
		total = sum(gprod.values())
		row = {'date':d[0].next(), 'total':total}
		row.update(gprod)
		res.append(row)
	return res

if __name__ == '__main__':
	save = hsave.Save.parse(sys.stdin)
	gprod = extract_gprod(save)
	print gprod
