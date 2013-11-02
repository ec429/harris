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
	cshr = save.init.cshr
	bvalues = [b*hdata.Bombers[i]['cost'] for i,b in enumerate(bcount)]
	bentry = [b.get('entry', hhist.date(0, 0, 0)) for b in hdata.Bombers]
	total = sum(bvalues) + cash
	res = [{'date':save.init.date, 'cash':cash, 'cshr':cshr, 'bombers':bcount, 'bvalues':bvalues, 'total':total}]
	days = sorted(hhist.group_by_date(save.history))
	broughton = hdata.Events.find('id', 'BROUGHTON')
	wlng = hdata.Bombers.find('name', 'Wellington')
	for d in days:
		if broughton and d[0] == broughton['date']:
			if wlng: wlng['cost'] *= 2/3.0
		spend = 0
		for h in d[1]:
			if h['class'] == 'A':
				if h['data']['type']['fb'] == 'B':
					if h['data']['etyp'] == 'CT':
						bcount[h['data']['type']['ti']] += 1
						if d[0].next() != bentry[h['data']['type']['ti']]:
							spend += hdata.Bombers[h['data']['type']['ti']]['cost']
					elif h['data']['etyp'] in ('CR', 'OB'):
						bcount[h['data']['type']['ti']] -= 1
			elif h['class'] == 'M':
				if h['data']['etyp'] == 'CA':
					cash = h['data']['data']['cash']
					cshr = h['data']['data']['cshr']
		for i,b in enumerate(hdata.Bombers):
			if b['exit'] and d[0] == b['exit'].next():
				if bcount[i]:
					print 'Warning: %d leftover %s' % (bcount[i], b['name'])
					bcount[i] = 0 # force it to 0
		bvalues = [b*hdata.Bombers[i]['cost'] for i,b in enumerate(bcount)]
		cash -= spend
		total = sum(bvalues) + cash
		res.append({'date':d[0].next(), 'cash':cash, 'cshr':cshr, 'bombers':bcount, 'bvalues':bvalues, 'total':total})
	return res

if __name__ == '__main__':
	save = hsave.Save.parse(sys.stdin)
	value = extract_value(save)
	print value
