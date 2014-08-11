#!/usr/bin/python

import optparse, sys

entities = ['dclass',]

changes = {'0.2.1':[('dclass', '*', 8)], # first version supported by upconvert
           '0.2.2':[],
          }

firstver = min(changes)
lastver = max(changes)

counts = {}
for ver in sorted(changes):
    if ver == firstver:
        counts[ver] = {}
        for ent in entities:
            counts[ver][ent] = 0
        for change in changes[ver]:
            ent, op, num = change
            assert op == '*', change
            counts[ver][ent] = num
    else:
        counts[ver] = dict(counts[prev])
        for change in changes[ver]:
            ent, op, num = change
            if op == '+':
                counts[ver][ent] += 1
            elif op == '-':
                counts[ver][ent] -= 1
            else:
                assert 0, change
    prev = ver

def parse_args():
	x = optparse.OptionParser(usage="%prog infile >outfile")
	x.add_option('-t', '--to-version', type='string', default=lastver,
	             dest="tover", help="Version to update to (default newest known)")
	opts, args = x.parse_args()
	if not args:
	    x.error("No filename specified!")
	if len(args) > 1:
		x.error("Multiple files specified!")
	if opts.tover not in changes:
	    x.error("Unrecognised version %s (have: %s)"%(opts.tover, ', '.join(sorted(changes))))
	return opts, args

if __name__ == "__main__":
    opts, args = parse_args()
    fn = args.pop()
    with open(fn, "r") as fi:
        verline = fi.readline().strip()
        assert verline.startswith('HARR:'), verline
        ver = verline[5:]
        if ver not in changes:
            sys.stderr.write("Save file is too old (version %s)\n"%ver)
            sys.stderr.write("  Supported: %s\n"%(', '.join(sorted(changes))))
            sys.exit(1)
        print 'HARR:%s'%opts.tover
        # prepare: accumulate changes
        cbe = {} # changes by entity, ent => {oldindex => newindex}
        for ent in entities:
            cbe[ent] = range(counts[ver][ent])
            for v in sorted(changes):
                if v <= ver: continue
                if v > opts.tover: break
                for change in changes[v]:
                    ent, op, num = change
                    if op == '+':
                        cbe[ent] = [val + int(val > num) for val in cbe[ent]]
                    elif op == '-':
                        cbe[ent] = [val - int(val > num) for val in cbe[ent]]
                    else:
                        assert 0, change
        newc = counts[opts.tover]
        # convert, line by line
        def poptag(tag, line):
            if line.startswith(tag+':'):
                return line[len(tag)+1:].split(',')
            return None
        for line in fi.readlines():
            line = line.strip()
            if line.startswith('DClasses:'):
                print 'DClasses:%d'%newc['dclass']
                continue
            diff = poptag('Difficulty', line)
            if diff:
                klass, level = map(int, diff)
                print 'Difficulty:%d,%d'%(cbe['dclass'][klass], level)
                continue
            print line
