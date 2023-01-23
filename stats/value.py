#!/usr/bin/python3
"""value - total value tracking"""

import sys
import os.path
import subprocess
import hhist, hdata

this_script_path = os.path.abspath(os.path.dirname(sys.argv[0]))

# Intention for this report is to compute (total value of all bombers + cash) for each day
def extract_value(f):
	output = subprocess.check_output([os.path.join(this_script_path, 'bombers'), '--localdat'], stdin=f).decode('utf8')
	records = {}
	first = None
	current = None
	crecord = {}
	for line in output.splitlines():
		key, data = line.split(None, 1)
		if key == '@':
			if current:
				records[current] = crecord
				crecord = {}
			current = hhist.date.parse(data)
			if first is None:
				first = current
		else:
			bcounts = data.split()
			if len(bcounts) != len(hdata.Bombers) and key != '$':
				raise Exception("Bad bcounts[%s] at %r, len(%r) != %d" % (key, current, bcounts, len(hdata.Bombers)))
			crecord[key] = list(map(int, bcounts))
	if crecord and current is not None:
		records[current] = crecord
	broughton = hdata.Events.find('id', 'BROUGHTON')
	wlng = hdata.Bombers.find('name', 'Wellington')
	if broughton and first > broughton['date']:
		if wlng: wlng['cost'] *= 2/3.0
	bentry = [b.get('entry', hhist.date(0, 0, 0)) for b in hdata.Bombers]
	old = None
	oldp = None
	res = []
	for d in sorted(records):
		if broughton and d == broughton['date']:
			if wlng: wlng['cost'] *= 2/3.0
		bcount = records[d]['=']
		for i,b in enumerate(hdata.Bombers):
			if b['exit'] and d == b['exit'].next():
				if bcount[i]:
					print('Warning: %d leftover %s' % (bcount[i], b['name']))
					bcount[i] = 0 # force it to 0
		bvalues = [b*hdata.Bombers[i]['cost'] for i,b in enumerate(bcount)]
		built = records[d]['+']
		spend = sum(b*hdata.Bombers[i]['cost'] for i,b in enumerate(built) if d != hdata.Bombers[i]['entry'])
		cash = records[d]['$'][1] - spend
		cshr = records[d]['$'][0]
		total = sum(bvalues) + cash
		project = total + 25 * cshr
		r = {'date':d, 'cash':cash, 'cshr':cshr, 'bombers':list(bcount), 'bvalues':bvalues, 'total':total, 'project': project}
		if old is not None:
			r['delta'] = total - old
		if oldp is not None:
			r['deltap'] = project - oldp
		old = total
		oldp = project
		res.append(r)
	return res

if __name__ == '__main__':
	value = extract_value(sys.stdin)
	for row in value:
		print('%s: %s; %7d+%6d/ => %8d (%+7d)'%(row['date'], ','.join('%3d'%b for b in row['bombers']), row['cash'], row['cshr'], row['project'], row.get('deltap', 0)))
