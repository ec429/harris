# Makefile for Harris static binary

# Installation directories
DESTDIR ?=
PREFIX ?= /usr/local
BINIDIR := $(DESTDIR)$(PREFIX)/games
DATIDIR := $(DESTDIR)$(PREFIX)/share/games/harris
# User directories (relative to $HOME)
USAVDIR := .local/share/harris

LDFLAGS := -Llib/ -Wl,-rpath -Wl,lib

LIBS := -latg -lm
INTEL_OBJS := intel_bombers.o intel_fighters.o intel_targets.o
SCREEN_OBJS := main_menu.o setup_game.o setup_difficulty.o load_game.o save_game.o control.o run_raid.o raid_results.o post_raid.o $(INTEL_OBJS)
OBJS := weather.o bits.o rand.o geom.o widgets.o date.o history.o routing.o saving.o render.o events.o ui.o load_data.o dclass.o $(SCREEN_OBJS)
INCLUDES := $(OBJS:.o=.h) types.h globals.h version.h
SAVES := save/qstart.sav save/civ.sav save/abd.sav save/ruhr.sav

SDL := `sdl-config --libs` -lSDL_ttf -lSDL_gfx -lSDL_image
SDLFLAGS := `sdl-config --cflags`

harris:
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) harris.o -o $@ $(SDL)

install:
	install -d $(BINIDIR) $(DATIDIR)
	install harris $(BINIDIR)/
	./install.py -d $(DATIDIR)

uninstall:
	rm $(BINIDIR)/harris
	rm -r $(DATIDIR)
