#pragma once
/*
	harris - a strategy game
	Copyright (C) 2012-2013 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	types: data type definitions
*/

#include <atg.h>

#define NNAVAIDS	4
#define MAXMSGS		8

#define HIST_LINE	240

typedef struct
{
	size_t nents;
	size_t nalloc;
	char (*ents)[HIST_LINE];
}
history;

typedef struct
{
	int year;
	unsigned int month; // 1-based
	unsigned int day; // also 1-based
}
date;

typedef struct
{
	unsigned int hour;
	unsigned int minute;
}
time;

typedef uint32_t acid; // a/c ID

typedef struct
{
	char *buf; // string data buffer (may contain embedded NULs)
	size_t l; // size of RAM allocation
	size_t i; // length of string data
	// invariant: i<l
}
string;

typedef struct _intel
{
	char *ident;
	char *text;
	struct _intel *next;
}
intel;

typedef struct
{
	double push, slant;
	double p[256][128];
	double t[256][128];
}
w_state;

typedef enum
{
	BL_ABNORMAL,
	BL_PPLUS,
	BL_PLUMDUFF,
	BL_PONLY,
	BL_USUAL,
	BL_ARSON,
	BL_ILLUM,
	NBOMBLOADS
}
bombload;

extern struct bombloadinfo
{
	const char *name;
	const char *fn;
	SDL_Surface *pic;
}
bombloads[NBOMBLOADS];

typedef struct
{
	//MANUFACTURER:NAME:COST:SPEED:CEILING:CAPACITY:SVP:DEFENCE:FAILURE:ACCURACY:RANGE:BLAT:BLONG:DD-MM-YYYY:DD-MM-YYYY:NAVAIDS,FLAGS,BOMBLOADS
	char * manu;
	char * name;
	unsigned int cost;
	unsigned int speed;
	unsigned int alt;
	unsigned int cap;
	unsigned int svp;
	unsigned int defn;
	unsigned int fail;
	unsigned int accu;
	unsigned int range;
	date entry;
	date novelty;
	date exit;
	bool nav[NNAVAIDS];
	bool load[NBOMBLOADS];
	bool noarm, pff, heavy, inc, broughton;
	unsigned int blat, blon;
	SDL_Surface *picture;
	char *text, *newtext;
	
	unsigned int count;
	unsigned int navcount[NNAVAIDS];
	unsigned int pffcount;
	unsigned int prio;
	unsigned int pribuf;
	atg_element *prio_selector;
	unsigned int pc;
	unsigned int pcbuf;
}
bombertype;

#define fuelcap(t)	types[t].range*180.0/(double)(types[t].speed)

typedef struct
{
	//MANUFACTURER:NAME:COST:SPEED:ARMAMENT:MNV:DD-MM-YYYY:DD-MM-YYYY:FLAGS
	char *manu;
	char *name;
	unsigned int cost;
	unsigned int speed;
	unsigned char arm;
	unsigned char mnv;
	date entry;
	date exit;
	bool night;
	char *text, *newtext;
}
fightertype;

typedef struct
{
	//LAT:LONG:ENTRY:EXIT
	unsigned int lat, lon;
	date entry, exit;
}
ftrbase;

enum t_class {TCLASS_CITY,TCLASS_SHIPPING,TCLASS_MINING,TCLASS_LEAFLET,TCLASS_AIRFIELD,TCLASS_BRIDGE,TCLASS_ROAD,TCLASS_INDUSTRY,};
enum i_class {ICLASS_BB, ICLASS_OIL, ICLASS_RAIL, ICLASS_UBOOT, ICLASS_ARM, ICLASS_STEEL, ICLASS_AC, ICLASS_MIXED,};

