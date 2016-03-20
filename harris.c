/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include "ui.h"
#include "events.h"
#include "load_data.h"
#include "globals.h"

/* UI screens */
#include "main_menu.h"
#include "setup_game.h"
#include "setup_difficulty.h"
#include "setup_types.h"
#include "load_game.h"
#include "save_game.h"
#include "control.h"
#include "run_raid.h"
#include "raid_results.h"
#include "post_raid.h"
#include "intel_bombers.h"
#include "intel_fighters.h"
#include "intel_targets.h"
#include "handle_crews.h"

#include "version.h"

//#define NOWEATHER	1	// skips weather phase of 'next-day' (w/o raid), allowing quicker time passage.  For testing/debugging

/* TODO
	Seasonal effects on weather
	More overlays for map display (incl. add new overlays for GEE, OBOE, GH range; perhaps also ability to show weather map as isobars/isotherms instead of cloudmap)
	Implement the remaining Navaids (just Gee-H now)
	Implement window-control (for diversions)
	Event effects on fighters (New radars, RCM, etc.)
	Sack player if confid or morale too low
*/

struct screen screens[NUM_SCREENS];

game state;

int main(int argc, char *argv[])
{
	int rc;
	
	for(int arg=1;arg<argc;arg++)
	{
#ifndef WINDOWS
		if(strcmp(argv[arg], "--localdat")==0)
		{
			localdat=true;
		}
		else if(strcmp(argv[arg], "--localsav")==0)
		{
			localsav=true;
		}
		else if(strcmp(argv[arg], "--local")==0)
		{
			localdat=localsav=true;
		}
		else
#endif
		{
			fprintf(stderr, "Unrecognised argument '%s'\n", argv[arg]);
			return(2);
		}
	}
	
#ifndef WINDOWS
	char cwd_buf[1024];
	if(!(cwd=getcwd(cwd_buf, 1024)))
	{
		perror("getcwd");
		return(1);
	}
#endif
	
	SDL_Init(SDL_INIT_VIDEO);
	
	// Load data files

	fprintf(stderr, "Loading data files...\n");
	
#ifndef WINDOWS
	if(chdir(localdat?cwd:DATIDIR))
	{
		perror("Failed to enter data dir: chdir");
		if(!localdat)
			fprintf(stderr, "(Maybe try with --localdat, or make install)\n");
		return(1);
	}
#endif

	// Set icon - must come before SDL_image usage and SDL_SetVideoMode
	SDL_Surface *icon=SDL_LoadBMP("art/icon.bmp");
	if(icon)
		SDL_WM_SetIcon(icon, NULL);

	rc = load_data();
	if (rc)
		return(rc);
	
	fprintf(stderr, "Data files loaded\n");
	
	fprintf(stderr, "Allocating game state...\n");
	
	if((rc = set_init_state(&state)))
		return(rc);

	srand(0);
	fprintf(stderr, "Game state allocated\n");
	
#ifndef WINDOWS
	if(localsav)
	{
		if(chdir(cwd))
		{
			perror("Failed to enter save dir");
			return(1);
		}
	}
	else
	{
		if(chdir(getenv("HOME")))
		{
			perror("Bad $HOME: chdir");
			return(1);
		}
		struct stat st_buf;
		if(stat(USAVDIR, &st_buf)&&errno==ENOENT)
		{
			fprintf(stderr, "save dir '"USAVDIR"' not found, creating it\n");
			char *s=strdup(USAVDIR);
			for(char *p=s;;p++)
			{
				char c=*p;
				if(c=='/' || !c)
				{
					*p=0;
					if(stat(s, &st_buf))
					{
						if(mkdir(s, 0755))
						{
							fprintf(stderr, "Failed to create '%s': mkdir: %s\n", s, strerror(errno));
							free(s);
							return(1);
						}
					}
				}
				*p=c;
				if(!c) break;
			}
			free(s);
		}
		if(chdir(USAVDIR))
		{
			perror("Failed to enter save dir: chdir");
			return(1);
		}
	}
#endif
	
	fprintf(stderr, "Instantiating GUI elements...\n");
		atg_canvas *canvas=atg_create_canvas_with_opts(1, 1, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE}, SDL_RESIZABLE);
	if(!canvas)
	{
		fprintf(stderr, "atg_create_canvas failed\n");
		return(1);
	}
	SDL_WM_SetCaption("Harris", "Harris");
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	#define MAKE_SCRN(t)	(struct screen){.name=#t, .create=t##_create, .func=t##_screen, .free=t##_free, .box=&t##_box}
	screens[SCRN_MAINMENU]=MAKE_SCRN(main_menu);
	screens[SCRN_SETPGAME]=MAKE_SCRN(setup_game);
	screens[SCRN_SETPDIFF]=MAKE_SCRN(setup_difficulty);
	screens[SCRN_SETPTYPS]=MAKE_SCRN(setup_types);
	screens[SCRN_LOADGAME]=MAKE_SCRN(load_game);
	screens[SCRN_SAVEGAME]=MAKE_SCRN(save_game);
	screens[SCRN_CONTROL] =MAKE_SCRN(control);
	screens[SCRN_RUNRAID] =MAKE_SCRN(run_raid);
	screens[SCRN_RRESULTS]=MAKE_SCRN(raid_results);
	screens[SCRN_POSTRAID]=MAKE_SCRN(post_raid);
	screens[SCRN_INTELBMB]=MAKE_SCRN(intel_bombers);
	screens[SCRN_INTELFTR]=MAKE_SCRN(intel_fighters);
	screens[SCRN_INTELTRG]=MAKE_SCRN(intel_targets);
	screens[SCRN_HCREWS]=MAKE_SCRN(handle_crews);
	#undef MAKE_SCRN

	for(unsigned int i=0;i<NUM_SCREENS;i++)
	{
		rc=screens[i].create();
		if(rc)
		{
			fprintf(stderr, "Failed to instantiate screen \"%s\"\n", screens[i].name);
			return(rc);
		}
	}
	fprintf(stderr, "Instantiated %d screens\n", NUM_SCREENS);
	
#ifndef WINDOWS
	if(chdir(localdat?cwd:DATIDIR)) // may need to load from $DATIDIR/save/
	{
		perror("Failed to enter data dir: chdir");
		return(1);
	}
#endif
	
	screen_id current=SCRN_MAINMENU, old;
	
	atg_free_element(canvas->content);
	
	while(current < NUM_SCREENS)
	{
		canvas->content=*screens[current].box;
		if((old=current)!=SCRN_MAINMENU)
			atg_resize_canvas(canvas, mainsizex, mainsizey);
		current=screens[current].func(canvas, &state);
		if(old!=SCRN_MAINMENU)
		{
			mainsizex=canvas->surface->w;
			mainsizey=canvas->surface->h;
		}
	}

	fprintf(stderr, "Exiting\n");
	canvas->content=NULL;
	atg_free_canvas(canvas);
	for(unsigned int i=0;i<NUM_SCREENS;i++)
		screens[i].free();
	SDL_FreeSurface(seltarg_overlay);
	SDL_FreeSurface(xhair_overlay);
	SDL_FreeSurface(weather_overlay);
	SDL_FreeSurface(target_overlay);
	SDL_FreeSurface(flak_overlay);
	SDL_FreeSurface(grey_overlay);
	SDL_FreeSurface(terrain);
	SDL_FreeSurface(location);
	return(0);
}
