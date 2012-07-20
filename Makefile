# Makefile for harris

CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic --std=gnu99 -g

LIBS := weather.o bits.o rand.o atg/atg.o widgets.o
INCLUDES := $(LIBS:.o=.h) atg/atg_internals.h

SDL := `sdl-config --libs` -lSDL_ttf -lSDL_image
SDLFLAGS := `sdl-config --cflags`

all: harris

harris: harris.o $(LIBS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) $(LDFLAGS) $(SDL) $(LIBS) harris.o -o $@

harris.o: harris.c $(INCLUDES)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) -o $@ -c $<

atg/atg.o: atg/atg.c atg/atg.h
	make -C atg

widgets.o: widgets.c widgets.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) -o $@ -c $<

%.o: %.c %.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

