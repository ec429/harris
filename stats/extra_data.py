#!/usr/bin/python
"""extra_data - additional data on Harris entities, for the stats

This module currently provides the following data:
Per bombertype:
	Crew counts
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
	"Blenheim":   {"crew":3, "short":"BLEN", "colour":'0.5',    },
	"Whitley":    {"crew":5, "short":"WHIT", "colour":'y',      },
	"Hampden":    {"crew":4, "short":"HAMP", "colour":'r',      },
	"Wellington": {"crew":6, "short":"WLNG", "colour":'c',      },
	"Manchester": {"crew":7, "short":"MANC", "colour":'m',      },
	"Stirling":   {"crew":7, "short":"STIR", "colour":'0.5',    },
	"Halifax I":  {"crew":7, "short":"HAL1", "colour":'#806000',},
	"Lancaster I":{"crew":7, "short":"LAN1", "colour":'b',      },
	"Mosquito":   {"crew":2, "short":"MOSQ", "colour":'r',      },
	"Halifax III":{"crew":7, "short":"HAL3", "colour":'y',      },
	"Lancaster X":{"crew":7, "short":"LANX", "colour":'c',      },
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
