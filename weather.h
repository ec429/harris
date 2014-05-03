#pragma once
/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	weather: simulates weather patterns
*/
#include <stdbool.h>

#include "types.h"

void w_init(w_state * buf, unsigned int prep, bool lorw[128][128]); // initialise weather state _buf_, with _prep_ iterations to smooth it out
void w_iter(w_state * ptr, bool lorw[128][128]); // iterate weather model
