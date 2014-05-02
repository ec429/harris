/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	geom: geometry functions
*/
#pragma once
#include <stdbool.h>

void linedist(int ldx, int ldy, int pdx, int pdy, double *d, double *lambda); // min. distance from the line segment (0,0)-(ldx,ldy) to the point (pdx, pdy), as well as value of lambda
bool xyr(double x, double y, double r); // is |(x,y)|<r
// rect/polar conversion.  rect format is [0:y, 1:x]
void rect_to_polar(const int a[2], const int b[2], double *r, double *th); // convert (b-a) to distance & bearing
void polar_to_rect(double r, double th, const int a[2], int b[2]); // convert distance & bearing to cartesian, with origin at (a)
