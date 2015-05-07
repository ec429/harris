/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	rand: random-number functions
*/
#include "rand.h"
#include <stdlib.h>
#include <math.h>

int irandu(int n)
{
	if(!n) return(0);
	// This is poor quality randomness, but that doesn't really matter here
	return(rand()%n);
}

double drandu(double m)
{
	return(rand()*m/(double)RAND_MAX);
}

bool brandp(double p)
{
	return(rand()<RAND_MAX*p);
}

acid rand_acid(void)
{
	// Again, lazy and rubbish implementation,
	// but 0xffffffff is probably RAND_MAX anyway
	return(rand()&0xffffffff);
}
