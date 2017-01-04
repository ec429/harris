#!/usr/bin/python
"""losstype - loss rates by type"""

import sys
import os.path
import subprocess
import hhist, hdata
import extra_data
import optparse

this_script_path = os.path.abspath(os.path.dirname(sys.argv[0]))

def extract_raids(f):
	output = subprocess.check_output(os.path.join(this_script_path, 'raids'), stdin=f)
	current = None
	crecord = {}
	records = {}
	for line in output.splitlines():
		key, data = line.split(None, 1)
		if key == '@':
			if current:
				records[current] = crecord
				crecord = {}
			current = hhist.date.parse(data)
		else:
			typ, targ = key.split(',', 1)
			typ = int(typ)
			targ = int(targ)
			raids, losses = data.split(',', 1)
			raids = int(raids)
			losses = int(losses)
			crecord[typ, targ] = (raids, losses)
	if current:
		records[current] = crecord
	return records

def extract_losstype(f, after=None, before=None):
	records = extract_raids(f)
	return extract_losstype_from_records(records, after, before)

def extract_losstype_from_records(records, after=None, before=None):
	rcount = [0 for i in hdata.Bombers]
	lcount = list(rcount)
	for d in records:
		if after and d < after: continue
		if before and d >= before: continue
		for key in records[d]:
			r, l = records[d][key]
			typ, targ = key
			lcount[typ] += l
			rcount[typ] += r
	return ((sum(lcount), sum(rcount), sum(lcount)*100/float(sum(rcount)) if sum(rcount) else None), [(l, r, l*100/float(r) if r else None) for l, r in zip(lcount, rcount)])

def stratified_losstype(f, after=None, before=None):
	"""Stratifies loss rates by target, scaling them to account for how dangerous the targets raided are"""
	records = extract_raids(f)
	uo, ul = extract_losstype_from_records(records, after, before)
	t = [0 for l in ul]
	w = [0 for l in ul]
	rcount = [[0 for i in hdata.Bombers] for j in hdata.Targets]
	lcount = [[0 for i in hdata.Bombers] for j in hdata.Targets]
	for d in records:
		if after and d < after: continue
		if before and d >= before: continue
		for key in records[d]:
			r, l = records[d][key]
			typ, targ = key
			rcount[targ][typ] += r
			lcount[targ][typ] += l
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

def parse_args(argv):
	x = optparse.OptionParser()
	x.add_option('-a', '--after', type='string')
	x.add_option('-b', '--before', type='string')
	x.add_option('-s', '--stratify', action='store_true')
	return x.parse_args()

if __name__ == '__main__':
	def tbl_row(n, s, l, p):
		ostr = "%s: %s %s %s"%(n.rjust(4), s.rjust(7), l.rjust(7), p.rjust(5))
		print(ostr)
	def tbl_nrow(n, s, l, p):
		tbl_row(n, str(s), str(l), "%5.2f"%p if p is not None else "  -  ")
	opts, args = parse_args(sys.argv)
	before = hhist.date.parse(opts.before) if opts.before else None
	after = hhist.date.parse(opts.after) if opts.after else None
	if opts.stratify:
		overall, losstype = stratified_losstype(sys.stdin, after, before)
		def tbl_row(n, s, l, p, q):
			ostr = "%s: %s %s %s %s"%(n.rjust(4), s.rjust(7), l.rjust(7), p.rjust(5), q.rjust(5))
			print(ostr)
		def tbl_nrow(n, s, l, p, q):
			tbl_row(n, str(s), str(l), "%5.2f"%p if p is not None else "  -  ", "%5.2f"%q if q is not None else "  -  ")
		tbl_row("NAME", "Sorties", "Losses", "Loss%", "Strat%")
		for i,l in enumerate(losstype):
			if not l[1]: continue
			name = extra_data.Bombers[hdata.Bombers[i]['name']]['short']
			tbl_nrow(name, l[1], l[0], l[2], l[3])
		tbl_nrow("****", overall[1], overall[0], overall[2], overall[3])
	else:
		overall, losstype = extract_losstype(sys.stdin, after, before)
		def tbl_row(n, s, l, p):
			ostr = "%s: %s %s %s"%(n.rjust(4), s.rjust(7), l.rjust(7), p.rjust(5))
			print(ostr)
		def tbl_nrow(n, s, l, p):
			tbl_row(n, str(s), str(l), "%5.2f"%p if p is not None else "  -  ")
		tbl_row("NAME", "Sorties", "Losses", "Loss%")
		for i,l in enumerate(losstype):
			if not l[1]: continue
			name = extra_data.Bombers[hdata.Bombers[i]['name']]['short']
			tbl_nrow(name, l[1], l[0], l[2])
		tbl_nrow("****", overall[1], overall[0], overall[2])
