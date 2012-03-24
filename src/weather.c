#include <stdlib.h>
#include "weather.h"

#define max(a,b)	((a)>(b)?(a):(b))
#define min(a,b)	((a)<(b)?(a):(b))
#define sgn(x)		((x)>0?1:(x)<0?-1:0)

void w_init(w_state * buf, int prep)
{
	if(buf!=NULL)
	{
		int x,y,t;
		for(x=0;x<128;x++)
		{
			for(y=0;y<128;y++)
			{
				for(t=0;t<3;t++)
				{
					buf->p[x][y][t]=984+(rand()*32.0/RAND_MAX);
				}
			}
		}
		buf->fx=-16;
		buf->fc=1.5;
		buf->fdy=0.3;
		int p;
		for(p=0;p<prep;p++)
		{
			w_iter(buf);
		}
	}
}

void w_iter(w_state * ptr)
{
	if(ptr!=NULL)
	{
		int x,y;
		double r1=(ptr->p[0][0][2]*16.0)-floor(ptr->p[0][0][2]*16.0);
		double r2=(ptr->p[1][0][2]*16.0)-floor(ptr->p[1][0][2]*16.0);
		double r3=(ptr->p[2][0][2]*16.0)-floor(ptr->p[2][0][2]*16.0);
		double r4=(ptr->p[3][0][2]*16.0)-floor(ptr->p[3][0][2]*16.0);
		double r5=(ptr->p[4][0][2]*16.0)-floor(ptr->p[4][0][2]*16.0);
		double tmp[128][128];
		double fd=(r1*4.0);
		ptr->fx+=(r2*4.5);
		if(ptr->fx>180)
		{
			ptr->fx=(r3*64.0)-16.0;
			ptr->fdy=(r4*1.6)-0.8;
		}
		else
			ptr->fdy=(ptr->fdy*.99)+(r4-0.5)*0.02;
		ptr->fc=(ptr->fc*.95)+(r5*0.15);
		for(x=0;x<128;x++)
		{
			for(y=0;y<128;y++)
			{
				double r6=(ptr->p[x][y][2]*16.0)-floor(ptr->p[x][y][2]*16.0);
				double ns;
				int dx,dy,nsn;
				ns=0;
				nsn=0;
				for(dx=-8;dx<=8;dx++)
				{
					for(dy=-8;dy<=8;dy++)
					{
						if((x+dx>=0) && (x+dx<128) && (y+dy>=0) && (y+dy<128))
						{
							ns+=ptr->p[x+dx][y+dy][0]*(2-sgn(dx));
							nsn+=(2-sgn(dx));
						}
					}
				}
				ns/=nsn;
				double d2p=ptr->p[x][y][0]-(2.0*ptr->p[x][y][1])+ptr->p[x][y][2];
				double dp=ptr->p[x][y][0]-ptr->p[x][y][1];
				ptr->p[x][y][2]=ptr->p[x][y][1];
				ptr->p[x][y][1]=ptr->p[x][y][0];
				tmp[x][y]=(ptr->p[x][y][0]*.96)+40.14+(d2p*0.16)-(0.15*dp)+(sqrt(abs(ns-ptr->p[x][y][0]))*sgn(ns-ptr->p[x][y][0])*1.8)+(r6*16.0)-8;
				tmp[x][y]-=(fd/(2.0+abs(ptr->fx-x-pow((64+((y-64)*ptr->fdy))*0.25, ptr->fc))));
			}
		}
		for(x=0;x<128;x++)
		{
			for(y=0;y<128;y++)
			{
				ptr->p[x][y][0]=min(max(tmp[x][y], 872), 1127);
			}
		}
	}
}
