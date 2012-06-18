#include "rand.h"
#include <stdlib.h>
#include <math.h>

int irandu(int n)
{
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
