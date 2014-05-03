#pragma once
/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	globals: game entity data
*/
#include "types.h"

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

extern SDL_Surface *fullbtn, *exitbtn;

extern bool lorw[128][128];

extern unsigned int mainsizex, mainsizey;
extern bool fullscreen;
