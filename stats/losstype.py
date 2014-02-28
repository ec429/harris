#!/usr/bin/python
"""losstype - loss rates by type"""

import sys
import hhist, hsave, hdata
import extra_data

def extract_losstype(save, targ=None):
	rcount = [0 for i in xrange(save.ntypes)]
	lcount = list(rcount)
	days = sorted(hhist.group_by_date(save.history))
	for d in days:
		raiding = {}
		for h in d[1]:
			if h['class'] == 'A':
				if h['data']['type']['fb'] == 'B':
					if h['data']['etyp'] == 'RA':
						raiding[h['data']['acid']] = h['data']['data']['target']
						if targ is not None and h['data']['data']['target'] != targ: continue
						rcount[h['data']['type']['ti']] += 1
					elif h['data']['etyp'] == 'CR':
						if targ is not None and raiding[h['data']['acid']] != targ: continue
						lcount[h['data']['type']['ti']] += 1
	return ((sum(lcount), sum(rcount), sum(lcount)*100/float(sum(rcount)) if sum(rcount) else None), [(l, r, l*100/float(r) if r else None) for l, r in zip(lcount, rcount)])

def stratified_losstype(save):
	"""Stratifies loss rates by target, scaling them to account for how dangerous the targets raided are"""
	uo, ul = extract_losstype(save)
	t = [0 for l in ul]
	w = [0 for l in ul]
	rcount = [[0 for i in xrange(save.ntypes)] for j in hdata.Targets]
	lcount = [[0 for i in xrange(save.ntypes)] for j in hdata.Targets]
	days = sorted(hhist.group_by_date(save.history))
	for d in days:
		raiding = {}
		for h in d[1]:
			if h['class'] == 'A':
				if h['data']['type']['fb'] == 'B':
					if h['data']['etyp'] == 'RA':
						raiding[h['data']['acid']] = h['data']['data']['target']
						rcount[h['data']['data']['target']][h['data']['type']['ti']] += 1
					elif h['data']['etyp'] == 'CR':
						lcount[raiding[h['data']['acid']]][h['data']['type']['ti']] += 1
	for targ in hdata.Targets:
		j = targ['i']
		overall, losstype = ((sum(lcount[j]), sum(rcount[j]), sum(lcount[j])*100/float(sum(rcount[j])) if sum(rcount[j]) else None), [(l, r, l*100/float(r) if r else None) for l, r in zip(lcount[j], rcount[j])])
		if overall[2]:
			lr = [l[0] / overall[2] if l[2] else 0 for l in losstype]
		else:
			lr = [0 for l in losstype]
		for i,l in enumerate(losstype):
			t[i] += lr[i]
			w[i] += l[1]
	for i in xrange(len(ul)):
		ul[i] = ul[i] + (t[i] * 100 * uo[2] / float(w[i]) if w[i] else None,)
	uo = uo + (sum(t) * 100 * uo[2] / float(sum(w)) if sum(w) else None,) # should come out equal to uo[2], unless there is a type with sorties but no losses
	return uo, ul

if __name__ == '__main__':
	def tbl_row(n, s, l, p):
		print "%s: %s %s %s"%(n.rjust(4), s.rjust(7), l.rjust(7), p.rjust(5))
	def tbl_nrow(n, s, l, p):
		tbl_row(n, str(s), str(l), "%5.2f"%p if p is not None else "  -  ")
	save = hsave.Save.parse(sys.stdin)
	if '--stratify' in sys.argv:
		overall, losstype = stratified_losstype(save)
		def tbl_row(n, s, l, p, q):
			print "%s: %s %s %s %s"%(n.rjust(4), s.rjust(7), l.rjust(7), p.rjust(5), q.rjust(5))
		def tbl_nrow(n, s, l, p, q):
			tbl_row(n, str(s), str(l), "%5.2f"%p if p is not None else "  -  ", "%5.2f"%q if q is not None else "  -  ")
		tbl_row("NAME", "Sorties", "Losses", "Loss%", "Strat%")
		for i,l in enumerate(losstype):
			name = extra_data.Bombers[hdata.Bombers[i]['name']]['short']
			tbl_nrow(name, l[1], l[0], l[2], l[3])
		tbl_nrow("****", overall[1], overall[0], overall[2], overall[3])
	else:
		overall, losstype = extract_losstype(save)
		def tbl_row(n, s, l, p):
			print "%s: %s %s %s"%(n.rjust(4), s.rjust(7), l.rjust(7), p.rjust(5))
		def tbl_nrow(n, s, l, p):
			tbl_row(n, str(s), str(l), "%5.2f"%p if p is not None else "  -  ")
		tbl_row("NAME", "Sorties", "Losses", "Loss%")
		for i,l in enumerate(losstype):
			name = extra_data.Bombers[hdata.Bombers[i]['name']]['short']
			tbl_nrow(name, l[1], l[0], l[2])
		tbl_nrow("****", overall[1], overall[0], overall[2])