typedef struct
{
	//NAME:PROD:FLAK:ESIZ:LAT:LONG:DD-MM-YYYY:DD-MM-YYYY:CLASS
	char *name;
	intel *p_intel;
	unsigned int prod, flak, esiz, lat, lon;
	date entry, exit;
	enum t_class class;
	enum i_class iclass;
	bool berlin; // raids on Berlin are more valuable
	bool flammable; // more easily damaged by incendiaries
	SDL_Surface *picture;
	unsigned int psiz; // 'physical' size (cities only) - #pixels in picture
	/* for Type I fighter control */
	double threat; // German-assessed threat level
	unsigned int nfighters; // # of fighters covering this target
	/* for bomber guidance */
	unsigned int route[8][2]; // [0123 out, 4 bmb, 567 in][0 lat, 1 lon]. {0, 0} indicates "not used, skip to next routestage"
	double fires; // level of fires (and TIs) illuminating the target
	int skym; // time of last skymarker over target
	/* misc */
	unsigned int shots; // number of shots the flak already fired this tick
}
target;

typedef struct
{
	//STRENGTH:LAT:LONG:ENTRY:RADAR:EXIT
	unsigned int strength, lat, lon;
	date entry, radar, exit;
	/* for Himmelbett fighter control */
	signed int ftr; // index of fighter under this radar's control (-1 for none)
	unsigned int shots; // number of shots already fired this tick
}
flaksite;

typedef struct
{
	acid id;
	unsigned int type;
	unsigned int targ;
	double lat, lon;
	double navlat, navlon; // error in "believed position" relative to true position
	double driftlat, driftlon; // rate of error
	unsigned int bmblat, bmblon; // true position where bombs were dropped (if any)
	unsigned int bt; // time when bombs were dropped (if any)
	unsigned int route[8][2]; // [0123 out, 4 bmb, 567 in][0 lat, 1 lon]. {0, 0} indicates "not used, skip to next routestage"
	unsigned int routestage; // 8 means "passed route[7], heading for base"
	bool nav[NNAVAIDS];
	bool pff; // a/c is assigned to the PFF
	unsigned int b_he, b_in, b_ti, b_le; // bombload carried: high explosive or mines, incendiaries, target indicator flares, leaflets
	bool bombed;
	bool crashed;
	bool failed; // for forces, read as !svble
	bool landed; // for forces, read as !assigned
	double speed;
	double damage; // increases the probability of mech.fail and of consequent crashes
	bool ldf; // last damage by a fighter?
	bool idtar; // identified target?  (for use if RoE require it)
	bool fix; // have a navaid fix?  (controls whether to drop skymarker)
	unsigned int startt; // take-off time
	unsigned int fuelt; // when t (ticks) exceeds this value, turn for home
}
ac_bomber;

#define loadweight(b)	((b).b_he+(b).b_in/2+(b).b_ti+(b).b_le/20)
#define loadness(b)	((((b).bombed?0:loadweight(b))/(double)(types[(b).type].cap)+((int)(2*(b).fuelt)-(b).startt-t)/(double)fuelcap((b).type)))

typedef struct
{
	acid id;
	unsigned int type;
	unsigned int base;
	double lat, lon;
	bool crashed;
	bool landed;
	double damage;
	unsigned int fuelt;
	signed int k; // which bomber this fighter is attacking (-1 for none)
	signed int targ; // which target this fighter is covering (-1 for none)
	signed int hflak; // which flaksite's radar is controlling this fighter? (Himmelbett/Kammhuber line) (-1 for none)
}
ac_fighter;

#define ftr_free(f)	(((f).targ<0)&&((f).hflak<0)&&((f).fuelt>t)) // f is of type ac_fighter

typedef struct
{
	unsigned int nbombers;
	unsigned int *bombers; // offsets into the game.bombers list
	bombload *loads; // indexed by type
}
raid;

typedef struct
{
	date now;
	unsigned int cash, cshr;
	double confid, morale;
	unsigned int nbombers;
	ac_bomber *bombers;
	unsigned int nap[NNAVAIDS];
	unsigned int napb[NNAVAIDS];
	w_state weather;
	unsigned int ntargs;
	double *dmg, *flk, *heat;
	double gprod[ICLASS_MIXED], dprod[ICLASS_MIXED];
	unsigned int nfighters;
	ac_fighter *fighters;
	raid *raids;
	struct
	{
		bool idtar;
	}
	roe;
	char *msg[MAXMSGS];
	history hist;
}
game;
