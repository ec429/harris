/*
	harris - a strategy game
	Copyright (C) 2012-2013 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	weather: simulates weather patterns
*/
#include <stdbool.h>

typedef struct
{
	double push, slant;
	double p[256][128];
	double t[256][128];
}
w_state;

void w_init(w_state * buf, unsigned int prep, bool lorw[128][128]); // initialise weather state _buf_, with _prep_ iterations to smooth it out
void w_iter(w_state * ptr, bool lorw[128][128]); // iterate weather model
