#pragma once
/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	globals: game entity data
*/
#include "types.h"
#include "events.h"

#define NAV_GEE		0
#define NAV_H2S		1
#define NAV_OBOE	2
#define NAV_GH		3

extern const char * const navaids[NNAVAIDS];
extern const char * const navpicfn[NNAVAIDS];
extern unsigned int navevent[NNAVAIDS];
extern unsigned int navprod[NNAVAIDS];

extern date event[NEVENTS];
extern char *evtext[NEVENTS];

extern struct oboe oboe;
extern struct gee gee;

extern unsigned int ntypes;
extern bombertype *types;
extern unsigned int nftypes;
extern fightertype *ftypes;
extern unsigned int nfbases;
extern ftrbase *fbases;
extern unsigned int ntargs;
extern target *targs;
extern unsigned int nflaks;
extern flaksite *flaks;

extern SDL_Surface *terrain, *location, *yellowhair, *intelbtn, *navpic[NNAVAIDS], *pffpic, *resizebtn, *fullbtn, *exitbtn;

extern SDL_Surface *grey_overlay, *yellow_overlay;
extern SDL_Surface *weather_overlay, *target_overlay, *flak_overlay, *xhair_overlay;

extern bool lorw[128][128];
extern unsigned char tnav[128][128];

extern unsigned int mainsizex, mainsizey;
extern bool fullscreen;
