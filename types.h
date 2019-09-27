#pragma once
/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	types: data type definitions
*/

#include <atg.h>
#include "dclass.h"
#include "crew.h"
#include "events.h"

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
harris_time;

typedef uint32_t acid; // a/c ID
typedef uint64_t cmid; // crewman ID

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
	unsigned int seed; // if nonzero, we need to randgen
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
	BL_HALFHALF,
	NBOMBLOADS
}
bombload;

struct bombloadinfo
{
	const char *name;
	const char *fn;
	SDL_Surface *pic;
};

#define MAX_CREW	7

struct bomberstats
{
	unsigned int cost;
	unsigned int speed;
	unsigned int alt;
	unsigned int capwt;
	unsigned int capbulk;
	unsigned int svp;
	unsigned int defn;
	unsigned int fail;
	unsigned int accu;
	unsigned int range;
};

#define MAX_MARKS	4

typedef struct
{
	//MANUFACTURER:NAME:COST:SPEED:CEILING:CAPACITY:SVP:DEFENCE:FAILURE:ACCURACY:RANGE:BLAT:BLONG:DD-MM-YYYY:DD-MM-YYYY:CREW:NAVAIDS,FLAGS,BOMBLOADS
	char * manu;
	char * name;
	struct bomberstats mark[MAX_MARKS];
	char *markname[MAX_MARKS];
	enum cclass crew[MAX_CREW];
	bool nav[NNAVAIDS];
	bool load[NBOMBLOADS];
	bool crewbg, crewwg;
	bool noarm, pff, heavy, inc, extra, ovltank, slowgrow, otub;
	date entry;
	date novelty;
	date exit;
	unsigned int blat, blon;
	SDL_Surface *picture, *side_image;
	char *text, *newtext;
	
	unsigned int count;
	unsigned int navcount[NNAVAIDS];
	unsigned int pffcount;
	unsigned int prio;
	unsigned int pribuf;
	atg_element *prio_selector;
	unsigned int pc;
	unsigned int pcbuf;
	unsigned int newmark;
}
bombertype;
#define newstats(t)	((t).mark[(t).newmark])

typedef enum
{
	BSTAT_COST,
	BSTAT_SPEED,
	BSTAT_ALT,
	BSTAT_CAPWT,
	BSTAT_CAPBULK,
	BSTAT_SVP,
	BSTAT_DEFN,
	BSTAT_FAIL,
	BSTAT_ACCU,
	BSTAT_RANGE,
	BSTAT__NUMERIC=BSTAT_RANGE,
	BSTAT_CREW,
	BSTAT_LOADS,
	BSTAT_FLAGS,
	NUM_BSTATS
}
bstat;

typedef enum
{
	BFLAG_OVLTANK,
	BFLAG_CREWBG,
	BFLAG_NCREWBG,
	BFLAG_CREWWG,
	BFLAG_NCREWWG,
}
bflag;

typedef struct
{
	// DESCRIPTION:ACNAME:STAT:OLD:NEW:DD-MM-YYYY:[MARK]
	char *desc;
	unsigned int bt;
	bstat s;
	union
	{
		unsigned int i;
		bflag f;
		enum cclass crew[MAX_CREW];
	}
	v;
	date d;
	unsigned int mark;
}
bmod;

