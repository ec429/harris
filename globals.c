/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	globals: game entity data
*/

#include "globals.h"
#include "run_raid.h"
#include "ui.h"

const char * const navaids[NNAVAIDS]={"GEE","H2S","OBOE","GH"};
const char * const navpicfn[NNAVAIDS]={"art/navaids/gee.png", "art/navaids/h2s.png", "art/navaids/oboe.png", "art/navaids/g-h.png"};
unsigned int navevent[NNAVAIDS]={EVENT_GEE, EVENT_H2S, EVENT_OBOE, EVENT_GH};
unsigned int navprod[NNAVAIDS]={3, 7, 22, 12}; // 10/productionrate; later 25/productionrate

double todays_delta, todays_eqn; // values for the almanack

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

struct overlay overlays[NUM_OVERLAYS] = {
	[OVERLAY_CITY] = {.ifn="art/overlays/cities.png", .selected=true},
	[OVERLAY_FLAK] = {.ifn="art/overlays/flak.png", .selected=true},
	[OVERLAY_TARGET] = {.ifn="art/overlays/targets.png", .selected=true},
	[OVERLAY_WEATHER] = {.ifn="art/overlays/weather.png", .selected=true},
	[OVERLAY_ROUTE] = {.ifn="art/overlays/routes.png", .selected=true},
};

unsigned int ntypes=0;
bombertype *types=NULL;
bombertype *rawtypes=NULL;
unsigned int ntechs=0;
tech *techs=NULL;
unsigned int nbtechs=0;
btech *btechs=NULL;
unsigned int netechs=0;
etech *etechs=NULL;
unsigned int nftypes=0;
fightertype *ftypes=NULL;
unsigned int nfbases=0;
ftrbase *fbases=NULL;
unsigned int nlocs=0;
locxn *locs=NULL;
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
SDL_Surface *elitepic=NULL;
SDL_Surface *studentpic=NULL;
SDL_Surface *resizebtn=NULL;
SDL_Surface *fullbtn=NULL;
SDL_Surface *exitbtn=NULL;
SDL_Surface *tick=NULL, *cross=NULL;

SDL_Surface *intelscreenbtn[3];

SDL_Surface *grey_overlay=NULL, *yellow_overlay=NULL;
SDL_Surface *weather_overlay=NULL, *sun_overlay=NULL, *city_overlay=NULL, *target_overlay=NULL, *flak_overlay=NULL, *route_overlay=NULL, *xhair_overlay=NULL, *seltarg_overlay=NULL;

bool lorw[128][128]; // TRUE for water
unsigned char tnav[128][128]; // Recognisability of terrain.  High for rivers, even higher for coastline

unsigned int nregions=0;
struct region *regions;
unsigned int region[256][256];

unsigned int mainsizex=default_w, mainsizey=default_h;
bool fullscreen=false;

#ifndef WINDOWS
bool localdat=false, localsav=false;
char *cwd;
#endif

int set_init_state(game *state)
{
	state->nbombers=state->nfighters=0;
	state->bombers=NULL;
	state->fighters=NULL;
	state->ntargs=ntargs;
	if(!(state->dmg=calloc(ntargs, sizeof(*state->dmg))))
	{
		perror("calloc");
		return(1);
	}
	if(!(state->flk=calloc(ntargs, sizeof(*state->flk))))
	{
		perror("calloc");
		return(1);
	}
	if(!(state->heat=calloc(ntargs, sizeof(*state->heat))))
	{
		perror("calloc");
		return(1);
	}
	if(!(state->flam=calloc(ntargs, sizeof(*state->flam))))
	{
		perror("calloc");
		return(1);
	}
	if(!(state->raids=malloc(ntargs*sizeof(*state->raids))))
	{
		perror("malloc");
		return(1);
	}
	if(!(state->btypes=malloc(ntypes*sizeof(*state->btypes))))
	{
		perror("malloc");
		return(1);
	}
	for(unsigned int n=0;n<NNAVAIDS;n++)
	{
		state->nap[n]=0;
		state->napb[n]=0;
	}
	for(unsigned int i=0;i<ntargs;i++)
	{
		state->raids[i].bombers=NULL;
		if(!(state->raids[i].loads=calloc(ntypes, sizeof(bombload))))
		{
			perror("malloc");
			return(1);
		}
		if(!(state->raids[i].pffloads=calloc(ntypes, sizeof(bombload))))
		{
			perror("malloc");
			return(1);
		}
		for(unsigned int j=0;j<ntypes;j++)
		{
			state->raids[i].loads[j]=BL_USUAL;
			int limit=0;
			if(types[j].pff)
			{
				if(types[j].noarm)
					state->raids[i].pffloads[j]=BL_ILLUM;
				else
					state->raids[i].pffloads[j]=BL_PPLUS;
			}
			else
				state->raids[i].pffloads[j]=0; // doesn't matter
			while(!types[j].load[state->raids[i].loads[j]])
			{
				state->raids[i].loads[j]=(state->raids[i].loads[j]+1)%NBOMBLOADS;
				if(++limit>=NBOMBLOADS)
				{
					fprintf(stderr, "No valid bombloads for type %s\n", types[j].name);
					return(1);
				}
			}
			while(!types[j].load[state->raids[i].pffloads[j]])
			{
				state->raids[i].pffloads[j]=(state->raids[i].pffloads[j]+1)%NBOMBLOADS;
				if(++limit>=NBOMBLOADS)
				{
					fprintf(stderr, "No valid PFF bombloads for type %s\n", types[j].name);
					return(1);
				}
			}
		}
	}
	for(unsigned int t=0;t<ntechs;t++)
		techs[t].unlocked=(date){9999, 99, 99};
	clear_raids(state);
	state->roe.idtar=true;
	for(unsigned int i=0;i<MAXMSGS;i++)
		state->msg[i]=NULL;
	
	state->hist.nents=0;
	state->hist.nalloc=0;
	state->hist.ents=NULL;

	for(unsigned int i=0;i<2;i++)
	{
		state->tfav[i]=T_CLASSES;
		state->tfd[i]=0;
		state->ifav[i]=I_CLASSES;
		state->ifd[i]=0;
	}
	
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
	return(0);
}
