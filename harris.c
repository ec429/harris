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

#include "bits.h"
#include "weather.h"
#include "rand.h"
#include "geom.h"
#include "widgets.h"
#include "events.h"

//#define NOWEATHER	1	// skips weather phase of 'next-day' (w/o raid), allowing quicker time passage.  For testing/debugging

/* TODO
	Implement stats tracking
	Seasonal effects on weather
	Make Flak only be known to you after you've encountered it
	Implement later forms of Fighter control
	Better algorithm for building German fighters
	Implement the remaining Navaids (just Gee-H now)
	Implement Window & window-control (for diversions)
	Event effects on fighters (New radars, RCM, etc.)
	Refactoring, esp. of the GUI building, and splitting up this file (it's *far* too long right now)
	Sack player if confid or morale too low
*/

typedef struct
{
	int year;
	unsigned int month; // 1-based
	unsigned int day; // also 1-based
}
date;

const unsigned int monthdays[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#define NAV_GEE		0
#define NAV_H2S		1
#define NAV_OBOE	2
#define NAV_GH		3
#define NNAVAIDS	4
const char * const navaids[NNAVAIDS]={"GEE","H2S","OBOE","GH"};
const char * const navpicfn[NNAVAIDS]={"art/navaids/gee.png", "art/navaids/h2s.png", "art/navaids/oboe.png", "art/navaids/g-h.png"};
unsigned int navevent[NNAVAIDS]={EVENT_GEE, EVENT_H2S, EVENT_OBOE, EVENT_GH};
unsigned int navprod[NNAVAIDS]={5, 9, 22, 12}; // 10/productionrate

typedef struct
{
	//MANUFACTURER:NAME:COST:SPEED:CEILING:CAPACITY:SVP:DEFENCE:FAILURE:ACCURACY:RANGE:BLAT:BLONG:DD-MM-YYYY:DD-MM-YYYY:NAVAIDS,FLAGS
	char * manu;
	char * name;
	unsigned int cost;
	unsigned int speed;
	unsigned int alt;
	unsigned int cap;
	unsigned int svp;
	unsigned int defn;
	unsigned int fail;
	unsigned int accu;
	unsigned int range;
	date entry;
	date exit;
	bool nav[NNAVAIDS];
	bool noarm, pff;
	unsigned int blat, blon;
	SDL_Surface *picture;
	char *text, *newtext;
	
	unsigned int count;
	unsigned int navcount[NNAVAIDS];
	unsigned int pffcount;
	unsigned int prio;
	unsigned int pribuf;
	atg_element *prio_selector;
	unsigned int pc;
	unsigned int pcbuf;
}
bombertype;

typedef struct
{
	//MANUFACTURER:NAME:COST:SPEED:ARMAMENT:MNV:DD-MM-YYYY:DD-MM-YYYY:FLAGS
	char *manu;
	char *name;
	unsigned int cost;
	unsigned int speed;
	unsigned char arm;
	unsigned char mnv;
	date entry;
	date exit;
	bool night;
	char *text, *newtext;
}
fightertype;

typedef struct
{
	//LAT:LONG:ENTRY:EXIT
	unsigned int lat, lon;
	date entry, exit;
}
ftrbase;

typedef struct
{
	//NAME:PROD:FLAK:ESIZ:LAT:LONG:DD-MM-YYYY:DD-MM-YYYY:CLASS
	char * name;
	unsigned int prod, flak, esiz, lat, lon;
	date entry, exit;
	enum {TCLASS_CITY,TCLASS_SHIPPING,TCLASS_MINING,TCLASS_LEAFLET,TCLASS_AIRFIELD,TCLASS_BRIDGE,TCLASS_ROAD,TCLASS_INDUSTRY,} class;
	bool bb; // contains aircraft or ballbearing works (Pointblank Directive)
	bool oil; // industry-type OIL refineries
	bool rail; // industry-type RAIL yards
	bool uboot; // industry-type UBOOT factories
	bool arm; // industry-type ARMament production (incl. steel etc.)
	bool berlin; // raids on Berlin are more valuable
	SDL_Surface *picture;
	unsigned int psiz; // 'physical' size (cities only) - #pixels in picture
	/* for Type I fighter control */
	double threat; // German-assessed threat level
	unsigned int nfighters; // # of fighters covering this target
	/* for bomber guidance */
	unsigned int route[8][2]; // [0123 out, 4 bmb, 567 in][0 lat, 1 lon]. {0, 0} indicates "not used, skip to next routestage"
	double fires; // level of fires (and TIs) illuminating the target
}
target;

typedef struct
{
	//STRENGTH:LAT:LONG:ENTRY:RADAR:EXIT
	unsigned int strength, lat, lon;
	date entry, radar, exit;
	/* for Himmelbett fighter control */
	signed int ftr; // index of fighter under this radar's control (-1 for none)
}
flaksite;

date event[NEVENTS];
char *evtext[NEVENTS];

typedef struct
{
	unsigned int type;
	unsigned int targ;
	double lat, lon;
	double navlat, navlon; // error in "believed position" relative to true position
	double driftlat, driftlon; // rate of error
	unsigned int bmblat, bmblon; // true position where bombs were dropped (if any)
	unsigned int route[8][2]; // [0123 out, 4 bmb, 567 in][0 lat, 1 lon]. {0, 0} indicates "not used, skip to next routestage"
	unsigned int routestage; // 8 means "passed route[7], heading for base"
	bool nav[NNAVAIDS];
	bool pff; // a/c is assigned to the PFF
	unsigned int bmb; // bombload carried.  0 means leaflets
	bool bombed;
	bool crashed;
	bool failed; // for forces, read as !svble
	bool landed; // for forces, read as !assigned
	double speed;
	double damage; // increases the probability of mech.fail and of consequent crashes
	double df; // damage (by) fighters
	bool idtar; // identified target?  (for use if RoE require it)
	unsigned int startt; // take-off time
	unsigned int fuelt; // when t (ticks) exceeds this value, turn for home
}
ac_bomber;

typedef struct
{
	unsigned int type;
	unsigned int base;
	double lat, lon;
	bool crashed;
	bool landed;
	double damage;
	unsigned int fuelt;
	signed int k; // which bomber this fighter is attacking (-1 for none)
	signed int targ; // which target this fighter is covering (-1 for none)
	signed int hflak; // which flaksite's radar is controlling this fighter? (Himmelbett/Kammhuber line) (-1 for none)
}
ac_fighter;

#define ftr_free(f)	(((f).targ<0)&&((f).hflak<0)&&((f).fuelt>t)) // f is of type ac_fighter

typedef struct
{
	unsigned int nbombers;
	unsigned int *bombers; // offsets into the game.bombers list
}
raid;

#define MAXMSGS	8

typedef struct
{
	date now;
	unsigned int cash, cshr;
	double confid, morale;
	unsigned int nbombers;
	ac_bomber *bombers;
	unsigned int nap[NNAVAIDS];
	unsigned int napb[NNAVAIDS];
	w_state weather;
	unsigned int ntargs;
	double *dmg, *flk, *heat;
	double gprod;
	unsigned int nfighters;
	ac_fighter *fighters;
	raid *raids;
	struct
	{
		bool idtar;
	}
	roe;
	char *msg[MAXMSGS];
}
game;

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

#define GAME_BG_COLOUR	(atg_colour){31, 31, 15, ATG_ALPHA_OPAQUE}

#define RS_cell_w		112
#define RS_firstcol_w	128
#define RS_cell_h		56
#define RS_lastrow_h	100

#define VER_MAJ	0
#define VER_MIN	1
#define VER_REV	0

int loadgame(const char *fn, game *state, bool lorw[128][128]);
int savegame(const char *fn, game state);
int msgadd(game *state, date when, const char *ref, const char *msg);
void message_box(atg_canvas *canvas, const char *titletext, const char *bodytext, const char *signtext);
date readdate(const char *t, date nulldate);
bool datebefore(date date1, date date2); // returns true if date1 is strictly before date2
#define datewithin(now, start, end)		((!datebefore((now), (start)))&&datebefore((now), (end)))
int diffdate(date date1, date date2); // returns <0 if date1<date2, >0 if date1>date2, 0 if date1==date2
double pom(date when); // returns in [0,1); 0 for new moon, 0.5 for full moon
double foldpom(double pom); // returns illumination in [0,1]
void drawmoon(SDL_Surface *s, double phase);
bool version_newer(const unsigned char v1[3], const unsigned char v2[3]); // true iff v1 newer than v2
SDL_Surface *render_weather(w_state weather);
SDL_Surface *render_targets(date now);
SDL_Surface *render_flak(date now);
SDL_Surface *render_ac(game state);
int pset(SDL_Surface *s, unsigned int x, unsigned int y, atg_colour c);
int line(SDL_Surface *s, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, atg_colour c);
atg_colour pget(SDL_Surface *s, unsigned int x, unsigned int y);
unsigned int ntypes=0;
void update_navbtn(game state, atg_element *GB_navbtn[ntypes][NNAVAIDS], unsigned int i, unsigned int n, SDL_Surface *grey_overlay, SDL_Surface *yellow_overlay);
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
SDL_Surface *navpic[NNAVAIDS];

int main(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[])
{
	atg_canvas *canvas=atg_create_canvas(120, 24, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
	if(!canvas)
	{
		fprintf(stderr, "atg_create_canvas failed\n");
		return(1);
	}
	SDL_WM_SetCaption("Harris", "Harris");
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	
	srand(0); // predictable seed for creating 'random' target maps
	
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
				ftypes=realloc(ftypes, (nftypes+1)*sizeof(fightertype));
				ftypes[nftypes]=this;
				nftypes++;
			}
			next=strtok(NULL, "\n");
		}
		free(ftypefile);
		fprintf(stderr, "Loaded %u fighter types\n", nftypes);
	}
	
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
				this.bb=strstr(class, ",BB")||strstr(class, ",AC");
				this.oil=strstr(class, ",OIL");
				this.rail=strstr(class, ",RAIL");
				this.uboot=strstr(class, ",UBOOT");
				this.arm=strstr(class, ",ARM");
				this.berlin=strstr(class, ",BERLIN");
				targs=(target *)realloc(targs, (ntargs+1)*sizeof(target));
				targs[ntargs]=this;
				ntargs++;
			}
			next=strtok(NULL, "\n");
		}
		free(targfile);
		fprintf(stderr, "Loaded %u targets\n", ntargs);
		unsigned int citymap[256][256];
		memset(citymap, 0xff, sizeof(citymap)); // XXX non-portable way to set all elems to UINT_MAX
		for(unsigned int t=0;t<ntargs;t++)
		{
			if(targs[t].class==TCLASS_CITY)
				citymap[targs[t].lon][targs[t].lat]=t;
		}
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
					int sz=((int)((targs[t].esiz+1)/3))|1, hs=sz>>1;
					if(!(targs[t].picture=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, sz, sz, 32, 0xff000000, 0xff0000, 0xff00, 0xff)))
					{
						fprintf(stderr, "targs[%u].picture: SDL_CreateRGBSurface: %s\n", t, SDL_GetError());
						return(1);
					}
					SDL_SetAlpha(targs[t].picture, 0, 0);
					SDL_FillRect(targs[t].picture, &(SDL_Rect){.x=0, .y=0, .w=targs[t].picture->w, .h=targs[t].picture->h}, ATG_ALPHA_TRANSPARENT&0xff);
					for(unsigned int k=0;k<6;k++)
					{
						int x=hs, y=hs;
						for(unsigned int i=0;i<targs[t].esiz>>1;i++)
						{
							int gx=x+targs[t].lon, gy=y+targs[t].lat;
							if((gx>=0)&&(gx<256)&&(gy>=0)&&(gy<256))
							{
								if((citymap[gx][gy]<ntargs)&&(citymap[gx][gy]!=t)) {x=y=hs;i--;}
								gx=x+targs[t].lon;
								gy=y+targs[t].lat;
							}
							pset(targs[t].picture, x, y, (atg_colour){.r=7, .g=7, .b=7, .a=ATG_ALPHA_OPAQUE});
							if((gx>=0)&&(gx<256)&&(gy>=0)&&(gy<256))
								citymap[gx][gy]=t;
							unsigned int j=rand()&3;
							if(j&1) y+=j-2;
							else x+=j-1;
							if((x<0)||(x>=sz)||(y<0)||(y>=sz)) x=y=hs;
						}
					}
					targs[t].psiz=0;
					for(int x=0;x<sz;x++)
						for(int y=0;y<sz;y++)
							if(pget(targs[t].picture, x, y).a==ATG_ALPHA_OPAQUE)
								targs[t].psiz++;
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
		fprintf(stderr, "Generated target images\n");
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
	for(unsigned int n=0;n<NNAVAIDS;n++)
	{
		if(!(navpic[n]=IMG_Load(navpicfn[n])))
		{
			fprintf(stderr, "Navaid icon %u: IMG_Load: %s\n", n, IMG_GetError());
			return(1);
		}
		SDL_SetAlpha(navpic[n], 0, SDL_ALPHA_OPAQUE);
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
	
	fprintf(stderr, "Instantiating GUI elements...\n");
	
	atg_box *mainbox=canvas->box;
	atg_element *MainMenu=atg_create_element_label("HARRIS: Main Menu", 12, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
	if(!MainMenu)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_pack_element(mainbox, MainMenu))
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
	atg_element *GB_btrow[ntypes], *GB_btpic[ntypes], *GB_btdesc[ntypes], *GB_btnum[ntypes], *GB_btpc[ntypes], *GB_btp[ntypes], *GB_navrow[ntypes], *GB_navbtn[ntypes][NNAVAIDS], *GB_navgraph[ntypes][NNAVAIDS];
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
	SDL_Surface *weather_overlay=NULL, *target_overlay=NULL, *flak_overlay=NULL;
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
	atg_element *GB_rbrow[ntargs][ntypes], *GB_raidnum[ntargs][ntypes];
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
			GB_rbrow[i][j]->clickable=true;
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
					name->w=216;
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
	atg_element *GB_ttrow[ntargs], *GB_ttdmg[ntargs], *GB_ttflk[ntargs];
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
		atg_element *item=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
		if(!item)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
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
	LB_file->h=610;
	LB_file->w=800;
	if(atg_pack_element(loadbox, LB_file))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *LB_hbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE}), *LB_text=NULL, *LB_load=NULL;
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
		LB_text->w=760;
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
	SA_file->h=610;
	SA_file->w=800;
	if(atg_pack_element(savebox, SA_file))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *SA_hbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE}), *SA_text=NULL, *SA_save=NULL;
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
		SA_text->w=760;
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
	}
	
	atg_box *rstatbox=atg_create_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
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
		atg_element *RS_cont=atg_create_element_button("Continue", (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE}, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
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
	}
	state.roe.idtar=true;
	for(unsigned int i=0;i<MAXMSGS;i++)
		state.msg[i]=NULL;
	
	main_menu:
	canvas->box=mainbox;
	atg_resize_canvas(canvas, 120, 86);
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
	atg_resize_canvas(canvas, 800, 640);
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
							goto main_menu;
						break;
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
						else
						{
							char *file=malloc(strlen(f->curdir)+strlen(f->value)+1);
							sprintf(file, "%s%s", f->curdir, f->value);
							fprintf(stderr, "Loading game state from '%s'...\n", file);
							if(!loadgame(file, &state, lorw))
							{
								fprintf(stderr, "Game loaded\n");
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
	atg_resize_canvas(canvas, 800, 640);
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
							goto gameloop;
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
						else if(!f->value)
						{
							fprintf(stderr, "Error: no filename\n");
						}
						else if(!f->curdir)
						{
							fprintf(stderr, "Error: f->curdir==NULL\n");
						}
						else
						{
							char *file=malloc(strlen(f->curdir)+strlen(f->value)+1);
							sprintf(file, "%s%s", f->curdir, f->value);
							fprintf(stderr, "Saving game state to '%s'...\n", file);
							if(!savegame(file, state))
							{
								fprintf(stderr, "Game saved\n");
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
	atg_resize_canvas(canvas, 800, 640);
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
		state.bombers[j].df=0;
	}
	bool shownav=false;
	for(unsigned int n=0;n<NNAVAIDS;n++)
		if(!datebefore(state.now, event[navevent[n]])) shownav=true;
	for(unsigned int i=0;i<ntypes;i++)
	{
		if(GB_btrow[i])
			GB_btrow[i]->hidden=!datewithin(state.now, types[i].entry, types[i].exit);
		if(GB_btpc[i])
			GB_btpc[i]->h=18-min(types[i].pcbuf/10000, 18);
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
							goto main_menu;
						break;
					}
				break;
				case ATG_EV_CLICK:;
					atg_ev_click c=e.event.click;
					atg_mousebutton b=c.button;
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
											case ATG_MB_SCROLLUP:
											default:
												amount=10;
										}
										for(unsigned int j=0;j<state.nbombers;j++)
										{
											if(state.bombers[j].type!=i) continue;
											if(state.bombers[j].failed) continue;
											if(!state.bombers[j].landed) continue;
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
							if(c.e==GB_btdesc[i])
							{
								message_box(canvas, "From the Bomber Command files:", types[i].text, "R.H.M.S. Saundby, SASO");
							}
						}
						if(c.e==GB_ttl)
						{
							seltarg=-1;
							SDL_FreeSurface(GB_map->elem.image->data);
							GB_map->elem.image->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
							SDL_BlitSurface(flak_overlay, NULL, GB_map->elem.image->data, NULL);
							SDL_BlitSurface(target_overlay, NULL, GB_map->elem.image->data, NULL);
							SDL_BlitSurface(weather_overlay, NULL, GB_map->elem.image->data, NULL);
							free(GB_raid_label->elem.label->text);
							GB_raid_label->elem.label->text=strdup("Select a Target");
							GB_raid->elem.box=GB_raidbox_empty;
						}
						for(unsigned int i=0;i<ntargs;i++)
						{
							if(!datewithin(state.now, targs[i].entry, targs[i].exit)) continue;
							if(c.e==GB_ttrow[i])
							{
								seltarg=i;
								SDL_FreeSurface(GB_map->elem.image->data);
								GB_map->elem.image->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
								SDL_BlitSurface(flak_overlay, NULL, GB_map->elem.image->data, NULL);
								SDL_BlitSurface(target_overlay, NULL, GB_map->elem.image->data, NULL);
								SDL_BlitSurface(weather_overlay, NULL, GB_map->elem.image->data, NULL);
								SDL_BlitSurface(location, NULL, GB_map->elem.image->data, &(SDL_Rect){.x=targs[i].lon-3, .y=targs[i].lat-3});
								free(GB_raid_label->elem.label->text);
								size_t ll=9+strlen(targs[i].name);
								GB_raid_label->elem.label->text=malloc(ll);
								snprintf(GB_raid_label->elem.label->text, ll, "Raid on %s", targs[i].name);
								GB_raid->elem.box=GB_raidbox[i];
							}
							for(unsigned int j=0;j<ntypes;j++)
							{
								if(c.e==GB_rbrow[i][j])
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
										case ATG_MB_SCROLLUP:
										default:
											amount=10;
									}
									for(unsigned int l=0;l<state.raids[i].nbombers;l++)
									{
										unsigned int k=state.raids[i].bombers[l];
										if(state.bombers[k].type!=j) continue;
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
		if(seltarg>=0)
			SDL_BlitSurface(location, NULL, GB_map->elem.image->data, &(SDL_Rect){.x=targs[seltarg].lon, .y=targs[seltarg].lat});
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
		targs[i].threat=0;
		targs[i].nfighters=0;
		targs[i].fires=0;
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
		atg_resize_canvas(canvas, 800, 640);
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
		unsigned int it=0, startt=768;
		unsigned int plan[60], act[60][2];
		double fire[120];
		for(unsigned int dt=0;dt<60;dt++)
			plan[dt]=act[dt][0]=act[dt][1]=0;
		for(unsigned int dt=0;dt<120;dt++)
			fire[dt]=0;
		for(unsigned int i=0;i<ntargs;i++)
		{
			if(state.raids[i].nbombers)
			{
				targs[i].route[4][0]=targs[i].lat;
				targs[i].route[4][1]=targs[i].lon;
				for(unsigned int l=0;l<4;l++)
				{
					targs[i].route[l][0]=((67*(4-l)+targs[i].lat*(3+l))/7)+irandu(23)-11;
					targs[i].route[l][1]=((69*(4-l)+targs[i].lon*(3+l))/7)+irandu(9)-4;
				}
				for(unsigned int l=5;l<8;l++)
				{
					targs[i].route[l][0]=((118*(l-4)+targs[i].lat*(10-l))/6)+irandu(23)-11;
					targs[i].route[l][1]=((64*(l-4)+targs[i].lon*(10-l))/6)+irandu(11)-5;
				}
				for(unsigned int l=0;l<7;l++)
				{
					double z=1;
					while(true)
					{
						double scare[2]={0,0};
						for(unsigned int t=0;t<ntargs;t++)
						{
							if(t==i) continue;
							if(!datewithin(state.now, targs[t].entry, targs[t].exit)) continue;
							double d, lambda;
							linedist(targs[i].route[l+1][1]-targs[i].route[l][1],
								targs[i].route[l+1][0]-targs[i].route[l][0],
								targs[t].lon-targs[i].route[l][1],
								targs[t].lat-targs[i].route[l][0],
								&d, &lambda);
							if(d<6)
							{
								double s=targs[t].flak*0.18/(3.0+d);
								scare[0]+=s*(1-lambda);
								scare[1]+=s*lambda;
							}
						}
						for(unsigned int f=0;f<nflaks;f++)
						{
							if(!datewithin(state.now, flaks[f].entry, flaks[f].exit)) continue;
							double d, lambda;
							linedist(targs[i].route[l+1][1]-targs[i].route[l][1],
								targs[i].route[l+1][0]-targs[i].route[l][0],
								flaks[f].lon-targs[i].route[l][1],
								flaks[f].lat-targs[i].route[l][0],
								&d, &lambda);
							if(d<2)
							{
								double s=flaks[f].strength*0.01/(1.0+d);
								scare[0]+=s*(1-lambda);
								scare[1]+=s*lambda;
							}
						}
						z*=(scare[0]+scare[1])/(double)(1+scare[0]+scare[1]);
						double fs=scare[0]/(scare[0]+scare[1]);
						if(z<0.2) break;
						if(l!=4)
						{
							targs[i].route[l][0]+=z*fs*(irandu(43)-21);
							targs[i].route[l][1]+=z*fs*(irandu(21)-10);
						}
						if(l!=3)
						{
							targs[i].route[l+1][0]+=z*(1-fs)*(irandu(43)-21);
							targs[i].route[l+1][1]+=z*(1-fs)*(irandu(21)-10);
						}
					}
				}
			}
			for(unsigned int j=0;j<state.raids[i].nbombers;j++)
			{
				unsigned int k=state.raids[i].bombers[j], type=state.bombers[k].type;
				state.bombers[k].targ=i;
				state.bombers[k].lat=types[type].blat;
				state.bombers[k].lon=types[type].blon;
				state.bombers[k].routestage=0;
				if(datebefore(state.now, event[EVENT_GEE]))
				{
					state.bombers[k].route[4][0]=targs[i].lat;
					state.bombers[k].route[4][1]=targs[i].lon;
					for(unsigned int l=0;l<4;l++)
					{
						state.bombers[k].route[l][0]=((types[type].blat*(4-l)+targs[i].lat*(3+l))/7)+irandu(23)-11;
						state.bombers[k].route[l][1]=((types[type].blon*(4-l)+targs[i].lon*(3+l))/7)+irandu(9)-4;
					}
					for(unsigned int l=5;l<8;l++)
					{
						state.bombers[k].route[l][0]=((types[type].blat*(l-4)+targs[i].lat*(10-l))/6)+irandu(23)-11;
						state.bombers[k].route[l][1]=((types[type].blon*(l-4)+targs[i].lon*(10-l))/6)+irandu(11)-5;
					}
					for(unsigned int l=0;l<7;l++)
					{
						double z=1;
						while(true)
						{
							double scare[2]={0,0};
							for(unsigned int t=0;t<ntargs;t++)
							{
								if(t==i) continue;
								if(!datewithin(state.now, targs[t].entry, targs[t].exit)) continue;
								double d, lambda;
								linedist(state.bombers[k].route[l+1][1]-state.bombers[k].route[l][1],
									state.bombers[k].route[l+1][0]-state.bombers[k].route[l][0],
									targs[t].lon-state.bombers[k].route[l][1],
									targs[t].lat-state.bombers[k].route[l][0],
									&d, &lambda);
								if(d<6)
								{
									double s=targs[t].flak*0.18/(3.0+d);
									scare[0]+=s*(1-lambda);
									scare[1]+=s*lambda;
								}
							}
							for(unsigned int f=0;f<nflaks;f++)
							{
								if(!datewithin(state.now, flaks[f].entry, flaks[f].exit)) continue;
								double d, lambda;
								linedist(state.bombers[k].route[l+1][1]-state.bombers[k].route[l][1],
									state.bombers[k].route[l+1][0]-state.bombers[k].route[l][0],
									flaks[f].lon-state.bombers[k].route[l][1],
									flaks[f].lat-state.bombers[k].route[l][0],
									&d, &lambda);
								if(d<2)
								{
									double s=flaks[f].strength*0.01/(1.0+d);
									scare[0]+=s*(1-lambda);
									scare[1]+=s*lambda;
								}
							}
							z*=(scare[0]+scare[1])/(double)(1+scare[0]+scare[1]);
							double fs=scare[0]/(scare[0]+scare[1]);
							if(z<0.2) break;
							if(l!=4)
							{
								state.bombers[k].route[l][0]+=z*fs*(irandu(43)-21);
								state.bombers[k].route[l][1]+=z*fs*(irandu(21)-10);
							}
							if(l!=3)
							{
								state.bombers[k].route[l+1][0]+=z*(1-fs)*(irandu(43)-21);
								state.bombers[k].route[l+1][1]+=z*(1-fs)*(irandu(21)-10);
							}
						}
					}
				}
				else
				{
					for(unsigned int l=0;l<8;l++)
					{
						state.bombers[k].route[l][0]=targs[i].route[l][0];
						state.bombers[k].route[l][1]=targs[i].route[l][1];
					}
				}
				double dist=hypot((signed)types[type].blat-(signed)state.bombers[k].route[0][0], (signed)types[type].blon-(signed)state.bombers[k].route[0][1]), outward=dist;
				for(unsigned int l=0;l<7;l++)
				{
					double d=hypot((signed)state.bombers[k].route[l+1][0]-(signed)state.bombers[k].route[l][0], (signed)state.bombers[k].route[l+1][1]-(signed)state.bombers[k].route[l][1]);
					dist+=d;
					if(l<4) outward+=d;
				}
				if(targs[i].class==TCLASS_LEAFLET)
					state.bombers[k].bmb=0;
				else
					state.bombers[k].bmb=types[type].cap;
				if(state.bombers[k].pff) state.bombers[k].bmb*=.75;
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
				state.bombers[k].speed=(types[type].speed+irandu(4)-2)/300.0;
				if(datebefore(state.now, event[EVENT_GEE]))
					state.bombers[k].startt=irandu(90);
				else
				{
					// aim for Zero Hour 01:00 plus up to 10 minutes
					// PFF should arrive at Zero minus 6, and be finished by Zero minus 2
					// Zero Hour is t=480, and a minute is two t-steps
					int tt=state.bombers[k].pff?(468+irandu(8)):(480+irandu(20));
					int st=tt-(outward/state.bombers[k].speed)-3;
					if(state.bombers[k].pff) st-=2;
					if(st<0)
					{
						tt-=st;
						st=0;
					}
					state.bombers[k].startt=st;
					if((tt>=450)&&(tt<570)&&(targs[i].class==TCLASS_CITY))
						plan[(tt-450)/2]++;
				}
				startt=min(startt, state.bombers[k].startt);
				state.bombers[k].fuelt=state.bombers[k].startt+types[type].range*0.6/(double)state.bombers[k].speed;
				unsigned int eta=state.bombers[k].startt+outward*1.2/(double)state.bombers[k].speed;
				if(datebefore(state.now, event[EVENT_GEE])) eta+=24;
				if(eta>state.bombers[k].fuelt)
				{
					unsigned int fu=eta-state.bombers[k].fuelt;
					state.bombers[k].fuelt+=fu;
					state.bombers[k].bmb*=120/(120.0+fu);
				}
			}
		}
		oboe.k=-1;
		SDL_Surface *with_ac=SDL_ConvertSurface(with_weather, with_weather->format, with_weather->flags);
		SDL_Surface *ac_overlay=render_ac(state);
		SDL_BlitSurface(ac_overlay, NULL, with_ac, NULL);
		SDL_BlitSurface(with_ac, NULL, RB_map->elem.image->data, NULL);
		unsigned int inair=totalraids, t=0;
		double kills[2]={0, 0};
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
		bool stream=!datebefore(state.now, event[EVENT_GEE]);
		while(inair)
		{
			if(RB_time_label) snprintf(RB_time_label, 6, "%02u:%02u", (21+(t/120))%24, (t/2)%60);
			t++;
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
				}
			}
			for(unsigned int i=0;i<ntargs;i++)
				for(unsigned int j=0;j<state.raids[i].nbombers;j++)
				{
					unsigned int k=state.raids[i].bombers[j], type=state.bombers[k].type;
					if(t<state.bombers[k].startt) continue;
					if(state.bombers[k].crashed||state.bombers[k].landed) continue;
					if(state.bombers[k].damage>=100)
					{
						state.bombers[k].crashed=true;
						if(state.bombers[k].damage)
						{
							double kf=state.bombers[k].df/state.bombers[k].damage;
							kills[0]+=(1-kf);
							kills[1]+=kf;
						}
						inair--;
						continue;
					}
					if(brandp((types[type].fail+2.0*state.bombers[k].damage)/4000.0))
					{
						state.bombers[k].failed=true;
						if(brandp((1.0+state.bombers[k].damage/50.0)/200.0))
						{
							state.bombers[k].crashed=true;
							if(state.bombers[k].damage)
							{
								double kf=state.bombers[k].df/state.bombers[k].damage;
								kills[0]+=(1-kf);
								kills[1]+=kf;
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
							state.bombers[k].navlon=state.bombers[k].navlat=0;
						// the rest of these involve taking priority over an existing user and so they grab the k but don't get to use this turn
						if(!state.bombers[oboe.k].pff) // PFF priority and priority for bigger loads
						{
							if(state.bombers[k].pff||(state.bombers[k].bmb>state.bombers[oboe.k].bmb))
								oboe.k=k;
						}
					}
					else if(oboe.k==(int)k)
						oboe.k=-1;
					int destx=home?types[type].blon:state.bombers[k].route[stage][1],
						desty=home?types[type].blat:state.bombers[k].route[stage][0];
					double cx=destx-(state.bombers[k].lon+state.bombers[k].navlon), cy=desty-(state.bombers[k].lat+state.bombers[k].navlat);
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
						else if(t>768)
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
						bool leaf=!state.bombers[k].bmb;
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
							state.bombers[k].bombed=true;
							if(!leaf)
							{
								for(unsigned int ta=0;ta<ntargs;ta++)
								{
									if(targs[ta].class!=TCLASS_CITY) continue;
									int dx=floor(state.bombers[k].bmblon+.5)-targs[ta].lon, dy=floor(state.bombers[k].bmblat+.5)-targs[ta].lat;
									int hx=targs[ta].picture->w/2;
									int hy=targs[ta].picture->h/2;
									if((abs(dx)<=hx)&&(abs(dy)<=hy)&&(pget(targs[ta].picture, dx+hx, dy+hy).a==ATG_ALPHA_OPAQUE))
									{
										int dt=(t-450)/2;
										if((dt>=0)&&(dt<60))
											act[dt][state.bombers[k].pff?0:1]++;
										if(state.bombers[k].pff)
											targs[ta].fires+=30;
										else
											targs[ta].fires+=state.bombers[k].bmb/6000;
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
							if(brandp(state.flk[i]/200.0))
							{
								if(brandp(types[type].defn/200.0))
									state.bombers[k].damage+=100;
								else
									state.bombers[k].damage+=irandu(types[type].defn)/2.5;
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
						if(xyr(state.bombers[k].lon-flaks[i].lon, state.bombers[k].lat-flaks[i].lat, 3.0))
						{
							if(brandp(flaks[i].strength*(rad?3:1)/900.0))
								state.bombers[k].damage+=irandu(types[type].defn)/5.0;
						}
						if(brandp(0.1))
						{
							if(rad&&(xyr(state.bombers[k].lon-flaks[i].lon, state.bombers[k].lat-flaks[i].lat, 12))) // 36 miles range for Würzburg radar (Wikipedia gives range as "up to 43mi")
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
					for(unsigned int j=0;j<state.nfighters;j++)
					{
						if(state.fighters[j].landed) continue;
						if(state.fighters[j].k>=0) continue;
						if(state.fighters[j].hflak>=0) continue;
						unsigned int type=state.fighters[j].type;
						bool airad=ftypes[type].night&&!datebefore(state.now, event[EVENT_L_BC]);
						unsigned int x=state.fighters[j].lon/2, y=state.fighters[j].lat/2;
						double wea=((x<128)&&(y<128))?state.weather.p[x][y]-1000:0;
						double seerange=airad?1.5:(3.0*(.8*moonillum+.6)/(double)(8+max(4-wea, 0)));
						if(xyr(state.bombers[k].lon-state.fighters[j].lon, state.bombers[k].lat-state.fighters[j].lat, seerange))
						{
							double findp=airad?0.16:0.6/(double)(8+max(4-wea, 0));
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
					if(b)
					{
						unsigned char h=((x<128)&&(y<128))?tnav[x][y]:0;
						double cf=270.0-h*0.3;
						state.bombers[k].navlon*=(cf+state.bombers[k].lon)/512.0;
						state.bombers[k].navlat*=(cf+state.bombers[k].lon)/512.0;
						state.bombers[k].driftlon*=(cf+state.bombers[k].lon)/512.0;
						state.bombers[k].driftlat*=(cf+state.bombers[k].lon)/512.0;
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
					bool c=brandp(targs[dtm].fires/2e3);
					if(b||c)
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
						switch(targs[i].class)
						{
							case TCLASS_CITY:
							case TCLASS_LEAFLET:
							case TCLASS_SHIPPING:
							case TCLASS_AIRFIELD:
							case TCLASS_ROAD:
							case TCLASS_BRIDGE:
							case TCLASS_INDUSTRY:
								if(pget(target_overlay, state.bombers[k].lon, state.bombers[k].lat).a==ATG_ALPHA_OPAQUE)
								{
									state.bombers[k].idtar=true;
								}
								else if(c&&(targs[dm].class==TCLASS_CITY)&&(targs[dtm].class==TCLASS_CITY))
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
				}
			for(unsigned int j=0;j<state.nfighters;j++)
			{
				if(state.fighters[j].landed)
					continue;
				if(state.fighters[j].crashed) continue;
				if((state.fighters[j].damage>=100)||brandp(state.fighters[j].damage/4000.0))
				{
					state.fighters[j].crashed=true;
					int t=state.fighters[j].targ;
					if(t>=0)
						targs[t].nfighters--;
					int f=state.fighters[j].hflak;
					if(f>=0)
						flaks[f].ftr=-1;
					continue;
				}
				if(t>state.fighters[j].fuelt)
				{
					int t=state.fighters[j].targ;
					int f=state.fighters[j].hflak;
					if(t>=0)
						targs[t].nfighters--;
					if(f>=0)
						flaks[f].ftr=-1;
					if((t>=0)||(f>=0))
						fightersleft++;
					state.fighters[j].targ=-1;
					state.fighters[j].hflak=-1;
					state.fighters[j].k=-1;
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
					if(!datebefore(state.now, event[EVENT_WINDOW])) radcon=false;
					bool airad=ftypes[ft].night&&!datebefore(state.now, event[EVENT_L_BC]);
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
							double spd=ftypes[ft].speed/300.0;
							double cx=(tx-x)/d,
								cy=(ty-y)/d;
							state.fighters[j].lon+=cx*spd;
							state.fighters[j].lat+=cy*spd;
						}
						if(d<(radcon?0.6:0.38)*(.8*moonillum+.6))
						{
							if(brandp(ftypes[ft].mnv/100.0))
							{
								unsigned int dmg=irandu(ftypes[ft].arm)*types[bt].defn/20.0;
								state.bombers[k].damage+=dmg;
								state.bombers[k].df+=dmg;
								//fprintf(stderr, "F%u hit B%u for %u (%g)\n", ft, bt, dmg, state.bombers[k].damage);
							}
							if(!types[bt].noarm&&(brandp(1.0/types[bt].defn)))
							{
								unsigned int dmg=irandu(20);
								state.fighters[j].damage+=dmg;
								//fprintf(stderr, "B%u hit F%u for %u (%g)\n", bt, ft, dmg, state.fighters[j].damage);
							}
							if(brandp(0.6))
								state.fighters[j].k=-1;
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
					double spd=ftypes[type].speed/300.0;
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
					double spd=ftypes[type].speed/300.0;
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
					if(d>0.5)
					{
						unsigned int type=state.fighters[j].type;
						double spd=ftypes[type].speed/300.0;
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
					if(fightersleft)
					{
						unsigned int mind=1000000;
						int minj=-1;
						for(unsigned int j=0;j<state.nfighters;j++)
						{
							if(state.fighters[j].damage>=1) continue;
							if(state.fighters[j].landed)
							{
								const unsigned int base=state.fighters[j].base;
								signed int dx=(signed)flaks[i].lat-(signed)fbases[base].lat, dy=(signed)flaks[i].lon-(signed)fbases[base].lon;
								unsigned int dd=dx*dx+dy*dy;
								if(dd<mind)
								{
									mind=dd;
									minj=j;
								}
							}
							else if(ftr_free(state.fighters[j]))
							{
								signed int dx=(signed)flaks[i].lat-state.fighters[j].lat, dy=(signed)flaks[i].lon-state.fighters[j].lon;
								unsigned int dd=dx*dx+dy*dy;
								if(dd<mind)
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
								state.fighters[minj].fuelt=t+(ftypes[ft].night?(72+irandu(28)):(35+irandu(15)));
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
				if(targs[i].threat+state.heat[i]*10.0>thresh)
				{
					unsigned int mind=1000000;
					int minj=-1;
					for(unsigned int j=0;j<state.nfighters;j++)
					{
						if(state.fighters[j].damage>=1) continue;
						if(state.fighters[j].landed)
						{
							const unsigned int base=state.fighters[j].base;
							signed int dx=(signed)targs[i].lat-(signed)fbases[base].lat, dy=(signed)targs[i].lon-(signed)fbases[base].lon;
							unsigned int dd=dx*dx+dy*dy;
							if(dd<mind)
							{
								mind=dd;
								minj=j;
							}
						}
						else if(ftr_free(state.fighters[j]))
						{
							signed int dx=(signed)targs[i].lat-state.fighters[j].lat, dy=(signed)targs[i].lon-state.fighters[j].lon;
							unsigned int dd=dx*dx+dy*dy;
							if(dd<mind)
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
							state.fighters[minj].fuelt=t+(ftypes[ft].night?(72+irandu(28)):(35+irandu(15)));
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
		if(kills[0]+kills[1]>0)
			fprintf(stderr, "Kills: flak %.3g, fighters %.3g\n", kills[0], kills[1]);
		unsigned int dij[ntargs][ntypes], nij[ntargs][ntypes], tij[ntargs][ntypes], lij[ntargs][ntypes], heat[ntargs];
		bool canscore[ntargs];
		double cidam=0;
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
				bool leaf=!state.bombers[k].bmb;
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
								heat[i]++;
								if(pget(targs[l].picture, dx+hx, dy+hy).a==ATG_ALPHA_OPAQUE)
								{
									cidam+=state.dmg[l];
									state.dmg[l]=max(0, state.dmg[l]-state.bombers[k].bmb/(targs[i].psiz*10000.0));
									cidam-=state.dmg[l];
									nij[l][type]++;
									tij[l][type]+=state.bombers[k].bmb;
									state.bombers[k].bombed=false;
								}
							}
						break;
						case TCLASS_INDUSTRY:
							if(leaf) continue;
							if((abs(dx)<=1)&&(abs(dy)<=1))
							{
								heat[i]++;
								if(brandp(targs[l].esiz/30.0))
								{
									cidam+=state.dmg[l];
									state.dmg[l]=max(0, state.dmg[l]-state.bombers[k].bmb/12000.0);
									cidam-=state.dmg[l];
									nij[l][type]++;
									tij[l][type]+=state.bombers[k].bmb;
								}
								state.bombers[k].bombed=false;
							}
						break;
						case TCLASS_AIRFIELD:
						case TCLASS_ROAD:
							if(leaf) continue;
							if((abs(dx)<=hx)&&(abs(dy)<=hy))
							{
								heat[i]++;
								if(pget(targs[l].picture, dx+hx, dy+hy).a==ATG_ALPHA_OPAQUE)
								{
									if(brandp(targs[l].esiz/30.0))
									{
										nij[l][type]++;
										tij[l][type]+=state.bombers[k].bmb;
										cidam+=state.dmg[l];
										state.dmg[l]=max(0, state.dmg[l]-state.bombers[k].bmb/2000.0);
										cidam-=state.dmg[l];
									}
									state.bombers[k].bombed=false;
								}
							}
						break;
						case TCLASS_BRIDGE:
							if(leaf) continue;
							if((abs(dx)<=hx)&&(abs(dy)<=hy))
							{
								heat[i]++;
								if(pget(targs[l].picture, dx+hx, dy+hy).a==ATG_ALPHA_OPAQUE)
								{
									if(brandp(targs[l].esiz/30.0))
									{
										nij[l][type]++;
										tij[l][type]+=state.bombers[k].bmb;
										if(brandp(log2(state.bombers[k].bmb/25.0)/200.0))
										{
											cidam+=state.dmg[l];
											bridge+=state.dmg[l];
											state.dmg[l]=0;
										}
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
									state.dmg[l]=max(0, state.dmg[l]-types[type].cap/(targs[i].psiz*4000.0));
									nij[l][type]++;
									tij[l][type]+=types[type].cap*3;
									state.bombers[k].bombed=false;
								}
							}
						break;
						case TCLASS_SHIPPING:
							if(leaf) continue;
							if((abs(dx)<=2)&&(abs(dy)<=1))
							{
								heat[i]++;
								if(brandp(targs[l].esiz/100.0))
								{
									nij[l][type]++;
									if(brandp(log2(state.bombers[k].bmb/500.0)/8.0))
									{
										tij[l][type]++;
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
								state.dmg[l]=max(0, state.dmg[l]-state.bombers[k].bmb/400000.0);
								nij[l][type]++;
								tij[l][type]+=state.bombers[k].bmb;
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
			if(state.bombers[i].crashed||!datewithin(state.now, types[type].entry, types[type].exit))
			{
				if(state.bombers[i].crashed)
					lij[state.bombers[i].targ][state.bombers[i].type]++;
				state.nbombers--;
				for(unsigned int j=i;j<state.nbombers;j++)
					state.bombers[j]=state.bombers[j+1];
				i--;
				continue;
			}
			else if(state.bombers[i].damage>50)
				state.bombers[i].failed=true; // mark as u/s
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
		state.cshr+=scoreTb/200;
		state.cshr+=scoreTl/5000;
		state.cshr+=Ts*800;
		state.cshr+=scoreTm/1000;
		state.cshr+=bridge*16;
		double par=0.2+((state.now.year-1939)*0.15);
		state.confid+=(N/(double)D-par)*(1.0+log2(D)/2.0);
		state.confid+=Ts*0.25;
		state.confid+=cidam*0.1;
		state.confid=min(max(state.confid, 0), 100);
		state.morale+=(1.8-L*100.0/(double)D)/3.0;
		if(L==0) state.morale+=0.25;
		if(D>=100) state.morale+=0.2;
		if(D>=1000) state.morale+=1.0;
		state.morale=min(max(state.morale, 0), 100);
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
		atg_resize_canvas(canvas, 800, 640);
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
					case ATG_EV_TRIGGER: // it must have been RS_cont, that's the only button on the page
						errupt++;
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
		if(state.bombers[i].failed)
		{
			if(brandp((types[state.bombers[i].type].svp/100.0)/3.0)) state.bombers[i].failed=false;
		}
		else
		{
			if(brandp((1-types[state.bombers[i].type].svp/100.0)/3.0)) state.bombers[i].failed=true;
		}
	}
	// TODO: if confid or morale too low, SACKED
	state.cshr+=state.confid;
	state.cshr+=state.morale/2.0;
	state.cshr*=.96;
	state.cash+=state.cshr;
	// Update bomber prodn caps
	for(unsigned int i=0;i<ntypes;i++)
	{
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
				types[i].pribuf+=(unsigned int [4]){0, 1, 3, 6}[types[i].prio];
				if(types[i].prio) any=true;
			}
			if(!any) break;
			continue;
		}
		if(types[m].cost>state.cash) break;
		if(types[m].cost>types[m].pcbuf) break;
		unsigned int n=state.nbombers++;
		ac_bomber *nb=realloc(state.bombers, state.nbombers*sizeof(ac_bomber));
		if(!nb)
		{
			perror("realloc"); // TODO more visible warning
			state.nbombers=n;
			break;
		}
		(state.bombers=nb)[n]=(ac_bomber){.type=m, .failed=false};
		for(unsigned int j=0;j<NNAVAIDS;j++)
			nb[n].nav[j]=false;
		state.cash-=types[m].cost;
		types[m].pcbuf-=types[m].cost;
		types[m].pc+=types[m].cost/100;
		types[m].pribuf-=8;
	}
	// install navaids
	for(unsigned int n=0;n<NNAVAIDS;n++)
	{
		if(datebefore(state.now, event[navevent[n]])) continue;
		state.napb[n]+=10;
		unsigned int i=state.nap[n];
		if(!datewithin(state.now, types[i].entry, types[i].exit)) continue;
		unsigned int j=state.nbombers;
		unsigned int nac=0;
		while((state.napb[n]>=navprod[n])&&j--)
		{
			if(state.bombers[j].type!=i) continue;
			if(state.bombers[j].failed) continue;
			if(state.bombers[j].nav[n]) continue;
			state.bombers[j].nav[n]=true;
			state.napb[n]-=navprod[n];
			if(++nac>=4) break;
		}
	}
	// assign PFF
	if(!datebefore(state.now, event[EVENT_PFF]))
	{
		for(unsigned int j=0;j<ntypes;j++)
			types[j].count=types[j].pffcount=0;
		for(unsigned int i=0;i<state.nbombers;i++)
		{
			unsigned int type=state.bombers[i].type;
			if(types[type].pff&&types[type].noarm)
			{
				state.bombers[i].pff=true;
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
				pffneed--;
			}
		}
	}
	// German production
	double dprod=0;
	if(datebefore(state.now, event[EVENT_ABD]))
	{
		for(unsigned int i=0;i<ntargs;i++)
		{
			if(!datebefore(state.now, targs[i].entry)) continue;
			if(targs[i].class==TCLASS_CITY) dprod+=100.0*targs[i].prod;
		}
	}
	for(unsigned int i=0;i<ntargs;i++)
	{
		if(!datewithin(state.now, targs[i].entry, targs[i].exit)) continue;
		switch(targs[i].class)
		{
			case TCLASS_CITY:
				state.flk[i]=(state.flk[i]*.95)+(targs[i].flak*state.dmg[i]*.0005);
				/* fallthrough */
			case TCLASS_LEAFLET:
				state.dmg[i]=min(state.dmg[i]*1.01, 100);
				dprod+=state.dmg[i]*targs[i].prod;
			break;
			case TCLASS_SHIPPING:
			break;
			case TCLASS_MINING:
				dprod+=state.dmg[i]*targs[i].prod/6.0;
			break;
			case TCLASS_AIRFIELD:
			case TCLASS_BRIDGE:
			case TCLASS_ROAD:
				dprod+=state.dmg[i]*targs[i].prod/2.0;
			break;
			case TCLASS_INDUSTRY:
				state.dmg[i]=min(state.dmg[i]*1.05, 100);
				dprod+=state.dmg[i]*targs[i].prod/2.0;
			break;
			default: // shouldn't ever get here
				fprintf(stderr, "Bad targs[%d].class = %d\n", i, targs[i].class);
			break;
		}
	}
	// German fighters
	for(unsigned int i=0;i<state.nfighters;i++)
	{
		unsigned int type=state.fighters[i].type;
		if(state.fighters[i].crashed||!datewithin(state.now, ftypes[type].entry, ftypes[type].exit))
		{
			//fprintf(stderr, "Killed a fighter, type %u\n", type);
			state.nfighters--;
			for(unsigned int j=i;j<state.nfighters;j++)
				state.fighters[j]=state.fighters[j+1];
			i--;
			continue;
		}
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
	state.gprod+=dprod/12.0;
	while(state.gprod>=mfcost)
	{
		unsigned int i=irandu(nftypes);
		if(!datewithin(state.now, ftypes[i].entry, ftypes[i].exit)) continue;
		if(ftypes[i].cost>state.gprod) break;
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
		(state.fighters=newf)[n]=(ac_fighter){.type=i, .base=base, .crashed=false, .landed=true, .k=-1, .targ=-1, .damage=0};
		state.gprod-=ftypes[i].cost;
	}
	if(++state.now.day>monthdays[state.now.month-1]+(((state.now.month==2)&&!(state.now.year%4))?1:0))
	{
		state.now.day=1;
		if(++state.now.month>12)
		{
			state.now.month=1;
			state.now.year++;
		}
	}
	for(unsigned int i=0;i<ntypes;i++)
		if(!diffdate(state.now, types[i].entry))
		{
			if(msgadd(&state, state.now, types[i].name, types[i].newtext))
				fprintf(stderr, "failed to msgadd newtype: %s\n", types[i].name);
		}
	for(unsigned int i=0;i<nftypes;i++)
		if(!diffdate(state.now, ftypes[i].entry))
		{
			if(msgadd(&state, state.now, ftypes[i].name, ftypes[i].newtext))
				fprintf(stderr, "failed to msgadd newftype: %s\n", ftypes[i].name);
		}
	for(unsigned int ev=0;ev<NEVENTS;ev++)
	{
		if(!diffdate(state.now, event[ev]))
		{
			if(msgadd(&state, state.now, event_names[ev], evtext[ev]))
				fprintf(stderr, "failed to msgadd event: %s\n", event_names[ev]);
		}
	}
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
	SDL_FreeSurface(weather_overlay);
	SDL_FreeSurface(target_overlay);
	SDL_FreeSurface(flak_overlay);
	SDL_FreeSurface(grey_overlay);
	SDL_FreeSurface(terrain);
	SDL_FreeSurface(map);
	SDL_FreeSurface(location);
	return(0);
}

date readdate(const char *t, date nulldate)
{
	date d;
	if(t&&*t&&(sscanf(t, "%u-%u-%d", &d.day, &d.month, &d.year)==3)) return(d);
	return(nulldate);
}

bool datebefore(date date1, date date2) // returns true if date1 is strictly before date2
{
	if(date1.year!=date2.year) return(date1.year<date2.year);
	if(date1.month!=date2.month) return(date1.month<date2.month);
	return(date1.day<date2.day);
}

int diffdate(date date1, date date2) // returns <0 if date1<date2, >0 if date1>date2, 0 if date1==date2.  Value is difference in days
{
	if(date1.month<3)
	{
		date1.month+=12;
		date1.year--;
	}
	if(date2.month<3)
	{
		date2.month+=12;
		date2.year--;
	}
	int days1=365*date1.year + floor(date1.year/4) - floor(date1.year/100) + floor(date1.year/400) + floor(date1.day) + floor((153*date1.month+8)/5);
	int days2=365*date2.year + floor(date2.year/4) - floor(date2.year/100) + floor(date2.year/400) + floor(date2.day) + floor((153*date2.month+8)/5);
	return(days1-days2);
}

double pom(date when)
{
	// new moon at 14/08/1939
	// synodic month 29.530588853 days
	return(fmod(diffdate(when, (date){.day=14, .month=8, .year=1939})/29.530588853, 1));
}

double foldpom(double pom)
{
	return((1-cos(pom*M_PI*2))/2.0);
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
		double leftx=width*cos( left*M_PI)+halfx,
		      rightx=width*cos(right*M_PI)+halfx;
		for(unsigned int x=halfx-width;x<=halfx+width;x++)
		{
			if((leftx<x)&&(x<rightx))
				pset(s, x, y, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE});
			else if((floor(leftx)<=x)&&(x<rightx))
				pset(s, x, y, (atg_colour){223*(1+x-floor(leftx)), 223*(1+x-floor(leftx)), 223*(1+x-floor(leftx)), ATG_ALPHA_OPAQUE});
			else if((leftx<x)&&(x<ceil(rightx)))
				pset(s, x, y, (atg_colour){223*(1-fmod(rightx, 1)), 223*(1-fmod(rightx, 1)), 223*(1-fmod(rightx, 1)), ATG_ALPHA_OPAQUE});
			else
				pset(s, x, y, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
		}
		/*pset(s, leftx, y, (atg_colour){223, 0, 0, ATG_ALPHA_OPAQUE});
		pset(s, rightx, y, (atg_colour){0, 0, 223, ATG_ALPHA_OPAQUE});*/
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
					f=sscanf(line, "Type %u:%u,%u,%u\n", &j, &failed, &nav, &pff);
					// TODO: this is for oldsave compat and should probably be removed later
					if(f==3)
					{
						f=4;
						pff=0;
					}
					if(f!=4)
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
				}
			}
		}
		else if(strcmp(tag, "GProd")==0)
		{
			f=sscanf(dat, "%la\n", &state->gprod);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
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
					f=sscanf(line, "Type %u:%u\n", &j, &base);
					if(f!=2)
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
				}
			}
		}
		else if(strcmp(tag, "Targets init")==0)
		{
			double dmg,flk,heat;
			f=sscanf(dat, "%la,%la,%la\n", &dmg, &flk, &heat);
			if(f!=3)
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
					double dmg, flk, heat;
					f=sscanf(line, "Targ %u:%la,%la,%la\n", &j, &dmg, &flk, &heat);
					if(f!=4)
					{
						fprintf(stderr, "1 Too few arguments to part %u of tag \"%s\"\n", i, tag);
						e|=1;
						break;
					}
					while((i<j)&&(i<ntargs))
					{
						state->dmg[i]=100;
						state->flk[i]=targs[i].flak;
						state->heat[i]=0;
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
		fprintf(fs, "Type %u:%u,%u,%u\n", state.bombers[i].type, state.bombers[i].failed?1:0, nav, state.bombers[i].pff?1:0);
	}
	fprintf(fs, "GProd:%la\n", state.gprod);
	fprintf(fs, "FTypes:%u\n", nftypes);
	fprintf(fs, "FBases:%u\n", nfbases);
	fprintf(fs, "Fighters:%u\n", state.nfighters);
	for(unsigned int i=0;i<state.nfighters;i++)
		fprintf(fs, "Type %u:%u\n", state.fighters[i].type, state.fighters[i].base);
	fprintf(fs, "Targets:%hhu\n", ntargs);
	for(unsigned int i=0;i<ntargs;i++)
		fprintf(fs, "Targ %hhu:%la,%la,%la\n", i, state.dmg[i], targs[i].flak?state.flk[i]*100.0/(double)targs[i].flak:0, state.heat[i]);
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
		if(!datewithin(now, targs[i].entry, targs[i].exit)) continue;
		switch(targs[i].class)
		{
			case TCLASS_CITY:
			case TCLASS_LEAFLET:
			case TCLASS_AIRFIELD:
			case TCLASS_ROAD:
			case TCLASS_BRIDGE:
			case TCLASS_INDUSTRY:
				SDL_BlitSurface(targs[i].picture, NULL, rv, &(SDL_Rect){.x=targs[i].lon-targs[i].picture->w/2, .y=targs[i].lat-targs[i].picture->h/2});
			break;
			case TCLASS_SHIPPING:
			case TCLASS_MINING:
				SDL_BlitSurface(targs[i].picture, NULL, rv, &(SDL_Rect){.x=targs[i].lon-1, .y=targs[i].lat-1});
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

int pset(SDL_Surface *s, unsigned int x, unsigned int y, atg_colour c)
{
	if(!s)
		return(1);
	if((x>=(unsigned int)s->w)||(y>=(unsigned int)s->h))
		return(2);
	size_t s_off = (y*s->pitch) + (x*s->format->BytesPerPixel);
	uint32_t pixval = SDL_MapRGBA(s->format, c.r, c.g, c.b, c.a);
	*(uint32_t *)((char *)s->pixels + s_off)=pixval;
	return(0);
}

int line(SDL_Surface *s, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, atg_colour c)
{
	// Bresenham's line algorithm, based on http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
	int e=0;
	if((e=pset(s, x0, y0, c)))
		return(e);
	bool steep = abs(y1 - y0) > abs(x1 - x0);
	int tmp;
	if(steep)
	{
		tmp=x0;x0=y0;y0=tmp;
		tmp=x1;x1=y1;y1=tmp;
	}
	if(x0>x1)
	{
		tmp=x0;x0=x1;x1=tmp;
		tmp=y0;y0=y1;y1=tmp;
	}
	int dx=x1-x0,dy=abs(y1-y0);
	int ey=dx>>1;
	int dely=(y0<y1?1:-1),y=y0;
	for(int x=x0;x<(int)x1;x++)
	{
		if((e=pset(s, steep?y:x, steep?x:y, c)))
			return(e);
		ey-=dy;
		if(ey<0)
		{
			y+=dely;
			ey+=dx;
		}
	}
	return(0);
}

atg_colour pget(SDL_Surface *s, unsigned int x, unsigned int y)
{
	if(!s)
		return((atg_colour){.r=0, .g=0, .b=0, .a=0});
	if((x>=(unsigned int)s->w)||(y>=(unsigned int)s->h))
		return((atg_colour){.r=0, .g=0, .b=0, .a=0});
	size_t s_off = (y*s->pitch) + (x*s->format->BytesPerPixel);
	uint32_t pixval = *(uint32_t *)((char *)s->pixels + s_off);
	atg_colour c;
	SDL_GetRGBA(pixval, s->format, &c.r, &c.g, &c.b, &c.a);
	return(c);
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

int msgadd(game *state, date when, const char *ref, const char *msg)
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
				atg_element *cont=atg_create_element_button(signtext, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE}, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE});
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
	atg_free_box_box(msgbox);
}
