/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	saving: functions to save and load games from file
*/

#include "types.h"

#ifdef WINDOWS /* Because of this little bugger, savegames from Windows won't work on Linux/Unix and vice-versa */
#define FLOAT	"%I64x"
#define CAST_FLOAT_PTR	(unsigned long long *)
#define CAST_FLOAT	*(unsigned long long *)&
#else
#define FLOAT	"%la"
#define CAST_FLOAT_PTR
#define CAST_FLOAT
#endif

int loadgame(const char *fn, game *state);
int savegame(const char *fn, game state);
