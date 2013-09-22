#include "routing.h"
#include <stdbool.h>
#include "rand.h"
#include "globals.h"
#include "date.h"
#include "geom.h"

int genroute(unsigned int from[2], unsigned int ti, unsigned int route[8][2], game state)
{
	bool hf=from[0]||from[1];
	route[4][0]=targs[ti].lat;
	route[4][1]=targs[ti].lon;
	for(unsigned int l=0;l<4;l++)
	{
		route[l][0]=(((hf?from[0]:67)*(4-l)+targs[ti].lat*(3+l))/7)+irandu(23)-11;
		route[l][1]=(((hf?from[1]:69)*(4-l)+targs[ti].lon*(3+l))/7)+irandu(9)-4;
	}
	for(unsigned int l=5;l<8;l++)
	{
		route[l][0]=(((hf?from[0]:118)*(l-4)+targs[ti].lat*(10-l))/6)+irandu(23)-11;
		route[l][1]=(((hf?from[1]:64)*(l-4)+targs[ti].lon*(10-l))/6)+irandu(11)-5;
	}
	for(unsigned int l=0;l<7;l++)
	{
		double z=1;
		while(true)
		{
			double scare[2]={0,0};
			for(unsigned int t=0;t<ntargs;t++)
			{
				if(t==ti) continue;
				if(!datewithin(state.now, targs[t].entry, targs[t].exit)) continue;
				double d, lambda;
				linedist(route[l+1][1]-route[l][1],
					route[l+1][0]-route[l][0],
					targs[t].lon-route[l][1],
					targs[t].lat-route[l][0],
					&d, &lambda);
				if(d<6)
				{
					double sv=targs[t].flak*0.18;
					double s=sv/(3.0+d);
					scare[0]+=s*(1-lambda);
					scare[1]+=s*lambda;
				}
			}
			for(unsigned int f=0;f<nflaks;f++)
			{
				if(!datewithin(state.now, flaks[f].entry, flaks[f].exit)) continue;
				double d, lambda;
				linedist(route[l+1][1]-route[l][1],
					route[l+1][0]-route[l][0],
					flaks[f].lon-route[l][1],
					flaks[f].lat-route[l][0],
					&d, &lambda);
				if(d<2)
				{
					double s=flaks[f].strength*0.01/(1.0+d);
					scare[0]+=s*(1-lambda);
					scare[1]+=s*lambda;
				}
			}
			z*=(scare[0]+scare[1])/(double)(1+scare[0]+scare[1]);
			double fs=scare[0]/(scare[0]+scare[1]);
			if(z<0.2) break;
			if(l!=4)
			{
				route[l][0]+=z*fs*(irandu(43)-21);
				route[l][1]+=z*fs*(irandu(21)-10);
			}
			if(l!=3)
			{
				route[l+1][0]+=z*(1-fs)*(irandu(43)-21);
				route[l+1][1]+=z*(1-fs)*(irandu(21)-10);
			}
		}
	}
	return(0);
}
