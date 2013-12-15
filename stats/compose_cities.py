#!/usr/bin/python

import hdata
import sys
import math

def joinlist(l):
	o=[]
	for p in l:
		if isinstance(p, list):
			o.extend(p)
		else:
			o.append(p)
	return o

def readpbm(f):
	text = ' '.join(l for l in f.readlines() if not l.startswith('#'))
	tokens = text.split()
	tokens = [t for t in tokens if not t.startswith('#')]
	tokens[3:] = joinlist(map(list, tokens[3:]))
	magic = tokens[0]
	assert magic == 'P1'
	w, h = map(int, tokens[1:3])
	assert len(tokens) >= 3+w*h
	return w, h, [[int(tokens[3+x+y*h]) for x in xrange(w)] for y in xrange(h)]

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

cx, cy = None, None

if len(sys.argv) > 2:
	cx = int(sys.argv[1])
	cy = int(sys.argv[2])

print "P3"
print "256 256"
print "15"
for y in xrange(256):
	for x in xrange(256):
		if citymap[y][x]:
			stuff = hash(str(citymap[y][x]))
			if stuff&0xfff == 0xfff:
				stuff = 0x245
		else:
			stuff = 0xfff
		r, g, b = (stuff&0xf, (stuff>>4)&0xf, (stuff>>8)&0xf)
		if None not in [cx, cy]:
			d = math.hypot(x-cx, y-cy)
			def mark(d, l, f):
				s = math.pow(1-math.cos(d*math.pi/f), 0.2)
				br = 15/(1+s)-7
				return [max(v-br, 0) for v in l]
			[r, g, b] = mark(d, [r, g, b], 5)
			[r, g, b] = mark(d, [r, g, b], 20)
		print "%d %d %d" %(r, g, b)