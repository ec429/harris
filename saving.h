/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	saving: functions to save and load games from file
*/

#include "types.h"

int loadgame(const char *fn, game *state);
int savegame(const char *fn, game state);