typedef struct
{
	//MANUFACTURER:NAME:COST:SPEED:ARMAMENT:MNV:DD-MM-YYYY:DD-MM-YYYY:FLAGS
	char *manu;
	char *name;
	unsigned int cost;
	unsigned int speed;
	unsigned int arm;
	unsigned int mnv;
	unsigned char radpri;
	date entry;
	date exit;
	bool night;
	SDL_Surface *picture, *side_image;
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

typedef struct
{
	//NAME:LAT:LONG:RADIUS
	char *name;
	unsigned int lat, lon, radius;
}
locxn;

enum t_class {TCLASS_CITY,TCLASS_SHIPPING,TCLASS_MINING,TCLASS_LEAFLET,TCLASS_AIRFIELD,TCLASS_BRIDGE,TCLASS_ROAD,TCLASS_INDUSTRY, // INDUSTRY must be last!
T_CLASSES};
enum i_class {ICLASS_BB, ICLASS_OIL, ICLASS_RAIL, ICLASS_UBOOT, ICLASS_ARM, ICLASS_STEEL, ICLASS_AC, ICLASS_RADAR, ICLASS_MIXED,
I_CLASSES=ICLASS_MIXED};

typedef struct
{
	//NAME:PROD:FLAK:ESIZ:LAT:LONG:DD-MM-YYYY:DD-MM-YYYY:CLASS
	char *name;
	intel *p_intel;
	unsigned int prod, flak, esiz, lat, lon;
	date entry, exit;
	enum t_class class;
	enum i_class iclass;
	int city; // owning city for TCLASS_INDUSTRY (or -1)
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
	bool mapped; /* Have we learned of this flaksite's existence? */
	/* for Himmelbett fighter control */
	unsigned int heat; // number of bombers in the vicinity
	signed int ftr; // index of fighter under this radar's control (-1 for none)
	unsigned int shots; // number of shots already fired this tick
}
flaksite;

typedef struct
{
	enum {DS_NONE, DS_FIGHTER, DS_FLAK, DS_TFLK, DS_MECH, DS_BOMBER, DS_FUEL} ds;
	unsigned int idx; // index of bomber, fighter, flak or target if applicable
}
dmgsrc;

enum tpipe
{
	TPIPE_OTU,
	TPIPE_HCU,
	TPIPE_LFS,
	TPIPE__MAX
};

typedef struct
{
	acid id;
	unsigned int type;
	int crew[MAX_CREW]; // -1 for unassigned
	bool train; // is this a/c assigned to a training unit?
	unsigned int targ;
	double lat, lon;
	double navlat, navlon; // error in "believed position" relative to true position
	double driftlat, driftlon; // rate of error
	double bmblat, bmblon; // true position where bombs were dropped (if any)
	unsigned int bt; // time when bombs were dropped (if any)
	unsigned int route[8][2]; // [0123 out, 4 bmb, 567 in][0 lat, 1 lon]. {0, 0} indicates "not used, skip to next routestage"
	unsigned int routestage; // 8 means "passed route[7], heading for base"
	bool nav[NNAVAIDS];
	bool pff; // a/c is assigned to the PFF
	unsigned int mark;
	unsigned int b_hc, b_gp, b_in, b_ti, b_le; // bombload carried: high capacity explosive, general purpose high explosive or mines, incendiaries, target indicator flares, leaflets
	bool bombed;
	bool crashed;
	bool failed; // for forces, read as !svble
	bool landed; // for forces, read as !assigned
	double speed;
	double damage; // increases the probability of mech.fail and of consequent crashes
	dmgsrc ld; // last damage source
	bool idtar; // identified target?  (for use if RoE require it)
	bool fix; // have a navaid fix?  (controls whether to drop skymarker)
	unsigned int startt; // take-off time
	unsigned int fuelt; // when t (ticks) exceeds this value, turn for home
	int flakreport; // first unmapped flaksite we encountered
}
ac_bomber;

#define loadweight(b)	((b).b_hc+(b).b_gp+(b).b_in+(b).b_ti+(b).b_le/20)
#define loadbulk(b)	((b).b_hc+(b).b_gp+(b).b_in*1.5+(b).b_ti*2+(b).b_le/3)
#define bstats(b)	(types[(b).type].mark[(b).mark])
#define fuelcap(b)	(bstats(b).range*180.0/(double)(bstats(b).speed))
#define loadness(b)	((((b).bombed?0:loadweight(b))/(double)(bstats(b).capwt)+((int)(2*(b).fuelt)-(b).startt-t)/(double)fuelcap(b)))

typedef struct
{
	acid id;
	unsigned int type;
	unsigned int base;
	double lat, lon;
	bool crashed;
	bool landed;
	double damage;
	dmgsrc ld; // last damage source
	bool radar;
	unsigned int fuelt;
	signed int k; // which bomber this fighter is attacking (-1 for none)
	signed int targ; // which target this fighter is covering (-1 for none)
	signed int hflak; // which flaksite's radar is controlling this fighter? (Himmelbett/Kammhuber line) (-1 for none)
}
ac_fighter;

#define ftr_speed(f)	((ftypes[(f).type].speed-((f).radar?30:0)) / 400.0)
#define ftr_free(f)	(((f).targ<0)&&((f).hflak<0)&&((f).fuelt>t)) // f is of type ac_fighter

typedef struct
{
	unsigned int nbombers;
	unsigned int *bombers; // offsets into the game.bombers list
	bombload *loads; // indexed by type
	bombload *pffloads; // indexed by type, only used for LanI/LanX (which have partial PFF)
	unsigned int zerohour; // as an rrtime
	bool routed;
}
raid;

typedef struct
{
	cmid id;
	enum cclass class;
	enum cstatus status;
	double skill;
	double heavy;
	double lanc;
	unsigned int lrate;
	unsigned int tour_ops;
	/* meaning depends on status
		CREWMAN: number of ops completed this tour.  After 30, become INSTRUC
		STUDENT: number of days of current course completed
		INSTRUC: number of days spent instructing.  After 180, become CREWMAN
		ESCAPEE: as CREWMAN.  On safe return, become CREWMAN again
	*/
	union {
		enum tpipe training; // for STUDENT, which training stage we're at
		unsigned int full_tours; // otherwise, the count of complete CREWMAN tours
	};
	int assignment;
	/* meaning depends on status
		CREWMAN: bomber index (or -1)
		STUDENT: same as CREWMAN, but bomber must be assigned to training
		INSTRUC: currently not used
		ESCAPEE: days until return
	*/
}
crewman;

typedef struct
{
	date now;
	unsigned int difficulty[DIFFICULTY_CLASSES]; // [0] is ignored
	unsigned int cash, cshr;
	double confid, morale;
	unsigned int nbombers;
	ac_bomber *bombers;
	unsigned int ncrews;
	crewman *crews;
	bool *btypes;
	bool evented[NEVENTS];
	int nap[NNAVAIDS];
	unsigned int napb[NNAVAIDS];
	w_state weather;
	unsigned int ntargs;
	double *dmg, *flk, *heat, *flam; // arrays of target damage, flak strength, 'heat' (been attacked much lately), and increased flammability through cookie hits
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
	enum t_class tfav[2];
	unsigned int tfd[2];
	enum i_class ifav[2];
	unsigned int ifd[2];
	struct tpipe_settings
	{
		int dwell; // course duration, in days
		unsigned char cont; // % who continue to next stage
	}
	tpipe[TPIPE__MAX];
}
game;

typedef struct
{
	char *filename;
	char *title;
	char *description;
}
startpoint;

struct oboe
{
	signed int lat, lon;
	signed int k; // bomber number, or -1 for none
};

struct gee
{
	signed int lat, lon;
	unsigned int range, jrange;
};

struct region
{
	char *name;
	unsigned char rgb[3];
	enum {REGSTAT_FRIENDLY, REGSTAT_NEUTRAL, REGSTAT_ENEMY, REG_STATUSES} status;
	bool water;
};

enum overlay_type
{
	OVERLAY_CITY,
	OVERLAY_FLAK,
	OVERLAY_TARGET,
	OVERLAY_WEATHER,
	OVERLAY_ROUTE,

	NUM_OVERLAYS
};

struct overlay
{
	const char *ifn;
	SDL_Surface *icon;
	bool selected;
};
