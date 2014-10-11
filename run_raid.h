/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	run_raid: the raid in-progress screen
*/

#include <stdbool.h>
#include <atg.h>
#include "types.h"

extern atg_element *run_raid_box;
extern unsigned int totalraids;
extern int **dij, **nij, **tij, **lij;
extern unsigned int *heat;
extern bool *canscore;
extern double bridge, cidam;

int clear_raids(game *state);
