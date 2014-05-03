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
#include <math.h>

#include <atg.h>
#include <SDL_image.h>

#include "bits.h"
#include "weather.h"
#include "rand.h"
#include "geom.h"
#include "widgets.h"
#include "events.h"
#include "globals.h"
#include "history.h"
#include "date.h"
#include "types.h"
#include "routing.h"
#include "saving.h"
#include "render.h"
#include "ui.h"

#include "main_menu.h"
#include "load_game.h"
#include "save_game.h"
#include "control.h"
#include "run_raid.h"

#include "version.h"

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

SDL_Surface *terrain=NULL;
SDL_Surface *location=NULL;
SDL_Surface *yellowhair=NULL;
SDL_Surface *intelbtn=NULL;
SDL_Surface *navpic[NNAVAIDS];
SDL_Surface *pffpic=NULL;
SDL_Surface *resizebtn=NULL;
SDL_Surface *fullbtn=NULL;
SDL_Surface *exitbtn=NULL;

SDL_Surface *grey_overlay=NULL, *yellow_overlay=NULL;
SDL_Surface *weather_overlay=NULL, *target_overlay=NULL, *flak_overlay=NULL, *xhair_overlay=NULL;

bool lorw[128][128]; // TRUE for water
unsigned char tnav[128][128]; // Recognisability of terrain.  High for rivers, even higher for coastline

unsigned int mainsizex=default_w, mainsizey=default_h;
bool fullscreen=false;

game state;

int msgadd(atg_canvas *canvas, game *state, date when, const char *ref, const char *msg);
bool version_newer(const unsigned char v1[3], const unsigned char v2[3]); // true iff v1 newer than v2
void produce(int targ, game *state, double amount);

