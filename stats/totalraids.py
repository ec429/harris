#!/usr/bin/python3
"""totalraids - number of bombers deployed per night"""

import sys
import hhist, hdata, hsave
import optparse

def daily_total(d, bombers, start, stop): # updates bombers
	if stop:
		return
	totals = [0, 0] # MF,PFF
	for h in d[1]:
		if h['class'] == 'A':
			acid = h['data']['acid']
			if h['data']['type']['fb'] == 'B':
				if h['data']['etyp'] == 'CT':
					bombers[acid]=False
				elif h['data']['etyp'] in ['CR', 'SC', 'OB']:
					del bombers[acid]
				elif h['data']['etyp'] == 'RA':
					if acid not in bombers:
						print 'Warning: un-inited bomber %08x'%acid
						bombers[acid] = False
					totals[int(bombers[acid])] += 1
				elif h['data']['etyp'] == 'PF':
					if acid not in bombers:
						print 'Warning: un-inited bomber %08x'%acid
					bombers[acid] = True
	return totals

def extract_totals(save, before=None, after=None):
	bombers = {b['id']:b['pff'] for b in save.init.bombers}
	days = sorted(hhist.group_by_date(save.history))
	totals = {}
	for d in days:
		total = daily_total(d, bombers, d[0]>=after if after else True, d[0]>=before if before else False)
		if total is not None:
			totals[d[0]] = total
	return totals

def parse_args(argv):
	x = optparse.OptionParser()
	x.add_option('-a', '--after', type='string')
	x.add_option('-b', '--before', type='string')
	return x.parse_args()

if __name__ == '__main__':
	opts, args = parse_args(sys.argv)
	before = hhist.date.parse(opts.before) if opts.before else None
	after = hhist.date.parse(opts.after) if opts.after else None
	save = hsave.Save.parse(sys.stdin)
	totals = extract_totals(save, before, after)
	pff = hdata.Events.find('id', 'PFF')
	for d in sorted(totals):
		m,p = totals[d]
		if m or p:
			if pff and d < pff['date']:
				print '%s: %d'%(d,m)
				if p:
					print '(Unexpected PFF: %s)'%p
			else:
				print '%s: %d (%d main force, %d PFF)'%(d,m+p,m,p)
