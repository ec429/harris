#!/usr/bin/python
"""fighters - enemy fighter force tracking"""

import sys
import hhist, hdata, hsave

def extract_fighters(save):
	fcount = [0 for i in xrange(save.nftypes)]
	for f in save.init.fighters:
		fcount[f['type']] += 1
	res = [{'date':save.init.date, 'fighters':list(fcount), 'total':sum(fcount)}]
	days = sorted(hhist.group_by_date(save.history))
	for d in days:
		for h in d[1]:
			if h['class'] == 'A':
				if h['data']['type']['fb'] == 'F':
					if h['data']['etyp'] == 'CT':
						fcount[h['data']['type']['ti']] += 1
					elif h['data']['etyp'] in ('CR', 'OB'):
						fcount[h['data']['type']['ti']] -= 1
		for i,f in enumerate(hdata.Fighters):
			if hdata.Fighters[i]['exit'] and d[0] == f['exit'].next():
				assert fcount[i] == 0
		res.append({'date':d[0].next(), 'fighters':list(fcount), 'total':sum(fcount)})
	return res

if __name__ == '__main__':
	save = hsave.Save.parse(sys.stdin)
	fighters = extract_fighters(save)
	print fighters
