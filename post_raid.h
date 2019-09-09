/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	post_raid: post-raid processing (pretends to be a screen, but isn't really)
*/

#include <stdbool.h>
#include <atg.h>
#include "types.h"

extern atg_element *post_raid_box;
void refill_students(game *state, bool refill);
