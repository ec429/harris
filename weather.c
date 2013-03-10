/*
	harris - a strategy game
	Copyright (C) 2012-2013 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	weather: simulates weather patterns
*/
#include <stdlib.h>
#include <math.h>
#include "weather.h"
#include "rand.h"

#define max(a,b)	((a)>(b)?(a):(b))
#define min(a,b)	((a)<(b)?(a):(b))

void w_init(w_state * buf, unsigned int prep, bool lorw[128][128])
{
	if(buf!=NULL)
	{
		buf->push=1000;
		buf->slant=1;
		for(unsigned int x=0;x<256;x++)
		{
			for(unsigned int y=0;y<128;y++)
			{
				buf->p[x][y]=984+drandu(32.0);
				buf->t[x][y]=8+drandu(8.0); // this is not absolute Celsius, but relative to seasonal variations
			}
		}
		for(unsigned int p=0;p<prep;p++)
		{
			w_iter(buf, lorw);
		}
	}
}

void w_iter(w_state * ptr, bool lorw[128][128])
{
	if(ptr!=NULL)
	{
		ptr->push=(ptr->push*.9996)+drandu(0.8);
		ptr->slant=(ptr->slant*.9992)+drandu(0.0016);
		double tmp[256][128];
		for(unsigned int x=0;x<256;x++)
		{
			for(unsigned int y=0;y<128;y++)
			{
				ptr->t[x][y]=(ptr->t[x][y]*.96)+drandu(0.96);
				if(x) ptr->t[x][y]=(ptr->t[x][y]*.9+ptr->t[x-1][y]*.1);
				if(y) ptr->t[x][y]=(ptr->t[x][y]*.9+ptr->t[x][y-1]*.1);
				double base=(988+ptr->t[x][y]+ptr->push)/2.0;
				if((x<131)||(x>253))
					ptr->p[x][y]+=drandu(2.0)-1.0;
				else
					ptr->p[x][y]+=drandu(5.0)-2.5;
				double d2px=ptr->p[(x+255)%256][y]+ptr->p[(x+1)%256][y]-2*ptr->p[x][y];
				double d2py=ptr->p[x][max(y, 1)-1]+ptr->p[x][min(y+1, 127)]-2*ptr->p[x][y];
				double divp=d2px+d2py;
				double big=(abs(d2px)>abs(d2py))?d2px:d2py;
				tmp[x][y]=ptr->p[x][y]+divp*.09+big*.07+(ptr->p[(x+255)%256][y]-base)*.07;
				if(x<128)
				{
					if(lorw[x][y])
					{
						tmp[x][y]+=(tmp[x][y]-base)*0.05+(ptr->p[x][max(y, 1)-1]-base)*.04+(ptr->p[x][(y+1)%128]-base)*.04;
					}
					else tmp[x][y]-=(tmp[x][y]-base)*0.02;
				}
				tmp[x][y]-=pow((tmp[x][y]-base)*0.11, 3);
			}
		}
		if(brandp(0.2))
		{
			double bias=drandu(6.0)+6.0;
			if(brandp(0.5)) bias = -bias;
			double m=drandu(1.0)-0.5;
			for(int dx=-5;dx<6;dx++)
				//for(int dy=-7;dy<8;dy++)
				for(int y=0;y<128;y++)
					tmp[192+dx+(int)floor((y-64)*m)][y]+=bias*ptr->slant;
		}
		for(unsigned int x=0;x<256;x++)
		{
			for(unsigned int y=0;y<128;y++)
			{
				ptr->p[x][y]=min(max(tmp[x][y], 872), 1127);
			}
		}
	}
}
