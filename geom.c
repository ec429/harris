#include "geom.h"
#include <math.h>

unsigned int xxyy(int dx, int dy)
{
	return(dx*dx+dy*dy);
}

double linedist(int ldx, int ldy, int pdx, int pdy)
{
	unsigned int l=xxyy(ldx, ldy);
	if(!l) return(sqrt(xxyy(pdx, pdy)));
	double t=(pdx*ldx+pdy*ldy)/l;
	if(t<0) t=0;
	else if(t>1) t=1;
	return(sqrt(xxyy(pdx-t*ldx, pdy-t*ldy)));
}
