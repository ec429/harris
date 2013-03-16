#!/usr/bin/python
"""hsave - Harris savefiles

The hsave module reads Harris savefiles, including initfiles
"""

import sys, os
import hhist
import hdata

class BadSaveLine(Exception): pass
class UnrecognisedTag(BadSaveLine): pass
class UnrecognisedSubtag(BadSaveLine): pass
class SubtagOutOfOrder(BadSaveLine): pass

class IntegrityError(Exception): pass
class UnknownHistClass(IntegrityError): pass
class UnknownHistEvent(IntegrityError): pass
class CannotFindAcid(IntegrityError): pass

class Save(object):
	def __init__(self): pass
	@classmethod
	def parse(cls, f, read_init=True, check_integrity=False):
		self = cls()
		self.read_init = read_init
		stage = None
		nosplit = False
		for line in f:
			if nosplit:
				tag = None
				rest = line.rstrip('\n')
			else:
				tag, rest = line.rstrip('\n').split(':', 1)
			if stage:
				if stage(tag, rest):
					stage = None
					nosplit = False
			else:
				stage, nosplit = self.handle(tag, rest)
		if check_integrity:
			self.check_integrity()
		return self
	def clone(self):
		new = self.__class__()
		for a in self.__dict__:
			o = self.__dict__[a]
			if isinstance(o, hhist.date):
				n = o.copy()
			else:
				n = type(o)(o)
			new.__setattr__(a, n)
		return new
	def __cmp__(self, other):
		d = dict(self.__dict__)
		d.update(other.__dict__)
		for k in d.keys():
			s = self.__dict__[k]
			o = other.__dict__[k]
			if s != o:
				return cmp(s, o)
	def check_integrity(self):
		"""Not finished - always fails."""
		replay = self.init.clone()
		for h in self.history:
			replay.hist_replay(h)
		if self != replay:
			raise IntegrityError('Final state mismatch')
	def hist_replay(self, h):
		replayers = {'I':self.I_replay, 'A':self.A_replay}
		if h['class'] in replayers:
			replayer = replayers[h['class']]
			replayer(h['data'])
		else:
			raise UnknownHistClass(h['class'])
	def I_replay(self, data):
		if data['event'] != 'INIT':
			raise UnknownHistEvent('I', data['event'])
	def A_replay(self, data):
		def find_acid(ac_list, acid):
			for i,ac in enumerate(ac_list):
				if ac['id'] == acid: return i
			return None
		def CT_replay(self, data):
			if data['type']['fb'] == 'F':
				self.nfighters += 1
				self.fighters.append({'type':data['type']['ti'], 'id':data['acid']})
			elif data['type']['fb'] == 'B':
				self.nbombers += 1
				self.bombers.append({'type':data['type']['ti'], 'fail':False, 'nav':0, 'pff':False, 'id':data['acid']})
			else:
				raise hhist.NoSuchAcType(data['type']['fb'])
		def FA_replay(self, data):
			b = find_acid(self.bombers, data['acid'])
			if not b:
				raise CannotFindAcid(data['acid'])
			self.bombers[b]['fail'] = data['data']['failed']
		replayers = {'CT':CT_replay, 'RA':None, 'FA':FA_replay, 'DM':DM_replay}
		if data['etyp'] in replayers:
			replayer = replayers[data['etyp']]
			if replayer: replayer(self, data)
		else:
			raise UnknownHistEvent('A', data['etyp'])
	def handle(self, tag, rest):
		if tag == 'HARR':
			self.ver = map(int, rest.split('.'))
			return None, False
		if tag == 'DATE':
			self.date = hhist.date.parse(rest)
			return None, False
		if tag == 'Confid':
			self.confid = float.fromhex(rest)
			return None, False
		if tag == 'Morale':
			self.morale = float.fromhex(rest)
			return None, False
		if tag == 'Budget':
			self.cash, self.cshr = map(int, rest.split('+', 1))
			return None, False
		if tag == 'Types':
			self.ntypes = int(rest)
			self.prio = [0]*self.ntypes
			self._type = 0
			return self.Types, False
		if tag == 'Navaids':
			self.nnav = int(rest)
			self.npri = [0]*self.nnav
			self._nav = 0
			return self.Navaids, False
		if tag == 'Bombers':
			self.nbombers = int(rest)
			self.bombers = []
			return self.Bombers, False
		if tag == 'GProd':
			self.gprod = float.fromhex(rest)
			return None, False
		if tag == 'FTypes':
			self.nftypes = int(rest)
			return None, False
		if tag == 'FBases':
			self.nfbases = int(rest)
			return None, False
		if tag == 'Fighters':
			self.nfighters = int(rest)
			self.fighters = []
			return self.Fighters, False
		if tag == 'Targets':
			self.ntargets = int(rest)
			self.targets = []
			return self.Targets, False
		if tag == 'Targets init':
			self.ntargets = len(hdata.Targets)
			dmg, flk, heat = rest.split(',', 2)
			self.targets = [dict({'dmg':float.fromhex(dmg), 'flk':float.fromhex(flk), 'heat':float.fromhex(heat)}) for i in xrange(self.ntargets)]
			return None, False
		if tag == 'Weather state':
			self._wline = 0
			return self.Wstate, True
		if tag == 'Weather rand':
			return None, False
		if tag == 'Messages':
			self.nmessages = int(rest)
			self.messages = ['']
			return self.Messages, True
		if tag == 'History':
			self.nhistory = int(rest)
			self.history = []
			return self.History, True
		raise UnrecognisedTag(tag, rest)
	def Types(self, tag, rest):
		if tag.startswith('Prio '):
			typ = int(tag[5:])
			if typ != self._type:
				raise SubtagOutOfOrder('Types', self._type, tag, rest)
			prio, ignore, ignore, ignore = rest.split(',', 3)
			self.prio[typ] = int(prio)
			self._type += 1
			return self._type == self.ntypes
		raise UnrecognisedSubtag('Types', tag, rest)
	def Navaids(self, tag, rest):
		if tag.startswith('NPrio '):
			nav = int(tag[6:])
			if nav != self._nav:
				raise SubtagOutOfOrder('Navaids', self._nav, tag, rest)
			nprio, ignore = rest.split(',', 1)
			self.npri[nav] = int(nprio)
			self._nav += 1
			return self._nav == self.nnav
		raise UnrecognisedSubtag('Navaids', tag, rest)
	def Bombers(self, tag, rest):
		if tag.startswith('Type '):
			typ = int(tag[5:])
			fail, nav, pff, acid = rest.split(',', 3)
			self.bombers.append({'type':int(typ), 'fail':bool(fail), 'nav':int(nav), 'pff':bool(pff), 'id':int(acid, 16)})
			return len(self.bombers) == self.nbombers
		raise UnrecognisedSubtag('Bombers', tag, rest)
	def Fighters(self, tag, rest):
		if tag.startswith('Type '):
			typ = int(tag[5:])
			base, acid = rest.split(',', 1)
			self.fighters.append({'type':int(typ), 'id':int(acid, 16)})
			return len(self.fighters) == self.nfighters
		raise UnrecognisedSubtag('Fighters', tag, rest)
	def Targets(self, tag, rest):
		if tag.startswith('Targ '):
			targ = int(tag[5:])
			dmg, flk, heat = rest.split(',', 2)
			while len(self.targets) < targ:
				self.targets.append({'dmg':100, 'flk':100, 'heat':0})
			self.targets.append({'dmg':float.fromhex(dmg), 'flk':float.fromhex(flk), 'heat':float.fromhex(heat)})
			return len(self.targets) == self.ntargets
		raise UnrecognisedSubtag('Targets', tag, rest)
	def Wstate(self, tag, rest):
		self._wline += 1
		return self._wline == 256
	def Messages(self, tag, rest):
		if rest == '.':
			if len(self.messages) == self.nmessages: return True
			self.messages.append('')
			return False
		self.messages[-1] += rest
		return False
	def History(self, tag, rest):
		hist = hhist.raw_parse(rest)
		if self.read_init:
			if hist['class'] == 'I':
				if hist['data']['event'] == 'INIT':
					self.init = Save.parse(open(os.path.join('save', hist['data']['filename'])), read_init=False)
		self.history.append(hist)
		return len(self.history) == self.nhistory

if __name__ == '__main__':
	save = Save.parse(sys.stdin)
	print 'Parsed save OK'#; replaying event log...'
	#save.check_integrity()
	#print 'Integrity OK'
