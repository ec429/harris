#!/usr/bin/python

import sys

def multiply(line):
	if len(line) > 1:
		if line[0] == '*' and line[1].isdigit():
			l = line.find('*', 2)
			if l > 1:
				n = int(line[1:l])
				return n*[line[l+1:]]
	return [line]

def genids(line, i, j):
	if line.endswith(',NOID\n'):
		z = '_'.join((salt, str(i), str(j), line))
		h = hex(hash(z))
		if h[0] == '-': h=h[1:]
		h=h[2:10]
		return '%s,%s\n' % (line[:-6], h)
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

for i,line in enumerate(sys.stdin.readlines()):
	lines = multiply(line)
	for j,line in enumerate(lines):
		line = genids(line, i, j)
		sys.stdout.write(line)
