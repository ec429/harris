#!/usr/bin/python
"""fighters - enemy fighter force tracking"""

import sys
import hhist, hdata, hsave

def extract_fighters(save):
	def count(fighters, typ, rad):
		ret = 0
		for f in fighters:
			if f['type'] == typ and (f['radar'] or not rad):
				ret += 1
		return ret
	fighters = [dict(f) for f in save.init.fighters]
	res = [{'date':save.init.date, 'fighters':[count(fighters, typ, False) for typ in xrange(save.nftypes)], 'radar':[count(fighters, typ, True) for typ in xrange(save.nftypes)], 'total':len(fighters)}]
	days = sorted(hhist.group_by_date(save.history))
	for d in days:
		for h in d[1]:
			if h['class'] == 'A':
				acid = h['data']['acid']
				if h['data']['type']['fb'] == 'F':
					if h['data']['etyp'] == 'CT':
						assert len([f for f in fighters if f['id'] == acid]) == 0
						fighters.append({'type':h['data']['type']['ti'], 'radar':0, 'id':acid})
					elif h['data']['etyp'] in ('CR', 'OB'):
						assert len([f for f in fighters if f['id'] == acid]) == 1
						fighters = [f for f in fighters if f['id'] != acid]
					elif h['data']['etyp'] == 'NA':
						assert len([f for f in fighters if f['id'] == acid]) == 1
						for f in fighters:
							if f['id'] == acid:
								f['radar'] = True
		for i,f in enumerate(hdata.Fighters):
			if hdata.Fighters[i]['exit'] and d[0] == f['exit'].next():
				assert count(fighters, i, False) == 0
		res.append({'date':d[0].next(), 'fighters':[count(fighters, typ, False) for typ in xrange(save.nftypes)], 'radar':[count(fighters, typ, True) for typ in xrange(save.nftypes)], 'total':len(fighters)})
	return res

if __name__ == '__main__':
	save = hsave.Save.parse(sys.stdin)
	fighters = extract_fighters(save)
	print fighters
