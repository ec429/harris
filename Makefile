# Makefile for harris

CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic --std=gnu99 -g
SDL := `sdl-config --libs` -lSDL_image
SDLFLAGS := `sdl-config --cflags`
BINDIR := bin/
SRCDIR := src/
INCDIR := inc/
LIBDIR := lib/

INCLUDES := $(INCDIR)weather.h
LIBS := $(LIBDIR)harris.o $(LIBDIR)weather.o

all: harris $(BINDIR)harris

harris: $(BINDIR)harris
	ln -s $(BINDIR)harris harris

$(BINDIR)harris: $(LIBS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) $(GTKFLAGS) $(LDFLAGS) $(SDL) $(GTK) -o $@ $(LIBS)

$(LIBDIR)harris.o: $(SRCDIR)harris.c $(INCLUDES)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) $(GTKFLAGS) -o $@ -c $<

$(LIBDIR)%.o: $(SRCDIR)%.c $(INCDIR)%.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

