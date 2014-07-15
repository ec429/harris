/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

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

#include "main_menu.h"
#include "setup_game.h"
#include "setup_difficulty.h"
#include "load_game.h"
#include "save_game.h"
#include "control.h"
#include "run_raid.h"
#include "raid_results.h"
#include "post_raid.h"
#include "intel_bombers.h"
#include "intel_fighters.h"
#include "intel_targets.h"

#include "version.h"

//#define NOWEATHER	1	// skips weather phase of 'next-day' (w/o raid), allowing quicker time passage.  For testing/debugging

/* TODO
	Seasonal effects on weather
	Toggle overlays on map display (incl. add new overlays for GEE, OBOE, GH range; perhaps also ability to show weather map as isobars/isotherms instead of cloudmap)
	Make Flak only be known to you after you've encountered it
	Implement the remaining Navaids (just Gee-H now)
	Implement window-control (for diversions)
	Event effects on fighters (New radars, RCM, etc.)
	Sack player if confid or morale too low
*/

const char * const navaids[NNAVAIDS]={"GEE","H2S","OBOE","GH"};
const char * const navpicfn[NNAVAIDS]={"art/navaids/gee.png", "art/navaids/h2s.png", "art/navaids/oboe.png", "art/navaids/g-h.png"};
unsigned int navevent[NNAVAIDS]={EVENT_GEE, EVENT_H2S, EVENT_OBOE, EVENT_GH};
unsigned int navprod[NNAVAIDS]={3, 7, 22, 12}; // 10/productionrate; later 25/productionrate

date event[NEVENTS];
char *evtext[NEVENTS];

struct oboe oboe={.lat=95, .lon=63, .k=-1};
struct gee gee={.lat=107, .lon=64, .jrange=65};

struct bombloadinfo bombloads[NBOMBLOADS]=
{
	[BL_ABNORMAL]={.name="Ab", .fn="art/bombloads/abnormal.png"},
	[BL_PPLUS	]={.name="Pp", .fn="art/bombloads/plumduff-plus.png"},
	[BL_PLUMDUFF]={.name="Pd", .fn="art/bombloads/plumduff.png"},
	[BL_PONLY	]={.name="Po", .fn="art/bombloads/plumduff-only.png"},
	[BL_USUAL	]={.name="Us", .fn="art/bombloads/usual.png"},
	[BL_ARSON	]={.name="Ar", .fn="art/bombloads/arson.png"},
	[BL_ILLUM	]={.name="Il", .fn="art/bombloads/illuminator.png"},
	[BL_HALFHALF]={.name="Hh", .fn="art/bombloads/halfandhalf.png"},
};

struct screen screens[NUM_SCREENS];

unsigned int ntypes=0;
bombertype *types=NULL;
unsigned int nftypes=0;
fightertype *ftypes=NULL;
unsigned int nfbases=0;
ftrbase *fbases=NULL;
unsigned int ntargs=0;
target *targs=NULL;
unsigned int nflaks=0;
flaksite *flaks=NULL;
unsigned int nstarts=0;
startpoint *starts=NULL;

SDL_Surface *terrain=NULL;
SDL_Surface *location=NULL;
SDL_Surface *yellowhair=NULL;
SDL_Surface *intelbtn=NULL;
SDL_Surface *nointelbtn=NULL;
SDL_Surface *navpic[NNAVAIDS];
SDL_Surface *pffpic=NULL;
SDL_Surface *resizebtn=NULL;
SDL_Surface *fullbtn=NULL;
SDL_Surface *exitbtn=NULL;

SDL_Surface *grey_overlay=NULL, *yellow_overlay=NULL;
SDL_Surface *weather_overlay=NULL, *target_overlay=NULL, *flak_overlay=NULL, *xhair_overlay=NULL, *seltarg_overlay=NULL;

bool lorw[128][128]; // TRUE for water
unsigned char tnav[128][128]; // Recognisability of terrain.  High for rivers, even higher for coastline

