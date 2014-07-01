/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	dclass: difficulty class definitions
*/

#include "types.h"

dclass dclasses[DIFFICULTY_CLASSES]={
	[DCLASS_ALL]   = {.name="All",                 .desc="Set all categories to the same difficulty"},
	[DCLASS_RCITY] = {.name="Resilience (cities)", .desc="How much damage it takes to destroy a city",            .values={10, 20, 50}},
	[DCLASS_ROTHER]= {.name="Resilience (other)",  .desc="How much damage it takes to destroy a non-city target", .values={12, 20, 36}},
	[DCLASS_FLAK]  = {.name="Flak strength",       .desc="Strength of target flak defences and sited flak",       .values={10, 25, 40}},
};
