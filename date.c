/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	date: date & time handling functions
*/
#include "date.h"
#include <stdio.h>
#include <math.h>

#include "ui.h"
#include "bits.h"
#include "render.h"

date readdate(const char *t, date nulldate)
{
	date d;
	if(t&&*t&&(sscanf(t, "%u-%u-%d", &d.day, &d.month, &d.year)==3)) return(d);
	return(nulldate);
}

size_t writedate(date when, char *buf)
{
	if(!buf) return(0);
	return(sprintf(buf, "%02u-%02u-%04d", when.day, when.month, when.year));
}

bool datebefore(date date1, date date2) // returns true if date1 is strictly before date2
{
	if(date1.year!=date2.year) return(date1.year<date2.year);
	if(date1.month!=date2.month) return(date1.month<date2.month);
	return(date1.day<date2.day);
}

int diffdate(date date1, date date2) // returns <0 if date1<date2, >0 if date1>date2, 0 if date1==date2.  Value is difference in days
{
	if(date1.month<3)
	{
		date1.month+=12;
		date1.year--;
	}
	if(date2.month<3)
	{
		date2.month+=12;
		date2.year--;
	}
	int days1=365*date1.year + floor(date1.year/4) - floor(date1.year/100) + floor(date1.year/400) + floor(date1.day) + floor((153*date1.month+8)/5);
	int days2=365*date2.year + floor(date2.year/4) - floor(date2.year/100) + floor(date2.year/400) + floor(date2.day) + floor((153*date2.month+8)/5);
	return(days1-days2);
}

double pom(date when)
{
	// new moon at 14/08/1939
	// synodic month 29.530588853 days
	return(fmod(diffdate(when, (date){.day=14, .month=8, .year=1939})/29.530588853, 1));
}

double foldpom(double pom)
{
	return((1-cos(pom*M_PI*2))/2.0);
}

void drawmoon(SDL_Surface *s, double phase)
{
	SDL_FillRect(s, &(SDL_Rect){.x=0, .y=0, .w=s->w, .h=s->h}, SDL_MapRGB(s->format, GAME_BG_COLOUR.r, GAME_BG_COLOUR.g, GAME_BG_COLOUR.b));
	double left=(phase<0.5)?phase*2:1,
	      right=(phase>0.5)?phase*2-1:0;
	double halfx=(s->w-1)/2.0,
	       halfy=(s->h-1)/2.0;
	for(int y=1;y<s->h-1;y++)
	{
		double width=sqrt(((s->w/2)-1)*((s->w/2)-1)-(y-halfy)*(y-halfy));
		double startx=halfx-width, endx=halfx+width;
		double leftx=width*cos( left*M_PI)+halfx,
		      rightx=width*cos(right*M_PI)+halfx;
		for(unsigned int x=floor(startx);x<=ceil(endx);x++)
		{
			double a;
			if(x<startx)
				a=x+1-startx;
			else if(x>endx)
				a=endx+1-x;
			else
				a=1;
			unsigned int alpha=ceil(ATG_ALPHA_OPAQUE*a);
			double lf=x+1-leftx,
			       rf=rightx-x;
			clamp(lf, 0, 1);
			clamp(rf, 0, 1);
			unsigned int br=floor(lf*rf*224);
			pset(s, x, y, (atg_colour){br, br, br, alpha});
		}
	}
}

inline harris_time maketime(int t)
{
	return((harris_time){(12+(t/120))%24, (t/2)%60});
}

inline unsigned int rrtime(harris_time t)
{
	int h=t.hour-12;
	if(h<0) h+=24;
	return(h*120+t.minute*2);
}

const unsigned int monthdays[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
date nextday(date when)
{
	date d=when;
	if(++d.day>monthdays[d.month-1]+(((d.month==2)&&!(d.year%4))?1:0))
	{
		d.day=1;
		if(++d.month>12)
		{
			d.month=1;
			d.year++;
		}
	}
	return(d);
}
