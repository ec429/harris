#!/usr/bin/python
"""extra_data - additional data on Harris entities, for the stats

This module currently provides the following data:
Per bombertype:
	Short names
	Graphing colour
Per fightertype:
	Graphing colour
	Secondary graphing colour
Per industry class:
	Name
	Graphing colour
"""

Bombers = {
	"Blenheim":   {"short":"BLEN", "colour":'0.5',    },
	"Whitley":    {"short":"WHIT", "colour":'y',      },
	"Hampden":    {"short":"HAMP", "colour":'r',      },
	"Wellington": {"short":"WLNG", "colour":'c',      },
	"Manchester": {"short":"MANC", "colour":'m',      },
	"Southampton":{"short":"SOUT", "colour":'#800000',},
	"Stirling":   {"short":"STIR", "colour":'0.5',    },
	"Elswick":    {"short":"ELSW", "colour":'#80e000',},
	"Halifax I":  {"short":"HAL1", "colour":'#806000',},
	"Lancaster I":{"short":"LAN1", "colour":'b',      },
	"Mosquito":   {"short":"MOSQ", "colour":'r',      },
	"Halifax III":{"short":"HAL3", "colour":'y',      },
	"Windsor":    {"short":"WIND", "colour":'m',      },
	"Selkirk":    {"short":"SELK", "colour":'0.5',    },
	"Lancaster X":{"short":"LANX", "colour":'c',      },
	}

Fighters = {
	"Bf109E":{"colour":'0.6', "2colour":'0.9',    },
	"Bf109G":{"colour":'0.4', "2colour":'0.75',   },
	"Fw190A":{"colour":'r',   "2colour":'#ff7f7f',},
	"Do17Z": {"colour":'y',   "2colour":'#ffff7f',},
	"Bf110": {"colour":'m',   "2colour":'#ff7fff',},
	"Ju88C": {"colour":'g',   "2colour":'#7fff7f',},
	"Do217J":{"colour":'y',   "2colour":'#ffff7f',},
	"Me262": {"colour":'b',   "2colour":'#7f7fff',},
	}

Industries = {
	0:{"name":'Ball Bearings', "colour":'0.5',    },
	1:{"name":'Petrochemical', "colour":'r',      },
	2:{"name":'Rail',          "colour":'b',      },
	3:{"name":'U-boats',       "colour":'c',      },
	4:{"name":'Armament',      "colour":'y',      },
	5:{"name":'Steel',         "colour":'m',      },
	6:{"name":'Aircraft',      "colour":'g',      },
	7:{"name":'Radar',         "colour":'#80e000',},
	8:{"name":'Mixed',         "colour":'#806000',},
	}
