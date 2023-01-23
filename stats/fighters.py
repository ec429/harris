#!/usr/bin/python3
"""fighters - enemy fighter force tracking"""

import sys
import os.path
import subprocess
import hhist
import pprint

this_script_path = os.path.abspath(os.path.dirname(sys.argv[0]))

def extract_fighters(f):
	records = subprocess.check_output([os.path.join(this_script_path, 'fighters'), '--localdat'], stdin=f, stderr=open(os.path.devnull, 'w'))
	res = []
	d = None
	day = {}
	def save_day():
		if day:
			res.append({'date':d, 'fighters':{i:day[i][0] for i in day}, 'radar':{i:day[i][1] for i in day}, 'total':sum(day[i][0] for i in day)})
	for record in records.splitlines():
		# <date> <type> <count> <radar>
		date, typ, count, radar = record.split(" ")
		date = hhist.date.parse(date)
		if date != d:
			save_day()
			day = {}
			d = date
		typ = int(typ)
		count = int(count)
		radar = int(radar)
		day[typ] = (count, radar)
	save_day()
	return res

if __name__ == '__main__':
	fighters = extract_fighters(sys.stdin)
	pprint.pprint(fighters)
