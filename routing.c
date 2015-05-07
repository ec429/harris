/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	routing: code to generate bomber routes
*/
#include "routing.h"
#include <stdbool.h>
#include "rand.h"
#include "globals.h"
#include "date.h"
#include "geom.h"

int genroute(unsigned int from[2], unsigned int ti, unsigned int route[8][2], const game *state, unsigned int iter)
{
	for(unsigned int i=0;i<iter;i++)
	{
		int try[8][2];
		double rscore=0;
		bool hf=from[0]||from[1];
		try[4][0]=targs[ti].lat;
		try[4][1]=targs[ti].lon;
		unsigned int at[2];
		if(hf)
			for(unsigned int i=0;i<2;i++)
				at[i]=from[i];
		else
		{
			at[0]=67;
			at[1]=69;
		}
		for(unsigned int l=0;l<4;l++)
		{
			double r, th;
			rect_to_polar((const int *)at, (const int *)try[4], &r, &th);
			double dev=1.2/(1.0+r*0.04);
			r*=0.2+drandu(0.4);
			th+=drandu(dev*2)-dev;
			polar_to_rect(r, th, (const int *)at, (int *)try[l]);
			for(unsigned int i=0;i<2;i++)
				at[i]=try[l][i];
		}
		if(hf)
			for(unsigned int i=0;i<2;i++)
				at[i]=from[i];
		else
		{
			at[0]=118;
			at[1]=64;
		}
		for(unsigned int l=7;l>4;l--)
		{
			double r, th;
			rect_to_polar((const int *)at, (const int *)try[4], &r, &th);
			double dev=1.5/(1.0+r*0.04);
			r*=0.2+drandu(0.4);
			th+=drandu(dev*2)-dev;
			polar_to_rect(r, th, (const int *)at, (int *)try[l]);
			for(unsigned int i=0;i<2;i++)
				at[i]=try[l][i];
		}
		for(unsigned int l=0;l<7;l++)
		{
			clamp(try[l][0], 0, 256);
			clamp(try[l][1], 0, 256);
		}
		double score=1000;
		for(unsigned int l=0;l<7;l++)
		{
			double scare=0;
			for(unsigned int t=0;t<ntargs;t++)
			{
				if(t==ti) continue;
				if(!datewithin(state->now, targs[t].entry, targs[t].exit)) continue;
				double d, lambda;
				linedist(try[l+1][1]-try[l][1],
					try[l+1][0]-try[l][0],
					targs[t].lon-try[l][1],
					targs[t].lat-try[l][0],
					&d, &lambda);
				if(d<9)
				{
					double sv=targs[t].flak*0.6+state->heat[t]*3.0+3.6;
					double s=sv/(2.0+d);
					scare+=s;
				}
			}
			for(unsigned int f=0;f<nflaks;f++)
			{
				if(!datewithin(state->now, flaks[f].entry, flaks[f].exit)) continue;
				double d, lambda;
				linedist(try[l+1][1]-try[l][1],
					try[l+1][0]-try[l][0],
					flaks[f].lon-try[l][1],
					flaks[f].lat-try[l][0],
					&d, &lambda);
				if(d<3)
				{
					double s=flaks[f].strength*0.01/(1.0+d);
					scare+=s;
				}
			}
			score*=100.0/(100.0+scare);
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
