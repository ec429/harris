/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details

	crew: crew member handling
*/

#include <stdbool.h>

enum cclass
{
	CCLASS_P,
	CCLASS_N,
	CCLASS_B,
	CCLASS_E,
	CCLASS_W,
	CCLASS_G,

	CREW_CLASSES,
	CCLASS_NONE // for extra_pupil, and lookup failure
};
typedef struct
{
	char letter;
	const char *name;
	const char *desc;
	unsigned int initpool, pupils;
	enum cclass extra_pupil;
	bool eats;
}
cclass_data;
extern const cclass_data cclasses[CREW_CLASSES];

enum cstatus
{
	CSTATUS_CREWMAN,
	CSTATUS_STUDENT,
	CSTATUS_INSTRUC,
	SHOW_STATUSES,
	
	CSTATUS_ESCAPEE = SHOW_STATUSES,
	
	CREW_STATUSES,
	CSTATUS_NONE // for lookup failure
};
extern const char *cstatuses[CREW_STATUSES];

enum cclass lookup_crew_letter(char letter);
enum cstatus lookup_crew_status(char *status);
