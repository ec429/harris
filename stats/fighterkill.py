#!/usr/bin/python
"""fighters - enemy fighter kill&loss tracking"""

import sys
import hhist, hdata, hsave

def extract_kills(save):
	days = sorted(hhist.group_by_date(save.history))
	res = []
	ftr = {f['id']:f['type'] for f in save.init.fighters}
	for d in days:
		kills = [0 for i in xrange(save.nftypes)]
		losses = list(kills)
		ac = {}
		for h in d[1]:
			try:
				if h['class'] == 'A':
					b = h['data']['acid']
					typ = h['data']['type']
					if h['data']['etyp'] == 'DM':
						styp = h['data']['data']['styp']
						ac[b] = {'s':styp, 't':h['data']['type']}
						if h['data']['data']['styp'] == 'AC':
							ac[b]['k'] = h['data']['data']['ac']
						elif h['data']['data']['styp'] == 'FK':
							ac[b]['k'] = h['data']['data']['flak']
						elif h['data']['data']['styp'] == 'TF':
							ac[b]['k'] = h['data']['data']['targ']
						else:
							raise Exception('Unknown A/DM/styp', styp)
					elif h['data']['etyp'] == 'CR':
						fb = typ['fb']
						if fb == 'F':
							losses[typ['ti']] += 1
						elif fb == 'B':
							if b in ac.keys():
								if ac[b]['s'] == 'AC':
									f = ac[b]['k']
									if f in ftr.keys():
										kills[ftr[f]] += 1
									else:
										print 'Kill scored by unknown fighter', hex(f)[2:].zfill(8)
						else:
							raise Exception('Unknown A/type/fb', fb)
					if typ['fb'] == 'F':
						if h['data']['etyp'] == 'CT' or b not in ftr.keys():
							ftr[b] = typ['ti']
						if h['data']['etyp'] in ('CR','OB'):
							del ftr[b]
			except Exception:
				print h
				raise
		res.append({'date':d[0].next(), 'kills':kills, 'losses':losses, 'total':(sum(kills),sum(losses))})
	return res

if __name__ == '__main__':
	save = hsave.Save.parse(sys.stdin)
	kills = extract_kills(save)
	by_type = [(hdata.Fighters[i]['name'], sum([d['kills'][i] for d in kills]), sum([d['losses'][i] for d in kills])) for i in xrange(save.nftypes)]
	for b in by_type:
		print "%s: kills=%d losses=%d"%b
