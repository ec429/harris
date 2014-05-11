# Makefile for harris

# Installation directories
DESTDIR ?=
PREFIX ?= $(DESTDIR)/usr/local
BINIDIR := $(PREFIX)/games
DATIDIR := $(PREFIX)/share/games/harris
# User directories (relative to $HOME)
USAVDIR := .local/share/harris

CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic --std=gnu99 -g -DDATIDIR=\"$(DATIDIR)\" -DUSAVDIR=\"$(USAVDIR)\"

LIBS := -latg -lm
INTEL_OBJS := intel_bombers.o intel_fighters.o intel_targets.o
SCREEN_OBJS := main_menu.o load_game.o save_game.o control.o run_raid.o raid_results.o post_raid.o $(INTEL_OBJS)
OBJS := weather.o bits.o rand.o geom.o widgets.o date.o history.o routing.o saving.o render.o events.o ui.o load_data.o $(SCREEN_OBJS)
INCLUDES := $(OBJS:.o=.h) types.h globals.h version.h
SAVES := save/qstart.sav save/civ.sav save/abd.sav save/ruhr.sav

SDL := `sdl-config --libs` -lSDL_ttf -lSDL_gfx -lSDL_image
SDLFLAGS := `sdl-config --cflags`

all: harris $(SAVES)

install: all
	install -d $(BINIDIR) $(DATIDIR)/art/bombers $(DATIDIR)/art/bombloads $(DATIDIR)/art/fighters $(DATIDIR)/art/filters $(DATIDIR)/art/large/bombers $(DATIDIR)/art/large/fighters $(DATIDIR)/art/navaids $(DATIDIR)/dat/cities $(DATIDIR)/save $(DATIDIR)/map
	install harris $(BINIDIR)/
	install art/exit.png art/fullscreen.png art/intel.png art/location.png art/resize.png art/yellowhair.png $(DATIDIR)/art/
	install art/bombers/*.png $(DATIDIR)/art/bombers/
	install art/bombloads/*.png $(DATIDIR)/art/bombloads/
	install art/fighters/*.png $(DATIDIR)/art/fighters/
	install art/filters/*.png $(DATIDIR)/art/filters/
	install art/large/bombers/*.png $(DATIDIR)/art/large/bombers/
	install art/large/fighters/*.png $(DATIDIR)/art/large/fighters/
	install art/navaids/*.png $(DATIDIR)/art/navaids/
	install dat/bombers dat/events dat/fighters dat/flak dat/ftrbases dat/intel dat/targets dat/texts $(DATIDIR)/dat/
	install dat/cities/*.pbm $(DATIDIR)/dat/cities/
	install map/overlay_terrain.png map/overlay_coast.png map/overlay_water.png $(DATIDIR)/map/
	install $(SAVES) $(DATIDIR)/save/

clean:
	-rm harris harris.o $(OBJS) $(SAVES)

realclean: clean
	-rm events.h

harris: harris.o $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) harris.o -o $@ $(SDL)

harris.o: harris.c $(INCLUDES)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) -o $@ -c $<

events.h: dat/events mkevents.py
	./mkevents.py h >events.h

events.c: dat/events mkevents.py
	./mkevents.py c >events.c

widgets.o: widgets.c widgets.h bits.h types.h render.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) -o $@ -c $<

save/%.sav: save/%.sav.in gensave.py
	./gensave.py --salt $< <$< >$@

weather.o: rand.h

date.o: ui.h bits.h render.h

routing.o: rand.h globals.h events.h date.h geom.h

history.o: bits.h date.h types.h

saving.o: bits.h date.h globals.h events.h history.h rand.h weather.h version.h

render.o: bits.h globals.h events.h date.h

ui.o: types.h globals.h events.h date.h

load_data.o: globals.h events.h bits.h date.h render.h widgets.h

main_menu.o: ui.h globals.h events.h saving.h

load_game.o: ui.h globals.h events.h saving.h

save_game.o: ui.h globals.h events.h saving.h

control.o: ui.h globals.h events.h widgets.h date.h bits.h render.h intel_bombers.h intel_targets.h

run_raid.o: ui.h globals.h events.h date.h history.h render.h rand.h routing.h weather.h geom.h

raid_results.o: ui.h globals.h events.h bits.h date.h history.h weather.h run_raid.h

post_raid.o: ui.h globals.h events.h bits.h date.h history.h rand.h

intel_bombers.o: ui.h globals.h events.h bits.h date.h

intel_fighters.o: ui.h globals.h events.h bits.h date.h

intel_targets.o: ui.h globals.h events.h bits.h date.h render.h

%.o: %.c %.h types.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) -o $@ -c $<

