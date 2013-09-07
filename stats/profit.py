#!/usr/bin/python
"""profit - effectiveness tracking"""

import sys
import hhist, hdata, hsave, extra_data
import optparse

class TargetClassUnrecognised(Exception): pass
class EventMatchupError(Exception): pass
class NoLastHI(EventMatchupError): pass
class LastHIMismatch(EventMatchupError): pass

def daily_profit(d, bombers, targets, count): # updates bombers, targets
	lasthi = None
	for h in d[1]:
		if h['class'] == 'A':
			acid = h['data']['acid']
			if h['data']['type']['fb'] == 'B':
				if h['data']['etyp'] == 'CT':
					bombers[acid]=[int(h['data']['type']['ti']), 0, True]
				elif h['data']['etyp'] in ['CR', 'OB']:
					if count:
						bombers[acid][2] = False
					else:
						del bombers[acid]
				elif h['data']['etyp'] == 'HI':
					ti = h['data']['data']['target']
					targ = hdata.Targets[ti]
					lasthi = (targ, acid)
					if 'CITY' in targ['flags']:
						f = 0.2
					elif 'SHIPPING' in targ['flags']:
						f = 0
					elif 'MINING' in targ['flags']:
						f = 0.03
					elif 'LEAFLET' in targ['flags']:
						f = 0.015
					elif 'AIRFIELD' in targ['flags']:
						f = 0.2
					elif 'BRIDGE' in targ['flags']:
						f = 0.2
					elif 'ROAD' in targ['flags']:
						f = 0.2
					elif 'INDUSTRY' in targ['flags']:
						f = 0.4
					else:
						raise TargetClassUnrecognised(targ['flags'])
					if 'BERLIN' in targ['flags']:
						f *= 2
					if targets[ti]: # not 100% accurate but close enough
						if count:
							bombers[acid][1] += h['data']['data']['bombs'] * f
		elif h['class'] == 'T':
			ti = h['data']['target']
			targ = hdata.Targets[ti]
			if h['data']['etyp'] == 'SH':
				if lasthi is None:
					raise NoLastHI(h)
				if lasthi[0] != targ:
					raise LastHIMismatch(lasthi, h)
				if count:
					bombers[lasthi[1]][1] += 15000
			elif h['data']['etyp'] == 'DM':
				targets[ti]=h['data']['data']['cdmg']
				if 'BRIDGE' in targ['flags']:
					if lasthi is None:
						raise NoLastHI(h)
					if lasthi[0] != targ:
						raise LastHIMismatch(lasthi, h)
					if count:
						bombers[lasthi[1]][1] += 500*h['data']['data']['ddmg']

def extract_profit(save, after=None):
	bombers = {b['id']:[b['type'], 0, True] for b in save.init.bombers}
	targets = [t['dmg'] for t in save.init.targets]
	days = sorted(hhist.group_by_date(save.history))
	for d in days:
		daily_profit(d, bombers, targets, d[0]>=after if after else True)
	results = {i: {k:v for k,v in bombers.iteritems() if v[0] == i} for i in xrange(save.ntypes)}
	full = {i: (len(results[i]), sum(v[1] for v in results[i].itervalues())) for i in results}
	deadresults = {i: {k:v for k,v in results[i].iteritems() if not v[2]} for i in results}
	dead = {i: (len(deadresults[i]), sum(v[1] for v in deadresults[i].itervalues())) for i in results}
	return {i: {'full':full[i], 'fullr':full[i][1]/full[i][0] if full[i][0] else 0,
				'dead':dead[i], 'deadr':dead[i][1]/dead[i][0] if dead[i][0] else 0,
				'opti':full[i][1]/dead[i][0] if dead[i][0] else 0}
			for i in results}

def parse_args(argv):
	x = optparse.OptionParser()
	x.add_option('-a', '--after', type='string')
	return x.parse_args()

if __name__ == '__main__':
	opts, args = parse_args(sys.argv)
	after = hhist.date.parse(opts.after) if opts.after else None
	save = hsave.Save.parse(sys.stdin)
	profit = extract_profit(save, after)
	for i in profit:
		name = extra_data.Bombers[hdata.Bombers[i]['name']]['short']
		full = "(%d) = %g" % (profit[i]['full'][0], profit[i]['fullr'])
		dead = "(%d) = %g" % (profit[i]['dead'][0], profit[i]['deadr'])
		opti = "%g" % (profit[i]['opti'])
		print "%s: all%s, dead%s, optimistic %s, cost %d" % (name, full, dead, opti, hdata.Bombers[i]['cost'])
