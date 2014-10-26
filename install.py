#!/usr/bin/python
# coding=utf8
"""Script to install the Harris data files.

Manifest needs to be kept up to data when new resources are added.
"""

import os, sys, optparse

def parse_args(argv):
	x = optparse.OptionParser()
	x.add_option('-d', '--datidir', type='string', default="/usr/local/share/games/harris")
	opts, args = x.parse_args()
	return opts, args

manifest = {
	'art': {'bombers': {'.':[
					'blenheim.png',
					'whitley.png',
					'hampden.png',
					'wellington.png',
					'manchester.png',
					'southampton.png',
					'stirling.png',
					'elswick.png',
					'halifax i.png',
					'lancaster i.png',
					'mosquito.png',
					'halifax iii.png',
					'windsor.png',
					'selkirk.png',
					'lancaster x.png',
				]},
			'bombloads': {'.':[
					'abnormal.png',
					'arson.png',
					'halfandhalf.png',
					'illuminator.png',
					'plumduff-only.png',
					'plumduff-plus.png',
					'plumduff.png',
					'usual.png',
				]},
			'fighters': {'.':[
					'bf109e.png',
					'bf109g.png',
					'fw190a.png',
					'do17z.png',
					'bf110.png',
					'ju88c.png',
					'do217j.png',
					'me262.png',
				]},
			'filters': {'.':[
					'pff.png',
				]},
			'large': {
					'bombers': {'.':[
							'blenheim-side.png',
							'whitley-side.png',
							'hampden-side.png',
							'wellington-side.png',
							'manchester-side.png',
							'southampton-side.png',
							'stirling-side.png',
							'elswick-side.png',
							'halifax i-side.png',
							'lancaster i-side.png',
							'mosquito-side.png',
							'halifax iii-side.png',
							'windsor-side.png',
							'selkirk-side.png',
							'lancaster x-side.png',
						]},
					'fighters': {'.':[
							'bf109e-side.png',
							'bf109g-side.png',
							'fw190a-side.png',
							'do17z-side.png',
							'bf110-side.png',
							'ju88c-side.png',
							'do217j-side.png',
							'me262-side.png',
						]},
				},
			'navaids': {'.':[
					'gee.png',
					'h2s.png',
					'oboe.png',
					'g-h.png',
				]},
			'tclass': {'.':[
					'aircraft.png',
					'airfield.png',
					'arm.png',
					'bb.png',
					'bridge.png',
					'city.png',
					'leaflet.png',
					'mining.png',
					'mixed.png',
					'oil.png',
					'radar.png',
					'rail.png',
					'road.png',
					'shipping.png',
					'steel.png',
					'uboot.png',
				]},
			'.':['cross.png',
				'exit.png',
				'fullscreen.png',
				'icon.bmp',
				'intel.png',
				'location.png',
				'no-intel.png',
				'resize.png',
				'tick.png',
				'yellowhair.png',
				]
		},
	'dat': {'cities': {'.':[
					'Berlin.pbm',
					'Hamburg.pbm',
					'Frankfurt.pbm',
					'Munich.pbm',
					'Cologne.pbm',
					'Essen.pbm',
					'Dusseldorf.pbm',
					'Dortmund.pbm',
					'Bochum.pbm',
					'Duisburg.pbm',
					'Hagen.pbm',
					'Bonn.pbm',
					'Wuppertal.pbm',
					'Hamm.pbm',
					'Bremen.pbm',
					'Kiel.pbm',
					'Nuremberg.pbm',
					'Leipzig.pbm',
					'Stuttgart.pbm',
					'Hanover.pbm',
					'Dresden.pbm',
					'Magdeburg.pbm',
					'Schweinfurt.pbm',
					'Kassel.pbm',
					'Mainz.pbm',
					'Mannheim.pbm',
					'Wilhelmshaven.pbm',
					'Lübeck.pbm',
					'Rostock.pbm',
					'Peenemünde.pbm',
					'Turin.pbm',
					'Milan.pbm',
				]},
			'.':['bombers',
				'events',
				'fighters',
				'flak',
				'ftrbases',
				'intel',
				'locations',
				'starts',
				'targets',
				'texts',
				]
		},
	'map': {'.':[
			'overlay_coast.png',
			'overlay_terrain.png',
			'overlay_water.png',
		]},
	'save': {'.':[
			'qstart.sav',
			'civ.sav',
			'abd.sav',
			'ruhr.sav',
		]},
}

def quote(s):
    return "'" + s.replace("'", "'\\''") + "'"

def install_file(opts, f, dir):
	pd = os.path.join(*dir)
	dd = os.path.join(opts.datidir, pd, '')
	if not os.access(dd, os.F_OK):
		os.makedirs(dd)
	cmd = ' '.join(('install', '-m644', quote(os.path.join(pd, f)), quote(dd)))
	print "install.py:", cmd
	rc = os.system(cmd)
	if rc:
		raise Exception('Command', cmd, 'failed rc =', rc)

def install(opts, m, dir=[]):
	for d,v in m.iteritems():
		if d == '.':
			for f in v:
				install_file(opts, f, dir)
		else:
			install(opts, v, dir + [d])

if __name__ == '__main__':
	opts, args = parse_args(sys.argv)
	install(opts, manifest)
