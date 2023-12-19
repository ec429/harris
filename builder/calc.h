#ifndef _CALC_H
#define _CALC_H

#include <stdbool.h>
#include "data.h"
#include "../types.h"

struct engines {
	/* Inputs */
	unsigned int number;
	struct engine *typ, *mou;
	bool egg;
	/* Output cache */
	bool odd, manumatch;
	float power_factor;
	float vuln;
	float rely1, rely2;
	float serv;
	float cost;
	unsigned int scl;
	float fuelrate;
	float tare;
	float drag;
};

struct turrets {
	/* Inputs */
	struct turret *typ[LXN_COUNT];
	struct turret *mou[LXN_COUNT];
	/* Output cache */
	unsigned int need_gunners;
	bool gas[LXN_COUNT];
	float drag;
	float tare;
	float mtare;
	float ammo;
	float serv;
	float cost;
	float gc[GC_COUNT];
	float rate[2];
};

struct wing {
	/* Inputs */
	unsigned int area;
	unsigned int art; // AR in tenths
	/* Output cache */
	float ar;
	float span;
	float chord;
	float cl, ld;
	float tare;
	float cost;
	float wl;
	float drag;
};

struct crewman {
	enum cclass pos;
	bool gun;
};

struct crew {
	/* Inputs */
	unsigned int n;
	struct crewman men[MAX_CREW];
	/* Output cache */
	unsigned int gunners, engineers;
	bool pilot, nav;
	float tare;
	float gross;
	float dc;
	float bn;
	float cct; /* add to core_tare for core_cost */
	float es;
};

struct bombbay {
	/* Inputs */
	unsigned int cap;
	unsigned int load;
	enum bb_girth girth;
	bool csbs; /* not really bomb "bay" but whatever */
	/* Output cache */
	float factor;
	float bigfactor;
	float tare;
	float cost; /* csbs cost, rest is paid through core_tare */
	bool cookie;
};

struct fuselage {
	/* Inputs */
	enum fuse_type typ;
	/* Output cache */
	float serv, fail;
	float tare;
	float cost;
	float vuln;
	float drag;
};

struct electrics {
	/* Inputs */
	enum elec_level esl;
	bool navaid[NNAVAIDS];
	/* Output cache */
	float cost;
	float ncost;
};

struct tanks {
	/* Inputs */
	unsigned int hlb; // hundreds lb capacity
	unsigned int pct; // fill level %
	bool sst;
	/* Output cache */
	float hours;
	float cap;
	float mass;
	float tare;
	float cost;
	float ratio;
	float vuln;
};

struct randomisation {
	bool rolled;
	int drag;
	int serv;
	int vuln;
	int manu;
	int accu;
};

#define MAX_EW	16
#define EW_LEN	80
struct bomber {
	/* Inputs */
	const struct bomber *parent;
	struct manf *manf;
	struct engines engines;
	struct turrets turrets;
	struct wing wing;
	struct crew crew;
	struct bombbay bay;
	struct fuselage fuse;
	struct electrics elec;
	struct tanks tanks;
	enum refit_level refit;
	struct tech_numbers tn;
	struct randomisation dice;
	/* Output cache */
	bool error;
	unsigned int new;
	char ew[MAX_EW][EW_LEN];
	float serv;
	float fail;
	float core_tare;
	float core_mtare;
	float tare;
	float gross;
	unsigned int mtow; // structure stressed for this; mod/doc can't exceed
	bool user_mtow; // user specified mtow, don't recalculate
	float overgross;
	float drag;
	float core_cost;
	float stress_factor;
	float cost;
	float takeoff_spd;
	float ceiling; /* thousands ft */
	float cruise_alt, cruise_spd;
	float init_climb, deck_spd;
	float range;
	float roll_pen, turn_pen, manu_pen;
	float evade_factor;
	float vuln;
	float fight_factor[2], flak_factor;
	float defn[2];
	float accu;
	float tproto;
	float tprod;
	float cproto;
	float cprod;
};

const struct bomber *mod_ancestor(const struct bomber *b);
void count_crew(const struct crew *c, unsigned int *v);
void count_dcrew(const struct crew *c, unsigned int *v);

void init_bomber(struct bomber *b, struct manf *m, struct engine *e);
int calc_bomber(struct bomber *b, const struct tech_numbers *tn);
int do_randomise(struct bomber *b);

float wing_lift(const struct wing *w, float v);
#endif // _CALC_H
