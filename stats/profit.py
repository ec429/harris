#!/usr/bin/python3
"""profit - effectiveness tracking"""

import sys
import hhist, hdata, hsave, extra_data
import optparse

class TargetClassUnrecognised(Exception): pass
class EventMatchupError(Exception): pass
class NoLastHI(EventMatchupError): pass
class LastHIMismatch(EventMatchupError): pass
class NoLastRA(EventMatchupError): pass

london = hdata.Events.find('id', 'LONDON')
if london is None:
	raise Exception("No event LONDON found")

tcls = len(hdata.T_CLASSES)
icls = len(hdata.I_CLASSES)

# bombers = {acid: [type, earned, live, live_at_start, death_was_cr]}
# targets = {ti: [dmg, shbr_earned, losses]}
def daily_profit(d, bombers, targets, classes, start, stop, typ=None, targ_id=None): # updates bombers, targets
	# Callers don't (necessarily) provide death_was_cr, so tack it on the end
	for b in bombers.values():
		if len(b) == 4:
			b.append(False)
		else: # if any have it assume all the rest do, to save time
			break
	lasthi = None
	if stop:
		return
	berlin = (d[0] >= london['date'])
	ra = {}
	for h in d[1]:
		if h['class'] == 'A':
			acid = h['data']['acid']
			if h['data']['type']['fb'] == 'B':
				if h['data']['etyp'] == 'CT':
					bombers[acid]=[int(h['data']['type']['ti']), 0, True, True, False]
				else:
					if acid not in bombers:
						print('Warning: un-inited bomber %08x (%d)'%(acid, h['data']['type']['ti']))
						bombers[acid] = [int(h['data']['type']['ti']), 0, True, True]
					if h['data']['etyp'] in ['CR', 'SC', 'OB']:
						bombers[acid][2] = False
						bombers[acid][4] = h['data']['etyp'] == 'CR'
						if not start:
							bombers[acid][3] = False
						elif h['data']['etyp'] == 'CR':
							if acid not in ra:
								raise NoLastRA(h)
							rti = ra[acid]
							if typ is None or bombers[acid][0] == typ:
								targets[rti][2][bombers[acid][0]] += 1
					elif h['data']['etyp'] == 'RA':
						ra[acid] = h['data']['data']['target']
					elif h['data']['etyp'] == 'HI':
						ti = h['data']['data']['target']
						if acid not in ra:
							raise NoLastRA(h)
						rti = ra[acid]
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
							f = 0.6
						elif 'BRIDGE' in targ['flags']:
							f = 0.6
						elif 'ROAD' in targ['flags']:
							f = 0.6
						elif 'INDUSTRY' in targ['flags']:
							f = 0.4
						else:
							raise TargetClassUnrecognised(targ['flags'])
						if 'BERLIN' in targ['flags']:
							if 'LEAFLET' in targ['flags'] or berlin:
								f *= 2
						if 'UBOOT' in targ['flags']:
							f *= 1.2
						for ign in range(2):
							tp = classes[ign][0]
							if tp < tcls:
								if hdata.T_CLASSES[tp] in targ['flags']:
									if ign:
										f *= 0.8
									else:
										f *= 1.2
							ip = classes[ign][1]
							if ip < icls:
								if hdata.I_CLASSES[ip] in targ['flags']:
									if ign:
										f *= 0.8
									else:
										f *= 1.2
						if targets[ti][0]: # not 100% accurate but close enough
							if start and (typ is None or bombers[acid][0] == typ):
								if targ_id is None or rti == targ_id:
									bombers[acid][1] += h['data']['data']['bombs'] * f
								targets[rti][1] += h['data']['data']['bombs'] * f
		elif h['class'] == 'T':
			ti = h['data']['target']
			targ = hdata.Targets[ti]
			if h['data']['etyp'] == 'SH':
				if lasthi is None:
					raise NoLastHI(h)
				if lasthi[0] != targ:
					raise LastHIMismatch(lasthi, h)
				if acid not in ra:
					raise NoLastRA(h)
				rti = ra[acid]
				if start and (typ is None or bombers[lasthi[1]][0] == typ):
					sf = 1.0
					if 'UBOOT' in targ['flags']:
						sf *= 1.2
					if targ_id is None or rti == targ_id:
						bombers[lasthi[1]][1] += 15000 * sf
					targets[rti][1] += 15000 * sf
			elif h['data']['etyp'] == 'DM':
				targets[ti][0]=h['data']['data']['cdmg']
				if 'BRIDGE' in targ['flags']:
					if lasthi is None:
						raise NoLastHI(h)
					if lasthi[0] != targ:
						raise LastHIMismatch(lasthi, h)
					if acid not in ra:
						raise NoLastRA(h)
					rti = ra[acid]
					if start and (typ is None or bombers[lasthi[1]][0] == typ):
						if targ_id is None or rti == targ_id:
							bombers[lasthi[1]][1] += 500*h['data']['data']['ddmg']
						targets[rti][1] += 500*h['data']['data']['ddmg']
		elif h['class'] == 'M':
			if h['data']['etyp'] == 'TP':
				classes[0][h['data']['data']['ignore']] = h['data']['data']['tclass']
			elif h['data']['etyp'] == 'IP':
				classes[1][h['data']['data']['ignore']] = h['data']['data']['iclass']

