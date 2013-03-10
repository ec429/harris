/*
	harris - a strategy game
	Copyright (C) 2012-2013 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	date: date & time handling functions
*/
#include "date.h"
#include <stdio.h>
#include <math.h>

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
