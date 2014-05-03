/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	rand: random-number functions
*/
#include <stdbool.h>
#include "bits.h"

int irandu(int n); // generate integer from U{0, 1, ... n-1}
double drandu(double m); // generate real from U[0,m)
bool brandp(double p); // generate Bern(p)
acid rand_acid(void); // generate a/c ID
