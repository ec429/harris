# Makefile for harris

CC ?= gcc
CFLAGS += -Wall -Wextra -Werror --std=gnu11 -g -I..

LIBS := -latg -lm
OBJS := ../hist_record.o ../bits.o ../crew.o
INCLUDES := $(OBJS:.o=.h) ../types.h ../dclass.h ../saving.h

SDL := `sdl-config --libs` -lSDL_ttf -lSDL_gfx -lSDL_image
SDLFLAGS := `sdl-config --cflags`

all: read_history como cshr

FORCE:
../hist_record.o: FORCE
	make -C .. hist_record.o
../bits.o: FORCE
	make -C .. bits.o
../crew.o: FORCE
	make -C .. crew.o

%: %.c $(OBJS) $(INCLUDES)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) $(LDFLAGS) $(LIBS) $< $(OBJS) -o $@ $(SDL)

