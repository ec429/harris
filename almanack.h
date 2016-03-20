#pragma once
/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	almanack: approximate sun rise and set times
*/

#include "date.h"

// Convert internal lat/lon to (approximate) world lat/lon in degrees
void project_coords(int lat, int lon, double out[2]);

// Calculate global values for a day's sunrise/set calculations
void sun_precalc(date d, double *delta, double *eqn);
// Compute hour of sunrise and set for a given location
void sun_calc(double coords[2], double delta, double eqn, double *rise, double *set);
// Convert an hour-angle into a time
harris_time convert_ha(double ha);
// Convert a time into an hour-angle
double convert_ht(harris_time t);