unsigned int mainsizex=default_w, mainsizey=default_h;
bool fullscreen=false;

bool localdat=false, localsav=false;
char *cwd;

game state;

int main(int argc, char *argv[])
{
	int rc;
	
	for(int arg=1;arg<argc;arg++)
	{
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
		{
			fprintf(stderr, "Unrecognised argument '%s'\n", argv[arg]);
			return(2);
		}
	}
	
	char cwd_buf[1024];
	if(!(cwd=getcwd(cwd_buf, 1024)))
	{
		perror("getcwd");
		return(1);
	}
	
	SDL_Init(SDL_INIT_VIDEO);
	
	// Load data files
	fprintf(stderr, "Loading data files...\n");
	
	if(chdir(localdat?cwd:DATIDIR))
	{
		perror("Failed to enter data dir: chdir");
		return(1);
	}

	// Set icon - must come before SDL_image usage and SDL_SetVideoMode
	SDL_Surface *icon=SDL_LoadBMP("art/icon.bmp");
	if(icon)
		SDL_WM_SetIcon(icon, NULL);

	if((rc=load_bombers()))
	{
		fprintf(stderr, "Failed to load bombers, rc=%d\n", rc);
		return(rc);
	}
	if((rc=load_fighters()))
	{
		fprintf(stderr, "Failed to load fighters, rc=%d\n", rc);
		return(rc);
	}
	if((rc=load_ftrbases()))
	{
		fprintf(stderr, "Failed to load ftrbases, rc=%d\n", rc);
		return(rc);
	}
	if((rc=load_targets()))
	{
		fprintf(stderr, "Failed to load targets, rc=%d\n", rc);
		return(rc);
	}
	if((rc=load_flaksites()))
	{
		fprintf(stderr, "Failed to load flaksites, rc=%d\n", rc);
		return(rc);
	}
	if((rc=load_events()))
	{
		fprintf(stderr, "Failed to load events, rc=%d\n", rc);
		return(rc);
	}
	if((rc=load_texts()))
	{
		fprintf(stderr, "Failed to load texts, rc=%d\n", rc);
		return(rc);
	}
	if((rc=load_intel()))
	{
		fprintf(stderr, "Failed to load intel, rc=%d\n", rc);
		return(rc);
	}
	if((rc=load_images()))
	{
		fprintf(stderr, "Failed to load images, rc=%d\n", rc);
		return(rc);
	}
	if((rc=load_starts()))
	{
		fprintf(stderr, "Failed to load startpoints, rc=%d\n", rc);
		return(rc);
	}
	
	fprintf(stderr, "Data files loaded\n");
	
	fprintf(stderr, "Allocating game state...\n");
	
	state.nbombers=state.nfighters=0;
	state.bombers=NULL;
	state.fighters=NULL;
	state.ntargs=ntargs;
	if(!(state.dmg=calloc(ntargs, sizeof(*state.dmg))))
	{
		perror("calloc");
		return(1);
	}
	if(!(state.flk=calloc(ntargs, sizeof(*state.flk))))
	{
		perror("calloc");
		return(1);
	}
	if(!(state.heat=calloc(ntargs, sizeof(*state.heat))))
	{
		perror("calloc");
		return(1);
	}
	if(!(state.flam=calloc(ntargs, sizeof(*state.flam))))
	{
		perror("calloc");
		return(1);
	}
	if(!(state.raids=malloc(ntargs*sizeof(*state.raids))))
	{
		perror("malloc");
		return(1);
	}
	for(unsigned int n=0;n<NNAVAIDS;n++)
	{
		state.nap[n]=0;
		state.napb[n]=0;
	}
	for(unsigned int i=0;i<ntargs;i++)
	{
		state.raids[i].nbombers=0;
		state.raids[i].bombers=NULL;
		if(!(state.raids[i].loads=calloc(ntypes, sizeof(bombload))))
		{
			perror("malloc");
			return(1);
		}
		if(!(state.raids[i].pffloads=calloc(ntypes, sizeof(bombload))))
		{
			perror("malloc");
			return(1);
		}
		for(unsigned int j=0;j<ntypes;j++)
		{
			state.raids[i].loads[j]=BL_USUAL;
			int limit=0;
			if(types[j].pff)
			{
				if(types[j].noarm)
					state.raids[i].pffloads[j]=BL_ILLUM;
				else
					state.raids[i].pffloads[j]=BL_PPLUS;
			}
			else
				state.raids[i].pffloads[j]=0; // doesn't matter
			while(!types[j].load[state.raids[i].loads[j]])
			{
				state.raids[i].loads[j]=(state.raids[i].loads[j]+1)%NBOMBLOADS;
				if(++limit>=NBOMBLOADS)
				{
					fprintf(stderr, "No valid bombloads for type %s\n", types[j].name);
					return(1);
				}
			}
			while(!types[j].load[state.raids[i].pffloads[j]])
			{
				state.raids[i].pffloads[j]=(state.raids[i].pffloads[j]+1)%NBOMBLOADS;
				if(++limit>=NBOMBLOADS)
				{
					fprintf(stderr, "No valid PFF bombloads for type %s\n", types[j].name);
					return(1);
				}
			}
		}
	}
	state.roe.idtar=true;
	for(unsigned int i=0;i<MAXMSGS;i++)
		state.msg[i]=NULL;
	
	state.hist.nents=0;
	state.hist.nalloc=0;
	state.hist.ents=NULL;
	
	if(!(dij=malloc(ntargs*sizeof(int *))))
	{
		perror("malloc");
		return(1);
	}
	if(!(nij=malloc(ntargs*sizeof(int *))))
	{
		perror("malloc");
		return(1);
	}
	if(!(tij=malloc(ntargs*sizeof(int *))))
	{
		perror("malloc");
		return(1);
	}
	if(!(lij=malloc(ntargs*sizeof(int *))))
	{
		perror("malloc");
		return(1);
	}
	for(unsigned int i=0;i<ntargs;i++)
	{
		if(!(dij[i]=calloc(ntypes, sizeof(int))))
		{
			perror("calloc");
			return(1);
		}
		if(!(nij[i]=calloc(ntypes, sizeof(int))))
		{
			perror("calloc");
			return(1);
		}
		if(!(tij[i]=calloc(ntypes, sizeof(int))))
		{
			perror("calloc");
			return(1);
		}
		if(!(lij[i]=calloc(ntypes, sizeof(int))))
		{
			perror("calloc");
			return(1);
		}
	}
	if(!(heat=calloc(ntargs, sizeof(unsigned int))))
	{
		perror("calloc");
		return(1);
	}
	if(!(canscore=calloc(ntargs, sizeof(bool))))
	{
		perror("calloc");
		return(1);
	}

	srand(0);
	fprintf(stderr, "Game state allocated\n");
	
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
	screens[SCRN_LOADGAME]=MAKE_SCRN(load_game);
	screens[SCRN_SAVEGAME]=MAKE_SCRN(save_game);
	screens[SCRN_CONTROL] =MAKE_SCRN(control);
	screens[SCRN_RUNRAID] =MAKE_SCRN(run_raid);
	screens[SCRN_RRESULTS]=MAKE_SCRN(raid_results);
	screens[SCRN_POSTRAID]=MAKE_SCRN(post_raid);
	screens[SCRN_INTELBMB]=MAKE_SCRN(intel_bombers);
	screens[SCRN_INTELFTR]=MAKE_SCRN(intel_fighters);
	screens[SCRN_INTELTRG]=MAKE_SCRN(intel_targets);
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
	
	if(chdir(localdat?cwd:DATIDIR)) // may need to load from $DATIDIR/save/
	{
		perror("Failed to enter data dir: chdir");
		return(1);
	}
	
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
