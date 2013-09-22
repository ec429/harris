/*
	harris - a strategy game
	Copyright (C) 2012-2013 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	routing: code to generate bomber routes
*/
#include "routing.h"
#include <stdbool.h>
#include "rand.h"
#include "globals.h"
#include "date.h"
#include "geom.h"

int genroute(unsigned int from[2], unsigned int ti, unsigned int route[8][2], game state)
{
	for(unsigned int i=0;i<10;i++)
	{
		unsigned int try[8][2];
		double rscore=-1e300;
		bool hf=from[0]||from[1];
		try[4][0]=targs[ti].lat;
		try[4][1]=targs[ti].lon;
		for(unsigned int l=0;l<4;l++)
		{
			try[l][0]=(((hf?from[0]:67)*(4-l)+targs[ti].lat*(3+l))/7)+irandu(23)-11;
			try[l][1]=(((hf?from[1]:69)*(4-l)+targs[ti].lon*(3+l))/7)+irandu(9)-4;
		}
		for(unsigned int l=5;l<8;l++)
		{
			try[l][0]=(((hf?from[0]:118)*(l-4)+targs[ti].lat*(10-l))/6)+irandu(23)-11;
			try[l][1]=(((hf?from[1]:64)*(l-4)+targs[ti].lon*(10-l))/6)+irandu(11)-5;
		}
		double score=1000;
		for(unsigned int l=0;l<7;l++)
		{
			double z=1;
			double scare[2];
			while(true)
			{
				scare[0]=scare[1]=0;
				for(unsigned int t=0;t<ntargs;t++)
				{
					if(t==ti) continue;
					if(!datewithin(state.now, targs[t].entry, targs[t].exit)) continue;
					double d, lambda;
					linedist(try[l+1][1]-try[l][1],
						try[l+1][0]-try[l][0],
						targs[t].lon-try[l][1],
						targs[t].lat-try[l][0],
						&d, &lambda);
					if(d<6)
					{
						double sv=targs[t].flak*0.18+state.heat[t]*0.6+0.4;
						double s=sv/(2.0+d);
						scare[0]+=s*(1-lambda);
						scare[1]+=s*lambda;
					}
				}
				for(unsigned int f=0;f<nflaks;f++)
				{
					if(!datewithin(state.now, flaks[f].entry, flaks[f].exit)) continue;
					double d, lambda;
					linedist(try[l+1][1]-try[l][1],
						try[l+1][0]-try[l][0],
						flaks[f].lon-try[l][1],
						flaks[f].lat-try[l][0],
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
					try[l][0]+=z*fs*(irandu(43)-21);
					try[l][1]+=z*fs*(irandu(21)-10);
				}
				if(l!=3)
				{
					try[l+1][0]+=z*(1-fs)*(irandu(43)-21);
					try[l+1][1]+=z*(1-fs)*(irandu(21)-10);
				}
			}
			score-=scare[0]+scare[1];
		}
		if(score>rscore)
		{
			rscore=score;
			for(unsigned int l=0;l<8;l++)
			{
				route[l][0]=try[l][0];
				route[l][1]=try[l][1];
			}
		}
	}
	return(0);
}
