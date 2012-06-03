# Makefile for harris

CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic --std=gnu99 -g

INCLUDES := weather.h bits.h rand.h atg/atg.h
LIBS := harris.o weather.o bits.o rand.o atg/atg.o

SDL := `sdl-config --libs` -lSDL_ttf -lSDL_image
SDLFLAGS := `sdl-config --cflags`

all: harris

harris: $(LIBS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) $(LDFLAGS) $(SDL) $(LIBS) -o $@

harris.o: harris.c $(INCLUDES)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) -o $@ -c $<

atg/atg.o: atg/atg.c atg/atg.h
	make -C atg

%.o: %.c %.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