def extract_profit(save, before=None, after=None, targ_id=None):
	bombers = {b['id']:[b['type'], 0, True, True] for b in save.init.bombers}
	targets = [[t['dmg'], 0, {i:0 for i in range(save.ntypes)}] for t in save.init.targets]
	classes = [[tcls,tcls], [icls,icls]]
	days = sorted(hhist.group_by_date(save.history))
	for d in days:
		daily_profit(d, bombers, targets, classes, d[0]>=after if after else True, d[0]>=before if before else False, targ_id=targ_id)
	bombers = {i:bombers[i] for i in bombers if bombers[i][3]}
	results = {i: {k:v for k,v in bombers.items() if v[0] == i} for i in range(save.ntypes)}
	full = {i: (len(results[i]), sum(v[1] for v in results[i].values())) for i in results}
	deadresults = {i: {k:v for k,v in results[i].items() if not v[2]} for i in results}
	dead = {i: (len(deadresults[i]), sum(v[1] for v in deadresults[i].values())) for i in results}
	if targ_id is None:
		return {i: {'full':full[i], 'fullr':full[i][1]/full[i][0] if full[i][0] else 0,
					'dead':dead[i], 'deadr':dead[i][1]/dead[i][0] if dead[i][0] else 0,
					'opti':full[i][1]/dead[i][0] if dead[i][0] else 0}
				for i in results
				if save.prio[i] is not None or full[i][0]}
	else:
		return {i: {'full':full[i], 'fullr':full[i][1]/full[i][0] if full[i][0] else 0,
					'dead':dead[i], 'deadr':dead[i][1]/targets[targ_id][2][i] if targets[targ_id][2][i] else 0,
					'opti':full[i][1]/targets[targ_id][2][i] if targets[targ_id][2][i] else 0}
				for i in results
				if save.prio[i] is not None or full[i][0]}

def extract_targ_profit(save, before=None, after=None, typ=None):
	bombers = {b['id']:[b['type'], 0, True, True] for b in save.init.bombers}
	targets = [[t['dmg'], 0, dict((i,0) for i in range(save.ntypes))] for t in save.init.targets]
	days = sorted(hhist.group_by_date(save.history))
	classes = [[tcls,tcls], [icls,icls]]
	for d in days:
		daily_profit(d, bombers, targets, classes, d[0]>=after if after else True, d[0]>=before if before else False, typ=typ)
	gains, losses = zip(*targets)[1:]
	lossvalue = [sum(l*hdata.Bombers[i]['cost'] for i,l in loss.items()) for loss in losses]
	return {i: {'gain': gains[i], 'loss': losses[i], 'cost': lossvalue[i]} for i in range(save.init.ntargets)}

def parse_args(argv):
	x = optparse.OptionParser()
	x.add_option('-a', '--after', type='string')
	x.add_option('-b', '--before', type='string')
	x.add_option('-t', '--by-target', action='store_true')
	x.add_option('--type', type='string')
	x.add_option('--targ', type='string')
	x.add_option('--file', type='string')
	opts, args = x.parse_args()
	if opts.type:
		if not opts.by_target:
			x.error("--type requires --by-target")
		try:
			opts.type=[b['name'] for b in hdata.Bombers].index(opts.type)
		except ValueError:
			try:
				opts.type=int(opts.type)
			except ValueError:
				x.error("No such type", opts.type)
	if opts.targ:
		try:
			opts.targ=[t['name'] for t in hdata.Targets].index(opts.targ)
		except ValueError:
			try:
				opts.targ=int(opts.targ)
			except ValueError:
				x.error("No such targ", opts.targ)
	return opts, args

if __name__ == '__main__':
	opts, args = parse_args(sys.argv)
	before = hhist.date.parse(opts.before) if opts.before else None
	after = hhist.date.parse(opts.after) if opts.after else None
	f = sys.stdin
	if opts.file:
		f = open(opts.file, 'r')
	save = hsave.Save.parse(f)
	if opts.by_target:
		profit = extract_targ_profit(save, before, after, typ=opts.type)
		for i in profit:
			name = hdata.Targets[i]['name'].ljust(40)
			gain = profit[i]['gain']
			loss = profit[i]['cost']
			if loss:
				ratio = '%.4g'%(gain * 1.0 / loss)
			else:
				if not gain: continue
				ratio = '--'
			print("%s: %9d / %9d = %s" % (name, gain, loss, ratio))
	else:
		profit = extract_profit(save, before, after, targ_id=opts.targ)
		for i in profit:
			name = extra_data.Bombers[hdata.Bombers[i]['name']]['short']
			full = "(%d) = %g" % (profit[i]['full'][0], profit[i]['fullr'])
			dead = "(%d) = %g" % (profit[i]['dead'][0], profit[i]['deadr'])
			opti = "%g" % (profit[i]['opti'])
			print("%s: all%s, dead%s, optimistic %s, cost %d" % (name, full, dead, opti, hdata.Bombers[i]['cost']))
