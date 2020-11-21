/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details

	crew: crew member handling
*/

#include <string.h>
#include "crew.h"

const cclass_data cclasses[CREW_CLASSES]={
	[CCLASS_P] = {.letter='P', .name="Pilot",       .desc="The pilot flies the bomber and is responsible for the lives of the whole crew.", .initpool=60, .pupils=3, .extra_pupil=CCLASS_E},
	[CCLASS_N] = {.letter='N', .name="Navigator",   .desc="The navigator finds the way to the target - and home again.",                    .initpool=60, .pupils=3, .extra_pupil=CCLASS_B},
	[CCLASS_B] = {.letter='B', .name="Bomb-aimer",  .desc="The bomb-aimer aims the bombs, and mans the H2S if fitted.",                     .initpool=20, .pupils=5, .extra_pupil=CCLASS_G},
	[CCLASS_E] = {.letter='E', .name="Engineer",    .desc="The flight-engineer keeps the 'plane in good working order.",                    .initpool= 0, .pupils=4, .extra_pupil=CCLASS_NONE},
	[CCLASS_W] = {.letter='W', .name="Wireless-op", .desc="The W/Op handles signals to and from base.",                                     .initpool=30, .pupils=5, .extra_pupil=CCLASS_G},
	[CCLASS_G] = {.letter='G', .name="Gunner",      .desc="The air-gunner watches for enemy fighters, and tries to shoot them down.",       .initpool=75, .pupils=4, .extra_pupil=CCLASS_NONE},
};

const char *cstatuses[CREW_STATUSES]={
	[CSTATUS_CREWMAN] = "Crewman",
	[CSTATUS_STUDENT] = "Student",
	[CSTATUS_INSTRUC] = "Instructor",
	[CSTATUS_ESCAPEE] = "Escaping",
};

enum cclass lookup_crew_letter(char letter)
{
	for(unsigned int c=0;c<CREW_CLASSES;c++)
		if(letter==cclasses[c].letter)
			return c;
	return CCLASS_NONE;
}

enum cstatus lookup_crew_status(char *status)
{
	for(unsigned int s=0;s<CREW_STATUSES;s++)
		if(!strcmp(status, cstatuses[s]))
			return s;
	return CSTATUS_NONE;
}
