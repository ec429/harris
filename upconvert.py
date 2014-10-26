#!/usr/bin/python

import optparse, sys

entities = ['dclass', 'type']

changes = {'0.2.1':[('dclass', '*', 8), # first version supported by upconvert
                    ('type', '*', 11)],
           '0.2.2':[], # no relevant changes
           '0.2.3':[('type', '+', 5), # introduced the xbombers
                    ('type', '+', 7),
                    ('type', '+', 12),
                    ('type', '+', 13)],
           '0.2.4':[], # no relevant changes
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
                    cbe[ent] = [val + int(val >= num) for val in cbe[ent]]
                elif op == '-':
                    cbe[ent] = [val - int(val >= num) for val in cbe[ent]]
                else:
                    assert 0, change
        newc = counts[opts.tover]
        # convert, line by line
        def poptag(tag, line):
            if line.startswith(tag+':'):
                return line[len(tag)+1:].split(',')
            return None
        def popntag(tag, line):
            if not line.startswith(tag+' '):
                return None
            text = line[len(tag)+1:]
            if ':' in text:
                tn, tr = text.split(':', 1)
            else:
                tn = text
                tr = None
            try:
                n = int(tn)
            except ValueError:
                return None
            return n, tr
        cb = 0
        cf = 0
        ch = 0
        for line in fi.readlines():
            line = line[:-1] # strip trailing \n
            if ch:
                # <date> <time> <class> <data>
                date, time, klass, data = line.split(' ', 3)
                if klass == 'A':
                    # <data> ::= <ac-uid> <b-or-f><type:int> <a-event>
                    acid, bft, data = data.split(' ', 2)
                    if bft[0] == 'B':
                        typ = int(bft[1:])
                        print ' '.join((date, time, klass, acid, 'B%d'%cbe['type'][typ], data))
                        continue
                ch -= 1
            borf = popntag('Type', line)
            if borf:
                if cb: # Type <btype>:<failed>,<navs>,<pff>,<acid> // bomber
                    typ = int(borf[0])
                    print 'Type %d:%s'%(cbe['type'][typ], borf[1])
                    cb -= 1
                elif cf: # Type <ftype>:<base>,<radar>,<acid> // fighter
                    print line
                    cf -= 1
                else: # neither bomber nor fighter expected
                    raise Exception("Unexpected 'Type' tag in line", line)
                continue
            elif cb:
                raise Exception("Expected bomber ('Type' tag) but got line", line)
            elif cf:
                raise Exception("Expected fighter ('Type' tag) but got line", line)
            if line.startswith('DClasses:'):
                print 'DClasses:%d'%newc['dclass']
                continue
            diff = poptag('Difficulty', line)
            if diff:
                klass, level = map(int, diff)
                print 'Difficulty:%d,%d'%(cbe['dclass'][klass], level)
                continue
            if line.startswith('Types:'):
                print 'Types:%d'%newc['type']
                continue
            prio = popntag('Prio', line)
            if prio:
                typ = int(prio[0])
                for i in range(cbe['type'][typ-1]+1 if typ else 0, cbe['type'][typ]):
                    print 'NoType %d'%i
                print 'Prio %d:%s'%(cbe['type'][typ], prio[1])
                continue
            notype = popntag('NoType', line)
            if notype:
                typ = int(notype[0])
                for i in range(cbe['type'][typ-1]+1 if typ else 0, cbe['type'][typ]):
                    print 'NoType %d:'%i
                print 'NoType %d:'%cbe['type'][typ]
                continue
            nbombers = poptag('Bombers', line)
            if nbombers:
                cb = int(nbombers[0])
            nfighters = poptag('Fighters', line)
            if nfighters:
                cf = int(nfighters[0])
            history = poptag('History', line)
            if history:
                ch = int(history[0])
            print line
