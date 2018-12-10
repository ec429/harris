#!/usr/bin/python2
"""crewskills - graph of crew skill distribution against time

Requires matplotlib, see http://matplotlib.org or search your package
manager (Debian: apt-get install python-matplotlib)
"""

import os, sys, math
import hhist, hsave
import matplotlib.pyplot as plt
import numpy as np
import optparse

def parse_args():
	x = optparse.OptionParser()
	x.add_option('--classes', type='string', default='PNBWEG')
	x.add_option('--split-classes', action='store_true')
	x.add_option('--log', action='store_true')
	opts, args = x.parse_args()
	return opts, args

def histogram(opts, values):
    buckets = [0] * 121
    for v in values:
        bi = min(int(v), 120)
        buckets[bi] += 1
    if opts.log:
        return [math.log(b + 1.0) for b in buckets]
    return buckets

def ops_from_crews(crews, students):
    return [crews[c] for c in crews if c not in students]

def extract_ops(opts, entries, initsave):
    crews = dict((c['id'], (c['tops'] if c['status'] == 'Crewman' else 0) + 30 * c['ft']) for c in initsave.crews if c['type'] in opts.classes)
    students = set([c['id'] for c in initsave.crews if c['status'] == 'Student'])
    days = sorted(hhist.group_by_date(entries))
    first = days[0][0].prev()
    results = {first: histogram(opts, ops_from_crews(crews, students))}
    for day, ents in days:
        for ent in ents:
            if ent['class'] != 'C':
                continue
            ent = ent['data']
            if ent['class'] not in opts.classes:
                continue
            cmid = ent['cmid']
            if ent['etyp'] == 'GE':
                students.add(cmid)
            elif ent['etyp'] == 'ST':
                if cmid in students:
                    crews[cmid] = 0
                    students.remove(cmid)
            elif ent['etyp'] == 'OP':
                crews[cmid] += 1
            elif ent['etyp'] in ['DE', 'PW']:
                if cmid in crews: del crews[cmid]
        results[day] = histogram(opts, ops_from_crews(crews, students))
    return results

def graph_ops(fig, ops, n, i):
    top = max(max(day) for day in ops.values())
    ax = fig.add_subplot(1,n,i + 1)
    dates = sorted(ops)
    plt.axis(ymax=120.5, ymin=0)
    data = np.array([ops[d] for d in dates]).transpose()
    plt.imshow(data, vmin=0, vmax=top, aspect='auto')
    ticks = dates[::7]
    ax.set_xticks([i*7 for i,_ in enumerate(ticks)])
    ax.set_xticklabels(ticks)
    ax.minorticks_off()
    plt.setp(ax.get_xticklabels(), rotation=45, ha="right", rotation_mode="anchor")

def extract_all_ops(opts):
    entries = hhist.import_from_save(sys.stdin, crew_hist=True)
    init = entries.pop(0)
    assert init['class'] == 'I', init
    assert init['data']['event'] == 'INIT', init
    initfile = os.path.join('save', init['data']['filename'])
    initsave = hsave.Save.parse(open(initfile, 'r'))
    return extract_ops(opts, entries, initsave)

if __name__ == '__main__':
    opts, args = parse_args()
    entries = hhist.import_from_save(sys.stdin, crew_hist=True)
    init = entries.pop(0)
    assert init['class'] == 'I', init
    assert init['data']['event'] == 'INIT', init
    initfile = os.path.join('save', init['data']['filename'])
    initsave = hsave.Save.parse(open(initfile, 'r'))
    fig = plt.figure()
    if opts.split_classes:
        classes = opts.classes
        n = len(classes)
        for i,cls in enumerate(classes):
            opts.classes = cls
            ops = extract_ops(opts, entries, initsave)
            graph_ops(fig, ops, n, i)
    else:
        ops = extract_ops(opts, entries, initsave)
        graph_ops(fig, ops, 1, 0)
    plt.show()
