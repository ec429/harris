/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	geom: geometry functions
*/

#include "geom.h"
#include <math.h>

unsigned int xxyy(int dx, int dy) // computes dx**2+dy**2
{
	return(dx*dx+dy*dy);
}

void linedist(int ldx, int ldy, int pdx, int pdy, double *d, double *lambda)
{
	unsigned int l=xxyy(ldx, ldy);
	if(!l)
	{
		if(d) *d=sqrt(xxyy(pdx, pdy));
		if(lambda) *lambda=0;
		return;
	}
	double t=(pdx*ldx+pdy*ldy)/l;
	if(t<0) t=0;
	else if(t>1) t=1;
	if(d) *d=sqrt(xxyy(pdx-t*ldx, pdy-t*ldy));
	if(lambda) *lambda=t;
}

inline bool xyr(double x, double y, double r)
{
	if(fabs(x)>fabs(r)) return(false);
	if(fabs(y)>fabs(r)) return(false);
	return(x*x+y*y<r*r);
}

void rect_to_polar(const int a[2], const int b[2], double *r, double *th)
{
	if(r)
		*r = hypot(b[0]-a[0], b[1]-a[1]);
	if(th)
		*th = atan2(b[0]-a[0], b[1]-a[1]);
}

void polar_to_rect(double r, double th, const int a[2], int b[2])
{
	b[0]=a[0]+r*sin(th);
	b[1]=a[1]+r*cos(th);
}
