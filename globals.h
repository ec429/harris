#pragma once
/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	globals: game entity data
*/
#include "types.h"
#include "events.h"

#define NAV_GEE		0
#define NAV_H2S		1
#define NAV_OBOE	2
#define NAV_GH		3

int set_init_state(game *state);

extern const char * const navaids[NNAVAIDS];
extern const char * const navpicfn[NNAVAIDS];
extern unsigned int navevent[NNAVAIDS];
extern unsigned int navprod[NNAVAIDS];

extern date event[NEVENTS];
extern char *evtext[NEVENTS];

extern struct oboe oboe;
extern struct gee gee;

extern struct bombloadinfo bombloads[NBOMBLOADS];

extern struct overlay overlays[NUM_OVERLAYS];

extern unsigned int ntypes;
extern bombertype *types;
extern bombertype *rawtypes;
extern unsigned int nmods;
extern bmod *mods;
extern unsigned int nftypes;
extern fightertype *ftypes;
extern unsigned int nfbases;
extern ftrbase *fbases;
extern unsigned int nlocs;
extern locxn *locs;
extern unsigned int ntargs;
extern target *targs;
extern unsigned int nflaks;
extern flaksite *flaks;
extern unsigned int nstarts;
extern startpoint *starts;

extern SDL_Surface *terrain, *location, *yellowhair, *nointelbtn, *intelbtn, *navpic[NNAVAIDS], *pffpic, *elitepic, *studentpic, *resizebtn, *fullbtn, *exitbtn;

extern SDL_Surface *grey_overlay, *yellow_overlay;
extern SDL_Surface *weather_overlay, *city_overlay, *target_overlay, *flak_overlay, *route_overlay, *xhair_overlay, *seltarg_overlay;

extern SDL_Surface *tick, *cross;

extern SDL_Surface *intelscreenbtn[3];

extern bool lorw[128][128];
extern unsigned char tnav[128][128];
extern unsigned int nregions;
extern struct region *regions;
extern unsigned int region[256][256];

extern unsigned int mainsizex, mainsizey;
extern bool fullscreen;

extern bool localdat, localsav;
extern char *cwd;
