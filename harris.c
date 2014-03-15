/*
	harris - a strategy game
	Copyright (C) 2012-2013 Edward Cree

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
#include <math.h>

#include <atg.h>
#include <SDL_image.h>
#include <SDL_gfxBlitFunc.h>

#include "bits.h"
#include "weather.h"
#include "rand.h"
#include "geom.h"
#include "widgets.h"
#include "events.h"
#include "history.h"
#include "date.h"
#include "types.h"
#include "routing.h"

//#define NOWEATHER	1	// skips weather phase of 'next-day' (w/o raid), allowing quicker time passage.  For testing/debugging

/* TODO
	Seasonal effects on weather
	Toggle overlays on map display (incl. add new overlays for GEE, OBOE, GH range; perhaps also ability to show weather map as isobars/isotherms instead of cloudmap)
	Make Flak only be known to you after you've encountered it
	Implement the remaining Navaids (just Gee-H now)
	Implement window-control (for diversions)
	Event effects on fighters (New radars, RCM, etc.)
	Refactoring, esp. of the GUI building, and splitting up this file (it's *far* too long right now)
	Sack player if confid or morale too low
*/

#define NAV_GEE		0
#define NAV_H2S		1
#define NAV_OBOE	2
#define NAV_GH		3
const char * const navaids[NNAVAIDS]={"GEE","H2S","OBOE","GH"};
const char * const navpicfn[NNAVAIDS]={"art/navaids/gee.png", "art/navaids/h2s.png", "art/navaids/oboe.png", "art/navaids/g-h.png"};
unsigned int navevent[NNAVAIDS]={EVENT_GEE, EVENT_H2S, EVENT_OBOE, EVENT_GH};
unsigned int navprod[NNAVAIDS]={3, 8, 22, 12}; // 10/productionrate; later 25/productionrate

date event[NEVENTS];
char *evtext[NEVENTS];

struct oboe
{
	signed int lat, lon;
	signed int k; // bomber number, or -1 for none
}
oboe={.lat=95, .lon=63, .k=-1};

struct gee
{
	signed int lat, lon;
	unsigned int range, jrange;
}
gee={.lat=107, .lon=64, .jrange=65};

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

#define GAME_BG_COLOUR	(atg_colour){31, 31, 15, ATG_ALPHA_OPAQUE}

#define default_w		800
#define default_h		720

#define RS_cell_w		112
#define RS_firstcol_w	128
#define RS_cell_h		56
#define RS_lastrow_h	100

#define VER_MAJ	0
#define VER_MIN	1
#define VER_REV	0

int loadgame(const char *fn, game *state, bool lorw[128][128]);
int savegame(const char *fn, game state);
int msgadd(atg_canvas *canvas, game *state, date when, const char *ref, const char *msg);
void message_box(atg_canvas *canvas, const char *titletext, const char *bodytext, const char *signtext);
void drawmoon(SDL_Surface *s, double phase);
bool version_newer(const unsigned char v1[3], const unsigned char v2[3]); // true iff v1 newer than v2
SDL_Surface *render_weather(w_state weather);
SDL_Surface *render_targets(date now);
SDL_Surface *render_flak(date now);
SDL_Surface *render_ac(game state);
SDL_Surface *render_xhairs(game state, int seltarg);
unsigned int ntypes=0;
void update_navbtn(game state, atg_element *GB_navbtn[ntypes][NNAVAIDS], unsigned int i, unsigned int n, SDL_Surface *grey_overlay, SDL_Surface *yellow_overlay);
bool filter_apply(ac_bomber b, int filter_pff, int filter_nav[NNAVAIDS]);
void produce(int targ, game *state, double amount);
bombertype *types=NULL;
unsigned int nftypes=0;
fightertype *ftypes=NULL;
unsigned int nfbases=0;
ftrbase *fbases=NULL;
unsigned int ntargs=0;
target *targs=NULL;
unsigned int nflaks=0;
flaksite *flaks=NULL;
SDL_Surface *terrain=NULL;
SDL_Surface *location=NULL;
SDL_Surface *yellowhair=NULL;
SDL_Surface *intelbtn=NULL;
SDL_Surface *navpic[NNAVAIDS];
SDL_Surface *pffpic=NULL;
SDL_Surface *resizebtn=NULL;
SDL_Surface *fullbtn=NULL;
SDL_Surface *exitbtn=NULL;

unsigned int mainsizex=default_w, mainsizey=default_h;
bool fullscreen=false;

