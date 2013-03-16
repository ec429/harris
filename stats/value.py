#!/usr/bin/python
"""value - total value tracking"""

import sys
import hhist, hdata, hsave

# Intention for this report is to compute (total value of all bombers + cash) for each day
def extract_value(save):
	bcount = [0 for i in xrange(save.ntypes)]
	for b in save.init.bombers:
		bcount[b['type']] += 1
	cash = save.init.cash
	bvalues = [b*hdata.Bombers[i]['cost'] for i,b in enumerate(bcount)]
	total = sum(bvalues) + cash
	res = [{'date':save.init.date, 'cash':cash, 'bombers':bcount, 'bvalues':bvalues, 'total':total}]
	days = sorted(hhist.group_by_date(save.history))
	for d in days:
		for i,b in enumerate(hdata.Bombers):
			if hdata.Bombers[i]['exit'] and d[0] == hdata.Bombers[i]['exit']:
				bcount[i] = 0
		for h in d[1]:
			if h['class'] == 'A':
				if h['data']['type']['fb'] == 'B':
					if h['data']['etyp'] == 'CT':
						bcount[h['data']['type']['ti']] += 1
					elif h['data']['etyp'] == 'CR':
						bcount[h['data']['type']['ti']] -= 1
			elif h['class'] == 'M':
				if h['data']['etyp'] == 'CA':
					cash = h['data']['data']['cash']
		bvalues = [b*hdata.Bombers[i]['cost'] for i,b in enumerate(bcount)]
		total = sum(bvalues) + cash
		res.append({'date':d[0].next(), 'cash':cash, 'bombers':bcount, 'bvalues':bvalues, 'total':total})
	return res

if __name__ == '__main__':
	save = hsave.Save.parse(sys.stdin)
	value = extract_value(save)
	print value
