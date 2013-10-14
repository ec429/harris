#!/usr/bin/python
"""extra_data - additional data on Harris entities, for the stats

This module currently provides the following data:
Per bombertype:
	Crew counts
	Short names
	Graphing colour
Per fightertype:
	Graphing colour
"""

Bombers = {
	"Blenheim":   {"crew":3, "short":"BLEN", "colour":'0.5',},
	"Whitley":    {"crew":5, "short":"WHIT", "colour":'y',},
	"Hampden":    {"crew":4, "short":"HAMP", "colour":'r',},
	"Wellington": {"crew":6, "short":"WLNG", "colour":'c',},
	"Manchester": {"crew":7, "short":"MANC", "colour":'m',},
	"Stirling":   {"crew":7, "short":"STIR", "colour":'0.5',},
	"Halifax I":  {"crew":7, "short":"HAL1", "colour":'#806000',},
	"Lancaster I":{"crew":7, "short":"LAN1", "colour":'b',},
	"Mosquito":   {"crew":2, "short":"MOSQ", "colour":'r',},
	"Halifax III":{"crew":7, "short":"HAL3", "colour":'y',},
	"Lancaster X":{"crew":7, "short":"LANX", "colour":'c',},
	}

Fighters = {
	"Bf109E":{"colour":'0.6',},
	"Bf109G":{"colour":'0.4',},
	"Fw190A":{"colour":'r',},
	"Do17Z": {"colour":'y',},
	"Bf110": {"colour":'m',},
	"Ju88C": {"colour":'g',},
	"Do217J":{"colour":'y',},
	"Me262": {"colour":'b',},
	}