int main(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[])
{
	atg_canvas *canvas=atg_create_canvas_with_opts(136, 24, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE}, SDL_RESIZABLE);
	if(!canvas)
	{
		fprintf(stderr, "atg_create_canvas failed\n");
		return(1);
	}
	SDL_WM_SetCaption("Harris", "Harris");
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	
	srand(0);
	
	// Load data files
	fprintf(stderr, "Loading data files...\n");
	FILE *typefp=fopen("dat/bombers", "r");
	if(!typefp)
	{
		fprintf(stderr, "Failed to open data file `bombers'!\n");
		return(1);
	}
	else
	{
		char *typefile=slurp(typefp);
		fclose(typefp);
		char *next=typefile?strtok(typefile, "\n"):NULL;
		while(next)
		{
			if(*next&&(*next!='#'))
			{
				bombertype this;
				// MANUFACTURER:NAME:COST:SPEED:CEILING:CAPACITY:SVP:DEFENCE:FAILURE:ACCURACY:RANGE:BLAT:BLONG:DD-MM-YYYY:DD-MM-YYYY:NAVAIDS,FLAGS
				this.name=strdup(next); // guarantees that enough memory will be allocated
				this.manu=(char *)malloc(strcspn(next, ":")+1);
				ssize_t db;
				int e;
				if((e=sscanf(next, "%[^:]:%[^:]:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%zn", this.manu, this.name, &this.cost, &this.speed, &this.alt, &this.cap, &this.svp, &this.defn, &this.fail, &this.accu, &this.range, &this.blat, &this.blon, &db))!=13)
				{
					fprintf(stderr, "Malformed `bombers' line `%s'\n", next);
					fprintf(stderr, "  sscanf returned %d\n", e);
					return(1);
				}
				size_t nlen=strlen(this.name)+1;
				this.name=realloc(this.name, nlen);
				this.entry=readdate(next+db, (date){0, 0, 0});
				this.novelty=this.entry;
				this.novelty.month+=4;
				if(this.novelty.month>12)
				{
					this.novelty.month-=12;
					this.novelty.year++;
				}
				const char *exit=strchr(next+db, ':');
				if(!exit)
				{
					fprintf(stderr, "Malformed `bombers' line `%s'\n", next);
					fprintf(stderr, "  missing :EXIT\n");
					return(1);
				}
				exit++;
				this.exit=readdate(exit, (date){9999, 99, 99});
				const char *nav=strchr(exit, ':');
				if(!nav)
				{
					fprintf(stderr, "Malformed `bombers' line `%s'\n", next);
					fprintf(stderr, "  missing :NAVAIDS\n");
					return(1);
				}
				nav++;
				for(unsigned int i=0;i<NNAVAIDS;i++)
					this.nav[i]=strstr(nav, navaids[i]);
				this.noarm=strstr(nav, "NOARM");
				this.pff=strstr(nav, "PFF");
				this.heavy=strstr(nav, "HEAVY");
				this.inc=strstr(nav, "INC");
				this.broughton=strstr(nav, "BROUGHTON");
				for(unsigned int l=0;l<NBOMBLOADS;l++)
					this.load[l]=strstr(nav, bombloads[l].name);
				char pn[12+nlen+4];
				strcpy(pn, "art/bombers/");
				for(size_t p=0;p<=nlen;p++) pn[12+p]=tolower(this.name[p]);
				strcat(pn, ".png");
				if(!(this.picture=IMG_Load(pn)))
				{
					fprintf(stderr, "Failed to load %s: %s\n", pn, IMG_GetError());
					return(1);
				}
				this.prio=2;
				this.pribuf=0;
				this.text=this.newtext=NULL;
				types=(bombertype *)realloc(types, (ntypes+1)*sizeof(bombertype));
				types[ntypes]=this;
				ntypes++;
			}
			next=strtok(NULL, "\n");
		}
		free(typefile);
		for(unsigned int i=0;i<ntypes;i++)
		{
			if(!(types[i].prio_selector=create_priority_selector(&types[i].prio)))
			{
				fprintf(stderr, "create_priority_selector failed (i=%u)\n", i);
				return(1);
			}
		}
		fprintf(stderr, "Loaded %u bomber types\n", ntypes);
	}
	
	FILE *ftypefp=fopen("dat/fighters", "r");
	if(!ftypefp)
	{
		fprintf(stderr, "Failed to open data file `fighters'!\n");
		return(1);
	}
	else
	{
		char *ftypefile=slurp(ftypefp);
		fclose(ftypefp);
		char *next=ftypefile?strtok(ftypefile, "\n"):NULL;
		while(next)
		{
			if(*next&&(*next!='#'))
			{
				fightertype this;
				// MANUFACTURER:NAME:COST:SPEED:ARMAMENT:MNV:DD-MM-YYYY:DD-MM-YYYY:FLAGS
				this.name=strdup(next); // guarantees that enough memory will be allocated
				this.manu=(char *)malloc(strcspn(next, ":")+1);
				ssize_t db;
				int e;
				if((e=sscanf(next, "%[^:]:%[^:]:%u:%u:%hhu:%hhu:%zn", this.manu, this.name, &this.cost, &this.speed, &this.arm, &this.mnv, &db))!=6)
				{
					fprintf(stderr, "Malformed `fighters' line `%s'\n", next);
					fprintf(stderr, "  sscanf returned %d\n", e);
					return(1);
				}
				size_t nlen=strlen(this.name)+1;
				this.name=realloc(this.name, nlen);
				this.entry=readdate(next+db, (date){0, 0, 0});
				const char *exit=strchr(next+db, ':');
				if(!exit)
				{
					fprintf(stderr, "Malformed `fighters' line `%s'\n", next);
					fprintf(stderr, "  missing :EXIT\n");
					return(1);
				}
				exit++;
				this.exit=readdate(exit, (date){9999, 99, 99});
				const char *flag=strchr(exit, ':');
				if(!flag)
				{
					fprintf(stderr, "Malformed `fighters' line `%s'\n", next);
					fprintf(stderr, "  missing :FLAGS\n");
					return(1);
				}
				flag++;
				if(strstr(flag, "NIGHT"))
					this.night=true;
				else if(strstr(flag, "DAY"))
					this.night=false;
				else
				{
					fprintf(stderr, "Malformed `fighters' line `%s'\n", next);
					fprintf(stderr, "  FLAGS has neither DAY not NIGHT\n");
					return(1);
				}
				this.text=this.newtext=NULL;
				ftypes=realloc(ftypes, (nftypes+1)*sizeof(fightertype));
				ftypes[nftypes]=this;
				nftypes++;
			}
			next=strtok(NULL, "\n");
		}
		free(ftypefile);
		fprintf(stderr, "Loaded %u fighter types\n", nftypes);
	}
	unsigned int fcount[nftypes];
	
	FILE *fbfp=fopen("dat/ftrbases", "r");
	if(!fbfp)
	{
		fprintf(stderr, "Failed to open data file `ftrbases'!\n");
		return(1);
	}
	else
	{
		char *fbfile=slurp(fbfp);
		fclose(fbfp);
		char *next=fbfile?strtok(fbfile, "\n"):NULL;
		while(next)
		{
			if(*next!='#')
			{
				ftrbase this;
				// LAT:LONG:ENTRY:EXIT
				ssize_t db;
				int e;
				if((e=sscanf(next, "%u:%u:%zn", &this.lat, &this.lon, &db))!=2)
				{
					fprintf(stderr, "Malformed `ftrbases' line `%s'\n", next);
					fprintf(stderr, "  sscanf returned %d\n", e);
					return(1);
				}
				this.entry=readdate(next+db, (date){0, 0, 0});
				const char *exit=strchr(next+db, ':');
				if(!exit)
				{
					fprintf(stderr, "Malformed `ftrbases' line `%s'\n", next);
					fprintf(stderr, "  missing :EXIT\n");
					return(1);
				}
				exit++;
				this.exit=readdate(exit, (date){9999, 99, 99});
				fbases=(ftrbase *)realloc(fbases, (nfbases+1)*sizeof(ftrbase));
				fbases[nfbases]=this;
				nfbases++;
			}
			next=strtok(NULL, "\n");
		}
		free(fbfile);
		fprintf(stderr, "Loaded %u ftrbases\n", nfbases);
	}
	
	FILE *targfp=fopen("dat/targets", "r");
	if(!targfp)
	{
		fprintf(stderr, "Failed to open data file `targets'!\n");
		return(1);
	}
	else
	{
		char *targfile=slurp(targfp);
		fclose(targfp);
		char *next=targfile?strtok(targfile, "\n"):NULL;
		while(next)
		{
			if(*next!='#')
			{
				target this;
				// NAME:PROD:FLAK:ESIZ:LAT:LONG:DD-MM-YYYY:DD-MM-YYYY:CLASS
				this.name=(char *)malloc(strcspn(next, ":")+1);
				this.p_intel=NULL;
				ssize_t db;
				int e;
				if((e=sscanf(next, "%[^:]:%u:%u:%u:%u:%u:%zn", this.name, &this.prod, &this.flak, &this.esiz, &this.lat, &this.lon, &db))!=6)
				{
					fprintf(stderr, "Malformed `targets' line `%s'\n", next);
					fprintf(stderr, "  sscanf returned %d\n", e);
					return(1);
				}
				this.entry=readdate(next+db, (date){0, 0, 0});
				const char *exit=strchr(next+db, ':');
				if(!exit)
				{
					fprintf(stderr, "Malformed `targets' line `%s'\n", next);
					fprintf(stderr, "  missing :EXIT\n");
					return(1);
				}
				exit++;
				this.exit=readdate(exit, (date){9999, 99, 99});
				const char *class=strchr(exit, ':');
				if(!class)
				{
					fprintf(stderr, "Malformed `targets' line `%s'\n", next);
					fprintf(stderr, "  missing :CLASS\n");
					return(1);
				}
				class++;
				if(strstr(class, "CITY"))
				{
					this.class=TCLASS_CITY;
				}
				else if(strstr(class, "SHIPPING"))
				{
					this.class=TCLASS_SHIPPING;
				}
				else if(strstr(class, "MINING"))
				{
					this.class=TCLASS_MINING;
				}
				else if(strstr(class, "LEAFLET"))
				{
					this.class=TCLASS_LEAFLET;
				}
				else if(strstr(class, "AIRFIELD"))
				{
					this.class=TCLASS_AIRFIELD;
				}
				else if(strstr(class, "BRIDGE"))
				{
					this.class=TCLASS_BRIDGE;
				}
				else if(strstr(class, "ROAD"))
				{
					this.class=TCLASS_ROAD;
				}
				else if(strstr(class, "INDUSTRY"))
				{
					this.class=TCLASS_INDUSTRY;
				}
				else
				{
					fprintf(stderr, "Bad `targets' line `%s'\n", next);
					fprintf(stderr, "  unrecognised :CLASS `%s'\n", class);
					return(1);
				}
				this.iclass=ICLASS_MIXED;
				if(strstr(class, ",BB"))
					this.iclass=ICLASS_BB;
				if(strstr(class, ",OIL"))
					this.iclass=ICLASS_OIL;
				if(strstr(class, ",RAIL"))
					this.iclass=ICLASS_RAIL;
				if(strstr(class, ",UBOOT"))
					this.iclass=ICLASS_UBOOT;
				if(strstr(class, ",ARM"))
					this.iclass=ICLASS_ARM;
				if(strstr(class, ",STEEL"))
					this.iclass=ICLASS_STEEL;
				if(strstr(class, ",AC"))
					this.iclass=ICLASS_AC;
				this.berlin=strstr(class, ",BERLIN");
				this.flammable=strstr(class, ",FLAMMABLE");
				targs=(target *)realloc(targs, (ntargs+1)*sizeof(target));
				targs[ntargs]=this;
				ntargs++;
			}
			next=strtok(NULL, "\n");
		}
		free(targfile);
		fprintf(stderr, "Loaded %u targets\n", ntargs);
		for(unsigned int t=0;t<ntargs;t++)
		{
			switch(targs[t].class)
			{
				case TCLASS_LEAFLET:
					if(!t)
					{
						fprintf(stderr, "Error: First target is a TCLASS_LEAFLET\n");
						return(1);
					}
					if(targs[t-1].class!=TCLASS_CITY)
					{
						fprintf(stderr, "Error: TCLASS_LEAFLET not preceded by its CITY\n");
						fprintf(stderr, "\t(targs[%u].name == \"%s\")\n", t, targs[t].name);
						return(1);
					}
					(targs[t].picture=targs[t-1].picture)->refcount++;
					targs[t].psiz=targs[t-1].psiz;
				break;
				case TCLASS_CITY:;
					char cfn[48];
					snprintf(cfn, 48, "dat/cities/%s.pbm", targs[t].name);
					SDL_Surface *picsrc=IMG_Load(cfn);
					if(!picsrc)
					{
						fprintf(stderr, "targs[%u].picture: IMG_Load: %s\n", t, IMG_GetError());
						return(1);
					}
					int sz=((int)((targs[t].esiz+1)/3))|1;
					if((sz!=picsrc->w)||(sz!=picsrc->h))
					{
						fprintf(stderr, "targs[%u].picture: picsrc has wrong dimensions (%d,%d not %d)\n", t, picsrc->w, picsrc->h, sz);
						fprintf(stderr, "\t(name: %s)\n", targs[t].name);
						return(1);
					}
					if(!(targs[t].picture=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, sz, sz, 32, 0xff000000, 0xff0000, 0xff00, 0xff)))
					{
						fprintf(stderr, "targs[%u].picture: SDL_CreateRGBSurface: %s\n", t, SDL_GetError());
						return(1);
					}
					SDL_SetAlpha(targs[t].picture, 0, 0);
					SDL_FillRect(targs[t].picture, &(SDL_Rect){.x=0, .y=0, .w=targs[t].picture->w, .h=targs[t].picture->h}, ATG_ALPHA_TRANSPARENT&0xff);
					targs[t].psiz=0;
					for(int x=0;x<sz;x++)
					{
						for(int y=0;y<sz;y++)
						{
							if(!pget(picsrc, x, y).r)
							{
								pset(targs[t].picture, x, y, (atg_colour){.r=7, .g=7, .b=7, .a=ATG_ALPHA_OPAQUE});
								targs[t].psiz++;
							}
						}
					}
					SDL_FreeSurface(picsrc);
				break;
				case TCLASS_SHIPPING:
					if(!(targs[t].picture=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 4, 3, 32, 0xff000000, 0xff0000, 0xff00, 0xff)))
					{
						fprintf(stderr, "targs[%u].picture: SDL_CreateRGBSurface: %s\n", t, SDL_GetError());
						return(1);
					}
					SDL_SetAlpha(targs[t].picture, 0, 0);
					SDL_FillRect(targs[t].picture, &(SDL_Rect){.x=0, .y=0, .w=targs[t].picture->w, .h=targs[t].picture->h}, ATG_ALPHA_TRANSPARENT&0xff);
					pset(targs[t].picture, 1, 0, (atg_colour){.r=47, .g=51, .b=47, .a=ATG_ALPHA_OPAQUE});
					pset(targs[t].picture, 0, 1, (atg_colour){.r=39, .g=43, .b=39, .a=ATG_ALPHA_OPAQUE});
					pset(targs[t].picture, 1, 1, (atg_colour){.r=47, .g=51, .b=47, .a=ATG_ALPHA_OPAQUE});
					pset(targs[t].picture, 2, 1, (atg_colour){.r=47, .g=51, .b=47, .a=ATG_ALPHA_OPAQUE});
					pset(targs[t].picture, 3, 1, (atg_colour){.r=39, .g=43, .b=39, .a=ATG_ALPHA_OPAQUE});
					pset(targs[t].picture, 1, 2, (atg_colour){.r=39, .g=43, .b=39, .a=ATG_ALPHA_OPAQUE});
					pset(targs[t].picture, 2, 2, (atg_colour){.r=39, .g=43, .b=39, .a=ATG_ALPHA_OPAQUE});
				break;
				case TCLASS_AIRFIELD:
					if(!(targs[t].picture=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 3, 1, 32, 0xff000000, 0xff0000, 0xff00, 0xff)))
					{
						fprintf(stderr, "targs[%u].picture: SDL_CreateRGBSurface: %s\n", t, SDL_GetError());
						return(1);
					}
					SDL_SetAlpha(targs[t].picture, 0, 0);
					SDL_FillRect(targs[t].picture, &(SDL_Rect){.x=0, .y=0, .w=targs[t].picture->w, .h=targs[t].picture->h}, SDL_MapRGB(targs[t].picture->format, 0, 0, 7));
				break;
				case TCLASS_BRIDGE:
					if(!(targs[t].picture=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 3, 2, 32, 0xff000000, 0xff0000, 0xff00, 0xff)))
					{
						fprintf(stderr, "targs[%u].picture: SDL_CreateRGBSurface: %s\n", t, SDL_GetError());
						return(1);
					}
					SDL_SetAlpha(targs[t].picture, 0, 0);
					SDL_FillRect(targs[t].picture, &(SDL_Rect){.x=0, .y=0, .w=targs[t].picture->w, .h=targs[t].picture->h}, ATG_ALPHA_TRANSPARENT&0xff);
					pset(targs[t].picture, 0, 0, (atg_colour){.r=7, .g=7, .b=7, .a=ATG_ALPHA_OPAQUE});
					pset(targs[t].picture, 1, 0, (atg_colour){.r=7, .g=7, .b=7, .a=ATG_ALPHA_OPAQUE});
					pset(targs[t].picture, 2, 0, (atg_colour){.r=7, .g=7, .b=7, .a=ATG_ALPHA_OPAQUE});
					pset(targs[t].picture, 0, 1, (atg_colour){.r=31, .g=31, .b=31, .a=ATG_ALPHA_OPAQUE});
					pset(targs[t].picture, 2, 1, (atg_colour){.r=31, .g=31, .b=31, .a=ATG_ALPHA_OPAQUE});
				break;
				case TCLASS_ROAD:
					if(!(targs[t].picture=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 1, 3, 32, 0xff000000, 0xff0000, 0xff00, 0xff)))
					{
						fprintf(stderr, "targs[%u].picture: SDL_CreateRGBSurface: %s\n", t, SDL_GetError());
						return(1);
					}
					SDL_SetAlpha(targs[t].picture, 0, 0);
					SDL_FillRect(targs[t].picture, &(SDL_Rect){.x=0, .y=0, .w=targs[t].picture->w, .h=targs[t].picture->h}, SDL_MapRGB(targs[t].picture->format, 47, 31, 0));
				break;
				case TCLASS_INDUSTRY:
					if(!(targs[t].picture=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 1, 1, 32, 0xff000000, 0xff0000, 0xff00, 0xff)))
					{
						fprintf(stderr, "targs[%u].picture: SDL_CreateRGBSurface: %s\n", t, SDL_GetError());
						return(1);
					}
					SDL_SetAlpha(targs[t].picture, 0, 0);
					SDL_FillRect(targs[t].picture, &(SDL_Rect){.x=0, .y=0, .w=targs[t].picture->w, .h=targs[t].picture->h}, SDL_MapRGB(targs[t].picture->format, 127, 127, 31));
				break;
				case TCLASS_MINING:
					if(!(targs[t].picture=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 2, 4, 32, 0xff000000, 0xff0000, 0xff00, 0xff)))
					{
						fprintf(stderr, "targs[%u].picture: SDL_CreateRGBSurface: %s\n", t, SDL_GetError());
						return(1);
					}
					SDL_SetAlpha(targs[t].picture, 0, 0);
					SDL_FillRect(targs[t].picture, &(SDL_Rect){.x=0, .y=0, .w=targs[t].picture->w, .h=targs[t].picture->h}, SDL_MapRGB(targs[t].picture->format, 0, 0, 0));
					SDL_FillRect(targs[t].picture, &(SDL_Rect){.x=0, .y=0, .w=targs[t].picture->w, .h=1}, SDL_MapRGB(targs[t].picture->format, 63, 63, 63));
				break;
				default: // shouldn't ever get here
					fprintf(stderr, "Bad targs[%u].class = %d\n", t, targs[t].class);
					return(1);
				break;
			}
		}
	}
	
	FILE *flakfp=fopen("dat/flak", "r");
	if(!flakfp)
	{
		fprintf(stderr, "Failed to open data file `flak'!\n");
		return(1);
	}
	else
	{
		char *flakfile=slurp(flakfp);
		fclose(flakfp);
		char *next=flakfile?strtok(flakfile, "\n"):NULL;
		while(next)
		{
			if(*next!='#')
			{
				flaksite this;
				// STRENGTH:LAT:LONG:ENTRY:RADAR:EXIT
				ssize_t db;
				int e;
				if((e=sscanf(next, "%u:%u:%u:%zn", &this.strength, &this.lat, &this.lon, &db))!=3)
				{
					fprintf(stderr, "Malformed `flak' line `%s'\n", next);
					fprintf(stderr, "  sscanf returned %d\n", e);
					return(1);
				}
				this.entry=readdate(next+db, (date){0, 0, 0});
				const char *radar=strchr(next+db, ':');
				if(!radar)
				{
					fprintf(stderr, "Malformed `flak' line `%s'\n", next);
					fprintf(stderr, "  missing :RADAR\n");
					return(1);
				}
				radar++;
				this.radar=readdate(radar, (date){9999, 99, 99});
				const char *exit=strchr(radar, ':');
				if(!exit)
				{
					fprintf(stderr, "Malformed `flak' line `%s'\n", next);
					fprintf(stderr, "  missing :EXIT\n");
					return(1);
				}
				exit++;
				this.exit=readdate(exit, (date){9999, 99, 99});
				flaks=(flaksite *)realloc(flaks, (nflaks+1)*sizeof(flaksite));
				flaks[nflaks]=this;
				nflaks++;
			}
			next=strtok(NULL, "\n");
		}
		free(flakfile);
		fprintf(stderr, "Loaded %u flaksites\n", nflaks);
	}
	
	FILE *evfp=fopen("dat/events", "r");
	if(!evfp)
	{
		fprintf(stderr, "Failed to open data file `events'!\n");
		return(1);
	}
	else
	{
		unsigned int nevs=0;
		char *evfile=slurp(evfp);
		fclose(evfp);
		char *next=evfile?strtok(evfile, "\n"):NULL;
		while(next)
		{
			if(*next!='#')
			{
				// ID:DATE
				char *colon=strchr(next, ':');
				if(!colon)
				{
					fprintf(stderr, "Malformed `events' line `%s'\n", next);
					fprintf(stderr, "  missing :DATE\n");
					return(1);
				}
				*colon++=0;
				if(strcmp(next, event_names[nevs]))
				{
					fprintf(stderr, "Out-of-order `events' line `%s'\n", next);
					fprintf(stderr, "  expected ID=%s\n", event_names[nevs]);
					return(1);
				}
				event[nevs++]=readdate(colon, (date){0, 0, 0});
			}
			next=strtok(NULL, "\n");
		}
		free(evfile);
		fprintf(stderr, "Loaded %u events\n", nevs);
		if(nevs!=NEVENTS)
		{
			fprintf(stderr, "  expected %u\n", NEVENTS);
			return(1);
		}
	}
	
	FILE *txfp=fopen("dat/texts", "r");
	if(!txfp)
	{
		fprintf(stderr, "Failed to open data file `texts'!\n");
		return(1);
	}
	else
	{
		char *txfile=slurp(txfp);
		fclose(txfp);
		char *next=txfile?strtok(txfile, "=="):NULL;
		char *item=NULL;
		while(next)
		{
			if(item)
			{
				while(*next=='\n') next++;
				unsigned int event;
				for(event=0;event<NEVENTS;event++)
					if(!strcmp(event_names[event], item)) break;
				if(event<NEVENTS)
				{
					evtext[event]=strdup(next);
				}
				else if(!strcmp(item, "beginning")); // pass
				else
				{
					unsigned int i;
					bool new=false;
					if(!strncmp(item, "NEW_", 4))
					{
						new=true;
						item+=4;
					}
					for(i=0;i<ntypes;i++)
						if(!strcmp(types[i].name, item)) break;
					if(i<ntypes)
					{
						if(new)
							types[i].newtext=strdup(next);
						else
							types[i].text=strdup(next);
					}
					else
					{
						for(i=0;i<nftypes;i++)
							if(!strcmp(ftypes[i].name, item)) break;
						if(i<nftypes)
						{
							if(new)
								ftypes[i].newtext=strdup(next);
							else
								ftypes[i].text=strdup(next);
						}
						else
						{
							fprintf(stderr, "Unrecognised item in dat/texts\n%s\n", item);
							return(1);
						}
					}
				}
				item=NULL;
			}
			else
				item=next;
			next=strtok(NULL, "==");
		}
		if(item)
		{
			fprintf(stderr, "Leftover item in dat/texts\n%s\n", item);
			return(1);
		}
	}
	
	intel *intel_head=NULL;
	FILE *intfp=fopen("dat/intel", "r");
	if(!intfp)
	{
		fprintf(stderr, "Failed to open data file `intel'!\n");
		return(1);
	}
	else
	{
		char *intfile=slurp(intfp);
		fclose(intfp);
		char *next=intfile?strtok(intfile, "=="):NULL;
		char *item=NULL;
		while(next)
		{
			if(item)
			{
				while(*next=='\n') next++;
				if(*next) // skip over blank entries, they're just placeholders
				{
					intel *new=malloc(sizeof(intel));
					new->ident=strdup(item);
					new->text=strdup(next);
					new->next=intel_head;
					intel_head=new;
				}
				item=NULL;
			}
			else
				item=next;
			next=strtok(NULL, "==");
		}
		if(item)
		{
			fprintf(stderr, "Leftover item in dat/intel\n%s\n", item);
			return(1);
		}
	}
	
	// Intel hookup: Targets
	for(unsigned int t=0;t<ntargs;t++)
	{
		const char *name=targs[t].name;
		for(intel *i=intel_head;i;i=i->next)
		{
			if(!strcmp(name, i->ident))
			{
				targs[t].p_intel=i;
				break;
			}
		}
	}
	
	bool lorw[128][128]; // TRUE for water
	{
		SDL_Surface *water;
		if(!(water=IMG_Load("map/overlay_water.png")))
		{
			fprintf(stderr, "Water overlay: IMG_Load: %s\n", IMG_GetError());
			return(1);
		}
		for(unsigned int x=0;x<128;x++)
		{
			for(unsigned int y=0;y<128;y++)
			{
				size_t s_off = (y*water->pitch) + (x*water->format->BytesPerPixel);
				uint32_t pixval = *(uint32_t *)((char *)water->pixels + s_off);
				Uint8 r,g,b;
				SDL_GetRGB(pixval, water->format, &r, &g, &b);
				lorw[x][y]=(r+g+b<381);
			}
		}
		SDL_FreeSurface(water);
	}
	unsigned char tnav[128][128]; // Recognisability of terrain.  High for rivers, even higher for coastline
	{
		SDL_Surface *coast;
		if(!(coast=IMG_Load("map/overlay_coast.png")))
		{
			fprintf(stderr, "Coastline overlay: IMG_Load: %s\n", IMG_GetError());
			return(1);
		}
		for(unsigned int x=0;x<128;x++)
		{
			for(unsigned int y=0;y<128;y++)
			{
				size_t s_off = (y*coast->pitch) + (x*coast->format->BytesPerPixel);
				uint32_t pixval = *(uint32_t *)((char *)coast->pixels + s_off);
				Uint8 r,g,b;
				SDL_GetRGB(pixval, coast->format, &r, &g, &b);
				tnav[x][y]=(r+g+b)/3;
			}
		}
		SDL_FreeSurface(coast);
	}
	if(!(terrain=IMG_Load("map/overlay_terrain.png")))
	{
		fprintf(stderr, "Terrain overlay: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(location=IMG_Load("art/location.png")))
	{
		fprintf(stderr, "Location icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(yellowhair=IMG_Load("art/yellowhair.png")))
	{
		fprintf(stderr, "Yellow crosshairs: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(intelbtn=IMG_Load("art/intel.png")))
	{
		fprintf(stderr, "Intel icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(resizebtn=IMG_Load("art/resize.png")))
	{
		fprintf(stderr, "Resize button: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(fullbtn=IMG_Load("art/fullscreen.png")))
	{
		fprintf(stderr, "Fullscreen button: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(exitbtn=IMG_Load("art/exit.png")))
	{
		fprintf(stderr, "Exit button: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	for(unsigned int n=0;n<NNAVAIDS;n++)
	{
		if(!(navpic[n]=IMG_Load(navpicfn[n])))
		{
			fprintf(stderr, "Navaid icon %s: IMG_Load: %s\n", navpicfn[n], IMG_GetError());
			return(1);
		}
		SDL_SetAlpha(navpic[n], 0, SDL_ALPHA_OPAQUE);
	}
	if(!(pffpic=IMG_Load("art/filters/pff.png")))
	{
		fprintf(stderr, "PFF icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	for(unsigned int l=0;l<NBOMBLOADS;l++)
	{
		if(!(bombloads[l].pic=IMG_Load(bombloads[l].fn)))
		{
			fprintf(stderr, "Bombload icon %s: IMG_Load: %s\n", bombloads[l].fn, IMG_GetError());
			return(1);
		}
	}
	
	SDL_Surface *grey_overlay=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 36, 40, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!grey_overlay)
	{
		fprintf(stderr, "Grey overlay: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(1);
	}
	SDL_FillRect(grey_overlay, &(SDL_Rect){.x=0, .y=0, .w=grey_overlay->w, .h=grey_overlay->h}, SDL_MapRGBA(grey_overlay->format, 127, 127, 127, 128));
	
	SDL_Surface *yellow_overlay=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 16, 16, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!yellow_overlay)
	{
		fprintf(stderr, "Yellow overlay: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(1);
	}
	SDL_FillRect(yellow_overlay, &(SDL_Rect){.x=0, .y=0, .w=yellow_overlay->w, .h=yellow_overlay->h}, SDL_MapRGBA(yellow_overlay->format, 255, 255, 0, 63));
	
	fprintf(stderr, "Data files loaded\n");
	
	fprintf(stderr, "Allocating game state...\n");
	
	game state;
	state.nbombers=state.nfighters=0;
	state.bombers=NULL;
	state.fighters=NULL;
	state.ntargs=ntargs;
	if(!(state.dmg=malloc(ntargs*sizeof(*state.dmg))))
	{
		perror("malloc");
		return(1);
	}
	if(!(state.flk=malloc(ntargs*sizeof(*state.flk))))
	{
		perror("malloc");
		return(1);
	}
	if(!(state.heat=malloc(ntargs*sizeof(*state.heat))))
	{
		perror("malloc");
		return(1);
	}
	if(!(state.flam=malloc(ntargs*sizeof(*state.flam))))
	{
		perror("malloc");
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
		if(!(state.raids[i].loads=malloc(ntypes*sizeof(bombload))))
		{
			perror("malloc");
			return(1);
		}
		if(!(state.raids[i].pffloads=malloc(ntypes*sizeof(bombload))))
		{
			perror("malloc");
			return(1);
		}
		for(unsigned int j=0;j<ntypes;j++)
		{
			state.raids[i].loads[j]=BL_PLUMDUFF;
			int limit=0;
			if(types[j].pff)
			{
				if(types[j].noarm)
					state.raids[i].pffloads[j]=BL_ILLUM;
				else
					state.raids[i].pffloads[j]=BL_USUAL;
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
	
	fprintf(stderr, "Game state allocated\n");
	
	fprintf(stderr, "Instantiating GUI elements...\n");
	
	atg_box *mainbox=canvas->box;
	atg_element *MM_top=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
	if(!MM_top)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(mainbox, MM_top))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_box *MM_tb=MM_top->elem.box;
	if(!MM_tb)
	{
		fprintf(stderr, "MM_top->elem.box==NULL\n");
		return(1);
	}
	atg_element *MM_title=atg_create_element_label("HARRIS: Main Menu", 12, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
	if(!MM_title)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_pack_element(MM_tb, MM_title))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *MM_full=atg_create_element_image(fullbtn);
	if(!MM_full)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	MM_full->clickable=true;
	if(atg_pack_element(MM_tb, MM_full))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *MM_QuickStart=atg_create_element_button("Quick Start Game", (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE});
	if(!MM_QuickStart)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_pack_element(mainbox, MM_QuickStart))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *MM_NewGame=atg_create_element_button("Set Up New Game", (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE});
	if(!MM_NewGame)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_pack_element(mainbox, MM_NewGame))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *MM_LoadGame=atg_create_element_button("Load Game", (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE});
	if(!MM_LoadGame)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_pack_element(mainbox, MM_LoadGame))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *MM_Exit=atg_create_element_button("Exit", (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE});
	if(!MM_Exit)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_pack_element(mainbox, MM_Exit))
	{
		perror("atg_pack_element");
		return(1);
	}
	MM_QuickStart->w=MM_NewGame->w=MM_LoadGame->w=MM_Exit->w=canvas->surface->w;
	
	atg_box *gamebox=atg_create_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!gamebox)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	atg_element *GB_bt=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!GB_bt)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(gamebox, GB_bt))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_box *GB_btb=GB_bt->elem.box;
	if(!GB_btb)
	{
		fprintf(stderr, "GB_bt->elem.box==NULL\n");
		return(1);
	}
	char *datestring=malloc(11);
	if(!datestring)
	{
		perror("malloc");
		return(1);
	}
	atg_element *GB_datetimebox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!GB_datetimebox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(GB_btb, GB_datetimebox))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_box *GB_dtb=GB_datetimebox->elem.box;
	if(!GB_dtb)
	{
		fprintf(stderr, "GB_datetimebox->elem.box==NULL\n");
		return(1);
	}
	atg_element *GB_date=atg_create_element_label("", 12, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
	if(!GB_date)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	GB_date->w=80;
	if(atg_pack_element(GB_dtb, GB_date))
	{
		perror("atg_pack_element");
		return(1);
	}
	else
	{
		atg_label *l=GB_date->elem.label;
		if(l)
			l->text=datestring;
		else
		{
			fprintf(stderr, "GB_date->elem.label==NULL\n");
			return(1);
		}
	}
	SDL_Surface *GB_moonimg=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 14, 14, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!GB_moonimg)
	{
		fprintf(stderr, "moonimg: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(1);
	}
	atg_element *GB_moonpic=atg_create_element_image(NULL);
	if(!GB_moonpic)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	GB_moonpic->w=18;
	if(atg_pack_element(GB_dtb, GB_moonpic))
	{
		perror("atg_pack_element");
		return(1);
	}
	else
	{
		atg_image *i=GB_moonpic->elem.image;
		if(!i)
		{
			fprintf(stderr, "GB_moonpic->elem.image==NULL\n");
			return(1);
		}
		i->data=GB_moonimg;
	}
	atg_element *GB_resize=atg_create_element_image(resizebtn);
	if(!GB_resize)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	GB_resize->w=16;
	GB_resize->clickable=true;
	if(atg_pack_element(GB_dtb, GB_resize))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *GB_full=atg_create_element_image(fullbtn);
	if(!GB_full)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	GB_full->w=16;
	GB_full->clickable=true;
	if(atg_pack_element(GB_dtb, GB_full))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *GB_exit=atg_create_element_image(exitbtn);
	if(!GB_exit)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	GB_exit->clickable=true;
	if(atg_pack_element(GB_dtb, GB_exit))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *GB_btrow[ntypes], *GB_btpic[ntypes], *GB_btdesc[ntypes], *GB_btnum[ntypes], *GB_btpc[ntypes], *GB_btnew[ntypes], *GB_btp[ntypes], *GB_navrow[ntypes], *GB_navbtn[ntypes][NNAVAIDS], *GB_navgraph[ntypes][NNAVAIDS];
	for(unsigned int i=0;i<ntypes;i++)
	{
		if(!(GB_btrow[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_pack_element(GB_btb, GB_btrow[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
		GB_btrow[i]->w=239;
		atg_box *b=GB_btrow[i]->elem.box;
		if(!b)
		{
			fprintf(stderr, "GB_btrow[i]->elem.box==NULL\n");
			return(1);
		}
		SDL_Surface *pic=SDL_CreateRGBSurface(SDL_HWSURFACE, 36, 40, types[i].picture->format->BitsPerPixel, types[i].picture->format->Rmask, types[i].picture->format->Gmask, types[i].picture->format->Bmask, types[i].picture->format->Amask);
		if(!pic)
		{
			fprintf(stderr, "pic=SDL_CreateRGBSurface: %s\n", SDL_GetError());
			return(1);
		}
		SDL_FillRect(pic, &(SDL_Rect){0, 0, pic->w, pic->h}, SDL_MapRGB(pic->format, 0, 0, 0));
		SDL_BlitSurface(types[i].picture, NULL, pic, &(SDL_Rect){(36-types[i].picture->w)>>1, (40-types[i].picture->h)>>1, 0, 0});
		GB_btpic[i]=atg_create_element_image(pic);
		SDL_FreeSurface(pic);
		if(!GB_btpic[i])
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		GB_btpic[i]->w=38;
		GB_btpic[i]->clickable=true;
		if(atg_pack_element(b, GB_btpic[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_element *vbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!vbox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_pack_element(b, vbox))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_box *vb=vbox->elem.box;
		if(!vb)
		{
			fprintf(stderr, "vbox->elem.box==NULL\n");
			return(1);
		}
		if(types[i].manu&&types[i].name)
		{
			size_t len=strlen(types[i].manu)+strlen(types[i].name)+2;
			char *fullname=malloc(len);
			if(fullname)
			{
				snprintf(fullname, len, "%s %s", types[i].manu, types[i].name);
				if(!(GB_btdesc[i]=atg_create_element_label(fullname, 10, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE})))
				{
					fprintf(stderr, "atg_create_element_label failed\n");
					return(1);
				}
				GB_btdesc[i]->w=201;
				GB_btdesc[i]->cache=true;
				GB_btdesc[i]->clickable=true;
				if(atg_pack_element(vb, GB_btdesc[i]))
				{
					perror("atg_pack_element");
					return(1);
				}
			}
			else
			{
				perror("malloc");
				return(1);
			}
			free(fullname);
		}
		else
		{
			fprintf(stderr, "Missing manu or name in type %u\n", i);
			return(1);
		}
		if(!(GB_btnum[i]=atg_create_element_label("svble/total", 12, (atg_colour){159, 191, 255, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_pack_element(vb, GB_btnum[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_element *hbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!hbox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_pack_element(vb, hbox))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_box *hb=hbox->elem.box;
		if(!hb)
		{
			fprintf(stderr, "hbox->elem.box==NULL\n");
			return(1);
		}
		if(atg_pack_element(hb, types[i].prio_selector))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_element *pcbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 79, 31, ATG_ALPHA_OPAQUE});
		if(!pcbox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		pcbox->w=3;
		pcbox->h=18;
		if(atg_pack_element(hb, pcbox))
		{
			perror("atg_pack_element");
			return(1);
		}
		if(!(GB_btpc[i]=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		GB_btpc[i]->w=3;
		if(atg_ebox_pack(pcbox, GB_btpc[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(GB_btnew[i]=atg_create_element_label("N", 12, (atg_colour){159, 191, 63, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_pack_element(hb, GB_btnew[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
		if(!(GB_btp[i]=atg_create_element_label("P!", 12, (atg_colour){191, 159, 31, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_pack_element(hb, GB_btp[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
		GB_navrow[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!GB_navrow[i])
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_pack_element(vb, GB_navrow[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
		for(unsigned int n=0;n<NNAVAIDS;n++)
		{
			SDL_Surface *pic=SDL_CreateRGBSurface(SDL_HWSURFACE, 16, 16, navpic[n]->format->BitsPerPixel, navpic[n]->format->Rmask, navpic[n]->format->Gmask, navpic[n]->format->Bmask, navpic[n]->format->Amask);
			if(!pic)
			{
				fprintf(stderr, "pic=SDL_CreateRGBSurface: %s\n", SDL_GetError());
				return(1);
			}
			SDL_FillRect(pic, &(SDL_Rect){.x=0, .y=0, .w=16, .h=16}, SDL_MapRGBA(pic->format, 0, 0, 0, SDL_ALPHA_TRANSPARENT));
			SDL_BlitSurface(navpic[n], NULL, pic, NULL);
			if(!(GB_navbtn[i][n]=atg_create_element_image(pic)))
			{
				fprintf(stderr, "atg_create_element_image failed\n");
				return(1);
			}
			GB_navbtn[i][n]->w=GB_navbtn[i][n]->h=16;
			GB_navbtn[i][n]->clickable=true;
			if(!types[i].nav[n])
				SDL_FillRect(pic, &(SDL_Rect){.x=0, .y=0, .w=16, .h=16}, SDL_MapRGBA(pic->format, 63, 63, 63, SDL_ALPHA_OPAQUE));
			if(atg_ebox_pack(GB_navrow[i], GB_navbtn[i][n]))
			{
				perror("atg_ebox_pack");
				return(1);
			}
			atg_element *graphbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 79, 31, ATG_ALPHA_OPAQUE});
			if(!graphbox)
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				return(1);
			}
			graphbox->w=3;
			graphbox->h=16;
			if(atg_ebox_pack(GB_navrow[i], graphbox))
			{
				perror("atg_ebox_pack");
				return(1);
			}
			if(!(GB_navgraph[i][n]=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE})))
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				return(1);
			}
			GB_navgraph[i][n]->w=3;
			if(atg_ebox_pack(graphbox, GB_navgraph[i][n]))
			{
				perror("atg_ebox_pack");
				return(1);
			}
		}
	}
	atg_element *GB_go=atg_create_element_button("Run tonight's raids", (atg_colour){159, 191, 255, ATG_ALPHA_OPAQUE}, (atg_colour){31, 63, 31, ATG_ALPHA_OPAQUE});
	if(!GB_go)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	GB_go->w=159;
	if(atg_pack_element(GB_btb, GB_go))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *GB_save=atg_create_element_button("Save game", (atg_colour){159, 255, 191, ATG_ALPHA_OPAQUE}, (atg_colour){31, 63, 31, ATG_ALPHA_OPAQUE});
	if(!GB_save)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	GB_save->w=159;
	if(atg_pack_element(GB_btb, GB_save))
	{
		perror("atg_pack_element");
		return(1);
	}
	char *GB_budget_label=malloc(32);
	if(!GB_budget_label)
	{
		perror("malloc");
		return(1);
	}
	atg_element *GB_budget=atg_create_element_label(NULL, 12, (atg_colour){191, 255, 191, ATG_ALPHA_OPAQUE});
	if(!GB_budget)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	else
	{
		atg_label *l=GB_budget->elem.label;
		if(!l)
		{
			fprintf(stderr, "GB_budget->elem.label==NULL\n");
			return(1);
		}
		*(l->text=GB_budget_label)=0;
		GB_budget->w=159;
		if(atg_pack_element(GB_btb, GB_budget))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	char *GB_confid_label=malloc(32);
	if(!GB_confid_label)
	{
		perror("malloc");
		return(1);
	}
	atg_element *GB_confid=atg_create_element_label(NULL, 12, (atg_colour){191, 255, 191, ATG_ALPHA_OPAQUE});
	if(!GB_confid)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	else
	{
		atg_label *l=GB_confid->elem.label;
		if(!l)
		{
			fprintf(stderr, "GB_confid->elem.label==NULL\n");
			return(1);
		}
		*(l->text=GB_confid_label)=0;
		GB_confid->w=159;
		if(atg_pack_element(GB_btb, GB_confid))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	char *GB_morale_label=malloc(32);
	if(!GB_morale_label)
	{
		perror("malloc");
		return(1);
	}
	atg_element *GB_morale=atg_create_element_label(NULL, 12, (atg_colour){191, 255, 191, ATG_ALPHA_OPAQUE});
	if(!GB_morale)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	else
	{
		atg_label *l=GB_morale->elem.label;
		if(!l)
		{
			fprintf(stderr, "GB_morale->elem.label==NULL\n");
			return(1);
		}
		*(l->text=GB_morale_label)=0;
		GB_morale->w=159;
		if(atg_pack_element(GB_btb, GB_morale))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	atg_element *GB_msglbl=atg_create_element_label("Messages:", 12, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
	if(!GB_msglbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_pack_element(GB_btb, GB_msglbl))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *GB_msgrow[MAXMSGS];
	for(unsigned int i=0;i<MAXMSGS;i++)
	{
		if(!(GB_msgrow[i]=atg_create_element_button(NULL, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE}, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		GB_msgrow[i]->hidden=true;
		GB_msgrow[i]->w=GB_btrow[0]->w;
		if(atg_pack_element(GB_btb, GB_msgrow[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	atg_element *GB_filters=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!GB_filters)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(GB_btb, GB_filters))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_box *GB_fb=GB_filters->elem.box;
	if(!GB_fb)
	{
		fprintf(stderr, "GB_filters->elem.box==NULL\n");
		return(1);
	}
	atg_element *GB_pad=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!GB_pad)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(GB_fb, GB_pad))
	{
		perror("atg_pack_element");
		return(1);
	}
	GB_pad->h=12;
	atg_element *GB_ft=atg_create_element_label("Filters:", 12, (atg_colour){239, 239, 0, ATG_ALPHA_OPAQUE});
	if(!GB_ft)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_pack_element(GB_fb, GB_ft))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *GB_fi=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){23, 23, 31, ATG_ALPHA_OPAQUE});
	if(!GB_fi)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(GB_fb, GB_fi))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_box *GB_fib=GB_fi->elem.box;
	if(!GB_fib)
	{
		fprintf(stderr, "GB_fi->elem.box==NULL\n");
		return(1);
	}
	atg_element *GB_fi_nav[NNAVAIDS];
	int filter_nav[NNAVAIDS], filter_pff=0;
	for(unsigned int n=0;n<NNAVAIDS;n++)
	{
		filter_nav[n]=0;
		if(!(GB_fi_nav[n]=create_filter_switch(navpic[n], filter_nav+n)))
		{
			fprintf(stderr, "create_filter_switch failed\n");
			return(1);
		}
		if(atg_pack_element(GB_fib, GB_fi_nav[n]))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	atg_element *GB_fi_pff=create_filter_switch(pffpic, &filter_pff);
	if(!GB_fi_pff)
	{
		fprintf(stderr, "create_filter_switch failed\n");
		return(1);
	}
	if(atg_pack_element(GB_fib, GB_fi_pff))
	{
		perror("atg_pack_element");
		return(1);
	}
	SDL_Surface *map=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
	if(!map)
	{
		fprintf(stderr, "map: SDL_ConvertSurface: %s\n", SDL_GetError());
		return(1);
	}
	atg_element *GB_middle=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!GB_middle)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(gamebox, GB_middle))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_box *GB_mb=GB_middle->elem.box;
	if(!GB_mb)
	{
		fprintf(stderr, "GB_middle->elem.box==NULL\n");
		return(1);
	}
	atg_element *GB_map=atg_create_element_image(map);
	if(!GB_map)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	GB_map->h=map->h+2;
	if(atg_pack_element(GB_mb, GB_map))
	{
		perror("atg_pack_element");
		return(1);
	}
	SDL_Surface *weather_overlay=NULL, *target_overlay=NULL, *flak_overlay=NULL, *xhair_overlay=NULL;
	atg_element *GB_raid_label=atg_create_element_label("Select a Target", 12, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE});
	if(!GB_raid_label)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_pack_element(GB_mb, GB_raid_label))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *GB_raid=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!GB_raid)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(GB_mb, GB_raid))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_box *GB_raidbox[ntargs], *GB_raidbox_empty=GB_raid->elem.box;
	atg_element *GB_rbrow[ntargs][ntypes], *GB_rbpic[ntargs][ntypes], *GB_raidnum[ntargs][ntypes], *GB_raidload[ntargs][ntypes][2];
	for(unsigned int i=0;i<ntargs;i++)
	{
		GB_raidbox[i]=atg_create_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
		if(!GB_raidbox[i])
		{
			fprintf(stderr, "atg_create_box failed\n");
			return(1);
		}
		for(unsigned int j=0;j<ntypes;j++)
		{
			if(!(GB_rbrow[i][j]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE})))
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				return(1);
			}
			if(atg_pack_element(GB_raidbox[i], GB_rbrow[i][j]))
			{
				perror("atg_pack_element");
				return(1);
			}
			GB_rbrow[i][j]->w=256;
			atg_box *b=GB_rbrow[i][j]->elem.box;
			if(!b)
			{
				fprintf(stderr, "GB_rbrow[i][j]->elem.box==NULL\n");
				return(1);
			}
			SDL_Surface *pic=SDL_CreateRGBSurface(SDL_HWSURFACE, 36, 40, types[j].picture->format->BitsPerPixel, types[j].picture->format->Rmask, types[j].picture->format->Gmask, types[j].picture->format->Bmask, types[j].picture->format->Amask);
			if(!pic)
			{
				fprintf(stderr, "pic=SDL_CreateRGBSurface: %s\n", SDL_GetError());
				return(1);
			}
			SDL_FillRect(pic, &(SDL_Rect){0, 0, pic->w, pic->h}, SDL_MapRGB(pic->format, 0, 0, 0));
			SDL_BlitSurface(types[j].picture, NULL, pic, &(SDL_Rect){(36-types[j].picture->w)>>1, (40-types[j].picture->h)>>1, 0, 0});
			atg_element *picture=atg_create_element_image(pic);
			SDL_FreeSurface(pic);
			if(!picture)
			{
				fprintf(stderr, "atg_create_element_image failed\n");
				return(1);
			}
			picture->w=38;
			picture->cache=true;
			(GB_rbpic[i][j]=picture)->clickable=true;
			if(atg_pack_element(b, picture))
			{
				perror("atg_pack_element");
				return(1);
			}
			atg_element *vbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
			if(!vbox)
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				return(1);
			}
			vbox->w=186;
			if(atg_pack_element(b, vbox))
			{
				perror("atg_pack_element");
				return(1);
			}
			atg_box *vb=vbox->elem.box;
			if(!vb)
			{
				fprintf(stderr, "vbox->elem.box==NULL\n");
				return(1);
			}
			if(types[j].manu&&types[j].name)
			{
				size_t len=strlen(types[j].manu)+strlen(types[j].name)+2;
				char *fullname=malloc(len);
				if(fullname)
				{
					snprintf(fullname, len, "%s %s", types[j].manu, types[j].name);
					atg_element *name=atg_create_element_label(fullname, 10, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
					if(!name)
					{
						fprintf(stderr, "atg_create_element_label failed\n");
						return(1);
					}
					name->cache=true;
					name->w=184;
					if(atg_pack_element(vb, name))
					{
						perror("atg_pack_element");
						return(1);
					}
				}
				else
				{
					perror("malloc");
					return(1);
				}
				free(fullname);
			}
			else
			{
				fprintf(stderr, "Missing manu or name in type %u\n", j);
				return(1);
			}
			if(!(GB_raidnum[i][j]=atg_create_element_label("assigned", 12, (atg_colour){159, 191, 255, ATG_ALPHA_OPAQUE})))
			{
				fprintf(stderr, "atg_create_element_label failed\n");
				return(1);
			}
			if(atg_pack_element(vb, GB_raidnum[i][j]))
			{
				perror("atg_pack_element");
				return(1);
			}
			if(targs[i].class==TCLASS_CITY)
			{
				if(types[j].pff)
				{
					if(!(GB_raidload[i][j][1]=create_load_selector(&types[j], &state.raids[i].pffloads[j])))
					{
						fprintf(stderr, "create_load_selector failed\n");
						return(1);
					}
				}
				else
				{
					if(!(GB_raidload[i][j][1]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE})))
					{
						fprintf(stderr, "atg_create_element_box failed\n");
						return(1);
					}
					GB_raidload[i][j][1]->w=16;
				}
				if(atg_pack_element(b, GB_raidload[i][j][1]))
				{
					perror("atg_pack_element");
					return(1);
				}
				if(!types[j].noarm)
				{
					if(!(GB_raidload[i][j][0]=create_load_selector(&types[j], &state.raids[i].loads[j])))
					{
						fprintf(stderr, "create_load_selector failed\n");
						return(1);
					}
				}
				else
				{
					if(!(GB_raidload[i][j][0]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE})))
					{
						fprintf(stderr, "atg_create_element_box failed\n");
						return(1);
					}
					GB_raidload[i][j][0]->w=16;
				}
				if(atg_pack_element(b, GB_raidload[i][j][0]))
				{
					perror("atg_pack_element");
					return(1);
				}
			}
			else
			{
				GB_raidload[i][j][0]=NULL;
				GB_raidload[i][j][1]=NULL;
			}
		}
	}
	atg_element *GB_tt=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
	if(!GB_tt)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(gamebox, GB_tt))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_box *GB_ttb=GB_tt->elem.box;
	if(!GB_ttb)
	{
		fprintf(stderr, "GB_tt->elem.box==NULL\n");
		return(1);
	}
	atg_element *GB_ttl=atg_create_element_label("Targets", 12, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE});
	if(!GB_ttl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	GB_ttl->clickable=true;
	if(atg_pack_element(GB_ttb, GB_ttl))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *GB_ttrow[ntargs], *GB_ttdmg[ntargs], *GB_ttflk[ntargs], *GB_ttint[ntargs];
	for(unsigned int i=0;i<ntargs;i++)
	{
		GB_ttrow[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
		if(!GB_ttrow[i])
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		GB_ttrow[i]->w=305;
		GB_ttrow[i]->h=12;
		GB_ttrow[i]->clickable=true;
		if(atg_pack_element(GB_ttb, GB_ttrow[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_box *b=GB_ttrow[i]->elem.box;
		if(!b)
		{
			fprintf(stderr, "GB_ttrow[i]->elem.box==NULL\n");
			return(1);
		}
		atg_element *item;
		if(targs[i].p_intel)
		{
			GB_ttint[i]=atg_create_element_image(intelbtn);
			if(!GB_ttint[i])
			{
				fprintf(stderr, "atg_create_element_image failed\n");
				return(1);
			}
			GB_ttint[i]->clickable=true;
			item=GB_ttint[i];
		}
		else
		{
			GB_ttint[i]=NULL;
			item=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
			if(!item)
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				return(1);
			}
		}
		item->w=8;
		item->h=12;
		if(atg_pack_element(b, item))
		{
			perror("atg_pack_element");
			return(1);
		}
		item=atg_create_element_label(targs[i].name, 10, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE});
		if(!item)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		item->w=192;
		item->cache=true;
		if(atg_pack_element(b, item))
		{
			perror("atg_pack_element");
			return(1);
		}
		item=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
		if(!item)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		item->w=105;
		if(atg_pack_element(b, item))
		{
			perror("atg_pack_element");
			return(1);
		}
		b=item->elem.box;
		if(!b)
		{
			fprintf(stderr, "item->elem.box==NULL\n");
			return(1);
		}
		if(!(GB_ttdmg[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){0, 0, 255, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		GB_ttdmg[i]->w=1;
		GB_ttdmg[i]->h=6;
		if(atg_pack_element(b, GB_ttdmg[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
		if(!(GB_ttflk[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){183, 183, 199, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		GB_ttflk[i]->w=1;
		GB_ttflk[i]->h=6;
		if(atg_pack_element(b, GB_ttflk[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	
	atg_box *raidbox=atg_create_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!raidbox)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	atg_element *RB_hbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!RB_hbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(raidbox, RB_hbox))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *RB_time=atg_create_element_label("--:--", 12, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
	if(!RB_time)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	char *RB_time_label;
	if(!RB_time->elem.label)
	{
		fprintf(stderr, "RB_time->elem.label==NULL\n");
		return(1);
	}
	if(!(RB_time_label=RB_time->elem.label->text))
	{
		fprintf(stderr, "RB_time_label==NULL\n");
		return(1);
	}
	RB_time->w=239;
	if(atg_ebox_pack(RB_hbox, RB_time))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *RB_map=atg_create_element_image(map);
	if(!RB_map)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	RB_map->h=map->h+2;
	if(atg_ebox_pack(RB_hbox, RB_map))
	{
		perror("atg_pack_element");
		return(1);
	}
	SDL_Surface *RB_atime_image=SDL_CreateRGBSurface(SDL_HWSURFACE, 600, 240, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!RB_atime_image)
	{
		fprintf(stderr, "RB_atime_image: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(1);
	}
	atg_element *RB_atime=atg_create_element_image(RB_atime_image);
	if(!RB_atime)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	if(atg_pack_element(raidbox, RB_atime))
	{
		perror("atg_pack_element");
		return(1);
	}
	
	atg_box *loadbox=atg_create_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 47, ATG_ALPHA_OPAQUE});
	if(!loadbox)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	atg_element *LB_file=atg_create_element_filepicker("Load Game", NULL, (atg_colour){239, 239, 255, ATG_ALPHA_OPAQUE}, (atg_colour){31, 31, 47, ATG_ALPHA_OPAQUE});
	if(!LB_file)
	{
		fprintf(stderr, "atg_create_element_filepicker failed\n");
		return(1);
	}
	LB_file->h=mainsizey-30;
	LB_file->w=mainsizex;
	if(atg_pack_element(loadbox, LB_file))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *LB_hbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE}), *LB_text=NULL, *LB_load=NULL, *LB_full=NULL, *LB_exit=NULL;
	char **LB_btext=NULL;
	if(!LB_hbox)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	else if(atg_pack_element(loadbox, LB_hbox))
	{
		perror("atg_pack_element");
		return(1);
	}
	else
	{
		atg_box *b=LB_hbox->elem.box;
		if(!b)
		{
			fprintf(stderr, "LB_hbox->elem.box==NULL\n");
			return(1);
		}
		LB_load=atg_create_element_button("Load", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE});
		if(!LB_load)
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		LB_load->w=34;
		if(atg_pack_element(b, LB_load))
		{
			perror("atg_pack_element");
			return(1);
		}
		LB_text=atg_create_element_button(NULL, (atg_colour){239, 255, 239, ATG_ALPHA_OPAQUE}, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE});
		if(!LB_text)
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		LB_text->h=24;
		LB_text->w=mainsizex-64;
		if(atg_pack_element(b, LB_text))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_button *tb=LB_text->elem.button;
		if(!tb)
		{
			fprintf(stderr, "LB_text->elem.button==NULL\n");
			return(1);
		}
		atg_box *t=tb->content;
		if(!t)
		{
			fprintf(stderr, "tb->content==NULL\n");
			return(1);
		}
		if(t->nelems&&t->elems&&t->elems[0]->type==ATG_LABEL)
		{
			atg_label *l=t->elems[0]->elem.label;
			if(!l)
			{
				fprintf(stderr, "t->elems[0]->elem.label==NULL\n");
				return(1);
			}
			LB_btext=&l->text;
		}
		else
		{
			fprintf(stderr, "LB_text has wrong content\n");
			return(1);
		}
		LB_full=atg_create_element_image(fullbtn);
		if(!LB_full)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		LB_full->w=16;
		LB_full->clickable=true;
		if(atg_pack_element(b, LB_full))
		{
			perror("atg_pack_element");
			return(1);
		}
		LB_exit=atg_create_element_image(exitbtn);
		if(!LB_exit)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		LB_exit->clickable=true;
		if(atg_pack_element(b, LB_exit))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	
	atg_box *savebox=atg_create_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 47, ATG_ALPHA_OPAQUE});
	if(!savebox)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	atg_element *SA_file=atg_create_element_filepicker("Save Game", NULL, (atg_colour){239, 239, 255, ATG_ALPHA_OPAQUE}, (atg_colour){31, 31, 47, ATG_ALPHA_OPAQUE});
	if(!SA_file)
	{
		fprintf(stderr, "atg_create_element_filepicker failed\n");
		return(1);
	}
	SA_file->h=mainsizey-30;
	SA_file->w=mainsizex;
	if(atg_pack_element(savebox, SA_file))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *SA_hbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE}), *SA_text=NULL, *SA_save=NULL, *SA_full=NULL, *SA_exit=NULL;
	char **SA_btext=NULL;
	if(!SA_hbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	else if(atg_pack_element(savebox, SA_hbox))
	{
		perror("atg_pack_element");
		return(1);
	}
	else
	{
		atg_box *b=SA_hbox->elem.box;
		if(!b)
		{
			fprintf(stderr, "SA_hbox->elem.box==NULL\n");
			return(1);
		}
		SA_save=atg_create_element_button("Save", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE});
		if(!SA_save)
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		SA_save->w=34;
		if(atg_pack_element(b, SA_save))
		{
			perror("atg_pack_element");
			return(1);
		}
		SA_text=atg_create_element_button(NULL, (atg_colour){239, 255, 239, ATG_ALPHA_OPAQUE}, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE});
		if(!SA_text)
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		SA_text->h=24;
		SA_text->w=mainsizex-64;
		if(atg_pack_element(b, SA_text))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_button *tb=SA_text->elem.button;
		if(!tb)
		{
			fprintf(stderr, "SA_text->elem.button==NULL\n");
			return(1);
		}
		atg_box *t=tb->content;
		if(!t)
		{
			fprintf(stderr, "tb->content==NULL\n");
			return(1);
		}
		if(t->nelems&&t->elems&&t->elems[0]->type==ATG_LABEL)
		{
			atg_label *l=t->elems[0]->elem.label;
			if(!l)
			{
				fprintf(stderr, "t->elems[0]->elem.label==NULL\n");
				return(1);
			}
			SA_btext=&l->text;
		}
		else
		{
			fprintf(stderr, "SA_text has wrong content\n");
			return(1);
		}
		SA_full=atg_create_element_image(fullbtn);
		if(!SA_full)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		SA_full->w=16;
		SA_full->clickable=true;
		if(atg_pack_element(b, SA_full))
		{
			perror("atg_pack_element");
			return(1);
		}
		SA_exit=atg_create_element_image(exitbtn);
		if(!SA_exit)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		SA_exit->clickable=true;
		if(atg_pack_element(b, SA_exit))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	
	atg_box *rstatbox=atg_create_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
	atg_element *RS_resize=NULL, *RS_full=NULL, *RS_cont=NULL;
	if(!rstatbox)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	else
	{
		atg_element *RS_trow=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!RS_trow)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_pack_element(rstatbox, RS_trow))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_box *b=RS_trow->elem.box;
		if(!b)
		{
			fprintf(stderr, "RS_trow->elem.box==NULL\n");
			return(1);
		}
		atg_element *RS_title=atg_create_element_label("Raid Result Statistics ", 15, (atg_colour){223, 255, 0, ATG_ALPHA_OPAQUE});
		if(!RS_title)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_pack_element(b, RS_title))
		{
			perror("atg_pack_element");
			return(1);
		}
		RS_resize=atg_create_element_image(resizebtn);
		if(!RS_resize)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		RS_resize->w=16;
		RS_resize->clickable=true;
		if(atg_pack_element(b, RS_resize))
		{
			perror("atg_pack_element");
			return(1);
		}
		RS_full=atg_create_element_image(fullbtn);
		if(!RS_full)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		RS_full->w=24;
		RS_full->clickable=true;
		if(atg_pack_element(b, RS_full))
		{
			perror("atg_pack_element");
			return(1);
		}
		RS_cont=atg_create_element_button("Continue", (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE}, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!RS_cont)
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		if(atg_pack_element(b, RS_cont))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	atg_element *RS_typerow=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 33, 33, ATG_ALPHA_OPAQUE}), *RS_typecol[ntypes], *RS_tocol;
	if(!RS_typerow)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	RS_typerow->h=72;
	if(atg_pack_element(rstatbox, RS_typerow))
	{
		perror("atg_pack_element");
		return(1);
	}
	else
	{
		atg_box *b=RS_typerow->elem.box;
		if(!b)
		{
			fprintf(stderr, "RS_typerow->elem.box==NULL\n");
			return(1);
		}
		atg_element *padding=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 31, 31, ATG_ALPHA_TRANSPARENT});
		if(!padding)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		padding->h=72;
		padding->w=RS_firstcol_w;
		if(atg_pack_element(b, padding))
		{
			perror("atg_pack_element");
			return(1);
		}
		for(unsigned int i=0;i<ntypes;i++)
		{
			RS_typecol[i]=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 35, 35, ATG_ALPHA_OPAQUE});
			if(!RS_typecol[i])
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				return(1);
			}
			RS_typecol[i]->h=72;
			RS_typecol[i]->w=RS_cell_w;
			if(atg_pack_element(b, RS_typecol[i]))
			{
				perror("atg_pack_element");
				return(1);
			}
			atg_box *b2=RS_typecol[i]->elem.box;
			if(!b2)
			{
				fprintf(stderr, "RS_typecol[%u]->elem.box==NULL\n", i);
				return(1);
			}
			SDL_Surface *pic=SDL_CreateRGBSurface(SDL_HWSURFACE, 36, 40, types[i].picture->format->BitsPerPixel, types[i].picture->format->Rmask, types[i].picture->format->Gmask, types[i].picture->format->Bmask, types[i].picture->format->Amask);
			if(!pic)
			{
				fprintf(stderr, "pic=SDL_CreateRGBSurface: %s\n", SDL_GetError());
				return(1);
			}
			SDL_FillRect(pic, &(SDL_Rect){0, 0, pic->w, pic->h}, SDL_MapRGB(pic->format, 0, 0, 0));
			SDL_BlitSurface(types[i].picture, NULL, pic, &(SDL_Rect){(36-types[i].picture->w)>>1, (40-types[i].picture->h)>>1, 0, 0});
			atg_element *picture=atg_create_element_image(pic);
			SDL_FreeSurface(pic);
			if(!picture)
			{
				fprintf(stderr, "atg_create_element_image failed\n");
				return(1);
			}
			picture->w=38;
			if(atg_pack_element(b2, picture))
			{
				perror("atg_pack_element");
				return(1);
			}
			atg_element *manu=atg_create_element_label(types[i].manu, 10, (atg_colour){239, 239, 0, ATG_ALPHA_OPAQUE});
			if(!manu)
			{
				fprintf(stderr, "atg_create_element_label failed\n");
				return(1);
			}
			if(atg_pack_element(b2, manu))
			{
				perror("atg_pack_element");
				return(1);
			}
			atg_element *name=atg_create_element_label(types[i].name, 12, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
			if(!name)
			{
				fprintf(stderr, "atg_create_element_label failed\n");
				return(1);
			}
			if(atg_pack_element(b2, name))
			{
				perror("atg_pack_element");
				return(1);
			}
		}
		RS_tocol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){49, 37, 37, ATG_ALPHA_OPAQUE});
		if(!RS_tocol)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		RS_tocol->h=72;
		RS_tocol->w=RS_cell_w;
		if(atg_pack_element(b, RS_tocol))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_box *tb=RS_tocol->elem.box;
		if(!tb)
		{
			fprintf(stderr, "RS_tocol->elem.box==NULL\n");
			return(1);
		}
		atg_element *total=atg_create_element_label("Total", 18, (atg_colour){255, 255, 255, ATG_ALPHA_TRANSPARENT});
		if(!total)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_pack_element(tb, total))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	
	fprintf(stderr, "GUI instantiated\n");
	
	main_menu:
	canvas->box=mainbox;
	atg_resize_canvas(canvas, 136, 86);
	atg_event e;
	while(1)
	{
		atg_flip(canvas);
		while(atg_poll_event(&e, canvas))
		{
			switch(e.type)
			{
				case ATG_EV_RAW:;
					SDL_Event s=e.event.raw;
					switch(s.type)
					{
						case SDL_QUIT:
							goto do_exit;
						break;
					}
				break;
				case ATG_EV_CLICK:;
					atg_ev_click c=e.event.click;
					if(c.e==MM_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
					}
					else if(c.e)
					{
						fprintf(stderr, "Clicked on unknown clickable!\n");
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==MM_Exit)
						goto do_exit;
					else if(trigger.e==MM_QuickStart)
					{
						fprintf(stderr, "Loading game state from Quick Start file...\n");
						if(!loadgame("save/qstart.sav", &state, lorw))
						{
							fprintf(stderr, "Quick Start Game loaded\n");
							goto gameloop;
						}
						else
						{
							fprintf(stderr, "Failed to load Quick Start save file\n");
						}
					}
					else if(trigger.e==MM_LoadGame)
					{
						*LB_btext=NULL;
						free(LB_file->elem.filepicker->value);
						LB_file->elem.filepicker->value=NULL;
						goto loader;
					}
					else if(!trigger.e)
					{
						// internal error
					}
					else
						fprintf(stderr, "Clicked on unknown button!\n");
				break;
				default:
				break;
			}
		}
		SDL_Delay(50);
	}
	
	loader:
	canvas->box=loadbox;
	atg_resize_canvas(canvas, mainsizex, mainsizey);
	while(1)
	{
		LB_file->w=mainsizex;
		LB_file->h=mainsizey-30;
		LB_text->w=mainsizex-64;
		atg_flip(canvas);
		while(atg_poll_event(&e, canvas))
		{
			switch(e.type)
			{
				case ATG_EV_RAW:;
					SDL_Event s=e.event.raw;
					switch(s.type)
					{
						case SDL_QUIT:
							goto main_menu;
						case SDL_VIDEORESIZE:
							mainsizex=canvas->surface->w;
							mainsizey=canvas->surface->h;
						break;
					}
				break;
				case ATG_EV_CLICK:;
					atg_ev_click c=e.event.click;
					if(c.e==LB_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
					}
					else if (c.e==LB_exit)
						goto main_menu;
					else if(c.e)
					{
						fprintf(stderr, "Clicked on unknown clickable!\n");
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==LB_load)
					{
						atg_filepicker *f=LB_file->elem.filepicker;
						if(!f)
						{
							fprintf(stderr, "Error: LB_file->elem.filepicker==NULL\n");
						}
						else if(!f->curdir)
						{
							fprintf(stderr, "Error: f->curdir==NULL\n");
						}
						else if(!f->value)
						{
							fprintf(stderr, "Select a file first!\n");
						}
						else
						{
							char *file=malloc(strlen(f->curdir)+strlen(f->value)+1);
							sprintf(file, "%s%s", f->curdir, f->value);
							fprintf(stderr, "Loading game state from '%s'...\n", file);
							if(!loadgame(file, &state, lorw))
							{
								fprintf(stderr, "Game loaded\n");
								mainsizex=canvas->surface->w;
								mainsizey=canvas->surface->h;
								goto gameloop;
							}
							else
							{
								fprintf(stderr, "Failed to load from save file\n");
							}
						}
					}
					else if(trigger.e==LB_text)
					{
						if(LB_btext&&*LB_btext)
							**LB_btext=0;
					}
					else if(!trigger.e)
					{
						// internal error
					}
					else
						fprintf(stderr, "Clicked on unknown button!\n");
				break;
				case ATG_EV_VALUE:;
					atg_ev_value value=e.event.value;
					if(value.e==LB_file)
					{
						atg_filepicker *f=LB_file->elem.filepicker;
						if(!f)
						{
							fprintf(stderr, "Error: LB_file->elem.filepicker==NULL\n");
						}
						else
						{
							if(LB_btext) *LB_btext=f->value;
						}
					}
				break;
				default:
				break;
			}
		}
		SDL_Delay(50);
	}
	
	saver:
	canvas->box=savebox;
	atg_resize_canvas(canvas, mainsizex, mainsizey);
	while(1)
	{
		SA_file->w=mainsizex;
		SA_file->h=mainsizey-30;
		SA_text->w=mainsizex-64;
		atg_flip(canvas);
		while(atg_poll_event(&e, canvas))
		{
			switch(e.type)
			{
				case ATG_EV_RAW:;
					SDL_Event s=e.event.raw;
					switch(s.type)
					{
						case SDL_QUIT:
							goto gameloop;
						case SDL_VIDEORESIZE:
							mainsizex=canvas->surface->w;
							mainsizey=canvas->surface->h;
						break;
						case SDL_KEYDOWN:;
							switch(s.key.keysym.sym)
							{
								case SDLK_BACKSPACE:
									if(SA_btext&&*SA_btext)
									{
										size_t l=strlen(*SA_btext);
										if(l)
											(*SA_btext)[l-1]=0;
									}
								break;
								case SDLK_RETURN:
									goto do_save;
								default:
									if((s.key.keysym.unicode&0xFF80)==0) // TODO handle UTF-8 in filenames?
									{
										char what=s.key.keysym.unicode&0x7F;
										if(what)
										{
											atg_filepicker *f=SA_file->elem.filepicker;
											if(!f)
												fprintf(stderr, "Error: SA_file->elem.filepicker==NULL\n");
											else if(f->value)
											{
												size_t l=strlen(f->value);
												char *n=realloc(f->value, l+2);
												if(n)
												{
													(f->value=n)[l]=what;
													n[l+1]=0;
													if(SA_btext)
														*SA_btext=f->value;
												}
												else
													perror("realloc");
											}
											else
											{
												f->value=malloc(2);
												if(f->value)
												{
													f->value[0]=what;
													f->value[1]=0;
													if(SA_btext)
														*SA_btext=f->value;
												}
												else
													perror("malloc");
											}
										}
									}
								break;
							}
						break;
					}
				break;
				case ATG_EV_CLICK:;
					atg_ev_click c=e.event.click;
					if(c.e==SA_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
					}
					else if (c.e==SA_exit)
						goto gameloop;
					else if(c.e)
					{
						fprintf(stderr, "Clicked on unknown clickable!\n");
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==SA_save)
					{
						do_save:;
						atg_filepicker *f=SA_file->elem.filepicker;
						if(!f)
						{
							fprintf(stderr, "Error: SA_file->elem.filepicker==NULL\n");
						}
						else if(!f->curdir)
						{
							fprintf(stderr, "Error: f->curdir==NULL\n");
						}
						else if(!f->value)
						{
							fprintf(stderr, "Select/enter a filename first!\n");
						}
						else
						{
							char *file=malloc(strlen(f->curdir)+strlen(f->value)+1);
							sprintf(file, "%s%s", f->curdir, f->value);
							fprintf(stderr, "Saving game state to '%s'...\n", file);
							if(!savegame(file, state))
							{
								fprintf(stderr, "Game saved\n");
								mainsizex=canvas->surface->w;
								mainsizey=canvas->surface->h;
								goto gameloop;
							}
							else
							{
								fprintf(stderr, "Failed to save to file\n");
							}
						}
					}
					else if(trigger.e==SA_text)
					{
						if(SA_btext&&*SA_btext)
							**SA_btext=0;
					}
					else if(!trigger.e)
					{
						// internal error
					}
					else
						fprintf(stderr, "Clicked on unknown button!\n");
				break;
				case ATG_EV_VALUE:;
					atg_ev_value value=e.event.value;
					if(value.e==SA_file)
					{
						atg_filepicker *f=SA_file->elem.filepicker;
						if(!f)
						{
							fprintf(stderr, "Error: SA_file->elem.filepicker==NULL\n");
						}
						else
						{
							if(SA_btext) *SA_btext=f->value;
						}
					}
				break;
				default:
				break;
			}
		}
		SDL_Delay(50);
	}
	
	gameloop:
	canvas->box=gamebox;
	atg_resize_canvas(canvas, mainsizex, mainsizey);
	snprintf(datestring, 11, "%02u-%02u-%04u\n", state.now.day, state.now.month, state.now.year);
	snprintf(GB_budget_label, 32, "Budget: £%u/day", state.cshr);
	snprintf(GB_confid_label, 32, "Confidence: %u%%", (unsigned int)floor(state.confid+0.5));
	snprintf(GB_morale_label, 32, "Morale: %u%%", (unsigned int)floor(state.morale+0.5));
	double moonphase=pom(state.now);
	double moonillum=foldpom(moonphase);
	drawmoon(GB_moonimg, moonphase);
	for(unsigned int j=0;j<state.nbombers;j++)
	{
		state.bombers[j].landed=true;
		state.bombers[j].damage=0;
		state.bombers[j].ldf=false;
	}
	double flakscale=state.gprod[ICLASS_ARM]/250000.0;
	bool shownav=false;
	filter_pff=0;
	for(unsigned int n=0;n<NNAVAIDS;n++)
	{
		if(!datebefore(state.now, event[navevent[n]])) shownav=true;
		filter_nav[n]=0;
	}
	if(GB_filters)
		GB_filters->hidden=!(shownav||!datebefore(state.now, event[EVENT_PFF]));
	for(unsigned int i=0;i<ntypes;i++)
	{
		if(GB_btrow[i])
			GB_btrow[i]->hidden=!datewithin(state.now, types[i].entry, types[i].exit);
		if(GB_btpc[i])
			GB_btpc[i]->h=18-min(types[i].pcbuf/10000, 18);
		if(GB_btnew[i])
			GB_btnew[i]->hidden=!datebefore(state.now, types[i].novelty);
		if(GB_btp[i])
			GB_btp[i]->hidden=(types[i].pribuf<8)||(state.cash<types[i].cost)||(types[i].pcbuf>=types[i].cost);
		if(GB_btnum[i]&&GB_btnum[i]->elem.label&&GB_btnum[i]->elem.label->text)
		{
			unsigned int svble=0,total=0;
			types[i].count=0;
			for(unsigned int n=0;n<NNAVAIDS;n++)
				types[i].navcount[n]=0;
			for(unsigned int j=0;j<state.nbombers;j++)
				if(state.bombers[j].type==i)
				{
					total++;
					if(!state.bombers[j].failed) svble++;
					types[i].count++;
					for(unsigned int n=0;n<NNAVAIDS;n++)
						if(state.bombers[j].nav[n]) types[i].navcount[n]++;
				}
			snprintf(GB_btnum[i]->elem.label->text, 12, "%u/%u", svble, total);
		}
		GB_navrow[i]->hidden=!shownav;
		for(unsigned int n=0;n<NNAVAIDS;n++)
		{
			update_navbtn(state, GB_navbtn, i, n, grey_overlay, yellow_overlay);
			if(GB_navgraph[i][n])
				GB_navgraph[i][n]->h=16-(types[i].count?(types[i].navcount[n]*16)/types[i].count:0);
		}
	}
	if(GB_go&&GB_go->elem.button&&GB_go->elem.button->content)
		GB_go->elem.button->content->bgcolour=(atg_colour){31, 63, 31, ATG_ALPHA_OPAQUE};
	for(unsigned int i=0;i<MAXMSGS;i++)
	{
		if(!GB_msgrow[i]) continue;
		GB_msgrow[i]->hidden=!state.msg[i];
		if(state.msg[i])
			if(GB_msgrow[i]->elem.button&&GB_msgrow[i]->elem.button->content)
			{
				atg_box *c=GB_msgrow[i]->elem.button->content;
				if(c->nelems&&c->elems[0]&&(c->elems[0]->type==ATG_LABEL)&&c->elems[0]->elem.label)
				{
					atg_label *l=c->elems[0]->elem.label;
					free(l->text);
					l->text=strndup(state.msg[i], min(strcspn(state.msg[i], "\n"), 33));
				}
			}
	}
	for(unsigned int i=0;i<ntargs;i++)
	{
		if(GB_ttrow[i])
			GB_ttrow[i]->hidden=!datewithin(state.now, targs[i].entry, targs[i].exit);
		if(GB_ttdmg[i])
			switch(targs[i].class)
			{
				case TCLASS_CITY:
				case TCLASS_LEAFLET: // for LEAFLET shows morale rather than damage
				case TCLASS_MINING: // for MINING shows how thoroughly mined the lane is
				case TCLASS_BRIDGE:
				case TCLASS_INDUSTRY:
				case TCLASS_AIRFIELD:
				case TCLASS_ROAD:
					GB_ttdmg[i]->w=floor(state.dmg[i]);
					GB_ttdmg[i]->hidden=false;
				break;
				case TCLASS_SHIPPING:
					GB_ttdmg[i]->w=0;
				break;
				default: // shouldn't ever get here
					fprintf(stderr, "Bad targs[%d].class = %d\n", i, targs[i].class);
					return(1);
				break;
			}
		if(GB_ttflk[i])
			GB_ttflk[i]->w=floor(state.flk[i]);
		for(unsigned int j=0;j<ntypes;j++)
		{
			if(GB_rbrow[i][j])
				GB_rbrow[i][j]->hidden=GB_btrow[j]?GB_btrow[j]->hidden:true;
			if(GB_raidload[i][j][1])
				GB_raidload[i][j][1]->hidden=datebefore(state.now, event[EVENT_PFF]);
			if(GB_raidnum[i][j]&&GB_raidnum[i][j]->elem.label&&GB_raidnum[i][j]->elem.label->text)
			{
				unsigned int count=0;
				for(unsigned int k=0;k<state.raids[i].nbombers;k++)
					if(state.bombers[state.raids[i].bombers[k]].type==j) count++;
				snprintf(GB_raidnum[i][j]->elem.label->text, 9, "%u", count);
			}
		}
		for(unsigned int j=0;j<state.raids[i].nbombers;j++)
			state.bombers[state.raids[i].bombers[j]].landed=false;
	}
	SDL_FreeSurface(GB_map->elem.image->data);
	GB_map->elem.image->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
	SDL_FreeSurface(flak_overlay);
	flak_overlay=render_flak(state.now);
	SDL_BlitSurface(flak_overlay, NULL, GB_map->elem.image->data, NULL);
	SDL_FreeSurface(target_overlay);
	target_overlay=render_targets(state.now);
	SDL_BlitSurface(target_overlay, NULL, GB_map->elem.image->data, NULL);
	SDL_FreeSurface(weather_overlay);
	weather_overlay=render_weather(state.weather);
	SDL_BlitSurface(weather_overlay, NULL, GB_map->elem.image->data, NULL);
	int seltarg=-1;
	xhair_overlay=render_xhairs(state, seltarg);
	SDL_BlitSurface(xhair_overlay, NULL, GB_map->elem.image->data, NULL);
	free(GB_raid_label->elem.label->text);
	GB_raid_label->elem.label->text=strdup("Select a Target");
	GB_raid->elem.box=GB_raidbox_empty;
	bool rfsh=true;
	while(1)
	{
		if(rfsh)
		{
			for(unsigned int i=0;i<ntargs;i++)
			{
				if(GB_ttrow[i]&&GB_ttrow[i]->elem.box)
				{
					unsigned int raid[ntypes];
					for(unsigned int j=0;j<ntypes;j++)
						raid[j]=0;
					for(unsigned int j=0;j<state.raids[i].nbombers;j++)
						raid[state.bombers[state.raids[i].bombers[j]].type]++;
					for(unsigned int j=0;j<ntypes;j++)
						if(GB_rbrow[i][j])
							GB_rbrow[i][j]->hidden=(GB_btrow[j]?GB_btrow[j]->hidden:true)||!raid[j];
					GB_ttrow[i]->elem.box->bgcolour=(state.raids[i].nbombers?(atg_colour){127, 103, 95, ATG_ALPHA_OPAQUE}:(atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
					if((int)i==seltarg)
					{
						GB_ttrow[i]->elem.box->bgcolour.r=GB_ttrow[i]->elem.box->bgcolour.g=GB_ttrow[i]->elem.box->bgcolour.r+64;
					}
					if(GB_ttrow[i]->elem.box->nelems>1)
					{
						if(GB_ttrow[i]->elem.box->elems[1]&&(GB_ttrow[i]->elem.box->elems[1]->type==ATG_BOX)&&GB_ttrow[i]->elem.box->elems[1]->elem.box)
						{
							GB_ttrow[i]->elem.box->elems[1]->elem.box->bgcolour=GB_ttrow[i]->elem.box->bgcolour;
						}
					}
				}
			}
			GB_raid->elem.box=(seltarg<0)?GB_raidbox_empty:GB_raidbox[seltarg];
			for(unsigned int i=0;i<ntypes;i++)
			{
				if(!GB_btrow[i]->hidden&&GB_btpic[i])
				{
					atg_image *img=GB_btpic[i]->elem.image;
					if(img)
					{
						SDL_Surface *pic=img->data;
						SDL_FillRect(pic, &(SDL_Rect){0, 0, pic->w, pic->h}, SDL_MapRGB(pic->format, 0, 0, 0));
						SDL_BlitSurface(types[i].picture, NULL, pic, &(SDL_Rect){(36-types[i].picture->w)>>1, (40-types[i].picture->h)>>1, 0, 0});
						if(seltarg>=0)
						{
							double dist=hypot((signed)types[i].blat-(signed)targs[seltarg].lat, (signed)types[i].blon-(signed)targs[seltarg].lon)*1.6;
							if(types[i].range<dist)
								SDL_BlitSurface(grey_overlay, NULL, pic, NULL);
						}
					}
				}
			}
			SDL_FreeSurface(GB_map->elem.image->data);
			GB_map->elem.image->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
			SDL_BlitSurface(flak_overlay, NULL, GB_map->elem.image->data, NULL);
			SDL_BlitSurface(target_overlay, NULL, GB_map->elem.image->data, NULL);
			SDL_BlitSurface(weather_overlay, NULL, GB_map->elem.image->data, NULL);
			SDL_FreeSurface(xhair_overlay);
			xhair_overlay=render_xhairs(state, seltarg);
			SDL_BlitSurface(xhair_overlay, NULL, GB_map->elem.image->data, NULL);
			atg_flip(canvas);
			rfsh=false;
		}
		while(atg_poll_event(&e, canvas))
		{
			if(e.type!=ATG_EV_RAW) rfsh=true;
			switch(e.type)
			{
				case ATG_EV_RAW:;
					SDL_Event s=e.event.raw;
					switch(s.type)
					{
						case SDL_QUIT:
							do_quit:
							free(state.hist.ents);
							state.hist.ents=NULL;
							state.hist.nents=state.hist.nalloc=0;
							for(unsigned int i=0;i<ntargs;i++)
							{
								state.raids[i].nbombers=0;
								free(state.raids[i].bombers);
								state.raids[i].bombers=NULL;
							}
							mainsizex=canvas->surface->w;
							mainsizey=canvas->surface->h;
							goto main_menu;
						break;
						case SDL_VIDEORESIZE:
							rfsh=true;
						break;
					}
				break;
				case ATG_EV_CLICK:;
					atg_ev_click c=e.event.click;
					atg_mousebutton b=c.button;
					SDLMod m = SDL_GetModState();
					if(c.e)
					{
						for(unsigned int i=0;i<ntypes;i++)
						{
							if(!datewithin(state.now, types[i].entry, types[i].exit)) continue;
							for(unsigned int n=0;n<NNAVAIDS;n++)
							{
								if(c.e==GB_navbtn[i][n])
								{
									if(b==ATG_MB_LEFT)
									{
										if(types[i].nav[n]&&!datebefore(state.now, event[navevent[n]]))
										{
											state.nap[n]=i;
											for(unsigned int j=0;j<ntypes;j++)
												update_navbtn(state, GB_navbtn, j, n, grey_overlay, yellow_overlay);
										}
									}
									else if(b==ATG_MB_RIGHT)
										fprintf(stderr, "%ux %s in %ux %s %s\n", types[i].navcount[n], navaids[n], types[i].count, types[i].manu, types[i].name);
								}
							}
							if(c.e==GB_btpic[i])
							{
								if(seltarg<0)
									fprintf(stderr, "btrow %d\n", i);
								else
								{
									double dist=hypot((signed)types[i].blat-(signed)targs[seltarg].lat, (signed)types[i].blon-(signed)targs[seltarg].lon)*1.6;
									if(types[i].range<dist)
										fprintf(stderr, "insufficient range: %u<%g\n", types[i].range, dist);
									else
									{
										unsigned int amount;
										switch(b)
										{
											case ATG_MB_RIGHT:
											case ATG_MB_SCROLLDN:
												amount=1;
											break;
											case ATG_MB_MIDDLE:
												amount=-1;
											break;
											case ATG_MB_LEFT:
												if(m&KMOD_CTRL)
												{
													amount=-1;
													break;
												}
												/* else fallthrough */
											case ATG_MB_SCROLLUP:
											default:
												amount=10;
										}
										for(unsigned int j=0;j<state.nbombers;j++)
										{
											if(state.bombers[j].type!=i) continue;
											if(state.bombers[j].failed) continue;
											if(!state.bombers[j].landed) continue;
											if(!filter_apply(state.bombers[j], filter_pff, filter_nav)) continue;
											state.bombers[j].landed=false;
											amount--;
											unsigned int n=state.raids[seltarg].nbombers++;
											unsigned int *new=realloc(state.raids[seltarg].bombers, state.raids[seltarg].nbombers*sizeof(unsigned int));
											if(!new)
											{
												perror("realloc");
												state.raids[seltarg].nbombers=n;
												break;
											}
											(state.raids[seltarg].bombers=new)[n]=j;
											if(!amount) break;
										}
										if(GB_raidnum[seltarg][i]&&GB_raidnum[seltarg][i]->elem.label&&GB_raidnum[seltarg][i]->elem.label->text)
										{
											unsigned int count=0;
											for(unsigned int j=0;j<state.raids[seltarg].nbombers;j++)
												if(state.bombers[state.raids[seltarg].bombers[j]].type==i) count++;
											snprintf(GB_raidnum[seltarg][i]->elem.label->text, 9, "%u", count);
										}
									}
								}
							}
							if((c.e==GB_btdesc[i])&&types[i].text)
							{
								message_box(canvas, "From the Bomber Command files:", types[i].text, "R.H.M.S. Saundby, SASO");
							}
						}
						if(c.e==GB_ttl)
						{
							seltarg=-1;
							free(GB_raid_label->elem.label->text);
							GB_raid_label->elem.label->text=strdup("Select a Target");
							GB_raid->elem.box=GB_raidbox_empty;
						}
						for(unsigned int i=0;i<ntargs;i++)
						{
							if(!datewithin(state.now, targs[i].entry, targs[i].exit)) continue;
							if(c.e==GB_ttint[i])
							{
								intel *ti=targs[i].p_intel;
								if(ti&&ti->ident&&ti->text)
								{
									size_t il=strlen(ti->ident), ml=il+32;
									char *msg=malloc(ml);
									snprintf(msg, ml, "Target Intelligence File: %s", ti->ident);
									message_box(canvas, msg, ti->text, "Wg Cdr Fawssett, OC Targets");
								}
							}
							if(c.e==GB_ttrow[i])
							{
								seltarg=i;
								free(GB_raid_label->elem.label->text);
								size_t ll=9+strlen(targs[i].name);
								GB_raid_label->elem.label->text=malloc(ll);
								snprintf(GB_raid_label->elem.label->text, ll, "Raid on %s", targs[i].name);
								GB_raid->elem.box=GB_raidbox[i];
							}
							for(unsigned int j=0;j<ntypes;j++)
							{
								if(c.e==GB_rbpic[i][j])
								{
									unsigned int amount;
									switch(b)
									{
										case ATG_MB_RIGHT:
										case ATG_MB_SCROLLDN:
											amount=1;
										break;
										case ATG_MB_MIDDLE:
											amount=-1;
										break;
										case ATG_MB_LEFT:
											if(m&KMOD_CTRL)
											{
												amount=-1;
												break;
											}
											/* else fallthrough */
										case ATG_MB_SCROLLUP:
										default:
											amount=10;
									}
									for(unsigned int l=0;l<state.raids[i].nbombers;l++)
									{
										unsigned int k=state.raids[i].bombers[l];
										if(state.bombers[k].type!=j) continue;
										if(!filter_apply(state.bombers[k], filter_pff, filter_nav)) continue;
										state.bombers[k].landed=true;
										amount--;
										state.raids[i].nbombers--;
										for(unsigned int k=l;k<state.raids[i].nbombers;k++)
											state.raids[i].bombers[k]=state.raids[i].bombers[k+1];
										if(!amount) break;
										l--;
									}
									if(GB_raidnum[i][j]&&GB_raidnum[i][j]->elem.label&&GB_raidnum[i][j]->elem.label->text)
									{
										unsigned int count=0;
										for(unsigned int l=0;l<state.raids[i].nbombers;l++)
											if(state.bombers[state.raids[i].bombers[l]].type==j) count++;
										snprintf(GB_raidnum[i][j]->elem.label->text, 9, "%u", count);
									}
								}
							}
						}
						if(c.e==GB_resize)
						{
							mainsizex=default_w;
							mainsizey=default_h;
							atg_resize_canvas(canvas, mainsizex, mainsizey);
						}
						if(c.e==GB_full)
						{
							fullscreen=!fullscreen;
							atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
							rfsh=true;
						}
						if(c.e==GB_exit)
							goto do_quit;
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e)
					{
						if(trigger.e==GB_go)
						{
							if(GB_go&&GB_go->elem.button&&GB_go->elem.button->content)
								GB_go->elem.button->content->bgcolour=(atg_colour){55, 55, 55, ATG_ALPHA_OPAQUE};
							atg_flip(canvas);
							goto run_raid;
						}
						else if(trigger.e==GB_save)
						{
							mainsizex=canvas->surface->w;
							mainsizey=canvas->surface->h;
							goto saver;
						}
						for(unsigned int i=0;i<MAXMSGS;i++)
							if((trigger.e==GB_msgrow[i])&&state.msg[i])
							{
								message_box(canvas, "To the Commander-in-Chief, Bomber Command:", state.msg[i], "Air Chief Marshal C. F. A. Portal, CAS");
							}
					}
				break;
				case ATG_EV_VALUE:
					// ignore
				break;
				default:
					fprintf(stderr, "e.type %d\n", e.type);
				break;
			}
		}
		SDL_Delay(50);
		#if 0 // for testing weathersim code
		rfsh=true;
		for(unsigned int i=0;i<16;i++)
			w_iter(&state.weather, lorw);
		SDL_FreeSurface(GB_map->elem.image->data);
		GB_map->elem.image->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
		SDL_FillRect(GB_map->elem.image->data, &(SDL_Rect){.x=0, .y=0, .w=terrain->w, .h=terrain->h}, SDL_MapRGB(terrain->format, 0, 0, 0));
		SDL_FreeSurface(weather_overlay);
		weather_overlay=render_weather(state.weather);
		SDL_BlitSurface(weather_overlay, NULL, GB_map->elem.image->data, NULL);
		SDL_FreeSurface(xhair_overlay);
		xhair_overlay=render_xhairs(state, seltarg);
		SDL_BlitSurface(xhair_overlay, NULL, GB_map->elem.image->data, NULL);
		#endif
	}
	
	run_raid:
	state.roe.idtar=datebefore(state.now, event[EVENT_CIV]);
	unsigned int totalraids, fightersleft;
	totalraids=0;
	fightersleft=state.nfighters;
	for(unsigned int i=0;i<ntargs;i++)
	{
		totalraids+=state.raids[i].nbombers;
		for(unsigned int j=0;j<state.raids[i].nbombers;j++)
		{
			unsigned int k=state.raids[i].bombers[j];
			ra_append(&state.hist, state.now, (time){20, 00}, state.bombers[k].id, false, state.bombers[k].type, i);
		}
		targs[i].threat=0;
		targs[i].nfighters=0;
		targs[i].fires=0;
		targs[i].skym=-1;
	}
	for(unsigned int i=0;i<nflaks;i++)
		flaks[i].ftr=-1;
	for(unsigned int i=0;i<state.nfighters;i++)
	{
		state.fighters[i].targ=-1;
		state.fighters[i].k=-1;
		state.fighters[i].hflak=-1;
		state.fighters[i].damage=0;
		state.fighters[i].landed=true;
		state.fighters[i].lat=fbases[state.fighters[i].base].lat;
		state.fighters[i].lon=fbases[state.fighters[i].base].lon;
	}
	if(totalraids)
	{
		canvas->box=raidbox;
		if(RB_time_label) snprintf(RB_time_label, 6, "21:00");
		SDL_FreeSurface(RB_map->elem.image->data);
		RB_map->elem.image->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
		SDL_Surface *with_flak=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
		SDL_FreeSurface(flak_overlay);
		flak_overlay=render_flak(state.now);
		SDL_BlitSurface(flak_overlay, NULL, with_flak, NULL);
		SDL_Surface *with_target=SDL_ConvertSurface(with_flak, with_flak->format, with_flak->flags);
		SDL_FreeSurface(target_overlay);
		target_overlay=render_targets(state.now);
		SDL_BlitSurface(target_overlay, NULL, with_target, NULL);
		SDL_Surface *with_weather=SDL_ConvertSurface(with_target, with_target->format, with_target->flags);
		SDL_FreeSurface(weather_overlay);
		weather_overlay=render_weather(state.weather);
		SDL_BlitSurface(weather_overlay, NULL, with_weather, NULL);
		SDL_BlitSurface(with_weather, NULL, RB_map->elem.image->data, NULL);
		bool stream=!datebefore(state.now, event[EVENT_GEE]),
		     moonshine=!datebefore(state.now, event[EVENT_MOONSHINE]),
		     window=!datebefore(state.now, event[EVENT_WINDOW]),
		     wairad= datewithin(state.now, event[EVENT_WINDOW], event[EVENT_L_SN]);
		unsigned int it=0, startt=768;
		unsigned int plan[60], act[60][2];
		bool skym[120];
		double fire[120];
		for(unsigned int dt=0;dt<60;dt++)
			plan[dt]=act[dt][0]=act[dt][1]=0;
		for(unsigned int dt=0;dt<120;dt++)
		{
			fire[dt]=0;
			skym[dt]=false;
		}
		for(unsigned int i=0;i<ntargs;i++)
		{
			unsigned int halfhalf=0; // count for halfandhalf bombloads
			if(state.raids[i].nbombers && stream)
			{
				genroute((unsigned int [2]){0, 0}, i, targs[i].route, state, 10000);
			}
			for(unsigned int j=0;j<state.raids[i].nbombers;j++)
			{
				unsigned int k=state.raids[i].bombers[j], type=state.bombers[k].type;
				state.bombers[k].targ=i;
				state.bombers[k].lat=types[type].blat;
				state.bombers[k].lon=types[type].blon;
				state.bombers[k].routestage=0;
				if(stream)
					for(unsigned int l=0;l<8;l++)
					{
						state.bombers[k].route[l][0]=targs[i].route[l][0];
						state.bombers[k].route[l][1]=targs[i].route[l][1];
					}
				else
					genroute((unsigned int [2]){types[type].blat, types[type].blon}, i, state.bombers[k].route, state, 100);
				double dist=hypot((signed)types[type].blat-(signed)state.bombers[k].route[0][0], (signed)types[type].blon-(signed)state.bombers[k].route[0][1]), outward=dist;
				for(unsigned int l=0;l<7;l++)
				{
					double d=hypot((signed)state.bombers[k].route[l+1][0]-(signed)state.bombers[k].route[l][0], (signed)state.bombers[k].route[l+1][1]-(signed)state.bombers[k].route[l][1]);
					dist+=d;
					if(l<4) outward+=d;
				}
				unsigned int cap=types[type].cap;
				state.bombers[k].bombed=false;
				state.bombers[k].crashed=false;
				state.bombers[k].landed=false;
				state.bombers[k].idtar=false;
				state.bombers[k].lat+=rand()*3.0/RAND_MAX-1;
				state.bombers[k].lon+=rand()*3.0/RAND_MAX-1;
				state.bombers[k].navlat=0;
				state.bombers[k].navlon=0;
				state.bombers[k].driftlat=0;
				state.bombers[k].driftlon=0;
				state.bombers[k].speed=types[type].speed/450.0;
				if(stream)
				{
					// aim for Zero Hour 01:00 plus up to 10 minutes
					// PFF should arrive at Zero minus 6, and be finished by Zero minus 2
					// Zero Hour is t=480, and a minute is two t-steps
					int tt=state.bombers[k].pff?(468+irandu(8)):(480+irandu(20));
					int st=tt-(outward/state.bombers[k].speed)-3;
					if(state.bombers[k].pff) st-=3;
					if(st<0)
					{
						tt-=st;
						st=0;
					}
					state.bombers[k].startt=st;
					if((tt>=450)&&(tt<570)&&(targs[i].class==TCLASS_CITY))
						plan[(tt-450)/2]++;
				}
				else
					state.bombers[k].startt=irandu(90);
				startt=min(startt, state.bombers[k].startt);
				state.bombers[k].fuelt=state.bombers[k].startt+types[type].range*0.6/(double)state.bombers[k].speed;
				unsigned int eta=state.bombers[k].startt+outward*1.1/(double)state.bombers[k].speed+12;
				if(!stream) eta+=36;
				if(eta>state.bombers[k].fuelt)
				{
					unsigned int fu=eta-state.bombers[k].fuelt;
					state.bombers[k].fuelt+=fu;
					cap*=120.0/(120.0+fu);
				}
				else
					state.bombers[k].fuelt=eta;
				state.bombers[k].b_hc=0;
				state.bombers[k].b_gp=0;
				state.bombers[k].b_in=0;
				state.bombers[k].b_ti=0;
				state.bombers[k].b_le=0;
				unsigned int bulk=types[type].cap;
				bool inext=true;
				switch(targs[i].class)
				{
					case TCLASS_LEAFLET:
						state.bombers[k].b_le=min(types[type].cap*3, cap*20);
					break;
					case TCLASS_CITY:
						switch(state.bombers[k].pff?state.raids[i].pffloads[type]:state.raids[i].loads[type])
						{
							case BL_PLUMDUFF:
								transfer(4000, cap, state.bombers[k].b_hc);
								while(cap&&(bulk=types[type].cap-loadbulk(state.bombers[k])))
								{
									if(inext)
										transfer(min(bulk/1.5, 800), cap, state.bombers[k].b_in);
									else
										transfer(min(bulk, 500), cap, state.bombers[k].b_gp);
									inext=!inext;
								}
							break;
							case BL_USUAL:
								if(types[type].load[BL_PLUMDUFF]&&types[type].cap>=10000) // cookie + incendiaries
								{
									transfer(4000, cap, state.bombers[k].b_hc);
									bulk=types[type].cap-loadbulk(state.bombers[k]);
									state.bombers[k].b_in=min(cap, bulk/1.5);
								}
								else // gp+in mix
								{
									while(cap&&(bulk=types[type].cap-loadbulk(state.bombers[k])))
									{
										if(inext)
											transfer(min(bulk/1.5, 800), cap, state.bombers[k].b_in);
										else
											transfer(min(bulk, types[type].inc?250:500), cap, state.bombers[k].b_gp);
										inext=!inext;
									}
								}
							break;
							case BL_ARSON:
								state.bombers[k].b_in=min(cap, bulk/1.5);
							break;
							case BL_HALFHALF:
								if(cap>=4000&&(halfhalf++%2))
								{
									state.bombers[k].b_hc=(cap/4000)*4000;
									break;
								}
								// else fallthrough to BL_ILLUM
							case BL_ILLUM:
								while(cap&&(bulk=types[type].cap-loadbulk(state.bombers[k])))
								{
									if(inext)
										transfer(min(bulk/2.0, 250), cap, state.bombers[k].b_ti);
									else
										transfer(min(bulk, 500), cap, state.bombers[k].b_gp);
									inext=!inext;
								}
							break;
							case BL_PPLUS: // LanX, up to 12,000lb cookie; LanI, up to 8,000lb cookie
								transfer(min(types[type].cap/5333, cap/4000)*4000, cap, state.bombers[k].b_hc);
								state.bombers[k].b_gp=cap;
							break;
							case BL_PONLY:
								if(cap>=4000)
								{
									state.bombers[k].b_hc=(cap/4000)*4000;
									break;
								}
								// else fallthrough to BL_ABNORMAL
							case BL_ABNORMAL:
							default:
								state.bombers[k].b_gp=cap;
							break;
						}
					break;
					default: // all other targets use all-GP loads
						state.bombers[k].b_gp=cap;
					break;
				}
				#define b_roundto(what, n)	state.bombers[k].b_##what=(state.bombers[k].b_##what/n)*n
				b_roundto(hc, 4000); // should never affect anything
				b_roundto(gp, 50);
				b_roundto(in, 10);
				b_roundto(ti, 50);
				b_roundto(le, 1000);
				//fprintf(stderr, "%s: %ulb hc + %u lb gp + %ulb in + %ulb ti = %ulb\n", types[type].name, state.bombers[k].b_hc, state.bombers[k].b_gp, state.bombers[k].b_in, state.bombers[k].b_ti, loadweight(state.bombers[k]));
				#undef b_roundto
			}
		}
		oboe.k=-1;
		SDL_Surface *with_ac=SDL_ConvertSurface(with_weather, with_weather->format, with_weather->flags);
		SDL_Surface *ac_overlay=render_ac(state);
		SDL_BlitSurface(ac_overlay, NULL, with_ac, NULL);
		SDL_BlitSurface(with_ac, NULL, RB_map->elem.image->data, NULL);
		unsigned int inair=totalraids, t=0;
		double cidam=0;
		unsigned int kills[2]={0, 0};
		// Tame Boar raid tracking
		bool tameboar=!datebefore(state.now, event[EVENT_TAMEBOAR]);
		unsigned int boxes[16][16]; // 10x10 boxes starting at (89,40)
		int topx=-1, topy=-1; // co-ords of fullest box
		unsigned int sumx, sumy; // sum of offsets within box
		double velx, vely; // sum of velocity components ditto
		while(t<startt)
		{
			t++;
			if((!(t&1))&&(it<512))
			{
				w_iter(&state.weather, lorw);
				it++;
			}
		}
		SDL_FreeSurface(weather_overlay);
		weather_overlay=render_weather(state.weather);
		SDL_BlitSurface(with_target, NULL, with_weather, NULL);
		SDL_BlitSurface(weather_overlay, NULL, with_weather, NULL);
		SDL_FillRect(RB_atime_image, &(SDL_Rect){.x=0, .y=0, .w=RB_atime_image->w, .h=RB_atime_image->h}, SDL_MapRGBA(RB_atime_image->format, GAME_BG_COLOUR.r, GAME_BG_COLOUR.g, GAME_BG_COLOUR.b, GAME_BG_COLOUR.a));
		while(inair)
		{
			t++;
			if(RB_time_label) snprintf(RB_time_label, 6, "%02u:%02u", (21+(t/120))%24, (t/2)%60);
			time now = {(21+(t/120))%24, (t/2)%60};
			if((!(t&1))&&(it<512))
			{
				w_iter(&state.weather, lorw);
				SDL_FreeSurface(weather_overlay);
				weather_overlay=render_weather(state.weather);
				SDL_BlitSurface(with_target, NULL, with_weather, NULL);
				SDL_BlitSurface(weather_overlay, NULL, with_weather, NULL);
				it++;
			}
			if(stream&&(t>=450)&&(t<571))
			{
				SDL_FillRect(RB_atime_image, &(SDL_Rect){.x=0, .y=0, .w=RB_atime_image->w, .h=RB_atime_image->h}, SDL_MapRGB(RB_atime_image->format, 15, 15, 15));
				SDL_FillRect(RB_atime_image, &(SDL_Rect){.x=0, .y=239, .w=600, .h=1}, SDL_MapRGB(RB_atime_image->format, 255, 255, 255));
				unsigned int nrt=0;
				for(unsigned int i=0;i<ntargs;i++)
				{
					if(state.raids[i].nbombers&&(targs[i].class==TCLASS_CITY))
					{
						nrt++;
						fire[t-450]+=targs[i].fires;
					}
				}
				if(nrt)
					fire[t-450]/=nrt;
				for(unsigned int dt=0;dt<60;dt++)
				{
					unsigned int x=dt*10, ph=min(plan[dt]*1200/totalraids, 240);
					bool pff=(dt<15);
					SDL_FillRect(RB_atime_image, &(SDL_Rect){.x=x, .y=240-ph, .w=10, .h=ph}, SDL_MapRGB(RB_atime_image->format, pff?0:127, 0, pff?127:0));
					if(ph>2)
						SDL_FillRect(RB_atime_image, &(SDL_Rect){.x=x+1, .y=241-ph, .w=8, .h=ph-2}, SDL_MapRGB(RB_atime_image->format, 15, 15, 15));
					unsigned int h[2];
					for(unsigned int i=0;i<2;i++)
						h[i]=min(act[dt][i]*1200/totalraids, 240);
					h[1]=min(h[1], 240-h[0]);
					SDL_FillRect(RB_atime_image, &(SDL_Rect){.x=x+1, .y=240-h[0], .w=8, .h=h[0]}, SDL_MapRGB(RB_atime_image->format, 0, 0, 255));
					SDL_FillRect(RB_atime_image, &(SDL_Rect){.x=x+1, .y=240-h[0]-h[1], .w=8, .h=h[1]}, SDL_MapRGB(RB_atime_image->format, 255, 0, 0));
				}
				for(unsigned int dt=0;dt<min(119, t-451);dt++)
				{
					unsigned int x=dt*5+2, y[2]={max(240-fire[dt]/6, 0), max(240-fire[dt+1]/6, 0)};
					line(RB_atime_image, x, y[0], x+5, y[1], (atg_colour){127, 127, 0, ATG_ALPHA_OPAQUE});
					if(skym[dt])
						line(RB_atime_image, x, 235, x, 239, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
				}
			}
			if(tameboar)
			{
				memset(boxes, 0, sizeof(boxes));
				sumx=sumy=0;
				velx=vely=0;
			}
			for(unsigned int i=0;i<ntargs;i++)
				targs[i].shots=0;
			for(unsigned int i=0;i<nflaks;i++)
			{
				flaks[i].shots=0;
				flaks[i].heat=0;
			}
			for(unsigned int i=0;i<ntargs;i++)
				for(unsigned int j=0;j<state.raids[i].nbombers;j++)
				{
					unsigned int k=state.raids[i].bombers[j], type=state.bombers[k].type;
					if(t<state.bombers[k].startt) continue;
					if(state.bombers[k].crashed||state.bombers[k].landed) continue;
					if(state.bombers[k].damage>=100)
					{
						cr_append(&state.hist, state.now, now, state.bombers[k].id, false, state.bombers[k].type);
						state.bombers[k].crashed=true;
						if(state.bombers[k].damage)
						{
							if(state.bombers[k].ldf)
								kills[1]++;
							else
								kills[0]++;
						}
						inair--;
						continue;
					}
					if(brandp(types[type].fail/(50.0*min(240.0, 48.0+t-state.bombers[k].startt))+state.bombers[k].damage/2400.0))
					{
						fa_append(&state.hist, state.now, now, state.bombers[k].id, false, state.bombers[k].type, 1);
						state.bombers[k].failed=true;
						if(brandp((1.0+state.bombers[k].damage/50.0)/(240.0-types[type].fail*5.0)))
						{
							cr_append(&state.hist, state.now, now, state.bombers[k].id, false, state.bombers[k].type);
							state.bombers[k].crashed=true;
							if(state.bombers[k].damage)
							{
								if(state.bombers[k].ldf)
									kills[1]++;
								else
									kills[0]++;
							}
							inair--;
							continue;
						}
					}
					unsigned int stage=state.bombers[k].routestage;
					while((stage<8)&&!(state.bombers[k].route[stage][0]||state.bombers[k].route[stage][1]))
						stage=++state.bombers[k].routestage;
					bool home=state.bombers[k].failed||(stage>=8);
					if((stage==4)&&state.bombers[k].nav[NAV_OBOE]&&xyr(state.bombers[k].lon-oboe.lon, state.bombers[k].lat-oboe.lat, 50+types[type].alt*.3)) // OBOE
					{
						if(oboe.k==-1)
							oboe.k=k;
						if(oboe.k==(int)k)
						{
							state.bombers[k].navlon=state.bombers[k].navlat=0;
							state.bombers[k].fix=true;
						}
						// the rest of these involve taking priority over an existing user and so they grab the k but don't get to use this turn
						if(!state.bombers[oboe.k].b_ti) // PFF priority and priority for bigger loads
						{
							if(state.bombers[k].b_ti||((state.bombers[k].b_hc+state.bombers[k].b_gp)>(state.bombers[oboe.k].b_hc+state.bombers[oboe.k].b_gp)))
								oboe.k=k;
						}
					}
					else if(oboe.k==(int)k)
						oboe.k=-1;
					int destx=home?types[type].blon:state.bombers[k].route[stage][1],
						desty=home?types[type].blat:state.bombers[k].route[stage][0];
					double thinklon=state.bombers[k].lon+state.bombers[k].navlon,
						thinklat=state.bombers[k].lat+state.bombers[k].navlat;
					clamp(thinklon, 0, 256);
					clamp(thinklat, 0, 256);
					double cx=destx-thinklon, cy=desty-thinklat;
					double d=hypot(cx, cy);
					if(home)
					{
						if(d<0.7)
						{
							if(state.bombers[k].lon<63)
							{
								state.bombers[k].landed=true;
								inair--;
							}
							else
							{
								state.bombers[k].navlon=0;
							}
						}
						else if(t>1200)
						{
							state.bombers[k].navlon=state.bombers[k].navlat=0;
							state.bombers[k].lon=min(state.bombers[k].lon, 127);
						}
					}
					else if(stage==4)
					{
						bool fuel=(t>=state.bombers[k].fuelt);
						bool damaged=(state.bombers[k].damage>=8);
						bool roeok=state.bombers[k].idtar||(!state.roe.idtar&&brandp(0.2))||brandp(0.005);
						bool leaf=state.bombers[k].b_le;
						bool pffstop=stream&&(t<(state.bombers[k].pff?464:480));
						double cr=1.2;
						if(oboe.k==(int)k) cr=0.3;
						unsigned int dm=0; // target crew believes is nearest
						double mind=1e6;
						for(unsigned int i=0;i<ntargs;i++)
						{
							double dx=state.bombers[k].lon+state.bombers[k].navlon-targs[i].lon, dy=state.bombers[k].lat+state.bombers[k].navlat-targs[i].lat, dd=dx*dx+dy*dy;
							if(dd<mind)
							{
								mind=dd;
								dm=i;
							}
						}
						if(((fabs(cx)<cr)&&(fabs(cy)<cr)&&roeok&&!pffstop&&(dm==state.bombers[k].targ))||((fuel||damaged)&&(roeok||leaf)))
						{
							state.bombers[k].bmblon=state.bombers[k].lon;
							state.bombers[k].bmblat=state.bombers[k].lat;
							state.bombers[k].navlon=targs[dm].lon-state.bombers[k].lon;
							state.bombers[k].navlat=targs[dm].lat-state.bombers[k].lat;
							state.bombers[k].bt=t;
							state.bombers[k].bombed=true;
							if(!leaf)
							{
								for(unsigned int ta=0;ta<ntargs;ta++)
								{
									if(targs[ta].class!=TCLASS_CITY) continue;
									if(!datewithin(state.now, targs[ta].entry, targs[ta].exit)) continue;
									int dx=floor(state.bombers[k].bmblon+.5)-targs[ta].lon, dy=floor(state.bombers[k].bmblat+.5)-targs[ta].lat;
									int hx=targs[ta].picture->w/2;
									int hy=targs[ta].picture->h/2;
									if((abs(dx)<=hx)&&(abs(dy)<=hy)&&(pget(targs[ta].picture, dx+hx, dy+hy).a==ATG_ALPHA_OPAQUE))
									{
										unsigned int he=state.bombers[k].b_hc+state.bombers[k].b_gp;
										hi_append(&state.hist, state.now, maketime(t), state.bombers[k].id, false, type, ta, he+state.bombers[k].b_in+state.bombers[k].b_ti);
										state.flam[ta]=min(state.flam[ta]+state.bombers[k].b_hc/5000.0, 100); // HC (cookies) increase target flammability
										double maybe_dmg=(he*1.2+state.bombers[k].b_in*(targs[ta].flammable?2.4:1.5)*state.flam[ta]/40.0)/(targs[ta].psiz*10000.0);
										double dmg=min(state.dmg[ta], maybe_dmg);
										cidam+=dmg*(targs[ta].berlin?2.0:1.0);
										state.dmg[ta]-=dmg;
										tdm_append(&state.hist, state.now, maketime(t), ta, dmg, state.dmg[ta]);
										int dt=(t-450)/2;
										if((dt>=0)&&(dt<60))
											act[dt][state.bombers[k].pff?0:1]++;
										if(state.bombers[k].pff&&state.bombers[k].fix)
										{
											targs[ta].skym=t;
											int dt=t-450;
											if((dt>0)&&(dt<120))
												skym[dt]=true;
										}
										targs[ta].fires+=state.bombers[k].b_in*(state.flam[ta]/40.0)/1500+state.bombers[k].b_ti/30;
										break;
									}
								}
							}
							stage=++state.bombers[k].routestage;
						}
						else if(fuel)
							stage=++state.bombers[k].routestage;
					}
					else if(stage<4)
					{
						if(cx<0.2)
							stage=++state.bombers[k].routestage;
					}
					else
						if(cx>-0.4)
							stage=++state.bombers[k].routestage;
					state.bombers[k].idtar=false;
					double dx=cx*state.bombers[k].speed/d,
						dy=cy*state.bombers[k].speed/d;
					if(d==0) dx=dy=-1;
					state.bombers[k].lon+=dx;
					state.bombers[k].lat+=dy;
					for(unsigned int i=0;i<ntargs;i++)
					{
						if(!datewithin(state.now, targs[i].entry, targs[i].exit)) continue;
						double dx=state.bombers[k].lon-targs[i].lon, dy=state.bombers[k].lat-targs[i].lat, dd=dx*dx+dy*dy;
						double range;
						switch(targs[i].class)
						{
							case TCLASS_CITY:
							case TCLASS_LEAFLET:
								range=3.6;
							break;
							case TCLASS_SHIPPING:
								range=2.2;
							break;
							case TCLASS_AIRFIELD:
							case TCLASS_ROAD:
								range=1.6;
							break;
							case TCLASS_MINING:
								range=2.0;
							break;
							case TCLASS_BRIDGE:
							case TCLASS_INDUSTRY:
								range=1.1;
							break;
							default:
								fprintf(stderr, "Bad targs[%d].class = %d\n", i, targs[i].class);
								range=0.6;
							break;
						}
						if(dd<range*range)
						{
							unsigned int x=targs[i].lon/2, y=targs[i].lat/2;
							double wea=((x<128)&&(y<128))?state.weather.p[x][y]-1000:0;
							double preccap=100.0/(5.0*(moonillum+.3)/(double)(8+max(4-wea, 0)));
							if(brandp(state.flk[i]*flakscale/min((9+targs[i].shots++)*40.0, preccap)))
							{
								double ddmg;
								if(brandp(types[type].defn/400.0))
									ddmg=100;
								else
									ddmg=irandu(types[type].defn)/2.5;
								state.bombers[k].damage+=ddmg;
								if(ddmg)
								{
									dmtf_append(&state.hist, state.now, now, state.bombers[k].id, false, state.bombers[k].type, ddmg, state.bombers[k].damage, i);
									state.bombers[k].ldf=false;
								}
							}
						}
						bool water=false;
						int x=state.bombers[k].lon/2;
						int y=state.bombers[k].lat/2;
						if((x>=0)&&(y>=0)&&(x<128)&&(y<128))
							water=lorw[x][y];
						if(dd<(water?8*8:30*30))
							targs[i].threat+=sqrt(targs[i].prod*types[type].cap/5000/(6.0+max(dd, 0.25)));
					}
					for(unsigned int i=0;i<nflaks;i++)
					{
						if(!datewithin(state.now, flaks[i].entry, flaks[i].exit)) continue;
						bool rad=!datebefore(state.now, flaks[i].radar);
						double dx=state.bombers[k].lon-flaks[i].lon,
						       dy=state.bombers[k].lat-flaks[i].lat;
						if(xyr(dx, dy, 3.0))
						{
							unsigned int x=flaks[i].lon/2, y=flaks[i].lat/2;
							double wea=((x<128)&&(y<128))?state.weather.p[x][y]-1000:0;
							double preccap=160.0/(5.0*(moonillum+.3)/(double)(8+max(4-wea, 0)));
							if(rad) preccap=min(preccap, window?960.0:480.0);
							if(brandp(flaks[i].strength*flakscale/min((12+flaks[i].shots++)*40.0, preccap)))
							{
								double ddmg;
								if(brandp(types[type].defn/500.0))
									ddmg=100;
								else
									ddmg=irandu(types[type].defn)/5.0;
								state.bombers[k].damage+=ddmg;
								if(ddmg)
								{
									dmfk_append(&state.hist, state.now, now, state.bombers[k].id, false, state.bombers[k].type, ddmg, state.bombers[k].damage, i);
									state.bombers[k].ldf=false;
								}
							}
						}
						if(rad&&xyr(dx, dy, 12)) // 36 miles range for Würzburg radar (Wikipedia gives range as "up to 43mi")
						{
							flaks[i].heat++;
							if(brandp(0.1))
							{
								if(flaks[i].ftr>=0)
								{
									unsigned int ftr=flaks[i].ftr;
									if((xyr(state.fighters[ftr].lon-(signed)flaks[i].lon, state.fighters[ftr].lat-(signed)flaks[i].lat, 10))&&(state.fighters[ftr].k<0))
									{
										state.fighters[ftr].k=k;
										//fprintf(stderr, "ftr%u k%u\n", ftr, k);
									}
								}
							}
						}
					}
					for(unsigned int j=0;j<state.nfighters;j++)
					{
						if(state.fighters[j].landed) continue;
						if(state.fighters[j].k>=0) continue;
						if(state.fighters[j].hflak>=0) continue;
						unsigned int type=state.fighters[j].type;
						bool airad=ftypes[type].night&&!datebefore(state.now, event[EVENT_L_BC]);
						unsigned int x=state.fighters[j].lon/2, y=state.fighters[j].lat/2;
						double wea=((x<128)&&(y<128))?state.weather.p[x][y]-1000:0;
						bool heavy=types[state.bombers[k].type].heavy;
						double seerange=airad?(heavy?1.9:1.2):((heavy?5.0:2.1)*(moonillum+.3)/(double)(8+max(4-wea, 0)));
						if(xyr(state.bombers[k].lon-state.fighters[j].lon, state.bombers[k].lat-state.fighters[j].lat, seerange))
						{
							double findp=airad?0.7:0.8/(double)(8+max(4-wea, 0));
							if(brandp(findp))
								state.fighters[j].k=k;
						}
					}
					// TODO: navigation affected by navaids
					double navacc=3.0/types[type].accu;
					double ex=drandu(navacc)-(navacc/2), ey=drandu(navacc)-(navacc/2);
					state.bombers[k].driftlon=state.bombers[k].driftlon*.98+ex;
					state.bombers[k].driftlat=state.bombers[k].driftlat*.98+ey;
					state.bombers[k].lon+=state.bombers[k].driftlon;
					state.bombers[k].lat+=state.bombers[k].driftlat;
					clamp(state.bombers[k].lon, 0, 256);
					clamp(state.bombers[k].lat, 0, 256);
					state.bombers[k].navlon-=state.bombers[k].driftlon;
					state.bombers[k].navlat-=state.bombers[k].driftlat;
					if(state.bombers[k].nav[NAV_GEE]&&xyr(state.bombers[k].lon-gee.lon, state.bombers[k].lat-gee.lat, datebefore(state.now, event[EVENT_GEEJAM])?56+(types[type].alt*.3):gee.jrange))
					{
						state.bombers[k].navlon=(state.bombers[k].navlon>0?1:-1)*min(fabs(state.bombers[k].navlon), 0.4);
						state.bombers[k].navlat=(state.bombers[k].navlat>0?1:-1)*min(fabs(state.bombers[k].navlat), 2.4);
					}
					unsigned int x=state.bombers[k].lon/2, y=state.bombers[k].lat/2;
					double wea=((x<128)&&(y<128))?state.weather.p[x][y]-1000:0;
					if(state.bombers[k].nav[NAV_H2S]) // TODO: restrict usage after NAXOS
					{
						unsigned char h=((x<128)&&(y<128))?tnav[x][y]:0;
						wea=max(wea, h*0.08-12);
					}
					double navp=types[type].accu*0.05*(sqrt(moonillum)*.8+.5)/(double)(8+max(16-wea, 8));
					if(home&&(state.bombers[k].lon<64)) navp=1;
					bool b=brandp(navp);
					state.bombers[k].fix=b;
					if(b)
					{
						unsigned char h=((x<128)&&(y<128))?tnav[x][y]:0;
						double cf=(700.0+state.bombers[k].lon-h*0.6)/1e3;
						state.bombers[k].navlon*=cf;
						state.bombers[k].navlat*=cf;
						state.bombers[k].driftlon*=cf;
						state.bombers[k].driftlat*=cf;
					}
					unsigned int dtm=0; // target nearest to bomber
					double mind=1e6;
					for(unsigned int i=0;i<ntargs;i++)
					{
						double dx=state.bombers[k].lon-targs[i].lon, dy=state.bombers[k].lat-targs[i].lat, dd=dx*dx+dy*dy;
						if(dd<mind)
						{
							mind=dd;
							dtm=i;
						}
					}
					unsigned int sage=t-targs[dtm].skym;
					bool sk=targs[dtm].skym>=0?brandp(10/(5.0+sage)-0.6):false;
					bool c=brandp(targs[dtm].fires/2e3);
					if(b||c||sk)
					{
						unsigned int dm=0; // target crew believes is nearest
						double mind=1e6;
						for(unsigned int i=0;i<ntargs;i++)
						{
							double dx=state.bombers[k].lon+state.bombers[k].navlon-targs[i].lon, dy=state.bombers[k].lat+state.bombers[k].navlat-targs[i].lat, dd=dx*dx+dy*dy;
							if(dd<mind)
							{
								mind=dd;
								dm=i;
							}
						}
						int bx=state.bombers[k].lon+0.5,
							by=state.bombers[k].lat+0.5;
						switch(targs[i].class)
						{
							case TCLASS_INDUSTRY:
								if(mind<0.9)
									state.bombers[k].idtar=true;
							break;
							case TCLASS_CITY:
							case TCLASS_LEAFLET:
							case TCLASS_SHIPPING:
							case TCLASS_AIRFIELD:
							case TCLASS_ROAD:
							case TCLASS_BRIDGE:
								if(pget(target_overlay, bx, by).a==ATG_ALPHA_OPAQUE) // XXX this might behave oddly as we have all cities on target_overlay
								{
									state.bombers[k].idtar=true;
								}
								else if((c||sk)&&(targs[dm].class==TCLASS_CITY)&&(targs[dtm].class==TCLASS_CITY))
								{
									state.bombers[k].navlon=(signed)targs[dtm].lon-(signed)targs[dm].lon;
									state.bombers[k].navlat=(signed)targs[dtm].lat-(signed)targs[dm].lat;
								}
							break;
							case TCLASS_MINING:;
								int x=state.bombers[k].lon/2;
								int y=state.bombers[k].lat/2;
								if((x>=0)&&(y>=0)&&(x<128)&&(y<128)&&lorw[x][y])
									state.bombers[k].idtar=true;
							break;
							default: // shouldn't ever get here
								fprintf(stderr, "Bad targs[%d].class = %d\n", i, targs[i].class);
							break;
						}
					}
					if(tameboar)
					{
						int boxx=floor((state.bombers[k].lon-89)/10.0),
							boxy=floor((state.bombers[k].lat-40)/10.0);
						if(0<=boxx&&boxx<16&&0<=boxy&&boxy<16)
						{
							unsigned int type=state.bombers[k].type;
							unsigned int size=types[type].cap/1000; // how big a signal on Freya ground radar
							// TODO: extra WINDOW (will need UI support in raid planning)
							size*=irandu(3);
							boxes[boxx][boxy]+=size;
							if(boxx==topx&&boxy==topy)
							{
								sumx+=(state.bombers[k].lon-89-boxx*10)*size;
								sumy+=(state.bombers[k].lat-40-boxy*10)*size;
								velx+=dx*size;
								vely+=dy*size;
							}
						}
					}
				}
			bool boar_up=false;
			double boarx,boary,boarvx,boarvy;
			if(tameboar)
			{
				int ox=topx;
				int oy=topy;
				topx=-1;
				topy=-1;
				unsigned int topval=0;
				for(int findx=0;findx<16;findx++)
					for(int findy=0;findy<16;findy++)
					{
						// once raid is identified, prefer to move contiguously
						unsigned int val=boxes[findx][findy];
						if(ox>=0&&oy>=0)
						{
							if(abs(findx-ox)+abs(findy-oy)<2)
								val*=3;
						}
						if(val>topval)
						{
							topx=findx;
							topy=findy;
							topval=val;
						}
					}
				if(topx>=0&&topy>=0)
				{
					boar_up=true;
					boarx=sumx/(double)topval+89+topx*10;
					boary=sumy/(double)topval+40+topy*10;
					boarvx=velx/(double)topval;
					boarvy=vely/(double)topval;
				}
			}
			for(unsigned int j=0;j<state.nfighters;j++)
			{
				if(state.fighters[j].landed)
				{
					if(tameboar) goto boarable;
					continue;
				}
				if(state.fighters[j].crashed) continue;
				if((state.fighters[j].damage>=100)||brandp(state.fighters[j].damage/4000.0))
				{
					cr_append(&state.hist, state.now, now, state.fighters[j].id, true, state.fighters[j].type);
					state.fighters[j].crashed=true;
					int t=state.fighters[j].targ;
					if(t>=0)
						targs[t].nfighters--;
					int f=state.fighters[j].hflak;
					if(f>=0)
						flaks[f].ftr=-1;
					continue;
				}
				unsigned int type=state.fighters[j].type;
				if(t>state.fighters[j].fuelt)
				{
					int ta=state.fighters[j].targ;
					int fl=state.fighters[j].hflak;
					if(ta>=0)
						targs[ta].nfighters--;
					if(fl>=0)
						flaks[fl].ftr=-1;
					if((ta>=0)||(fl>=0))
						fightersleft++;
					state.fighters[j].targ=-1;
					state.fighters[j].hflak=-1;
					state.fighters[j].k=-1;
					if(t>state.fighters[j].fuelt+(ftypes[type].night?96:56))
					{
						cr_append(&state.hist, state.now, now, state.fighters[j].id, true, state.fighters[j].type);
						state.fighters[j].crashed=true;
						fightersleft--;
					}
				}
				unsigned int x=state.fighters[j].lon/2, y=state.fighters[j].lat/2;
				double wea=((x<128)&&(y<128))?state.weather.p[x][y]-1000:0;
				if(wea<6)
				{
					double nav=log2(6-wea)/(ftypes[type].night?6.0:2.0);
					double dx=drandu(nav)-nav/2.0,
					       dy=drandu(nav)-nav/2.0;
					state.fighters[j].lon+=dx;
					state.fighters[j].lat+=dy;
				}
				int k=state.fighters[j].k;
				if(k>=0)
				{
					unsigned int ft=state.fighters[j].type, bt=state.bombers[k].type;
					bool radcon=false; // radar controlled
					if(state.fighters[j].hflak>=0)
					{
						unsigned int f=state.fighters[j].hflak;
						radcon=xyr((signed)flaks[f].lat-state.fighters[j].lat, (signed)flaks[f].lon-state.fighters[j].lon, 12);
					}
					if(window) radcon=false;
					bool airad=ftypes[ft].night&&!datebefore(state.now, event[EVENT_L_BC]);
					if(airad&&wairad) airad=brandp(0.8);
					unsigned int x=state.fighters[j].lon/2, y=state.fighters[j].lat/2;
					double wea=((x<128)&&(y<128))?state.weather.p[x][y]-1000:0;
					double seerange=airad?2.0:7.0/(double)(8+max(4-wea, 0));
					if(state.bombers[k].crashed||!xyr(state.bombers[k].lon-state.fighters[j].lon, state.bombers[k].lat-state.fighters[j].lat, (radcon?12.0:seerange))||brandp(airad?0.03:0.12))
						state.fighters[j].k=-1;
					else
					{
						double x=state.fighters[j].lon,
							y=state.fighters[j].lat;
						double tx=state.bombers[k].lon,
							ty=state.bombers[k].lat;
						double d=hypot(x-tx, y-ty);
						if(d)
						{
							double spd=ftypes[ft].speed/400.0;
							double cx=(tx-x)/d,
								cy=(ty-y)/d;
							state.fighters[j].lon+=cx*spd;
							state.fighters[j].lat+=cy*spd;
						}
						if(d<(airad?0.4:radcon?0.34:0.25)*(.7*moonillum+.6))
						{
							if(brandp(ftypes[ft].mnv*(2.7+loadness(state.bombers[k]))/400.0))
							{
								unsigned int dmg=irandu(ftypes[ft].arm)*types[bt].defn/15.0;
								state.bombers[k].damage+=dmg;
								if(dmg)
								{
									dmac_append(&state.hist, state.now, now, state.bombers[k].id, false, state.bombers[k].type, dmg, state.bombers[k].damage, state.fighters[j].id);
									state.bombers[k].ldf=true;
								}
							}
							if(brandp(0.35))
								state.fighters[j].k=-1;
						}
						if(d<(ftypes[ft].night?0.3:0.2)*(.8*moonillum+.6)) // easier to spot nightfighters as they're bigger
						{
							if(!types[bt].noarm&&(brandp(0.2/types[bt].defn)))
							{
								unsigned int dmg=irandu(20);
								state.fighters[j].damage+=dmg;
								if(dmg)
									dmac_append(&state.hist, state.now, now, state.fighters[j].id, true, state.fighters[j].type, dmg, state.fighters[j].damage, state.bombers[k].id);
								if(brandp(0.6)) // fighter breaks off to avoid return fire, but 40% chance to maintain contact
									state.fighters[j].k=-1;
							}
						}
					}
				}
				else if(state.fighters[j].targ>=0)
				{
					double x=state.fighters[j].lon+drandu(3.0)-1.5,
						y=state.fighters[j].lat+drandu(3.0)-1.5;
					unsigned int t=state.fighters[j].targ;
					int tx=targs[t].lon,
						ty=targs[t].lat;
					double d=hypot(x-tx, y-ty);
					unsigned int type=state.fighters[j].type;
					double spd=ftypes[type].speed/400.0;
					if(d>0.2)
					{
						double cx=(tx-x)/d,
							cy=(ty-y)/d;
						state.fighters[j].lon+=cx*spd;
						state.fighters[j].lat+=cy*spd;
					}
					else
					{
						double theta=drandu(2*M_PI);
						double cx=cos(theta), cy=sin(theta);
						state.fighters[j].lon+=cx*spd;
						state.fighters[j].lat+=cy*spd;
					}
				}
				else if(state.fighters[j].hflak>=0)
				{
					double x=state.fighters[j].lon+drandu(3.0)-1.5,
						y=state.fighters[j].lat+drandu(3.0)-1.5;
					unsigned int f=state.fighters[j].hflak;
					int fx=flaks[f].lon,
						fy=flaks[f].lat;
					double d=hypot(x-fx, y-fy);
					unsigned int type=state.fighters[j].type;
					double spd=ftypes[type].speed/400.0;
					if(d>0.2)
					{
						double cx=(fx-x)/d,
							cy=(fy-y)/d;
						state.fighters[j].lon+=cx*spd;
						state.fighters[j].lat+=cy*spd;
					}
					else
					{
						double theta=drandu(2*M_PI);
						double cx=cos(theta), cy=sin(theta);
						state.fighters[j].lon+=cx*spd;
						state.fighters[j].lat+=cy*spd;
					}
				}
				else
				{
					boarable:;
					unsigned int type=state.fighters[j].type;
					double spd=ftypes[type].speed/400.0;
					bool boared=false;
					if(tameboar&&ftypes[type].night&&boar_up&&(state.fighters[j].landed||t<state.fighters[j].fuelt))
					{
						// d = b - f
						double dx=boarx-state.fighters[j].lon;
						double dy=boary-state.fighters[j].lat;
						// vector u is boarv, velocity of the bomber stream
						// vector v is the fighter's velocity - only its modulus is known
						// then T = (u.d + sqrt((u.d)^2 + d^2 (v^2 - u^2))) / (v^2 - u^2) is the time to intercept
						// and v = d/t + u is the heading (and speed) to follow
						double vv = spd*spd;
						double ud = boarvx*dx+boarvy*dy;
						double uu = boarvx*boarvx+boarvy*boarvy;
						double dd = dx*dx+dy*dy;
						if(uu<vv)
						{
							double T = (ud + sqrt(ud*ud + dd*(vv-uu))) / (vv-uu);
							double fuelt=state.fighters[j].landed?40:state.fighters[j].fuelt-t-40;
							if(state.gprod[ICLASS_OIL]<20) fuelt=-1;
							if(T<fuelt)
							{
								boared=true;
								if(state.fighters[j].landed)
								{
									state.fighters[j].landed=false;
									int famount=72+irandu(28);
									state.fighters[j].fuelt=t+famount;
									state.gprod[ICLASS_OIL]-=famount*0.2;
								}
								double vx = dx/T + boarvx;
								double vy = dy/T + boarvy;
								state.fighters[j].lon+=vx;
								state.fighters[j].lat+=vy;
							}
						}
					}
					if(!boared&&!state.fighters[j].landed)
					{
						double mrr=1e6;
						unsigned int mb=0;
						for(unsigned int b=0;b<nfbases;b++)
						{
							if(!datewithin(state.now, fbases[b].entry, fbases[b].exit)) continue;
							int bx=fbases[b].lon,
								by=fbases[b].lat;
							double dx=state.fighters[j].lon-bx,
								dy=state.fighters[j].lat-by;
							double rr=dx*dx+dy*dy;
							if(rr<mrr)
							{
								mb=b;
								mrr=rr;
							}
						}
						state.fighters[j].base=mb;
						double x=state.fighters[j].lon,
							y=state.fighters[j].lat;
						int bx=fbases[mb].lon,
							by=fbases[mb].lat;
						double d=hypot(x-bx, y-by);
						if(d>0.8)
						{
							double cx=(bx-x)/d,
								cy=(by-y)/d;
							state.fighters[j].lon+=cx*spd;
							state.fighters[j].lat+=cy*spd;
						}
						else
						{
							state.fighters[j].landed=true;
							state.fighters[j].lon=bx;
							state.fighters[j].lat=by;
						}
					}
				}
			}
			if(datebefore(state.now, event[EVENT_TAMEBOAR])&&!datebefore(state.now, event[EVENT_WURZBURG]))
			{
				for(unsigned int i=0;i<nflaks;i++)
				{
					if(flaks[i].ftr>=0)
					{
						unsigned int ftr=flaks[i].ftr;
						if(state.fighters[ftr].k>=0)
						{
							unsigned int k=state.fighters[ftr].k;
							if(xyr((signed)flaks[i].lat-state.bombers[k].lat, (signed)flaks[i].lon-state.bombers[k].lon, 12))
								state.fighters[ftr].k=-1;
						}
						continue;
					}
					if(!datewithin(state.now, flaks[i].radar, flaks[i].exit)) continue;
					if(!flaks[i].heat) continue;
					if(fightersleft)
					{
						unsigned int mind=1000000;
						int minj=-1;
						unsigned int oilme=300/max(flaks[i].heat, 15); // don't use up the last of your oil just to chase one bomber...
						for(unsigned int j=0;j<state.nfighters;j++)
						{
							if(state.fighters[j].damage>=1) continue;
							unsigned int type=state.fighters[j].type;
							unsigned int range=(ftypes[type].night?100:50)*(ftypes[type].speed/400.0);
							if(state.fighters[j].landed)
							{
								if(state.gprod[ICLASS_OIL]>=oilme)
								{
									const unsigned int base=state.fighters[j].base;
									signed int dx=(signed)flaks[i].lat-(signed)fbases[base].lat, dy=(signed)flaks[i].lon-(signed)fbases[base].lon;
									unsigned int dd=dx*dx+dy*dy;
									if(dd<mind&&dd<range*range)
									{
										mind=dd;
										minj=j;
									}
								}
							}
							else if(ftr_free(state.fighters[j]))
							{
								signed int dx=(signed)flaks[i].lat-state.fighters[j].lat, dy=(signed)flaks[i].lon-state.fighters[j].lon;
								unsigned int dd=dx*dx+dy*dy;
								if(dd<mind&&dd<range*range)
								{
									mind=dd;
									minj=j;
								}
							}
						}
						if(minj>=0)
						{
							unsigned int ft=state.fighters[minj].type;
							if(state.fighters[minj].landed)
							{
								int famount=(ftypes[ft].night?(72+irandu(28)):(35+irandu(15)));
								state.gprod[ICLASS_OIL]-=famount*(ftypes[ft].night?0.2:0.1);
								state.fighters[minj].fuelt=t+famount;
							}
							state.fighters[minj].landed=false;
							state.fighters[minj].hflak=i;
							fightersleft--;
							flaks[i].ftr=minj;
							//fprintf(stderr, "Assigned fighter to hflak %u\n", i);
						}
						else
						{
							//fprintf(stderr, "Out of fighters (hflak %u)\n", i);
						}
					}
				}
			}
			for(unsigned int i=0;i<ntargs;i++)
			{
				if(targs[i].class==TCLASS_MINING) continue;
				targs[i].fires*=.92;
				if(!datewithin(state.now, targs[i].entry, targs[i].exit)) continue;
				double thresh=3e3*targs[i].nfighters/(double)fightersleft;
				if(targs[i].threat*(moonshine?0.7:1.0)+state.heat[i]*(moonshine?12.0:10.0)>thresh)
				{
					unsigned int mind=1000000;
					int minj=-1;
					for(unsigned int j=0;j<state.nfighters;j++)
					{
						if(state.fighters[j].damage>=1) continue;
						unsigned int type=state.fighters[j].type;
						unsigned int range=(ftypes[type].night?100:50)*(ftypes[type].speed/400.0);
						if(state.fighters[j].landed)
						{
							if(state.gprod[ICLASS_OIL]>=20)
							{
								const unsigned int base=state.fighters[j].base;
								signed int dx=(signed)targs[i].lat-(signed)fbases[base].lat, dy=(signed)targs[i].lon-(signed)fbases[base].lon;
								unsigned int dd=dx*dx+dy*dy;
								if(dd<mind&&dd<range*range)
								{
									mind=dd;
									minj=j;
								}
							}
						}
						else if(ftr_free(state.fighters[j]))
						{
							signed int dx=(signed)targs[i].lat-state.fighters[j].lat, dy=(signed)targs[i].lon-state.fighters[j].lon;
							unsigned int dd=dx*dx+dy*dy;
							if(dd<mind&&dd<range*range)
							{
								mind=dd;
								minj=j;
							}
						}
					}
					if(minj>=0)
					{
						unsigned int ft=state.fighters[minj].type;
						if(state.fighters[minj].landed)
						{
							int famount=(ftypes[ft].night?(72+irandu(28)):(35+irandu(15)));
							state.gprod[ICLASS_OIL]-=famount*(ftypes[ft].night?0.2:0.1);
							state.fighters[minj].fuelt=t+famount;
						}
						state.fighters[minj].landed=false;
						state.fighters[minj].targ=i;
						targs[i].threat-=(thresh*0.6);
						fightersleft--;
						targs[i].nfighters++;
						//fprintf(stderr, "Assigned fighter #%u to %s\n", targs[i].nfighters, targs[i].name);
					}
					else
					{
						/*fprintf(stderr, "Out of fighters (%s)\n", targs[i].name);
						targs[i].threat=0;*/
					}
				}
				targs[i].threat*=.98;
				targs[i].threat-=targs[i].nfighters*0.01;
				if(targs[i].nfighters&&(targs[i].threat<thresh/(2.0+max(fightersleft, 2))))
				{
					for(unsigned int j=0;j<state.nfighters;j++)
						if(state.fighters[j].targ==(int)i)
						{
							//fprintf(stderr, "Released fighter #%u from %s\n", targs[i].nfighters, targs[i].name);
							targs[i].nfighters--;
							fightersleft++;
							state.fighters[j].targ=-1;
							break;
						}
				}
			}
			SDL_FreeSurface(ac_overlay);
			ac_overlay=render_ac(state);
			SDL_BlitSurface(with_weather, NULL, with_ac, NULL);
			SDL_BlitSurface(ac_overlay, NULL, with_ac, NULL);
			SDL_BlitSurface(with_ac, NULL, RB_map->elem.image->data, NULL);
			atg_flip(canvas);
		}
		// incorporate the results, and clear the raids ready for next cycle
		if(kills[0]||kills[1])
			fprintf(stderr, "Kills: flak %u, fighters %u\n", kills[0], kills[1]);
		unsigned int dij[ntargs][ntypes], nij[ntargs][ntypes], tij[ntargs][ntypes], lij[ntargs][ntypes], heat[ntargs];
		bool canscore[ntargs];
		double bridge=0;
		for(unsigned int i=0;i<ntargs;i++)
		{
			heat[i]=0;
			canscore[i]=(state.dmg[i]>0.1);
			for(unsigned int j=0;j<ntypes;j++)
				dij[i][j]=nij[i][j]=tij[i][j]=lij[i][j]=0;
		}
		for(unsigned int i=0;i<ntargs;i++)
		{
			for(unsigned int j=0;j<state.raids[i].nbombers;j++)
			{
				unsigned int k=state.raids[i].bombers[j], type=state.bombers[k].type;
				dij[i][type]++;
				bool leaf=state.bombers[k].b_le;
				bool mine=(targs[i].class==TCLASS_MINING);
				for(unsigned int l=0;l<ntargs;l++)
				{
					if(!state.bombers[k].bombed) break;
					if(!datewithin(state.now, targs[l].entry, targs[l].exit)) continue;
					int dx=floor(state.bombers[k].bmblon+.5)-targs[l].lon, dy=floor(state.bombers[k].bmblat+.5)-targs[l].lat;
					int hx=targs[l].picture->w/2;
					int hy=targs[l].picture->h/2;
					switch(targs[l].class)
					{
						case TCLASS_CITY:
							if(leaf) continue;
							if((abs(dx)<=hx)&&(abs(dy)<=hy))
							{
								if(pget(targs[l].picture, dx+hx, dy+hy).a==ATG_ALPHA_OPAQUE)
								{
									// most of it was already handled when the bombs were dropped
									heat[l]++;
									nij[l][type]++;
									tij[l][type]+=state.bombers[k].b_hc+state.bombers[k].b_gp+state.bombers[k].b_in+state.bombers[k].b_ti;
									state.bombers[k].bombed=false;
								}
							}
						break;
						case TCLASS_INDUSTRY:
							if(leaf) continue;
							if((abs(dx)<=1)&&(abs(dy)<=1))
							{
								heat[l]++;
								if(brandp(targs[l].esiz/30.0))
								{
									unsigned int he=state.bombers[k].b_hc+state.bombers[k].b_gp;
									hi_append(&state.hist, state.now, maketime(state.bombers[k].bt), state.bombers[k].id, false, type, l, he);
									double dmg=min(he/12000.0, state.dmg[l]);
									cidam+=dmg*(targs[l].berlin?2.0:1.0);
									state.dmg[l]-=dmg;
									tdm_append(&state.hist, state.now, maketime(state.bombers[k].bt), l, dmg, state.dmg[l]);
									nij[l][type]++;
									tij[l][type]+=he;
								}
								state.bombers[k].bombed=false;
							}
						break;
						case TCLASS_AIRFIELD:
						case TCLASS_ROAD:
							if(leaf) continue;
							if((abs(dx)<=hx)&&(abs(dy)<=hy))
							{
								if(pget(targs[l].picture, dx+hx, dy+hy).a==ATG_ALPHA_OPAQUE)
								{
									heat[l]++;
									if(brandp(targs[l].esiz/30.0))
									{
										unsigned int he=state.bombers[k].b_hc+state.bombers[k].b_gp;
										hi_append(&state.hist, state.now, maketime(state.bombers[k].bt), state.bombers[k].id, false, type, l, he);
										double dmg=min(he/2000.0, state.dmg[l]);
										cidam+=dmg*(targs[l].berlin?2.0:1.0);
										state.dmg[l]-=dmg;
										tdm_append(&state.hist, state.now, maketime(state.bombers[k].bt), l, dmg, state.dmg[l]);
										nij[l][type]++;
										tij[l][type]+=he;
									}
									state.bombers[k].bombed=false;
								}
							}
						break;
						case TCLASS_BRIDGE:
							if(leaf) continue;
							if((abs(dx)<=hx)&&(abs(dy)<=hy))
							{
								if(pget(targs[l].picture, dx+hx, dy+hy).a==ATG_ALPHA_OPAQUE)
								{
									heat[l]++;
									if(brandp(targs[l].esiz/30.0))
									{
										unsigned int he=state.bombers[k].b_hc+state.bombers[k].b_gp;
										hi_append(&state.hist, state.now, maketime(state.bombers[k].bt), state.bombers[k].id, false, type, l, he);
										if(brandp(log2(he/25.0)/200.0))
										{
											cidam+=state.dmg[l];
											bridge+=state.dmg[l];
											tdm_append(&state.hist, state.now, maketime(state.bombers[k].bt), l, state.dmg[l], 0);
											state.dmg[l]=0;
										}
										nij[l][type]++;
										tij[l][type]+=he;
									}
									state.bombers[k].bombed=false;
								}
							}
						break;
						case TCLASS_LEAFLET:
							if(!leaf) continue;
							if((abs(dx)<=hx)&&(abs(dy)<=hy))
							{
								if(pget(targs[l].picture, dx+hx, dy+hy).a==ATG_ALPHA_OPAQUE)
								{
									hi_append(&state.hist, state.now, maketime(state.bombers[k].bt), state.bombers[k].id, false, type, l, state.bombers[k].b_le);
									double dmg=min(state.bombers[k].b_le/(targs[l].psiz*12000.0), state.dmg[l]);
									state.dmg[l]-=dmg;
									tdm_append(&state.hist, state.now, maketime(state.bombers[k].bt), l, dmg, state.dmg[l]);
									nij[l][type]++;
									tij[l][type]+=state.bombers[k].b_le;
									state.bombers[k].bombed=false;
								}
							}
						break;
						case TCLASS_SHIPPING:
							if(leaf) continue;
							if((abs(dx)<=2)&&(abs(dy)<=1))
							{
								heat[l]++;
								if(brandp(targs[l].esiz/100.0))
								{
									unsigned int he=state.bombers[k].b_hc+state.bombers[k].b_gp;
									nij[l][type]++;
									hi_append(&state.hist, state.now, maketime(state.bombers[k].bt), state.bombers[k].id, false, type, l, he);
									if(brandp(log2(he/500.0)/8.0))
									{
										tij[l][type]++;
										tsh_append(&state.hist, state.now, maketime(state.bombers[k].bt), l);
									}
								}
								state.bombers[k].bombed=false;
							}
						break;
						case TCLASS_MINING:
							if(leaf||!mine) continue;
							int x=state.bombers[k].bmblon/2;
							int y=state.bombers[k].bmblat/2;
							bool water=(x>=0)&&(y>=0)&&(x<128)&&(y<128)&&lorw[x][y];
							if((abs(dx)<=8)&&(abs(dy)<=8)&&water)
							{
								unsigned int he=state.bombers[k].b_hc+state.bombers[k].b_gp;
								hi_append(&state.hist, state.now, maketime(state.bombers[k].bt), state.bombers[k].id, false, type, l, he);
								double dmg=min(he/400000.0, state.dmg[l]);
								state.dmg[l]-=dmg;
								tdm_append(&state.hist, state.now, maketime(state.bombers[k].bt), l, dmg, state.dmg[l]);
								nij[l][type]++;
								tij[l][type]+=he;
								state.bombers[k].bombed=false;
							}
						break;
						default: // shouldn't ever get here
							fprintf(stderr, "Bad targs[%d].class = %d\n", l, targs[l].class);
						break;
					}
				}
			}
			state.raids[i].nbombers=0;
			free(state.raids[i].bombers);
			state.raids[i].bombers=NULL;
		}
		for(unsigned int i=0;i<state.nbombers;i++)
		{
			unsigned int type=state.bombers[i].type;
			if(state.bombers[i].crashed)
			{
				lij[state.bombers[i].targ][state.bombers[i].type]++;
				state.nbombers--;
				for(unsigned int j=i;j<state.nbombers;j++)
					state.bombers[j]=state.bombers[j+1];
				i--;
				continue;
			}
			else if(state.bombers[i].damage>50)
			{
				fa_append(&state.hist, state.now, (time){11, 00}, state.bombers[i].id, false, type, 1);
				state.bombers[i].failed=true; // mark as u/s
			}
		}
		// finish the weather
		for(; it<512;it++)
			w_iter(&state.weather, lorw);
		
		// Raid Results table
		for(unsigned int i=2;i<rstatbox->nelems;i++)
			atg_free_element(rstatbox->elems[i]);
		rstatbox->nelems=2;
		unsigned int dj[ntypes], nj[ntypes], tbj[ntypes], tlj[ntypes], tsj[ntypes], tmj[ntypes], lj[ntypes];
		unsigned int D=0, N=0, Tb=0, Tl=0, Ts=0, Tm=0, L=0;
		unsigned int scoreTb=0, scoreTl=0, scoreTm=0;
		unsigned int ntcols=0;
		for(unsigned int j=0;j<ntypes;j++)
		{
			dj[j]=nj[j]=tbj[j]=tlj[j]=tsj[j]=tmj[j]=lj[j]=0;
			unsigned int scoretbj=0, scoretlj=0, scoretmj=0;
			for(unsigned int i=0;i<ntargs;i++)
			{
				dj[j]+=dij[i][j];
				nj[j]+=nij[i][j];
				switch(targs[i].class)
				{
					case TCLASS_CITY:
					case TCLASS_AIRFIELD:
					case TCLASS_ROAD:
					case TCLASS_BRIDGE:
					case TCLASS_INDUSTRY:
						tbj[j]+=tij[i][j];
						unsigned int sf=1;
						if(targs[i].berlin) sf*=2;
						if(targs[i].class==TCLASS_INDUSTRY) sf*=2;
						if(targs[i].iclass==ICLASS_UBOOT) sf*=1.2;
						if(canscore[i]) scoretbj+=tij[i][j]*sf;
					break;
					case TCLASS_LEAFLET:
						tlj[j]+=tij[i][j];
						if(canscore[i]) scoretlj+=tij[i][j]*(targs[i].berlin?2:1);
					break;
					case TCLASS_SHIPPING:
						tsj[j]+=tij[i][j];
					break;
					case TCLASS_MINING:
						tmj[j]+=tij[i][j];
						if(canscore[i]) scoretmj+=tij[i][j];
					break;
					default: // shouldn't ever get here
						fprintf(stderr, "Bad targs[%d].class = %d\n", i, targs[i].class);
					break;
				}
				lj[j]+=lij[i][j];
			}
			D+=dj[j];
			N+=nj[j];
			Tb+=tbj[j];
			scoreTb+=scoretbj;
			Tl+=tlj[j];
			scoreTl+=scoretlj;
			Ts+=tsj[j];
			Tm+=tmj[j];
			scoreTm+=scoretmj;
			L+=lj[j];
			RS_typecol[j]->hidden=!dj[j];
			if(dj[j]) ntcols++;
		}
		state.cshr+=scoreTb*  80e-4;
		state.cshr+=scoreTl*   6e-4;
		state.cshr+=scoreTm*  12e-4;
		state.cshr+=Ts*      600   ;
		state.cshr+=bridge* 2000e-2;
		double par=0.2+((state.now.year-1939)*0.125);
		state.confid+=(N/(double)D-par)*(1.0+log2(D)/2.0)*0.6;
		state.confid+=Ts*0.15;
		state.confid+=cidam*0.08;
		state.confid=min(max(state.confid, 0), 100);
		co_append(&state.hist, state.now, (time){11, 05}, state.confid);
		state.morale+=(1.75-L*100.0/(double)D)/5.0;
		if((L==0)&&(D>5)) state.morale+=0.3;
		if(D>=100) state.morale+=0.2;
		if(D>=1000) state.morale+=1.0;
		state.morale=min(max(state.morale, 0), 100);
		mo_append(&state.hist, state.now, (time){11, 05}, state.morale);
		if(RS_tocol)
			RS_tocol->hidden=(ntcols==1);
		unsigned int ntrows=0;
		for(unsigned int i=0;i<ntargs;i++)
		{
			state.heat[i]=state.heat[i]*.8+heat[i]/(double)D;
			unsigned int di=0, ni=0, ti=0, li=0;
			for(unsigned int j=0;j<ntypes;j++)
			{
				di+=dij[i][j];
				ni+=nij[i][j];
				ti+=tij[i][j];
				li+=lij[i][j];
			}
			if(di||ni)
			{
				ntrows++;
				atg_element *row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 33, 33, ATG_ALPHA_OPAQUE});
				if(row)
				{
					row->h=RS_cell_h;
					atg_pack_element(rstatbox, row);
					atg_box *b=row->elem.box;
					if(b)
					{
						atg_element *tncol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 35, 35, ATG_ALPHA_OPAQUE});
						if(tncol)
						{
							tncol->h=RS_cell_h;
							tncol->w=RS_firstcol_w;
							atg_pack_element(b, tncol);
							atg_box *b2=tncol->elem.box;
							if(b2&&targs[i].name)
							{
								const char *np=targs[i].name;
								char line[17]="";
								size_t x=0, y;
								while(*np)
								{
									y=strcspn(np, " ");
									if(x&&(x+y>15))
									{
										atg_element *tname=atg_create_element_label(line, 12, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
										if(tname) atg_pack_element(b2, tname);
										line[x=0]=0;
									}
									if(x)
										line[x++]=' ';
									strncpy(line+x, np, y);
									x+=y;
									np+=y;
									while(*np==' ') np++;
									line[x]=0;
								}
								if(x)
								{
									atg_element *tname=atg_create_element_label(line, 12, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
									if(tname) atg_pack_element(b2, tname);
								}
							}
						}
						for(unsigned int j=0;j<ntypes;j++)
						{
							if(!dj[j]) continue;
							atg_element *tbcol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 35, 35, ATG_ALPHA_OPAQUE});
							if(tbcol)
							{
								tbcol->h=RS_cell_h;
								tbcol->w=RS_cell_w;
								atg_pack_element(b, tbcol);
								atg_box *b2=tbcol->elem.box;
								if(b2)
								{
									char dt[20],nt[20],tt[20],lt[20];
									if(dij[i][j]||nij[i][j])
									{
										snprintf(dt, 20, "Dispatched:%u", dij[i][j]);
										atg_element *dl=atg_create_element_label(dt, 10, (atg_colour){191, 191, 0, ATG_ALPHA_OPAQUE});
										if(dl) atg_pack_element(b2, dl);
										snprintf(nt, 20, "Hit Target:%u", nij[i][j]);
										atg_element *nl=atg_create_element_label(nt, 10, (atg_colour){191, 191, 0, ATG_ALPHA_OPAQUE});
										if(nl) atg_pack_element(b2, nl);
										if(nij[i][j])
										{
											switch(targs[i].class)
											{
												case TCLASS_CITY:
												case TCLASS_AIRFIELD:
												case TCLASS_ROAD:
												case TCLASS_BRIDGE:
												case TCLASS_INDUSTRY:
													snprintf(tt, 20, "Bombs (lb):%u", tij[i][j]);
												break;
												case TCLASS_MINING:
													snprintf(tt, 20, "Mines (lb):%u", tij[i][j]);
												break;
												case TCLASS_LEAFLET:
													snprintf(tt, 20, "Leaflets  :%u", tij[i][j]);
												break;
												case TCLASS_SHIPPING:
													snprintf(tt, 20, "Ships sunk:%u", tij[i][j]);
												break;
												default: // shouldn't ever get here
													fprintf(stderr, "Bad targs[%d].class = %d\n", i, targs[i].class);
												break;
											}
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){191, 191, 0, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
										if(lij[i][j])
										{
											snprintf(lt, 20, "A/c Lost  :%u", lij[i][j]);
											atg_element *ll=atg_create_element_label(lt, 10, (atg_colour){191, 0, 0, ATG_ALPHA_OPAQUE});
											if(ll) atg_pack_element(b2, ll);
										}
									}
								}
							}
						}
						if(ntcols!=1)
						{
							atg_element *totcol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){49, 37, 37, ATG_ALPHA_OPAQUE});
							if(totcol)
							{
								totcol->h=RS_cell_h;
								totcol->w=RS_cell_w;
								atg_pack_element(b, totcol);
								atg_box *b2=totcol->elem.box;
								if(b2)
								{
									char dt[20],nt[20],tt[20],lt[20];
									if(di||ni)
									{
										snprintf(dt, 20, "Dispatched:%u", di);
										atg_element *dl=atg_create_element_label(dt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
										if(dl) atg_pack_element(b2, dl);
										snprintf(nt, 20, "Hit Target:%u", ni);
										atg_element *nl=atg_create_element_label(nt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
										if(nl) atg_pack_element(b2, nl);
										if(ni)
										{
											switch(targs[i].class)
											{
												case TCLASS_CITY:
												case TCLASS_AIRFIELD:
												case TCLASS_ROAD:
												case TCLASS_BRIDGE:
												case TCLASS_INDUSTRY:
													snprintf(tt, 20, "Bombs (lb):%u", ti);
												break;
												case TCLASS_MINING:
													snprintf(tt, 20, "Mines (lb):%u", ti);
												break;
												case TCLASS_LEAFLET:
													snprintf(tt, 20, "Leaflets  :%u", ti);
												break;
												case TCLASS_SHIPPING:
													snprintf(tt, 20, "Ships sunk:%u", ti);
												break;
												default: // shouldn't ever get here
													fprintf(stderr, "Bad targs[%d].class = %d\n", i, targs[i].class);
												break;
											}
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
										if(li)
										{
											snprintf(lt, 20, "A/c Lost  :%u", li);
											atg_element *ll=atg_create_element_label(lt, 10, (atg_colour){255, 0, 0, ATG_ALPHA_OPAQUE});
											if(ll) atg_pack_element(b2, ll);
										}
									}
								}
							}
						}
					}
				}
			}
		}
		if(ntrows!=1)
		{
			atg_element *row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){49, 37, 37, ATG_ALPHA_OPAQUE});
			if(row)
			{
				row->h=RS_lastrow_h;
				atg_pack_element(rstatbox, row);
				atg_box *b=row->elem.box;
				if(b)
				{
					atg_element *tncol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){51, 39, 39, ATG_ALPHA_OPAQUE});
					if(tncol)
					{
						tncol->h=RS_lastrow_h;
						tncol->w=RS_firstcol_w;
						atg_pack_element(b, tncol);
						atg_box *b2=tncol->elem.box;
						if(b2)
						{
							atg_element *tname=atg_create_element_label("Total", 18, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
							if(tname) atg_pack_element(b2, tname);
						}
					}
					for(unsigned int j=0;j<ntypes;j++)
					{
						if(!dj[j]) continue;
						atg_element *tbcol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){49, 37, 37, ATG_ALPHA_OPAQUE});
						if(tbcol)
						{
							tbcol->h=RS_lastrow_h;
							tbcol->w=RS_cell_w;
							atg_pack_element(b, tbcol);
							atg_box *b2=tbcol->elem.box;
							if(b2)
							{
								char dt[20],nt[20],tt[20],lt[20];
								if(dj[j]||nj[j])
								{
									snprintf(dt, 20, "Dispatched:%u", dj[j]);
									atg_element *dl=atg_create_element_label(dt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
									if(dl) atg_pack_element(b2, dl);
									snprintf(nt, 20, "Hit Target:%u", nj[j]);
									atg_element *nl=atg_create_element_label(nt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
									if(nl) atg_pack_element(b2, nl);
									if(nj[j])
									{
										if(tbj[j])
										{
											snprintf(tt, 20, "Bombs (lb):%u", tbj[j]);
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
										if(tmj[j])
										{
											snprintf(tt, 20, "Mines (lb):%u", tmj[j]);
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
										if(tlj[j])
										{
											snprintf(tt, 20, "Leaflets  :%u", tlj[j]);
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
										if(tsj[j])
										{
											snprintf(tt, 20, "Ships sunk:%u", tsj[j]);
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
									}
									if(lj[j])
									{
										snprintf(lt, 20, "A/c Lost  :%u", lj[j]);
										atg_element *ll=atg_create_element_label(lt, 10, (atg_colour){255, 0, 0, ATG_ALPHA_OPAQUE});
										if(ll) atg_pack_element(b2, ll);
									}
								}
							}
						}
					}
					if(ntcols!=1)
					{
						atg_element *totcol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){51, 39, 39, ATG_ALPHA_OPAQUE});
						if(totcol)
						{
							totcol->h=RS_lastrow_h;
							totcol->w=RS_cell_w;
							atg_pack_element(b, totcol);
							atg_box *b2=totcol->elem.box;
							if(b2)
							{
								char dt[20],nt[20],tt[20],lt[20];
								if(D||N)
								{
									snprintf(dt, 20, "Dispatched:%u", D);
									atg_element *dl=atg_create_element_label(dt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
									if(dl) atg_pack_element(b2, dl);
									snprintf(nt, 20, "Hit Target:%u", N);
									atg_element *nl=atg_create_element_label(nt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
									if(nl) atg_pack_element(b2, nl);
									if(N)
									{
										if(Tb)
										{
											snprintf(tt, 20, "Bombs (lb):%u", Tb);
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
										if(Tm)
										{
											snprintf(tt, 20, "Mines (lb):%u", Tm);
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
										if(Tl)
										{
											snprintf(tt, 20, "Leaflets  :%u", Tl);
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
										if(Ts)
										{
											snprintf(tt, 20, "Ships sunk:%u", Ts);
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
									}
									if(L)
									{
										snprintf(lt, 20, "A/c Lost  :%u", L);
										atg_element *ll=atg_create_element_label(lt, 10, (atg_colour){255, 0, 0, ATG_ALPHA_OPAQUE});
										if(ll) atg_pack_element(b2, ll);
									}
								}
							}
						}
					}
				}
			}
		}
		canvas->box=rstatbox;
		atg_flip(canvas);
		int errupt=0;
		while(!errupt)
		{
			while(atg_poll_event(&e, canvas))
			{
				switch(e.type)
				{
					case ATG_EV_RAW:;
						SDL_Event s=e.event.raw;
						switch(s.type)
						{
							case SDL_QUIT:
								errupt++;
							break;
							case SDL_KEYDOWN:
								switch(s.key.keysym.sym)
								{
									case SDLK_SPACE:
										errupt++;
									break;
									default:
									break;
								}
							break;
						}
					break;
					case ATG_EV_CLICK:;
						atg_ev_click c=e.event.click;
						if(c.e==RS_resize)
						{
							mainsizex=default_w;
							mainsizey=default_h;
							atg_resize_canvas(canvas, mainsizex, mainsizey);
							atg_flip(canvas);
						}
						if(c.e==RS_full)
						{
							fullscreen=!fullscreen;
							atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
							atg_flip(canvas);
						}
					break;
					case ATG_EV_TRIGGER:;
						atg_ev_trigger t=e.event.trigger;
						if(t.e==RS_cont)
							errupt++;
						else
							fprintf(stderr, "Clicked unknown button %p\n", (void *)t.e);
					break;
					default:
					break;
				}
			}
			SDL_Delay(50);
		}
	}
	else
	{
		#if !NOWEATHER
		for(unsigned int it=0;it<512;it++)
			w_iter(&state.weather, lorw);
		#endif
	}
	for(unsigned int i=0;i<state.nbombers;i++)
	{
		unsigned int type=state.bombers[i].type;
		if(!datewithin(state.now, types[type].entry, types[type].exit))
		{
			ob_append(&state.hist, state.now, (time){11, 10}, state.bombers[i].id, false, type);
			state.nbombers--;
			for(unsigned int j=i;j<state.nbombers;j++)
				state.bombers[j]=state.bombers[j+1];
			i--;
			continue;
		}
		if(state.bombers[i].failed)
		{
			if(brandp((types[type].svp/100.0)/2.0))
			{
				fa_append(&state.hist, state.now, (time){11, 10}, state.bombers[i].id, false, type, 0);
				state.bombers[i].failed=false;
			}
		}
		else
		{
			if(brandp((1-types[type].svp/100.0)/2.4))
			{
				fa_append(&state.hist, state.now, (time){11, 10}, state.bombers[i].id, false, type, 1);
				state.bombers[i].failed=true;
			}
		}
	}
	date tomorrow=nextday(state.now);
	// TODO: if confid or morale too low, SACKED
	state.cshr+=state.confid*2.0;
	state.cshr+=state.morale;
	state.cshr*=.96;
	state.cash+=state.cshr;
	ca_append(&state.hist, state.now, (time){11, 20}, state.cshr, state.cash);
	// Update bomber prodn caps
	for(unsigned int i=0;i<ntypes;i++)
	{
		if(!diffdate(tomorrow, types[i].entry)) // when a type is new, get 15 of them free
		{
			for(unsigned int a=0;a<15;a++)
			{
				unsigned int n=state.nbombers++;
				ac_bomber *nb=realloc(state.bombers, state.nbombers*sizeof(ac_bomber));
				if(!nb)
				{
					perror("realloc"); // TODO more visible warning
					state.nbombers=n;
					break;
				}
				(state.bombers=nb)[n]=(ac_bomber){.type=i, .failed=false, .id=rand_acid()};
				for(unsigned int j=0;j<NNAVAIDS;j++)
					nb[n].nav[j]=false;
				if((!datebefore(state.now, event[EVENT_ALLGEE]))&&types[i].nav[NAV_GEE])
					nb[n].nav[NAV_GEE]=true;
				ct_append(&state.hist, state.now, (time){11, 25}, state.bombers[n].id, false, state.bombers[n].type);
			}
		}
		if(!datewithin(state.now, types[i].entry, types[i].exit)) continue;
		types[i].pcbuf=min(types[i].pcbuf, 180000)+types[i].pc;
	}
	// purchase additional planes based on priorities and subject to production capacity constraints
	while(true)
	{
		unsigned int m=0;
		for(unsigned int i=1;i<ntypes;i++)
		{
			if(!datewithin(state.now, types[i].entry, types[i].exit)) continue;
			if(types[i].pribuf>types[m].pribuf) m=i;
		}
		if(types[m].pribuf<8)
		{
			bool any=false;
			for(unsigned int i=0;i<ntypes;i++)
			{
				if(!datewithin(state.now, types[i].entry, types[i].exit)) continue;
				unsigned int prio=(unsigned int [4]){0, 1, 3, 6}[types[i].prio];
				if(datebefore(state.now, types[i].novelty)) prio=max(prio, 1);
				types[i].pribuf+=prio;
				if(prio) any=true;
			}
			if(!any) break;
			continue;
		}
		unsigned int cost=types[m].cost;
		if(types[m].broughton&&!datebefore(state.now, event[EVENT_BROUGHTON]))
			cost=(cost*2)/3;
		if(cost>state.cash) break;
		if(cost>types[m].pcbuf) break;
		unsigned int n=state.nbombers++;
		ac_bomber *nb=realloc(state.bombers, state.nbombers*sizeof(ac_bomber));
		if(!nb)
		{
			perror("realloc"); // TODO more visible warning
			state.nbombers=n;
			break;
		}
		(state.bombers=nb)[n]=(ac_bomber){.type=m, .failed=false, .id=rand_acid()};
		for(unsigned int j=0;j<NNAVAIDS;j++)
			nb[n].nav[j]=false;
		if((!datebefore(state.now, event[EVENT_ALLGEE]))&&types[m].nav[NAV_GEE])
			nb[n].nav[NAV_GEE]=true;
		state.cash-=cost;
		types[m].pcbuf-=cost;
		types[m].pc+=cost/100;
		types[m].pribuf-=8;
		ct_append(&state.hist, state.now, (time){11, 30}, state.bombers[n].id, false, state.bombers[n].type);
	}
	// install navaids
	for(unsigned int n=0;n<NNAVAIDS;n++)
	{
		if(datebefore(state.now, event[navevent[n]])) continue;
		date x=event[navevent[n]];
		x.month+=2;
		if(x.month>12) {x.month-=12;x.year++;}
		bool notnew=datebefore(x, state.now);
		state.napb[n]+=notnew?25:10;
		unsigned int i=state.nap[n];
		if(!datewithin(state.now, types[i].entry, types[i].exit)) continue;
		unsigned int j, nac=0;
		for(j=0;(state.napb[n]>=navprod[n])&&(j<state.nbombers);j++)
		{
			if(state.bombers[j].type!=i) continue;
			if(state.bombers[j].failed) continue;
			if(state.bombers[j].nav[n]) continue;
			state.bombers[j].nav[n]=true;
			na_append(&state.hist, state.now, (time){11, 35}, state.bombers[j].id, false, state.bombers[j].type, n);
			state.napb[n]-=navprod[n];
			if(++nac>=(notnew?10:4)) break;
		}
	}
	// assign PFF
	if(!datebefore(tomorrow, event[EVENT_PFF]))
	{
		for(unsigned int j=0;j<ntypes;j++)
			types[j].count=types[j].pffcount=0;
		for(unsigned int i=0;i<state.nbombers;i++)
		{
			unsigned int type=state.bombers[i].type;
			if(types[type].pff&&types[type].noarm)
			{
				state.bombers[i].pff=true;
				pf_append(&state.hist, state.now, (time){11, 40}, state.bombers[i].id, false, type);
				continue;
			}
			types[type].count++;
			if(state.bombers[i].pff) types[type].pffcount++;
		}
		for(unsigned int j=0;j<ntypes;j++)
		{
			if(!types[j].pff) continue;
			int pffneed=ceil(types[j].count*0.2)-types[j].pffcount;
			for(unsigned int i=0;(pffneed>0)&&(i<state.nbombers);i++)
			{
				if(state.bombers[i].type!=j) continue;
				if(state.bombers[i].pff) continue;
				state.bombers[i].pff=true;
				pf_append(&state.hist, state.now, (time){11, 40}, state.bombers[i].id, false, j);
				pffneed--;
			}
		}
	}
	// German production
	for(unsigned int i=0;i<ICLASS_MIXED;i++)
		state.dprod[i]=0;
	for(unsigned int i=0;i<ntargs;i++)
	{
		if(!datebefore(state.now, targs[i].exit)) continue;
		switch(targs[i].class)
		{
			case TCLASS_CITY:
			{
				state.flam[i]=(state.flam[i]*0.8)+8.0;
				double dflk=(targs[i].flak*state.dmg[i]*.0005)-(state.flk[i]*.05);
				state.flk[i]+=dflk;
				if(dflk)
					tfk_append(&state.hist, state.now, (time){11, 45}, i, dflk, state.flk[i]);
				double ddmg=min(state.dmg[i]*.01, 100-state.dmg[i]);
				state.dmg[i]+=ddmg;
				if(ddmg)
					tdm_append(&state.hist, state.now, (time){11, 45}, i, ddmg, state.dmg[i]);
				if(datebefore(state.now, targs[i].entry))
					produce(i, &state, 80*targs[i].prod);
				else
					produce(i, &state, state.dmg[i]*targs[i].prod);
			}
			break;
			case TCLASS_LEAFLET:
			{
				double ddmg=min(state.dmg[i]*.01, 100-state.dmg[i]);
				state.dmg[i]+=ddmg;
				if(ddmg)
					tdm_append(&state.hist, state.now, (time){11, 45}, i, ddmg, state.dmg[i]);
				produce(i, &state, state.dmg[i]*targs[i].prod/20.0);
			}
			break;
			case TCLASS_SHIPPING:
			case TCLASS_MINING:
			break;
			case TCLASS_AIRFIELD:
			case TCLASS_BRIDGE:
			case TCLASS_ROAD:
				if(!state.dmg[i])
					goto unflak;
			break;
			case TCLASS_INDUSTRY:
				if(state.dmg[i])
				{
					double ddmg=min(state.dmg[i]*.05, 100-state.dmg[i]);
					state.dmg[i]+=ddmg;
					tdm_append(&state.hist, state.now, (time){11, 45}, i, ddmg, state.dmg[i]);
					produce(i, &state, state.dmg[i]*targs[i].prod/2.0);
				}
				else
				{
					unflak:
					if(state.flk[i])
					{
						tfk_append(&state.hist, state.now, (time){11, 45}, i, -state.flk[i], 0);
						state.flk[i]=0;
					}
				}
			break;
			default: // shouldn't ever get here
				fprintf(stderr, "Bad targs[%d].class = %d\n", i, targs[i].class);
			break;
		}
	}
	state.gprod[ICLASS_ARM]*=datebefore(state.now, event[EVENT_BARBAROSSA])?0.96:0.94;
	state.gprod[ICLASS_BB]*=0.99;
	state.gprod[ICLASS_RAIL]*=0.99;
	state.gprod[ICLASS_OIL]*=0.98;
	state.gprod[ICLASS_UBOOT]*=0.95; // not actually used for anything
	// German fighters
	memset(fcount, 0, sizeof(fcount));
	for(unsigned int i=0;i<state.nfighters;i++)
	{
		unsigned int type=state.fighters[i].type;
		if(state.fighters[i].crashed||!datewithin(state.now, ftypes[type].entry, ftypes[type].exit))
		{
			if(!state.fighters[i].crashed&&!datewithin(state.now, ftypes[type].entry, ftypes[type].exit))
				ob_append(&state.hist, state.now, (time){11, 48}, state.fighters[i].id, true, type);
			state.nfighters--;
			for(unsigned int j=i;j<state.nfighters;j++)
				state.fighters[j]=state.fighters[j+1];
			i--;
			continue;
		}
		fcount[type]++;
		if(brandp(0.1))
		{
			unsigned int base;
			do
				base=state.fighters[i].base=irandu(nfbases);
			while(!datewithin(state.now, fbases[base].entry, fbases[base].exit));
		}
	}
	unsigned int mfcost=0;
	for(unsigned int i=0;i<nftypes;i++)
	{
		if(!datewithin(state.now, ftypes[i].entry, ftypes[i].exit)) continue;
		mfcost=max(mfcost, ftypes[i].cost);
	}
	while(state.gprod[ICLASS_AC]>=mfcost)
	{
		double p[nftypes], cumu_p=0;
		for(unsigned int j=0;j<nftypes;j++)
		{
			if(!datewithin(state.now, ftypes[j].entry, ftypes[j].exit))
			{
				p[j]=0;
				continue;
			}
			date start=ftypes[j].entry, end=ftypes[j].exit;
			if(start.year<1939) start=(date){1938, 6, 1};
			if(end.year>1945) end=(date){1946, 8, 1};
			int dur=diffdate(end, start), age=diffdate(state.now, start);
			double fr_age=age/(double)dur;
			p[j]=(0.8-fr_age)/sqrt(1+fcount[j]);
			p[j]=max(p[j], 0);
			cumu_p+=p[j];
		}
		double d=drandu(cumu_p);
		unsigned int i=0;
		while(d>=p[i]) d-=p[i++];
		if(!datewithin(state.now, ftypes[i].entry, ftypes[i].exit)) continue; // should be impossible as p[i] == 0
		if(ftypes[i].cost>state.gprod[ICLASS_AC]) break; // should also be impossible as cost <= mfcost <= state.gprod
		unsigned int n=state.nfighters++;
		ac_fighter *newf=realloc(state.fighters, state.nfighters*sizeof(ac_fighter));
		if(!newf)
		{
			perror("realloc"); // TODO more visible warning
			state.nfighters=n;
			break;
		}
		unsigned int base;
		do
			base=irandu(nfbases);
		while(!datewithin(state.now, fbases[base].entry, fbases[base].exit));
		(state.fighters=newf)[n]=(ac_fighter){.type=i, .base=base, .crashed=false, .landed=true, .k=-1, .targ=-1, .damage=0, .id=rand_acid()};
		state.gprod[ICLASS_AC]-=ftypes[i].cost;
		ct_append(&state.hist, state.now, (time){11, 50}, state.fighters[n].id, true, i);
	}
	for(unsigned int i=0;i<ICLASS_MIXED;i++)
		gp_append(&state.hist, state.now, (time){11, 55}, i, state.gprod[i], state.dprod[i]);
	state.now=tomorrow;
	for(unsigned int i=0;i<ntypes;i++)
		if(!diffdate(state.now, types[i].entry))
		{
			if(msgadd(canvas, &state, state.now, types[i].name, types[i].newtext))
				fprintf(stderr, "failed to msgadd newtype: %s\n", types[i].name);
		}
	for(unsigned int i=0;i<nftypes;i++)
		if(!diffdate(state.now, ftypes[i].entry))
		{
			if(msgadd(canvas, &state, state.now, ftypes[i].name, ftypes[i].newtext))
				fprintf(stderr, "failed to msgadd newftype: %s\n", ftypes[i].name);
		}
	for(unsigned int ev=0;ev<NEVENTS;ev++)
	{
		if(!diffdate(state.now, event[ev]))
		{
			if(msgadd(canvas, &state, state.now, event_names[ev], evtext[ev]))
				fprintf(stderr, "failed to msgadd event: %s\n", event_names[ev]);
		}
	}
	mainsizex=canvas->surface->w;
	mainsizey=canvas->surface->h;
	goto gameloop;
	
	do_exit:
	canvas->box=NULL;
	atg_free_canvas(canvas);
	atg_free_box_box(mainbox);
	atg_free_box_box(gamebox);
	if(LB_btext) *LB_btext=NULL;
	atg_free_box_box(loadbox);
	if(SA_btext) *SA_btext=NULL;
	atg_free_box_box(savebox);
	SDL_FreeSurface(xhair_overlay);
	SDL_FreeSurface(weather_overlay);
	SDL_FreeSurface(target_overlay);
	SDL_FreeSurface(flak_overlay);
	SDL_FreeSurface(grey_overlay);
	SDL_FreeSurface(terrain);
	SDL_FreeSurface(map);
	SDL_FreeSurface(location);
	return(0);
}

void drawmoon(SDL_Surface *s, double phase)
{
	SDL_FillRect(s, &(SDL_Rect){.x=0, .y=0, .w=s->w, .h=s->h}, SDL_MapRGB(s->format, GAME_BG_COLOUR.r, GAME_BG_COLOUR.g, GAME_BG_COLOUR.b));
	double left=(phase<0.5)?phase*2:1,
	      right=(phase>0.5)?phase*2-1:0;
	double halfx=(s->w-1)/2.0,
	       halfy=(s->h-1)/2.0;
	for(int y=1;y<s->h-1;y++)
	{
		double width=sqrt(((s->w/2)-1)*((s->w/2)-1)-(y-halfy)*(y-halfy));
		double startx=halfx-width, endx=halfx+width;
		double leftx=width*cos( left*M_PI)+halfx,
		      rightx=width*cos(right*M_PI)+halfx;
		for(unsigned int x=floor(startx);x<=ceil(endx);x++)
		{
			double a;
			if(x<startx)
				a=x+1-startx;
			else if(x>endx)
				a=endx+1-x;
			else
				a=1;
			unsigned int alpha=ceil(ATG_ALPHA_OPAQUE*a);
			double lf=x+1-leftx,
			       rf=rightx-x;
			clamp(lf, 0, 1);
			clamp(rf, 0, 1);
			unsigned int br=floor(lf*rf*224);
			pset(s, x, y, (atg_colour){br, br, br, alpha});
		}
	}
}

bool version_newer(const unsigned char v1[3], const unsigned char v2[3])
{
	for(unsigned int i=0;i<3;i++)
	{
		if(v1[i]>v2[i]) return(true);
		if(v1[i]<v2[i]) return(false);
	}
	return(false);
}

int loadgame(const char *fn, game *state, bool lorw[128][128])
{
	FILE *fs = fopen(fn, "r");
	if(!fs)
	{
		fprintf(stderr, "Failed to open %s!\n", fn);
		perror("fopen");
		return(1);
	}
	unsigned char s_version[3]={0,0,0};
	unsigned char version[3]={VER_MAJ,VER_MIN,VER_REV};
	bool warned_pff=false, warned_acid=false;
	while(!feof(fs))
	{
		char *line=fgetl(fs);
		if(!line) break;
		if((!*line)||(*line=='#'))
		{
			free(line);
			continue;
		}
		char tag[64];
		char *dat=strchr(line, ':');
		if(dat) *dat++=0;
		strncpy(tag, line, 64);
		int e=0,f; // poor-man's try...
		if(strcmp(tag, "HARR")==0)
		{
			f=sscanf(dat, "%hhu.%hhu.%hhu\n", s_version, s_version+1, s_version+2);
			if(f!=3)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			if(version_newer(s_version, version))
			{
				fprintf(stderr, "Warning - file is newer version than program;\n may cause strange behaviour\n");
			}
		}
		else if(!(s_version[0]||s_version[1]||s_version[2]))
		{
			fprintf(stderr, "8 File does not start with valid HARR tag\n");
			e|=8;
		}
		else if(strcmp(tag, "DATE")==0)
		{
			state->now=readdate(dat, (date){3, 9, 1939});
		}
		else if(strcmp(tag, "Confid")==0)
		{
			f=sscanf(dat, "%la\n", &state->confid);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
		}
		else if(strcmp(tag, "Morale")==0)
		{
			f=sscanf(dat, "%la\n", &state->morale);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
		}
		else if(strcmp(tag, "Budget")==0)
		{
			f=sscanf(dat, "%u+%u\n", &state->cash, &state->cshr);
			if(f!=2)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
		}
		else if(strcmp(tag, "Types")==0)
		{
			unsigned int sntypes;
			f=sscanf(dat, "%u\n", &sntypes);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(sntypes!=ntypes)
			{
				fprintf(stderr, "2 Value mismatch: different ntypes value\n");
				e|=2;
			}
			else
			{
				for(unsigned int i=0;i<ntypes;i++)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					unsigned int j, prio, pribuf, pc, pcbuf;
					f=sscanf(line, "Prio %u:%u,%u,%u,%u\n", &j, &prio, &pribuf, &pc, &pcbuf);
					if(f!=5)
					{
						fprintf(stderr, "1 Too few arguments to part %u of tag \"%s\"\n", i, tag);
						e|=1;
						break;
					}
					if(j!=i)
					{
						fprintf(stderr, "4 Index mismatch in part %u (%u?) of tag \"%s\"\n", i, j, tag);
						e|=4;
						break;
					}
					types[j].prio=prio;
					types[j].pribuf=pribuf;
					types[j].pc=pc;
					types[j].pcbuf=pcbuf;
				}
			}
		}
		else if(strcmp(tag, "Navaids")==0)
		{
			unsigned int snnav;
			f=sscanf(dat, "%u\n", &snnav);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(snnav!=NNAVAIDS)
			{
				fprintf(stderr, "2 Value mismatch: different nnav value\n");
				e|=2;
			}
			else
			{
				for(unsigned int i=0;i<NNAVAIDS;i++)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					unsigned int j, prio, pbuf;
					f=sscanf(line, "NPrio %u:%u,%u\n", &j, &prio, &pbuf);
					if(f!=3)
					{
						fprintf(stderr, "1 Too few arguments to part %u of tag \"%s\"\n", i, tag);
						e|=1;
						break;
					}
					if(j!=i)
					{
						fprintf(stderr, "4 Index mismatch in part %u (%u?) of tag \"%s\"\n", i, j, tag);
						e|=4;
						break;
					}
					state->nap[j]=prio;
					state->napb[j]=pbuf;
				}
			}
		}
		else if(strcmp(tag, "Bombers")==0)
		{
			f=sscanf(dat, "%u\n", &state->nbombers);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else
			{
				free(state->bombers);
				state->bombers=malloc(state->nbombers*sizeof(ac_bomber));
				for(unsigned int i=0;i<state->nbombers;i++)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					unsigned int j;
					unsigned int failed,nav,pff;
					char p_id[9];
					f=sscanf(line, "Type %u:%u,%u,%u,%8s\n", &j, &failed, &nav, &pff, p_id);
					// TODO: this is for oldsave compat and should probably be removed later
					if(f==3)
					{
						f=4;
						if(!warned_pff) fprintf(stderr, "Warning: added missing 'PFF' field in tag \"%s\"\n", tag);
						warned_pff=true;
						pff=0;
					}
					if(f==4)
					{
						f=5;
						if(!warned_acid) fprintf(stderr, "Warning: added missing 'A/c ID' field in tag \"%s\"\n", tag);
						warned_acid=true;
						pacid(rand_acid(), p_id);
					}
					if(f!=5)
					{
						fprintf(stderr, "1 Too few arguments to part %u of tag \"%s\"\n", i, tag);
						e|=1;
						break;
					}
					if(j>ntypes)
					{
						fprintf(stderr, "4 Index mismatch in part %u of tag \"%s\"\n", j, tag);
						e|=4;
						break;
					}
					state->bombers[i]=(ac_bomber){.type=j, .failed=failed, .pff=pff};
					for(unsigned int n=0;n<NNAVAIDS;n++)
						state->bombers[i].nav[n]=(nav>>n)&1;
					if(gacid(p_id, &state->bombers[i].id))
					{
						fprintf(stderr, "32 Invalid value \"%s\" for a/c ID in tag \"%s\"\n", p_id, tag);
						e|=32;
						break;
					}
				}
			}
		}
		else if(strcmp(tag, "GProd")==0)
		{
			unsigned int snclass;
			f=sscanf(dat, "%u\n", &snclass);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(snclass!=ICLASS_MIXED)
			{
				fprintf(stderr, "2 Value mismatch: different iclasses value\n");
				e|=2;
			}
			else
			{
				for(unsigned int i=0;i<snclass;i++)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					if(*line=='#')
					{
						i--;
						continue;
					}
					unsigned int j;
					f=sscanf(line, "IClass %u:%la\n", &j, &state->gprod[i]);
					if(f!=2)
					{
						fprintf(stderr, "1 Too few arguments to part %u of tag \"%s\"\n", i, tag);
						e|=1;
						break;
					}
					if(j!=i)
					{
						fprintf(stderr, "4 Index mismatch in part %u (%u?) of tag \"%s\"\n", i, j, tag);
						e|=4;
						break;
					}
				}
			}
		}
		else if(strcmp(tag, "FTypes")==0)
		{
			unsigned int snftypes;
			f=sscanf(dat, "%u\n", &snftypes);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(snftypes!=nftypes)
			{
				fprintf(stderr, "2 Value mismatch: different nftypes value\n");
				e|=2;
			}
		}
		else if(strcmp(tag, "FBases")==0)
		{
			unsigned int snfbases;
			f=sscanf(dat, "%u\n", &snfbases);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(snfbases!=nfbases)
			{
				fprintf(stderr, "2 Value mismatch: different nfbases value\n");
				e|=2;
			}
		}
		else if(strcmp(tag, "Fighters")==0)
		{
			f=sscanf(dat, "%u\n", &state->nfighters);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else
			{
				free(state->fighters);
				state->fighters=malloc(state->nfighters*sizeof(ac_fighter));
				for(unsigned int i=0;i<state->nfighters;i++)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					unsigned int j;
					unsigned int base;
					char p_id[9];
					f=sscanf(line, "Type %u:%u,%8s\n", &j, &base, p_id);
					// TODO: this is for oldsave compat and should probably be removed later
					if(f==2)
					{
						f=3;
						strcpy(p_id, "NOID");
					}
					if(f!=3)
					{
						fprintf(stderr, "1 Too few arguments to part %u of tag \"%s\"\n", i, tag);
						e|=1;
						break;
					}
					if(j>nftypes)
					{
						fprintf(stderr, "4 Index mismatch in part %u of tag \"%s\"\n", j, tag);
						e|=4;
						break;
					}
					state->fighters[i]=(ac_fighter){.type=j, .base=base};
					if(strcmp(p_id, "NOID")==0)
						state->fighters[i].id=rand_acid();
					else if(gacid(p_id, &state->fighters[i].id))
					{
						fprintf(stderr, "32 Invalid value \"%s\" for a/c ID in tag \"%s\"\n", p_id, tag);
						e|=32;
						break;
					}
				}
			}
		}
		else if(strcmp(tag, "Targets init")==0)
		{
			double dmg,flk,heat,flam;
			f=sscanf(dat, "%la,%la,%la,%la\n", &dmg, &flk, &heat, &flam);
			if(f!=4)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
				break;
			}
			for(unsigned int i=0;i<ntargs;i++)
			{
				state->dmg[i]=dmg;
				state->flk[i]=flk*targs[i].flak/100.0;
				state->heat[i]=heat;
				state->flam[i]=flam;
			}
		}
		else if(strcmp(tag, "Targets")==0)
		{
			unsigned int sntargs;
			f=sscanf(dat, "%u\n", &sntargs);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(sntargs!=ntargs)
			{
				fprintf(stderr, "2 Value mismatch: different ntargs value\n");
				e|=2;
			}
			else
			{
				for(unsigned int i=0;i<ntargs;i++)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					if(*line=='#')
					{
						i--;
						continue;
					}
					unsigned int j;
					double dmg, flk, heat, flam;
					f=sscanf(line, "Targ %u:%la,%la,%la,%la\n", &j, &dmg, &flk, &heat, &flam);
					if(f!=5)
					{
						fprintf(stderr, "1 Too few arguments to part %u of tag \"%s\"\n", j, tag);
						e|=1;
						break;
					}
					while((i<j)&&(i<ntargs))
					{
						state->dmg[i]=100;
						state->flk[i]=targs[i].flak;
						state->heat[i]=0;
						state->flam[i]=40;
						i++;
					}
					if(j!=i)
					{
						fprintf(stderr, "4 Index mismatch in part %u (%u?) of tag \"%s\"\n", i, j, tag);
						e|=4;
						break;
					}
					state->dmg[i]=dmg;
					state->flk[i]=flk*targs[i].flak/100.0;
					state->heat[i]=heat;
					state->flam[i]=flam;
				}
			}
		}
		else if(strcmp(tag, "Weather state")==0)
		{
			f=sscanf(dat, "%la,%la\n", &state->weather.push, &state->weather.slant);
			if(f!=2)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else
			{
				free(line);
				line=fgetl(fs);
				if(!line)
				{
					fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
					e|=64;
				}
				for(unsigned int x=0;x<256;x++)
				{
					size_t p=0;
					for(unsigned int y=0;y<128;y++)
					{
						int bytes;
						if(sscanf(line+p, "%la,%la,%n", &state->weather.p[x][y], &state->weather.t[x][y], &bytes)!=2)
						{
							fprintf(stderr, "1 Too few arguments to part (%u,%u) of tag \"%s\"\n", x, y, tag);
							e|=1;
							break;
						}
						p+=bytes;
					}
					if(e) break;
					if(x==255) break;
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
				}
			}
		}
		else if(strcmp(tag, "Weather rand")==0)
		{
			unsigned int seed;
			f=sscanf(dat, "%u\n", &seed);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			srand(seed);
			w_init(&state->weather, 256, lorw);
		}
		else if(strcmp(tag, "Messages")==0)
		{
			unsigned int snmsgs;
			f=sscanf(dat, "%u\n", &snmsgs);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(snmsgs > MAXMSGS)
			{
				fprintf(stderr, "2 Value mismatch: too many messages\n");
				e|=2;
			}
			else
			{
				for(unsigned int i=0;i<MAXMSGS;i++)
				{
					free(state->msg[i]);
					state->msg[i]=NULL;
				}
				unsigned int msg=0;
				size_t len=0;
				while(msg<snmsgs)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					if(strcmp(line, ".")==0)
					{
						msg++;
						len=0;
					}
					else
					{
						size_t nl=len+strlen(line);
						char *nmsg=realloc(state->msg[msg], nl+2);
						if(!nmsg)
						{
							perror("16 realloc");
							e|=16;
						}
						else
						{
							state->msg[msg]=nmsg;
							strcpy(state->msg[msg]+len, line);
							strcpy(state->msg[msg]+nl, "\n");
							len=nl+1;
						}
					}
				}
			}
		}
		else if(strcmp(tag, "History")==0)
		{
			size_t nents;
			f=sscanf(dat, "%zu\n", &nents);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if((f=hist_load(fs, nents, &state->hist)))
			{
				fprintf(stderr, "32 Invalid value: error %d somewhere in hist_load, in tag \"%s\"\n", f, tag);
				e|=32;
			}
		}
		else
		{
			fprintf(stderr, "128 Bad tag \"%s\"\n", tag);
			e|=128;
		}
		free(line);
		if(e!=0) // ...catch
		{
			fprintf(stderr, "Error (%d) reading from %s\n", e, fn);
			fclose(fs);
			return(e);
		}
	}
	return(0);
}

int savegame(const char *fn, game state)
{
	FILE *fs = fopen(fn, "w");
	if(!fs)
	{
		fprintf(stderr, "Failed to open %s!\n", fn);
		perror("fopen");
		return(1);
	}
	char p_id[9];
	fprintf(fs, "HARR:%hhu.%hhu.%hhu\n", VER_MAJ, VER_MIN, VER_REV);
	fprintf(fs, "DATE:%02d-%02d-%04d\n", state.now.day, state.now.month, state.now.year);
	fprintf(fs, "Confid:%la\n", state.confid);
	fprintf(fs, "Morale:%la\n", state.morale);
	fprintf(fs, "Budget:%u+%u\n", state.cash, state.cshr);
	fprintf(fs, "Types:%u\n", ntypes);
	for(unsigned int i=0;i<ntypes;i++)
		fprintf(fs, "Prio %u:%u,%u,%u,%u\n", i, types[i].prio, types[i].pribuf, types[i].pc, types[i].pcbuf);
	fprintf(fs, "Navaids:%u\n", NNAVAIDS);
	for(unsigned int n=0;n<NNAVAIDS;n++)
		fprintf(fs, "NPrio %u:%u,%u\n", n, state.nap[n], state.napb[n]);
	fprintf(fs, "Bombers:%u\n", state.nbombers);
	for(unsigned int i=0;i<state.nbombers;i++)
	{
		unsigned int nav=0;
		for(unsigned int n=0;n<NNAVAIDS;n++)
			nav|=(state.bombers[i].nav[n]?(1<<n):0);
		pacid(state.bombers[i].id, p_id);
		fprintf(fs, "Type %u:%u,%u,%u,%s\n", state.bombers[i].type, state.bombers[i].failed?1:0, nav, state.bombers[i].pff?1:0, p_id);
	}
	fprintf(fs, "GProd:%u\n", ICLASS_MIXED);
	for(unsigned int i=0;i<ICLASS_MIXED;i++)
		fprintf(fs, "IClass %u:%la\n", i, state.gprod[i]);
	fprintf(fs, "FTypes:%u\n", nftypes);
	fprintf(fs, "FBases:%u\n", nfbases);
	fprintf(fs, "Fighters:%u\n", state.nfighters);
	for(unsigned int i=0;i<state.nfighters;i++)
	{
		pacid(state.fighters[i].id, p_id);
		fprintf(fs, "Type %u:%u,%s\n", state.fighters[i].type, state.fighters[i].base, p_id);
	}
	fprintf(fs, "Targets:%hhu\n", ntargs);
	for(unsigned int i=0;i<ntargs;i++)
		fprintf(fs, "Targ %hhu:%la,%la,%la,%la\n", i, state.dmg[i], targs[i].flak?state.flk[i]*100.0/(double)targs[i].flak:0, state.heat[i], state.flam[i]);
	fprintf(fs, "Weather state:%la,%la\n", state.weather.push, state.weather.slant);
	for(unsigned int x=0;x<256;x++)
	{
		for(unsigned int y=0;y<128;y++)
			fprintf(fs, "%la,%la,", state.weather.p[x][y], state.weather.t[x][y]);
		fprintf(fs, "\n");
	}
	unsigned int msgs=0;
	for(unsigned int i=0;i<MAXMSGS;i++)
		if(state.msg[i]) msgs++;
	fprintf(fs, "Messages:%u\n", msgs);
	for(unsigned int i=0;i<MAXMSGS;i++)
		if(state.msg[i])
		{
			fputs(state.msg[i], fs);
			fputs(".\n", fs);
		}
	hist_save(state.hist, fs);
	fclose(fs);
	return(0);
}

SDL_Surface *render_weather(w_state weather)
{
	SDL_Surface *rv=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 256, 256, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!rv)
	{
		fprintf(stderr, "render_weather: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(NULL);
	}
	SDL_FillRect(rv, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, ATG_ALPHA_TRANSPARENT&0xff);
	for(unsigned int x=0;x<256;x++)
	{
		for(unsigned int y=0;y<256;y++)
		{
			Uint8 cl=min(max(floor(1016-weather.p[x>>1][y>>1])*8.0, 0), 255);
			pset(rv, x, y, (atg_colour){180+weather.t[x>>1][y>>1]*3, 210, 255-weather.t[x>>1][y>>1]*3, min(cl, 191)});
		}
	}
	return(rv);
}

SDL_Surface *render_targets(date now)
{
	SDL_Surface *rv=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 256, 256, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!rv)
	{
		fprintf(stderr, "render_targets: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(NULL);
	}
	SDL_FillRect(rv, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, ATG_ALPHA_TRANSPARENT&0xff);
	for(unsigned int i=0;i<ntargs;i++)
	{
		if((!datewithin(now, targs[i].entry, targs[i].exit)) && targs[i].class!=TCLASS_CITY) continue;
		switch(targs[i].class)
		{
			case TCLASS_CITY:
			case TCLASS_LEAFLET:
			case TCLASS_AIRFIELD:
			case TCLASS_ROAD:
			case TCLASS_BRIDGE:
			case TCLASS_INDUSTRY:
				SDL_gfxBlitRGBA(targs[i].picture, NULL, rv, &(SDL_Rect){.x=targs[i].lon-targs[i].picture->w/2, .y=targs[i].lat-targs[i].picture->h/2});
			break;
			case TCLASS_SHIPPING:
			case TCLASS_MINING:
				SDL_gfxBlitRGBA(targs[i].picture, NULL, rv, &(SDL_Rect){.x=targs[i].lon-1, .y=targs[i].lat-1});
			break;
			default:
				fprintf(stderr, "render_targets: Warning: bad targs[%d].class = %d\n", i, targs[i].class);
			break;
		}
	}
	return(rv);
}

SDL_Surface *render_flak(date now)
{
	SDL_Surface *rv=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 256, 256, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!rv)
	{
		fprintf(stderr, "render_targets: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(NULL);
	}
	SDL_FillRect(rv, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, ATG_ALPHA_TRANSPARENT&0xff);
	for(unsigned int i=0;i<nflaks;i++)
	{
		if(!datewithin(now, flaks[i].entry, flaks[i].exit)) continue;
		bool rad=!datebefore(now, flaks[i].radar);
		unsigned int x=flaks[i].lon, y=flaks[i].lat, str=flaks[i].strength;
		pset(rv, x, y, (atg_colour){rad?255:191, 0, rad?127:255, ATG_ALPHA_OPAQUE});
		if(str>5)
		{
			pset(rv, x+1, y, (atg_colour){191, 0, rad?127:255, ATG_ALPHA_OPAQUE});
			if(str>10)
			{
				pset(rv, x, y+1, (atg_colour){191, 0, rad?127:255, ATG_ALPHA_OPAQUE});
				if(str>20)
				{
					pset(rv, x-1, y, (atg_colour){191, 0, rad?127:255, ATG_ALPHA_OPAQUE});
					if(str>30)
					{
						pset(rv, x, y-1, (atg_colour){191, 0, rad?127:255, ATG_ALPHA_OPAQUE});
						if(str>40)
						{
							pset(rv, x+1, y+1, (atg_colour){127, 0, rad?127:191, ATG_ALPHA_OPAQUE});
							if(str>50)
							{
								pset(rv, x-1, y+1, (atg_colour){127, 0, rad?127:191, ATG_ALPHA_OPAQUE});
								if(str>60)
								{
									pset(rv, x+1, y-1, (atg_colour){127, 0, rad?127:191, ATG_ALPHA_OPAQUE});
									if(str>80)
									{
										pset(rv, x+1, y-1, (atg_colour){127, 0, rad?127:191, ATG_ALPHA_OPAQUE});
										if(str>=100)
										{
											pset(rv, x-1, y-1, (atg_colour){127, 0, rad?127:191, ATG_ALPHA_OPAQUE});
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return(rv);
}

SDL_Surface *render_ac(game state)
{
	SDL_Surface *rv=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 256, 256, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!rv)
	{
		fprintf(stderr, "render_ac: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(NULL);
	}
	SDL_FillRect(rv, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, ATG_ALPHA_TRANSPARENT&0xff);
	for(unsigned int i=0;i<state.ntargs;i++)
		for(unsigned int j=0;j<state.raids[i].nbombers;j++)
		{
			unsigned int k=state.raids[i].bombers[j];
			unsigned int x=floor(state.bombers[k].lon), y=floor(state.bombers[k].lat);
			if(state.bombers[k].crashed)
				pset(rv, x, y, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
			else if(state.bombers[k].damage>6)
				pset(rv, x, y, (atg_colour){255, 127, 0, ATG_ALPHA_OPAQUE});
			else if(state.bombers[k].failed)
				pset(rv, x, y, (atg_colour){0, 255, 255, ATG_ALPHA_OPAQUE});
			else if(state.bombers[k].bombed)
				pset(rv, x, y, (atg_colour){127, 255, 63, ATG_ALPHA_OPAQUE});
			else
				pset(rv, x, y, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
		}
	for(unsigned int i=0;i<state.nfighters;i++)
	{
		unsigned int x=floor(state.fighters[i].lon), y=floor(state.fighters[i].lat);
		if(state.fighters[i].crashed)
			pset(rv, x, y, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
		else if(state.fighters[i].landed)
			pset(rv, x, y, (atg_colour){127, 0, 0, ATG_ALPHA_OPAQUE});
		else
			pset(rv, x, y, (atg_colour){255, 0, 0, ATG_ALPHA_OPAQUE});
	}
	return(rv);
}

SDL_Surface *render_xhairs(game state, int seltarg)
{
	SDL_Surface *rv=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 256, 256, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!rv)
	{
		fprintf(stderr, "render_xhairs: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(NULL);
	}
	SDL_FillRect(rv, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, ATG_ALPHA_TRANSPARENT&0xff);
	for(unsigned int i=0;i<state.ntargs;i++)
		if(state.raids[i].nbombers)
		{
			SDL_gfxBlitRGBA(yellowhair, NULL, rv, &(SDL_Rect){.x=targs[i].lon-3, .y=targs[i].lat-3});
		}
	if(seltarg>=0)
	{
		SDL_gfxBlitRGBA(location, NULL, rv, &(SDL_Rect){.x=targs[seltarg].lon-3, .y=targs[seltarg].lat-3});
	}
	return(rv);
}

void update_navbtn(game state, atg_element *GB_navbtn[ntypes][NNAVAIDS], unsigned int i, unsigned int n, SDL_Surface *grey_overlay, SDL_Surface *yellow_overlay)
{
	if(GB_navbtn[i][n]&&GB_navbtn[i][n]->elem.image)
	{
		SDL_Surface *pic=GB_navbtn[i][n]->elem.image->data;
		if(pic)
		{
			if(!types[i].nav[n])
				SDL_FillRect(pic, &(SDL_Rect){.x=0, .y=0, .w=16, .h=16}, SDL_MapRGBA(pic->format, 63, 63, 63, SDL_ALPHA_OPAQUE));
			else
			{
				SDL_BlitSurface(navpic[n], NULL, pic, NULL);
				if(datebefore(state.now, event[navevent[n]]))
					SDL_BlitSurface(grey_overlay, NULL, pic, NULL);
				if(state.nap[n]==i)
					SDL_BlitSurface(yellow_overlay, NULL, pic, NULL);
			}
		}
	}
}

int msgadd(atg_canvas *canvas, game *state, date when, const char *ref, const char *msg)
{
	if(!state) return(1);
	if(!ref) return(2);
	if(!msg) return(2);
	size_t rl=strlen(ref);
	char *res=malloc(13+rl+strlen(msg));
	if(!res)
		return(-1);
	snprintf(res, 12, "%02u-%02u-%04u:", when.day, when.month, when.year);
	strcpy(res+11, ref);
	strcat(res+11+rl, "\n");
	strcat(res+12+rl, msg);
	unsigned int in=0, out=0;
	while(in<MAXMSGS)
	{
		if(state->msg[in])
			state->msg[out++]=state->msg[in];
		in++;
	}
	unsigned int fill=out;
	while(fill<MAXMSGS)
		state->msg[fill++]=NULL;
	if(out<MAXMSGS)
		state->msg[out]=res;
	else
	{
		for(in=1;in<MAXMSGS;in++)
			state->msg[in-1]=state->msg[in];
		state->msg[MAXMSGS-1]=res;
	}
	message_box(canvas, "To the Commander-in-Chief, Bomber Command:", res, "Air Chief Marshal C. F. A. Portal, CAS");
	return(0);
}

void message_box(atg_canvas *canvas, const char *titletext, const char *bodytext, const char *signtext)
{
	atg_box *oldbox=canvas->box;
	atg_box *msgbox=atg_create_box(ATG_BOX_PACK_VERTICAL, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE});
	if(!msgbox)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return;
	}
	atg_element *title=atg_create_element_label(titletext, 16, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
	if(!title)
		fprintf(stderr, "atg_create_element_label failed\n");
	else
	{
		if(atg_pack_element(msgbox, title))
			perror("atg_pack_element");
		else
		{
			size_t x=0;
			while(bodytext[x])
			{
				size_t l=strcspn(bodytext+x, "\n");
				char *t=l?strndup(bodytext+x, l):strdup(" ");
				atg_element *r=atg_create_element_label(t, 12, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
				free(t);
				if(!r)
				{
					fprintf(stderr, "atg_create_element_label failed\n");
					break;
				}
				if(atg_pack_element(msgbox, r))
				{
					perror("atg_pack_element");
					break;
				}
				x+=l;
				if(bodytext[x]=='\n') x++;
			}
			if(!bodytext[x])
			{
				atg_element *sign=atg_create_element_label(signtext, 14, (atg_colour){2, 2, 9, ATG_ALPHA_OPAQUE});
				if(!sign)
					fprintf(stderr, "atg_create_element_label failed\n");
				else
				{
					if(atg_pack_element(msgbox, sign))
						perror("atg_pack_element");
					else
					{
						atg_element *cont = atg_create_element_button("Continue", (atg_colour){31, 31, 31, ATG_ALPHA_OPAQUE}, (atg_colour){239, 239, 224, ATG_ALPHA_OPAQUE});
						if(!cont)
							fprintf(stderr, "atg_create_element_button failed\n");
						else
						{
							if(atg_pack_element(msgbox, cont))
								perror("atg_pack_element");
							else
							{
								canvas->box=msgbox;
								atg_flip(canvas);
								atg_event e;
								while(1)
								{
									if(atg_poll_event(&e, canvas))
									{
										if(e.type==ATG_EV_TRIGGER) break;
										if(e.type==ATG_EV_RAW)
										{
											SDL_Event s=e.event.raw;
											if(s.type==SDL_QUIT) break;
										}
									}
									else
										SDL_Delay(50);
									atg_flip(canvas);
								}
								canvas->box=oldbox;
							}
						}
					}
				}
			}
		}
	}
	atg_free_box_box(msgbox);
}

bool filter_apply(ac_bomber b, int filter_pff, int filter_nav[NNAVAIDS])
{
	if(filter_pff>0&&!b.pff) return(false);
	if(filter_pff<0&&b.pff) return(false);
	for(unsigned int n=0;n<NNAVAIDS;n++)
	{
		if(filter_nav[n]>0&&!b.nav[n]) return(false);
		if(filter_nav[n]<0&&b.nav[n]) return(false);
	}
	return(true);
}

void produce(int targ, game *state, double amount)
{
	if((targs[targ].iclass!=ICLASS_RAIL)&&(targs[targ].iclass!=ICLASS_MIXED))
		amount=min(amount, state->gprod[ICLASS_RAIL]*8.0);
	switch(targs[targ].iclass)
	{
		case ICLASS_BB:
		case ICLASS_RAIL:
		case ICLASS_STEEL:
		case ICLASS_OIL:
		case ICLASS_UBOOT:
		break;
		case ICLASS_ARM:
			amount=min(amount, state->gprod[ICLASS_STEEL]);
			state->gprod[ICLASS_STEEL]-=amount;
		break;
		case ICLASS_AC:
			amount=min(amount, state->gprod[ICLASS_BB]*2.0);
			state->gprod[ICLASS_BB]-=amount/2.0;
		break;
		case ICLASS_MIXED:
#define ADD(class, qty)	state->gprod[class]+=qty; state->dprod[class]+=qty;
			ADD(ICLASS_OIL, amount/7.0);
			ADD(ICLASS_RAIL, amount/21.0);
			ADD(ICLASS_ARM, amount/7.0);
			ADD(ICLASS_AC, amount/21.0);
			ADD(ICLASS_BB, amount/14.0);
#undef ADD
			return;
		default:
			fprintf(stderr, "Bad targs[%d].iclass = %d\n", targ, targs[targ].iclass);
		break;
	}
	if(targs[targ].iclass!=ICLASS_RAIL)
		state->gprod[ICLASS_RAIL]-=amount/8.0;
	state->gprod[targs[targ].iclass]+=amount;
	state->dprod[targs[targ].iclass]+=amount;
}
