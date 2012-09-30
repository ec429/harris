# Makefile for harris
PREFIX := /usr/local

CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic --std=gnu99 -g

LIBS := -latg -lm
OBJS := weather.o bits.o rand.o geom.o widgets.o
INCLUDES := $(OBJS:.o=.h) events.h

SDL := `sdl-config --libs` -lSDL_ttf -lSDL_image
SDLFLAGS := `sdl-config --cflags`

all: harris save/qstart.sav

harris: harris.o $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) $(LDFLAGS) $(SDL) $(OBJS) $(LIBS) harris.o -o $@

harris.o: harris.c $(INCLUDES)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) -o $@ -c $<

events.h: evh mkevh.sh
	sh ./mkevh.sh <evh >events.h

evh: dat/events mkevh.awk
	awk -f mkevh.awk -- dat/events >evh

widgets.o: widgets.c widgets.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) -o $@ -c $<

save/qstart.sav: gensave save/qstart.sav.in
	./gensave <save/qstart.sav.in >$@

gensave: gensave.c bits.h bits.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(LDFLAGS) bits.o -o $@

weather.o: rand.h rand.o

%.o: %.c %.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

