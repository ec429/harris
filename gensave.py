#!/usr/bin/python

import sys, zlib

def multiply(line):
	if len(line) > 1:
		if line[0] == '*' and line[1].isdigit():
			l = line.find('*', 2)
			if l > 1:
				n = int(line[1:l])
				return n*[line[l+1:]]
	return [line]

def genids(line, i):
	if line.endswith(',NOID\n'):
		z = '_'.join((salt, str(i), line))
		ha = zlib.crc32(z) & 0xffffffff
		h = hex(ha)[2:].rstrip('L')
		return '%s,%s\n' % (line[:-6], h.zfill(8))
	else:
		return line

salt = ''
if '--salt' in sys.argv:
	sarg = sys.argv.index('--salt')
	try:
		salt = sys.argv[sarg+1]
	except IndexError:
		sys.stderr.write('--salt requires argument!\n')
		sys.exit(1)

for line in sys.stdin.readlines():
	lines = multiply(line)
	for i,line in enumerate(lines):
		line = genids(line, i)
		sys.stdout.write(line)
