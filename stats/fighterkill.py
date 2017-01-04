#!/usr/bin/python
"""fighters - enemy fighter kill&loss tracking"""

import sys
import os.path
import subprocess
import hhist, hdata, hsave

this_script_path = os.path.abspath(os.path.dirname(sys.argv[0]))

def extract_kills(f):
	records = subprocess.check_output([os.path.join(this_script_path, 'kills'), '--localdat'], stdin=f, stderr=open(os.path.devnull, 'w'))
	res = {}
	d = None
	for record in records.splitlines():
		# <date> <b-or-f> <type> <ds> [<idx>]
		date, bf, typ, ds = record.split(" ", 3)
		ds, _, idx = ds.partition(" ")
		date = hhist.date.parse(date)
		if d:
			while d < date:
				d = d.next()
				res[d] = {'kills':{i:0 for i in xrange(len(hdata.Fighters))}, 'losses':{i:0 for i in xrange(len(hdata.Fighters))}, 'total':{'kills':0, 'losses':0}}
		else:
			d = date
			res[d] = {'kills':{i:0 for i in xrange(len(hdata.Fighters))}, 'losses':{i:0 for i in xrange(len(hdata.Fighters))}, 'total':{'kills':0, 'losses':0}}
		typ = int(typ)
		if bf == 'B':
			if ds == 'FT':
				ftyp = int(idx)
				res[d]['kills'][ftyp] = res[d]['kills'].get(ftyp, 0) + 1
				res[d]['total']['kills'] += 1
		elif bf == 'F':
			res[d]['losses'][typ] = res[d]['losses'].get(typ, 0) + 1
			res[d]['total']['losses'] += 1
		else:
			raise Exception("Bad b-or-f", bf, "in record", record)
	return res

if __name__ == '__main__':
	kills = extract_kills(sys.stdin)
	by_type = [(hdata.Fighters[i]['name'], sum([d['kills'][i] for d in kills.values()]), sum([d['losses'][i] for d in kills.values()])) for i in xrange(len(hdata.Fighters))]
	for b in by_type:
		ostr = "%s: kills=%d losses=%d"%b
		print(ostr)
