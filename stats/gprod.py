#!/usr/bin/python2
"""gprod - German production tracking"""

import sys
import hhist, hsave
import subprocess
import pprint

def extract_gprod(f):
	records = subprocess.check_output(['grep', ' 11:55 M GP '], stdin=f)
	res = []
	d = None
	day = {}
	def save_day():
		if day:
			grow = {i:day[i][0] for i in day}
			drow = {i:day[i][1] for i in day}
			for dct in [grow, drow]:
				dct['total'] = sum(dct.values())
			res.append({'date':d, 'gprod':grow, 'dprod':drow})
	for record in records.splitlines():
		# <date> 11:55 M GP <class> <current> <delta>
		date, time, M, GP, cls, current, delta = record.split(" ")
		assert time == '11:55', record
		assert M == 'M', record
		assert GP == 'GP', record
		date = hhist.date.parse(date)
		if date != d:
			save_day()
			day = {}
			d = date
		cls = int(cls)
		current = hsave.readfloat(current)
		delta = hsave.readfloat(delta)
		day[cls] = (current, delta)
	save_day()
	return res

if __name__ == '__main__':
	gprod = extract_gprod(sys.stdin)
	pprint.pprint(gprod)
