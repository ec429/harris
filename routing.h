/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	routing: code to generate bomber routes
*/
#include "types.h"

int genroute(unsigned int from[2], unsigned int ti, unsigned int route[8][2], const game *state, unsigned int iter); // [0=lat, 1=long]
