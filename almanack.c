#include "almanack.h"
#include <math.h>

// Approximate lat/lon of the four corners of the internal map
// [NS] [EW] [XY]
static double vertices[2][2][2] = {
	{
		{56.285055, -3.194158},
		{56.528201, 15.131037},
	},
	{
		{44.784450, -3.282048},
		{44.932415, 15.021174},
	},
};

// Convert internal lat/lon to (approximate) world lat/lon in degrees
void project_coords(int lat, int lon, double out[2])
{
	double sfx=lon/255.0, sfy=lat/255.0;
	for(int i=0;i<2;i++)
	{
		out[i]=0;
		for(int j=0;j<2;j++)
			for(int k=0;k<2;k++)
				out[i]+=vertices[k][j][i]*(j?sfx:1.0-sfx)*(k?sfy:1.0-sfy);
	}
}

/* Sunrise/set computation */
// Based on https://en.wikipedia.org/wiki/Sunrise_equation

// Approximate ecliptic longitude, computed at Greenwich
// This is assumed to be valid for the whole map
double greenwich_lambda(date d, double *rM)
{
	int n=diffdate(d, (date){.year=2000, .month=1, .day=1}); // Julian day since 2000-01-01
	double M=fmod(357.5291+0.98560028*n, 360); // solar mean anomaly
	double radM=M*M_PI/180.0; // in radians
	if(rM) *rM=radM;
	double C=1.9148*sin(radM)+0.02*sin(2*radM)+0.0003*sin(3*radM); // equation of the center
	double lambda=fmod(M+C+282.9372, 360); // ecliptic longitude
	if(lambda<0) lambda+=360;
	return(lambda*M_PI/180.0); // result in radians
}

// Approximate equation of time, similarly
double equation_of_time(date d, double *lbd)
{
	double M;
	double lambda=greenwich_lambda(d, &M);
	if(lbd) *lbd=lambda;
	return(-0.0069*sin(2*lambda)+0.0053*sin(M));
}

// Approximate δ
double solar_declination(double lambda)
{
	return(asin(sin(lambda)*sin(23.44*M_PI/180.0)));
}

// latitude is given in degrees
double hour_angle(double latitude, double delta)
{
	return(acos(-tan(latitude*M_PI/180.0)*tan(delta)));
}

// Calculate global values for a day's sunrise/set calculations
void sun_precalc(date d, double *delta, double *eqn)
{
	double lambda, eq, de;
	eq=equation_of_time(d, &lambda);
	if(eqn) *eqn=eq;
	de=solar_declination(lambda);
	if(delta) *delta=de;
}

// Compute hour of sunrise and set for a given location
void sun_calc(double coords[2], double delta, double eqn, double *rise, double *set)
{
	double ha=hour_angle(coords[0], delta);
	double noon=coords[1]*M_PI/180.0-eqn*M_PI*2.0;
	if(rise) *rise=fmod(noon-ha+M_PI*2.0, M_PI*2.0);
	if(set) *set=fmod(noon+ha+M_PI*2.0, M_PI*2.0);
}

// Convert an hour-angle into a time
harris_time convert_ha(double ha)
{
	double hours=fmod(13.0 + ha*12.0/M_PI, 24.0);
	return((harris_time){.hour=floor(hours), .minute=floor(fmod(hours*60, 60))});
}

// Convert a time into an hour-angle
double convert_ht(harris_time t)
{
	double hours=fmod(t.hour+(t.minute/60.0)+11.0, 24.0);
	return(hours*M_PI/12.0);
}
