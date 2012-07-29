# Makefile for harris
PREFIX := /usr/local

CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic --std=gnu99 -g

LIBS := -latg
OBJS := weather.o bits.o rand.o widgets.o
INCLUDES := $(OBJS:.o=.h)

SDL := `sdl-config --libs` -lSDL_ttf -lSDL_image
SDLFLAGS := `sdl-config --cflags`

all: harris

harris: harris.o $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) $(LDFLAGS) $(SDL) $(OBJS) $(LIBS) harris.o -o $@

harris.o: harris.c $(INCLUDES)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) -o $@ -c $<

widgets.o: widgets.c widgets.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) -o $@ -c $<

%.o: %.c %.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

