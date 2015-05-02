# Makefile for harris

# Installation directories
DESTDIR ?=
PREFIX ?= /usr/local
BINIDIR ?= $(PREFIX)/games
DATIDIR ?= $(PREFIX)/share/games/harris
# User directories (relative to $HOME)
USAVDIR := .local/share/harris

CC ?= gcc
CFLAGS += -Wall -Wextra -Werror -pedantic --std=gnu99 -g -DDATIDIR=\"$(DATIDIR)\" -DUSAVDIR=\"$(USAVDIR)\"

LIBS := -latg -lm
INTEL_OBJS := intel_bombers.o intel_fighters.o intel_targets.o
SCREEN_OBJS := main_menu.o setup_game.o setup_difficulty.o setup_types.o load_game.o save_game.o control.o run_raid.o raid_results.o post_raid.o $(INTEL_OBJS)
OBJS := weather.o bits.o rand.o geom.o widgets.o date.o history.o routing.o saving.o render.o events.o ui.o load_data.o dclass.o crew.o $(SCREEN_OBJS)
INCLUDES := $(OBJS:.o=.h) types.h globals.h version.h
SAVES := save/qstart.sav save/civ.sav save/abd.sav save/ruhr.sav

SDL := `sdl-config --libs` -lSDL_ttf -lSDL_gfx -lSDL_image
SDLFLAGS := `sdl-config --cflags`

all: harris $(SAVES)

install: all
	install -d $(DESTDIR)$(BINIDIR) $(DESTDIR)$(DATIDIR)
	install harris $(DESTDIR)$(BINIDIR)/
	./install.py -d $(DESTDIR)$(DATIDIR)

uninstall:
	rm $(DESTDIR)$(BINIDIR)/harris
	rm -r $(DESTDIR)$(DATIDIR)

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

widgets.o: widgets.c widgets.h bits.h date.h render.h

save/%.sav: save/%.sav.in gensave.py
	./gensave.py --salt $< <$< >$@

weather.o: rand.h

date.o: ui.h bits.h render.h

routing.o: rand.h globals.h events.h date.h geom.h

history.o: bits.h date.h

saving.o: bits.h date.h globals.h events.h history.h rand.h version.h

render.o: bits.h globals.h events.h date.h

ui.o: globals.h events.h date.h

load_data.o: globals.h events.h bits.h date.h render.h ui.h widgets.h

main_menu.o: ui.h globals.h events.h saving.h setup_game.h control.h

setup_game.o: ui.h globals.h events.h saving.h intel_bombers.h intel_fighters.h

setup_difficulty.o: ui.h globals.h events.h

setup_types.o: ui.h globals.h events.h control.h intel_bombers.h

load_game.o: ui.h globals.h events.h saving.h

save_game.o: ui.h globals.h events.h saving.h

control.o: ui.h globals.h events.h widgets.h date.h bits.h render.h routing.h weather.h intel_bombers.h intel_targets.h run_raid.h

run_raid.o: ui.h globals.h events.h date.h history.h render.h rand.h routing.h weather.h geom.h control.h

raid_results.o: ui.h globals.h events.h bits.h date.h history.h weather.h run_raid.h

post_raid.o: ui.h globals.h events.h bits.h date.h history.h rand.h control.h

intel_bombers.o: ui.h globals.h events.h bits.h date.h

intel_fighters.o: ui.h globals.h events.h bits.h date.h

intel_targets.o: ui.h globals.h events.h bits.h date.h render.h

%.o: %.c %.h types.h dclass.h crew.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SDLFLAGS) -o $@ -c $<

static: all
	mkdir static
	cp -r art dat lib map save stats *.o *.py README HOWTOPLAY STRATEGY COPYING static/
	mv static/lib/Makefile static/lib/INSTALL static/
	make -C static -f Makefile -B harris
	rm static/*.o

windows: all
	mkdir windows
	cp -r art dat map save stats *.c *.h *.o *.py README HOWTOPLAY STRATEGY COPYING windows/
	cp lib-w/* windows/
	make -C windows -f Makefile.w32 -B
	rm windows/*.o
