# Makefile for harris
PREFIX := /usr/local

CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic --std=gnu99 -g

LIBS := -latg -lm
OBJS := weather.o bits.o rand.o geom.o widgets.o date.o history.o routing.o
INCLUDES := $(OBJS:.o=.h) events.h types.h
SAVES := save/qstart.sav save/civ.sav save/abd.sav save/ruhr.sav

SDL := `sdl-config --libs` -lSDL_ttf -lSDL_gfx -lSDL_image
SDLFLAGS := `sdl-config --cflags`

all: harris $(SAVES)

clean:
	-rm harris $(OBJS) $(SAVES)

realclean: clean
	-rm events.h

harris: harris.o $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) harris.o -o $@ $(SDL)

harris.o: harris.c $(INCLUDES)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) -o $@ -c $<

events.h: dat/events mkevents.py
	./mkevents.py >events.h

widgets.o: widgets.c widgets.h bits.h types.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) -o $@ -c $<

save/%.sav: save/%.sav.in gensave.py
	./gensave.py --salt $< <$< >$@

weather.o: rand.h

routing.o: rand.h globals.h date.h geom.h

history.o: bits.h date.h types.h

%.o: %.c %.h types.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) -o $@ -c $<

