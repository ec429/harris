#!/usr/bin/python
"""tombstone - total number of aircrew lost"""

import sys
import hhist, hdata

def count_losses(ents):
	days = sorted(hhist.group_by_date(ents))
	res = 0
	for d in days:
		for ent in d[1]:
			if ent['class'] != 'A': continue
			if ent['data']['type']['fb'] != 'B': continue
			typ = ent['data']['type']['ti']
			if ent['data']['etyp'] == 'CR':
				res += len(hdata.Bombers[typ]['crew'])
	return(res)

if __name__ == '__main__':
	entries = hhist.import_from_save(sys.stdin)
	tombstone = count_losses(entries)
	if not tombstone:
		print 'No losses have yet been incurred!'
	else:
		# it can never be 1, so we can always use the plural
		print '%d airmen are Missing In Action' % tombstone
		print r'''
      ----
     /    \
    /      \
   |  RIP   |
   |        |
   |WE WILL |
   |REMEMBER|
   |  THEM  |
 \ | \ ,   /| o
-|/-\|-|--\|-\|/
'''
