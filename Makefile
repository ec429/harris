# Makefile for harris

BINDIR := bin/
SRCDIR := src/
INCDIR := inc/
LIBDIR := lib/

CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic --std=gnu99 -g
CPPFLAGS := -I$(INCDIR)
GTK := `pkg-config gtk+-2.0 --libs`
GTKFLAGS := `pkg-config gtk+-2.0 --cflags`

INCLUDES := $(INCDIR)weather.h $(INCDIR)bits.h
LIBS := $(LIBDIR)harris.o $(LIBDIR)weather.o $(LIBDIR)bits.o

all: harris $(BINDIR)harris

harris: $(BINDIR)harris
	-ln -s $(BINDIR)harris harris

$(BINDIR)harris: $(LIBS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GTKFLAGS) $(LDFLAGS) $(GTK) $(LIBS) -o $@

$(LIBDIR)harris.o: $(SRCDIR)harris.c $(INCLUDES)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GTKFLAGS) -o $@ -c $<

$(LIBDIR)%.o: $(SRCDIR)%.c $(INCDIR)%.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

