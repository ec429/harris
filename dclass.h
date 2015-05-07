/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	dclass: difficulty class definitions
*/

typedef struct
{
	const char *name;
	const char *desc;
	unsigned int values[3];
}
dclass;
enum
{
	DCLASS_ALL,
	DCLASS_RCITY,
	DCLASS_RINDUS,
	DCLASS_RSHIP,
	DCLASS_RLEAF,
	DCLASS_ROTHER,
	DCLASS_FLAK,
	DCLASS_FSR,
	DCLASS_TPOOL,

	DIFFICULTY_CLASSES
};
extern dclass dclasses[DIFFICULTY_CLASSES];

#define GET_DC(game, name)	(dclasses[DCLASS_##name].values[(game)->difficulty[DCLASS_##name]])
