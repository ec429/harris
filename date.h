#pragma once
/*
	harris - a strategy game
	Copyright (C) 2012-2013 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	date: date & time handling functions
*/
#include <stddef.h>
#include <stdbool.h>

typedef struct
{
	int year;
	unsigned int month; // 1-based
	unsigned int day; // also 1-based
}
date;

typedef struct
{
	unsigned int hour;
	unsigned int minute;
}
time;

date readdate(const char *t, date nulldate); // converts date from string
size_t writedate(date when, char *buf); // converts date to string, returns bytes written
bool datebefore(date date1, date date2); // returns true if date1 is strictly before date2
#define datewithin(now, start, end)		((!datebefore((now), (start)))&&datebefore((now), (end)))
int diffdate(date date1, date date2); // returns <0 if date1<date2, >0 if date1>date2, 0 if date1==date2
double pom(date when); // returns in [0,1); 0 for new moon, 0.5 for full moon
double foldpom(double pom); // returns illumination in [0,1]
time maketime(int t); // converts run_raid time to clock time
