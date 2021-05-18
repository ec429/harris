#!/usr/bin/python2

import sys, zlib, struct, random, math

acids = set()
cmids = set()

def multiply(line):
    if len(line) > 1:
        if line[0] == '*' and line[1].isdigit():
            l = line.find('*', 2)
            if l > 1:
                n = int(line[1:l])
                return n*[line[l+1:]]
        elif line[0] == '%' and line[1].isdigit():
            l = line.find('%', 2)
            if l > 1:
                n = int(line[1:l])
                return [line[l+1:] % i for i in xrange(n)]
    return [line]

def poisson(lamb):
    if lamb <= 0:
        return 0
    l = math.exp(-lamb)
    k = 0
    p = 1
    while p > l:
        k += 1
        p *= random.random()
    return k - 1

def genids(line, i, inb):
    if ',NOID' in line:
        z = '_'.join((salt, str(i), line))
        ha = zlib.crc32(z) & 0xffffffff
        random.seed(ha)
        h = hex(ha)[2:].rstrip('L')
        if line.startswith("Type "):
            if h in acids:
                raise ValueError("Duplicate ac", line, "idgen", h)
            acids.add(h)
            if inb and len(line.split(',')) == 8:
                typf, nav, wear, acid, mark, train, sqn, flt = line.split(',')
                wear = str(poisson(int(wear)))
                line = ','.join((typf, nav, wear, acid, mark, train, sqn, flt))
        elif line.startswith("CG ") or line.startswith("CG2 "):
            if h in cmids:
                raise ValueError("Duplicate cm", line, "idgen", h)
            cmids.add(h)
        else:
            raise ValueError("genids for", line)
        return line.replace('NOID', h.zfill(8))
    else:
        return line

def gencrews(line, i, cgv=1):
    words = line.split(':')
    _, stat, cls = words[0].split()
    words = words[1].split(',')
    ms = int(words[0])
    if cgv > 1:
        mheavy = int(words[1])
        mlanc = int(words[2])
        words = words[2:]
    else:
        mheavy = 0
        mlanc = 0
    ml = int(words[1])
    tops = int(words[2])
    ft = int(words[3])
    assi = int(words[4])
    more = words[5:-1]
    more.append('')
    more = ','.join(more)
    acid = words[-1]
    z = '_'.join((salt, str(i), line))
    ha = zlib.crc32(z) & 0xffffffff
    random.seed(ha)
    skill = poisson(ms)
    heavy = poisson(mheavy)
    lanc = poisson(mlanc)
    lrate = poisson(ml)
    tops = random.randint(0, tops)
    if cgv <= 1 and stat == 'Student':
        # Account for students' HCU/LFS training
        if ft==1:
            heavy = tops * 3
        elif ft==2:
            lanc = tops * 9
            heavy = 100
    if windows:
        return "%s %c:%s,%s,%s,%u,%u,%u,%d,%s00000000%s"%(stat, cls, float_to_hex(skill), float_to_hex(heavy), float_to_hex(lanc), lrate, tops, ft, assi, more, acid)
    else:
        return "%s %c:%u,%u,%u,%u,%u,%u,%d,%s00000000%s"%(stat, cls, skill, heavy, lanc, lrate, tops, ft, assi, more, acid)

windows = '--windows' in sys.argv

salt = ''
if '--salt' in sys.argv:
    sarg = sys.argv.index('--salt')
    try:
        salt = sys.argv[sarg+1]
    except IndexError:
        sys.stderr.write('--salt requires argument!\n')
        sys.exit(1)

def float_to_hex(value):
    f = float(value)
    bytes = struct.pack('>d', f)
    i, = struct.unpack('>q', bytes)
    return '%016x'%i

inb = False
for line in sys.stdin.readlines():
    if line.startswith("Bombers:"):
        inb = True
    elif line.startswith("Fighters:"):
        inb = False
    lines = multiply(line)
    for i,line in enumerate(lines):
        line = genids(line, i, inb)
        if line.startswith("CG "):
            line = gencrews(line, i, 1)
        elif line.startswith("CG2 "):
            line = gencrews(line, i, 2)
        if windows and ':' in line:
            to_conv = {'Confid':(0,),
                       'Morale':(0,),
                       'IClass':(0,),
                       'Targ':(0,1,2,3)}
            tag, values = line.rstrip('\n').split(':', 1)
            starttag, _, resttag = tag.partition(' ')
            values = values.split(',')
            if starttag in to_conv:
                for pos in to_conv[starttag]:
                    values[pos] = float_to_hex(values[pos])
                line = ':'.join((tag, ','.join(values))) + '\n'
        sys.stdout.write(line)