int main(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[])
{
	int rc;
	atg_event e; /* TODO remove */
	
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
				// MANUFACTURER:NAME:COST:SPEED:ARMAMENT:MNV:RADPRI:DD-MM-YYYY:DD-MM-YYYY:FLAGS
				this.name=strdup(next); // guarantees that enough memory will be allocated
				this.manu=(char *)malloc(strcspn(next, ":")+1);
				ssize_t db;
				int e;
				if((e=sscanf(next, "%[^:]:%[^:]:%u:%u:%hhu:%hhu:%hhu:%zn", this.manu, this.name, &this.cost, &this.speed, &this.arm, &this.mnv, &this.radpri, &db))!=7)
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
				// NAME:PROD:FLAK:ESIZ:LAT:LONG:DD-MM-YYYY:DD-MM-YYYY:CLASS[,Flags][@City]
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
				this.city=-1;
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
					const char *at=strchr(class, '@');
					if(at)
					{
						at++;
						for(unsigned int i=0;i<ntargs;i++)
						{
							if(targs[i].class!=TCLASS_CITY) continue;
							if(strcmp(at, targs[i].name)==0)
							{
								this.city=i;
								break;
							}
						}
						if(this.city<0)
						{
							fprintf(stderr, "Unrecognised `targets' @City reference `%s'\n", at);
							fprintf(stderr, "  (note: City must precede target referring to it)\n");
							return(1);
						}
					}
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
				if(strstr(class, ",RADAR"))
					this.iclass=ICLASS_RADAR;
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
	
	grey_overlay=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 36, 40, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!grey_overlay)
	{
		fprintf(stderr, "Grey overlay: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(1);
	}
	SDL_FillRect(grey_overlay, &(SDL_Rect){.x=0, .y=0, .w=grey_overlay->w, .h=grey_overlay->h}, SDL_MapRGBA(grey_overlay->format, 127, 127, 127, 128));
	
	yellow_overlay=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 16, 16, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!yellow_overlay)
	{
		fprintf(stderr, "Yellow overlay: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(1);
	}
	SDL_FillRect(yellow_overlay, &(SDL_Rect){.x=0, .y=0, .w=yellow_overlay->w, .h=yellow_overlay->h}, SDL_MapRGBA(yellow_overlay->format, 255, 255, 0, 63));
	
	fprintf(stderr, "Data files loaded\n");
	
	fprintf(stderr, "Allocating game state...\n");
	
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
	
	fprintf(stderr, "Game state allocated\n");
	
	fprintf(stderr, "Instantiating GUI elements...\n");
	#define MAKE_SCRN(t)	(struct screen){.name=#t, .create=t##_create, .func=t##_screen, .free=t##_free, .box=t##_box}
	screens[SCRN_MAINMENU]=MAKE_SCRN(main_menu);
	screens[SCRN_LOADGAME]=MAKE_SCRN(load_game);
	screens[SCRN_SAVEGAME]=MAKE_SCRN(save_game);
	screens[SCRN_CONTROL] =MAKE_SCRN(control);
	screens[SCRN_RUNRAID] =MAKE_SCRN(run_raid);
	screens[SCRN_RRESULTS]=MAKE_SCRN(raid_results);
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
	
	/*main_menu:*/
	/*canvas->box=mainbox;*/
	
	/*loader:*/
	/*canvas->box=loadbox;*/
	
	
	/*saver:*/
	/*canvas->box=savebox;*/
	
	
	gameloop:
	/*canvas->box=gamebox;*/
	
	
	/*run_raid:*/
	if(totalraids)
	{
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
		int i=state.nap[n];
		if(i>=0&&!datewithin(state.now, types[i].entry, types[i].exit)) continue;
		unsigned int nac=0;
		for(int j=state.nbombers-1;(state.napb[n]>=navprod[n])&&(j>=0);j--)
		{
			if(i<0)
			{
				if(!types[state.bombers[j].type].nav[n]) continue;
			}
			else
			{
				if((int)state.bombers[j].type!=i)	continue;
			}
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
		for(int i=state.nbombers-1;i>=0;i--)
		{
			unsigned int type=state.bombers[i].type;
			types[type].count++;
			if(state.bombers[i].pff) types[type].pffcount++;
			if(types[type].pff&&types[type].noarm)
			{
				if(!state.bombers[i].pff)
				{
					state.bombers[i].pff=true;
					pf_append(&state.hist, state.now, (time){11, 40}, state.bombers[i].id, false, type);
				}
				continue;
			}
		}
		for(unsigned int j=0;j<ntypes;j++)
		{
			if(!types[j].pff) continue;
			if(types[j].noarm) continue;
			int pffneed=ceil(types[j].count*0.2)-types[j].pffcount;
			for(int i=state.nbombers-1;(pffneed>0)&&(i>=0);i--)
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
		if(targs[i].city<0)
		{
			if(!datebefore(state.now, targs[i].exit)) continue;
		}
		else
		{
			if(!datebefore(state.now, targs[targs[i].city].exit)) continue;
		}
		switch(targs[i].class)
		{
			case TCLASS_CITY:
			{
				state.flam[i]=(state.flam[i]*0.8)+8.0;
				double dflk=(targs[i].flak*state.dmg[i]*.0005)-(state.flk[i]*.05);
				state.flk[i]+=dflk;
				if(dflk)
					tfk_append(&state.hist, state.now, (time){11, 45}, i, dflk, state.flk[i]);
				double ddmg=min(state.dmg[i]*.005, 100-state.dmg[i]);
				state.dmg[i]+=ddmg;
				if(ddmg)
					tdm_append(&state.hist, state.now, (time){11, 45}, i, ddmg, state.dmg[i]);
				produce(i, &state, state.dmg[i]*targs[i].prod*0.8);
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
					double ddmg=min(state.dmg[i]*.05, 100-state.dmg[i]), cscale=targs[i].city<0?1.0:state.dmg[targs[i].city]/100.0;
					if(cscale==0)
						ddmg=100-state.dmg[i];
					state.dmg[i]+=ddmg;
					tdm_append(&state.hist, state.now, (time){11, 45}, i, ddmg, state.dmg[i]);
					produce(i, &state, state.dmg[i]*targs[i].prod*cscale/2.0);
					if(cscale==0)
						goto unflak;
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
	state.gprod[ICLASS_OIL]*=0.984;
	state.gprod[ICLASS_UBOOT]*=0.95; // not actually used for anything
	// German fighters
	memset(fcount, 0, sizeof(fcount));
	unsigned int maxradpri=0;
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
		if((!state.fighters[i].radar)&&(ftypes[type].radpri>maxradpri))
			maxradpri=ftypes[type].radpri;
		fcount[type]++;
		if(brandp(0.1))
		{
			unsigned int base;
			do
				base=state.fighters[i].base=irandu(nfbases);
			while(!datewithin(state.now, fbases[base].entry, fbases[base].exit));
		}
	}
	if(!datebefore(state.now, event[EVENT_L_BC]) && maxradpri)
	{
		unsigned int rcount=4;
		for(int i=state.nfighters-1;i>=0;i--)
		{
			if(state.gprod[ICLASS_RADAR]<5000 || !rcount) break;
			unsigned int type=state.fighters[i].type;
			if((!state.fighters[i].radar)&&(ftypes[type].radpri==maxradpri))
			{
				state.fighters[i].radar=true;
				na_append(&state.hist, state.now, (time){11, 49}, state.fighters[i].id, true, type, 0);
				state.gprod[ICLASS_RADAR]-=5000;
				rcount--;
			}
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
	
	/*do_exit:*/
	fprintf(stderr, "Exiting\n");
	canvas->box=NULL;
	atg_free_canvas(canvas);
	for(unsigned int i=0;i<NUM_SCREENS;i++)
		screens[i].free();
	SDL_FreeSurface(xhair_overlay);
	SDL_FreeSurface(weather_overlay);
	SDL_FreeSurface(target_overlay);
	SDL_FreeSurface(flak_overlay);
	SDL_FreeSurface(grey_overlay);
	SDL_FreeSurface(terrain);
	SDL_FreeSurface(location);
	return(0);
}

void update_navbtn(game state, atg_element *(*GB_navbtn)[NNAVAIDS], unsigned int i, unsigned int n, SDL_Surface *grey_overlay, SDL_Surface *yellow_overlay)
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
				if((state.nap[n]==(int)i) || (state.nap[n]<0))
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
		case ICLASS_RADAR:
			if(datebefore(state->now, event[EVENT_LICHTENSTEIN]))
				return;
		break;
		case ICLASS_MIXED:
#define ADD(class, qty)	state->gprod[class]+=qty; state->dprod[class]+=qty;
			ADD(ICLASS_OIL, amount/7.0);
			ADD(ICLASS_RAIL, amount/21.0);
			ADD(ICLASS_ARM, amount/7.0);
			ADD(ICLASS_AC, amount/21.0);
			ADD(ICLASS_BB, amount/14.0);
			ADD(ICLASS_STEEL, amount/35.0);
			if(!datebefore(state->now, event[EVENT_LICHTENSTEIN]))
				ADD(ICLASS_RADAR, amount/200.0);
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
