#pragma once
/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	date: date & time handling functions
*/
#include <stddef.h>
#include <stdbool.h>
#include <SDL.h>
#include "types.h"

date readdate(const char *t, date nulldate); // converts date from string
size_t writedate(date when, char *buf); // converts date to string, returns bytes written
harris_time readtime(const char *text, harris_time nulltime);
size_t writetime(harris_time when, char *buf);
bool datebefore(date date1, date date2); // returns true if date1 is strictly before date2
#define datewithin(now, start, end)		((!datebefore((now), (start)))&&datebefore((now), (end)))
int diffdate(date date1, date date2); // returns <0 if date1<date2, >0 if date1>date2, 0 if date1==date2
double seasonal_temp(date when);
double pom(date when); // returns in [0,1); 0 for new moon, 0.5 for full moon
double foldpom(double pom); // returns illumination in [0,1]
void drawmoon(SDL_Surface *s, double phase); // renders moon to image
harris_time maketime(int t); // converts run_raid time to clock time
unsigned int rrtime(harris_time t); // converts clock time to run_raid time
date nextday(date when); // computes the date 1 day after the given one
#define TM(H,M)		(harris_time){.hour=(H), .minute=(M)}
#define RRT(H,M)	rrtime(TM((H),(M)))
