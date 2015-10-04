#!/usr/bin/python
"""tombstone - total number of aircrew lost"""

import sys
import hhist, hdata

def count_losses(ents):
	days = sorted(hhist.group_by_date(ents))
	de = 0
	pw = 0
	ex = 0
	for d in days:
		for ent in d[1]:
			if ent['class'] != 'C': continue
			if ent['data']['etyp'] == 'DE':
				de += 1
			elif ent['data']['etyp'] == 'PW':
				pw += 1
			elif ent['data']['etyp'] == 'EX':
				ex += 1
	return(de, pw, ex)

if __name__ == '__main__':
	entries = hhist.import_from_save(sys.stdin, crew_hist=True)
	tombstone, pw, ex = count_losses(entries)
	if not tombstone:
		print 'No losses have yet been incurred!'
	else:
		if tombstone == 1:
			print '1 airman has been Killed In Action'
		else:
			print '%d airmen have been Killed In Action' % tombstone
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
	if pw or ex:
		print
	if pw:
		print '(PoW count: %d)' % pw
	if ex:
		print '(esc count: %d)' % ex
