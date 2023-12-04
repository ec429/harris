#ifndef _DATA_H
#define _DATA_H

#include <stdbool.h>
#include "list.h"

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(*x))

enum turret_location {
	LXN_UNSPEC,
	LXN_NOSE,
	LXN_DORSAL,
	LXN_TAIL,
	LXN_WAIST,
	LXN_VENTRAL,
	LXN_CHIN,
	LXN_FIXED,

	LXN_COUNT
};

enum coverage_direction {
	GC_FRONT, // GCF
	GC_BEAM_HIGH, // GCD
	GC_BEAM_LOW, // GCV
	GC_TAIL_HIGH, // GCH
	GC_TAIL_LOW, // GCL
	GC_BENEATH, // GCB

	GC_COUNT
};

struct turret {
	struct list_head list;
	char ident[5];
	unsigned int srv;
	unsigned int twt;
	unsigned int drg;
	enum turret_location lxn;
	unsigned int gun;
	unsigned int gc[GC_COUNT];
	unsigned int ocp, ocn, ocb; /* operable by crew besides W* and G */
	unsigned int slb;
	unsigned int esl;
	char *name;
	char *desc;
	bool unlocked;
};

int load_guns(struct list_head *head);
int free_guns(struct list_head *head);

struct engine {
	struct list_head list;
	char ident[5];
	unsigned int bhp;
	unsigned int vul;
	unsigned int fai;
	unsigned int svc;
	unsigned int cos;
	unsigned int scl;
	unsigned int twt;
	unsigned int drg;
	struct engine *u; /* can mod to us with overbuilt mounts */
	char *manu;
	char *name;
	char *desc;
	bool unlocked;
};

int load_engines(struct list_head *head);
int free_engines(struct list_head *head);

enum bb_girth {
	BB_SMALL,
	BB_MEDIUM,
	BB_COOKIE,

	BB_COUNT
};

enum fuse_type {
	FT_NORMAL,
	FT_SLENDER,
	FT_SLABBY,
	FT_GEODETIC,

	FT_COUNT
};

struct manf {
	struct list_head list;
	char ident[3];
	unsigned int wap, wld, bt[BB_COUNT], bbb;
	unsigned int wcf, wcp, wc4, wt4, acc, act, geo;
	unsigned int tpl, fd[FT_COUNT], ft[FT_COUNT], svp, bof;
	char *eman; // engine manufacturer
	char *name;
	char *desc;
};

int load_manfs(struct list_head *head);
int free_manfs(struct list_head *head);

enum nav_aid {
	NA_GEE,
	NA_H2S,
	NA_OBOE,

	NA_COUNT
};

struct tech_numbers {
	/* These MUST all be `unsigned int`!  Copying code assumes this. */
	/* Core block.  Cannot change even in MARK refits. */
	unsigned int fwt; // Fuse Wing Tare * 100
	// Wings
	unsigned int wts; // Wing Tare Span exponent * 100
	unsigned int wtc; // Wing Tare Chord exponent * 100
	unsigned int wtf; // Wing Tare Factor * 100
	unsigned int wcf; // Wing Cost Factor * 100
	// Engine Mounts
	unsigned int etf; // Engine Tare Factor * 100
	// Bomb bay
	unsigned int bt[BB_COUNT]; // Bay Tare factor * 1000
	unsigned int bbb; // Bay Bigfactor Base / 1000lb
	unsigned int bbf; // Bay Bigfactor Frac / 1e5
	unsigned int ubl; // Unarmed Bomber Limit (engine count)
	/* MARK block.  Cannot change in MOD refits. */
	unsigned int mark_block[0];
	unsigned int ft[FT_COUNT]; // Fuse Tare * 100
	unsigned int fd[FT_COUNT]; // Fuse Drag * 10
	unsigned int fs[FT_COUNT]; // Fuse Serv * 10
	unsigned int ff[FT_COUNT]; // Fuse Fail * 10
	unsigned int fv[FT_COUNT]; // Fuse Vuln * 100
	unsigned int cc[FT_COUNT]; // Core Cost * 100
	unsigned int fc[FT_COUNT]; // Fuse Cost * 100
	// Wings
	unsigned int wld; // Wing Lift/Drag * 100
	// General Arrangement, 4 Engines
	unsigned int g4t; // tare penalty for 4+ engine mounts
	unsigned int g4c; // cost scaling for 4+ engines * 100
	// Fuel Tanks
	unsigned int fut; // Fuel Tare * 100
	unsigned int fuv; // Fuel Vuln * 100
	unsigned int fuc; // Fuel Cost * 100
	unsigned int fgv; // Fuel Geodetic Vuln factor * 100
	// Engine Mounts
	unsigned int edf; // Engine Drag Factor * 100
	unsigned int emc; // Engine Mounting Cost factor * 100
	unsigned int ees; // Power Egg Serv factor * 100
	unsigned int eet; // Power Egg Tare factor * 100
	unsigned int eec; // Power Egg Cost factor * 100
	// Gun Turrets
	unsigned int gtf; // Gun Tare Factor * 100
	unsigned int gdf; // Gun Drag Factor
	unsigned int gcf; // Gun Cost Factor * 100
	// Equipment
	unsigned int esl; // Electric Supply Level (enum elec_level)
	/* MOD block.  Cannot change in DOCTRINE refits. */
	unsigned int mod_block[0];
	// Fuel Tanks, Self Sealing
	unsigned int sft; // SST Tare scaling * 100
	unsigned int sfv; // SST Vuln scaling * 100
	unsigned int sfc; // SST Cost scaling * 100
	// Crew Stations
	unsigned int cmi; // crewman incidentals tare
	unsigned int ces; // crewman effective skill * 100
	unsigned int ccc; // crewman core cost scaling * 100
	// Gun Turrets
	unsigned int gam; // Gun Ammo Mass, lb/gun
	unsigned int gac; // Gun Ammo track Cost * 10
	// Equipment
	unsigned int csb; // Mk XIV Course-Setting Bombsight (flag)
	unsigned int na[NA_COUNT]; // Nav Aid (flag)
	/* Doctrine block.  Changes even without refit. */
	unsigned int doctrine_block[0];
	unsigned int clt; // CLimb Time
	unsigned int bmc; // Bay, Medium, Cookie carriage (flag)
	unsigned int rgs; // Max take-off speed, grass
	unsigned int rgg; // Max gross take-off weight, grass
	unsigned int rcs; // Max take-off speed, concrete
	unsigned int rcg; // Max gross take-off weight, concrete
};

struct tech {
	struct list_head list;
	char ident[4];
	unsigned int year, month;
	struct tech_numbers num;
	struct tech *req[8];
	struct engine *eng[16];
	struct turret *gun[16];
	char *name;
	char *desc;
	bool unlocked, have_reqs;
};

int try_load_tn_word(const char *key, const char *value,
		     struct tech_numbers *tn);
int load_techs(struct list_head *head, struct list_head *engines,
	       struct list_head *guns);
int free_techs(struct list_head *head);

struct entities {
	unsigned int ngun, neng, nmanf, ntech;
	struct turret **gun;
	struct engine **eng;
	struct manf **manf;
	struct tech **tech;
};

int populate_entities(struct entities *ent, struct list_head *guns,
		      struct list_head *engines, struct list_head *manfs,
		      struct list_head *techs);

int apply_techs(const struct entities *ent, struct tech_numbers *tn);

#endif // _DATA_H
