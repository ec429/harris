#include <stdlib.h>
#include <math.h>
#include "weather.h"

#define max(a,b)	((a)>(b)?(a):(b))
#define min(a,b)	((a)<(b)?(a):(b))

void w_init(w_state * buf, unsigned int prep, bool lorw[128][128])
{
	if(buf!=NULL)
	{
		for(unsigned int x=0;x<256;x++)
		{
			for(unsigned int y=0;y<128;y++)
			{
				buf->p[x][y]=984+(rand()*32.0/RAND_MAX);
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
		double tmp[256][128];
		for(unsigned int x=0;x<256;x++)
		{
			for(unsigned int y=0;y<128;y++)
			{
				if((x<131)||(x>253))
					ptr->p[x][y]+=(rand()*2.0/RAND_MAX)-1.0;
				else
					ptr->p[x][y]+=(rand()*5.0/RAND_MAX)-2.5;
				double d2px=ptr->p[(x+255)%256][y]+ptr->p[(x+1)%256][y]-2*ptr->p[x][y];
				double d2py=ptr->p[x][max(y, 1)-1]+ptr->p[x][min(y+1, 127)]-2*ptr->p[x][y];
				double divp=d2px+d2py;
				double big=(abs(d2px)>abs(d2py))?d2px:d2py;
				tmp[x][y]=ptr->p[x][y]+divp*.09+big*.07+(ptr->p[(x+255)%256][y]-1000)*.07;
				if(x<128)
				{
					if(lorw[x][y])
					{
						tmp[x][y]+=(tmp[x][y]-1000)*0.05+(ptr->p[x][max(y, 1)-1]-1000)*.04+(ptr->p[x][(y+1)%128]-1000)*.04;
					}
					else tmp[x][y]-=(tmp[x][y]-1000)*0.02;
				}
				tmp[x][y]-=pow((tmp[x][y]-1000)*0.1, 3);
			}
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
