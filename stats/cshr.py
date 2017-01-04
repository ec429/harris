#!/usr/bin/python2
"""cshr - cash rate (budget) tracking"""

import sys
import os.path
import subprocess
import hhist, hsave

this_script_path = os.path.abspath(os.path.dirname(sys.argv[0]))

def extract_cshr(f):
	records = subprocess.check_output(os.path.join(this_script_path, 'cshr'), stdin=f)
	entries = {}
	for record in records.splitlines():
		date, etyp, delta, current = record.split(" ")
		date = hhist.date.parse(date)
		delta = hsave.readfloat(delta)
		if etyp == 'CA':
			entries[date] = delta
		else:
			raise Exception("Bad etyp", etyp, "in record", record)
	return entries

if __name__ == '__main__':
	cshr = extract_cshr(sys.stdin)
	for c in sorted(cshr):
		print '%s: cshr=%d' % (c, cshr[c])
