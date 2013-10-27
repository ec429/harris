#!/usr/bin/python

import hdata
import sys

def readpbm(f):
	text = ' '.join(f.readlines())
	tokens = text.split()
	tokens = [t for t in tokens if not t.startswith('#')]
	magic = tokens[0]
	assert magic == 'P1'
	w, h = map(int, tokens[1:3])
	assert len(tokens) >= 3+w*h
	return (w, h, [[int(tokens[3+x+y*h]) for x in xrange(w)] for y in xrange(h)])

citymap = [[0 for x in xrange(256)] for y in xrange(256)]

index = 0

names = {}

for t in hdata.Targets:
	if 'CITY' not in t['flags']: continue
	index += 1
	names[index] = t['name']
	cfn = "dat/cities/%s.pbm" % t['name']
	with open(cfn, "r") as cf:
		w, h, cp = readpbm(cf)
		for x in xrange(w):
			for y in xrange(h):
				if cp[y][x]:
					if citymap[y+t['lat']-h/2][x+t['long']-w/2]:
						sys.stderr.write("Warning: %s overwrote %s at %d,%d\n"%(t['name'], names[citymap[y+t['lat']-h/2][x+t['long']-w/2]], x, y))
					citymap[y+t['lat']-h/2][x+t['long']-w/2] = index

print "P3"
print "256 256"
print "15"
for y in xrange(256):
	for x in xrange(256):
		stuff = hash(str(citymap[y][x])) if citymap[y][x] else 0xfff
		print "%d %d %d" %(stuff&0xf, (stuff>>4)&0xf, (stuff>>8)&0xf)
