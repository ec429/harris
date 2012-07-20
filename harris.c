#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "atg/atg.h"
#include <SDL_image.h>

#include "bits.h"
#include "weather.h"
#include "rand.h"
#include "widgets.h"

typedef struct
{
	unsigned int year;
	unsigned int month;
	unsigned int day;
}
date;

#define NAV_GEE		0
#define NAV_H2S		1
#define NAV_OBOE	2
#define NAV_GH		3
#define NAV_ABC		4
#define NNAVAIDS	5
const char * const navaids[NNAVAIDS]={"GEE","H2S","OBOE","GH","ABC"}; // ABC is actually a type of RCM

typedef struct
{
	//MANUFACTURER:NAME:COST:SPEED:CAPACITY:SVP:DEFENCE:FAILURE:ACCURACY:BLAT:BLONG:DD-MM-YYYY:DD-MM-YYYY:NAVAIDS
	char * manu;
	char * name;
	unsigned int cost;
	unsigned int speed;
	unsigned int cap;
	unsigned int svp;
	unsigned int defn;
	unsigned int fail;
	unsigned int accu;
	date entry;
	date exit;
	bool nav[NNAVAIDS];
	unsigned int blat, blon;
	SDL_Surface *picture;
	
	unsigned int prio;
	atg_element *prio_selector;
}
bombertype;

#define CITYSIZE	17
#define HALFCITY	8

typedef struct
{
	//NAME:PROD:FLAK:ESIZ:LAT:LONG:DD-MM-YYYY:DD-MM-YYYY:CLASS
	unsigned int id;
	char * name;
	unsigned int prod, flak, esiz, lat, lon;
	date entry, exit;
	enum {TCLASS_CITY,} class;
	SDL_Surface *picture;
}
target;

typedef struct
{
	unsigned int type;
	unsigned int targ;
	double lat, lon;
	double navlat, navlon; // error in "believed position" relative to true position
	double driftlat, driftlon; // rate of error
	unsigned int bmblat, bmblon; // true position where bombs were dropped (if any)
	bool nav[NNAVAIDS];
	unsigned int bmb; // bombload carried
	bool bombed;
	bool crashed;
	bool failed; // for forces, read as !svble
	bool landed; // for forces, read as !assigned
	double speed;
	bool idtar; // identified target?  (for use if RoE require it)
	unsigned int fuelt; // when t (ticks) exceeds this value, turn for home
}
ac_bomber;

typedef struct
{
	unsigned int nbombers;
	unsigned int *bombers; // offsets into the game.bombers list
	// TODO routing
}
raid;

typedef struct
{
	date now;
	unsigned int hour;
	unsigned int nbombers;
	ac_bomber *bombers;
	w_state weather;
	unsigned int ntargs;
	double *dmg, *flk;
	raid *raids;
	struct
	{
		bool idtar;
	}
	roe;
}
game;

#define VER_MAJ	0
#define VER_MIN	1
#define VER_REV	0

int loadgame(const char *fn, game *state, bool lorw[128][128]);
date readdate(const char *t, date nulldate);
int diffdate(date date1, date date2); // returns <0 if date1<date2, >0 if date1>date2, 0 if date1==date2
bool version_newer(const unsigned char v1[3], const unsigned char v2[3]); // true iff v1 newer than v2
SDL_Surface *render_weather(w_state weather);
SDL_Surface *render_cities(unsigned int ntargs, target *targs);
SDL_Surface *render_ac_b(game state);
int pset(SDL_Surface *s, unsigned int x, unsigned int y, atg_colour c);
atg_colour pget(SDL_Surface *s, unsigned int x, unsigned int y);
unsigned int ntypes=0;
unsigned int ntargs=0;
SDL_Surface *terrain=NULL;
SDL_Surface *location=NULL;

int main(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[])
{
	atg_canvas *canvas=atg_create_canvas(120, 24, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
	if(!canvas)
	{
		fprintf(stderr, "atg_create_canvas failed\n");
		return(1);
	}
	
	srand(0); // predictable seed for creating 'random' target maps
	
	// Load data files
	fprintf(stderr, "Loading data files...\n");
	FILE *typefp=fopen("dat/bombers", "r");
	bombertype *types=NULL;
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
				// MANUFACTURER:NAME:COST:SPEED:CAPACITY:SVP:DEFENCE:FAILURE:ACCURACY:BLAT:BLONG:DD-MM-YYYY:DD-MM-YYYY:NAVAIDS
				this.name=strdup(next); // guarantees that enough memory will be allocated
				this.manu=(char *)malloc(strcspn(next, ":")+1);
				ssize_t db;
				int e;
				if((e=sscanf(next, "%[^:]:%[^:]:%u:%u:%u:%u:%u:%u:%u:%u:%u:%zn", this.manu, this.name, &this.cost, &this.speed, &this.cap, &this.svp, &this.defn, &this.fail, &this.accu, &this.blat, &this.blon, &db))!=11)
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
	
	FILE *targfp=fopen("dat/targets", "r");
	target *targs=NULL;
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
				if(strcmp(class, "CITY")==0)
				{
					this.class=TCLASS_CITY;
				}
				else
				{
					fprintf(stderr, "Bad `targets' line `%s'\n", next);
					fprintf(stderr, "  unrecognised :CLASS `%s'\n", class);
					return(1);
				}
				switch(this.class)
				{
					case TCLASS_CITY:
						if(!(this.picture=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, CITYSIZE, CITYSIZE, 32, 0xff000000, 0xff0000, 0xff00, 0xff)))
						{
							fprintf(stderr, "this.picture: SDL_CreateRGBSurface: %s\n", SDL_GetError());
							return(1);
						}
						SDL_SetAlpha(this.picture, 0, 0);
						SDL_FillRect(this.picture, &(SDL_Rect){.x=0, .y=0, .w=this.picture->w, .h=this.picture->h}, ATG_ALPHA_TRANSPARENT&0xff);
						{
							for(unsigned int k=0;k<6;k++)
							{
								int x=HALFCITY, y=HALFCITY;
								for(unsigned int i=0;i<this.esiz>>1;i++)
								{
									pset(this.picture, x, y, (atg_colour){.r=7, .g=7, .b=7, .a=ATG_ALPHA_OPAQUE});
									unsigned int j=rand()&3;
									if(j&1) y+=j-2;
									else x+=j-1;
									if((x<0)||(x>=CITYSIZE)) x=HALFCITY;
									if((y<0)||(y>=CITYSIZE)) x=HALFCITY;
								}
							}
						}
					break;
					default: // shouldn't ever get here
						fprintf(stderr, "Bad this.class = %d\n", this.class);
						return(1);
					break;
				}
				targs=(target *)realloc(targs, (ntargs+1)*sizeof(target));
				targs[ntargs]=this;
				ntargs++;
			}
			next=strtok(NULL, "\n");
		}
		free(targfile);
		fprintf(stderr, "Loaded %u targets\n", ntargs);
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
	
	fprintf(stderr, "Data files loaded\n");
	
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
	
	atg_box *gamebox=atg_create_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){63, 47, 0, ATG_ALPHA_OPAQUE});
	if(!gamebox)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	atg_element *GB_bt=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){63, 47, 0, ATG_ALPHA_OPAQUE});
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
	atg_element *GB_btrow[ntypes], *GB_btpic[ntypes], *GB_btnum[ntypes];
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
		GB_btrow[i]->w=159;
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
		SDL_BlitSurface(types[i].picture, NULL, pic, &(SDL_Rect){(36-types[i].picture->w)>>1, 0, 0, 0});
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
				atg_element *name=atg_create_element_label(fullname, 10, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
				if(!name)
				{
					fprintf(stderr, "atg_create_element_label failed\n");
					return(1);
				}
				name->w=121;
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
		if(atg_pack_element(vb, types[i].prio_selector))
		{
			perror("atg_pack_element");
			return(1);
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
	SDL_Surface *weather_overlay=NULL, *city_overlay=NULL;
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
			SDL_BlitSurface(types[j].picture, NULL, pic, &(SDL_Rect){(36-types[j].picture->w)>>1, 0, 0, 0});
			atg_element *picture=atg_create_element_image(pic);
			SDL_FreeSurface(pic);
			if(!picture)
			{
				fprintf(stderr, "atg_create_element_image failed\n");
				return(1);
			}
			picture->w=38;
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
		atg_element *item=atg_create_element_label(targs[i].name, 10, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE});
		if(!item)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		item->w=120;
		if(atg_pack_element(b, item))
		{
			perror("atg_pack_element");
			return(1);
		}
		item=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){95, 95, 119, ATG_ALPHA_OPAQUE});
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
	
	atg_box *raidbox=atg_create_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){63, 47, 0, ATG_ALPHA_OPAQUE});
	if(!raidbox)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	atg_element *RB_map=atg_create_element_image(map);
	if(!RB_map)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	RB_map->h=map->h+2;
	if(atg_pack_element(raidbox, RB_map))
	{
		perror("atg_pack_element");
		return(1);
	}
	
	game state;
	if(!(state.bombers=malloc(state.nbombers*sizeof(*state.bombers))))
	{
		perror("malloc");
		return(1);
	}
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
	if(!(state.raids=malloc(ntargs*sizeof(*state.raids))))
	{
		perror("malloc");
		return(1);
	}
	for(unsigned int i=0;i<ntargs;i++)
	{
		state.raids[i].nbombers=0;
		state.raids[i].bombers=NULL;
	}
	state.roe.idtar=true;
	
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
	
	gameloop:
	canvas->box=gamebox;
	atg_resize_canvas(canvas, 640, 480);
	for(unsigned int j=0;j<state.nbombers;j++)
	{
		state.bombers[j].landed=true;
	}
	for(unsigned int i=0;i<ntypes;i++)
	{
		if(GB_btrow[i])
			GB_btrow[i]->hidden=((diffdate(types[i].entry, state.now)>0)||(diffdate(types[i].exit, state.now)<0));
		if(GB_btnum[i]&&GB_btnum[i]->elem.label&&GB_btnum[i]->elem.label->text)
		{
			unsigned int svble=0,total=0;
			for(unsigned int j=0;j<state.nbombers;j++)
				if(state.bombers[j].type==i)
				{
					total++;
					if(!state.bombers[j].failed) svble++;
				}
			snprintf(GB_btnum[i]->elem.label->text, 12, "%u/%u", svble, total);
		}
	}
	for(unsigned int i=0;i<ntargs;i++)
	{
		if(GB_ttdmg[i])
			GB_ttdmg[i]->w=floor(state.dmg[i]);
		if(GB_ttflk[i])
			GB_ttflk[i]->w=floor(state.flk[i]);
		for(unsigned int j=0;j<ntypes;j++)
		{
			if(GB_rbrow[i][j])
				GB_rbrow[i][j]->hidden=GB_btrow[j]?GB_btrow[j]->hidden:true;
			if(GB_raidnum[i][j]&&GB_raidnum[i][j]->elem.label&&GB_raidnum[i][j]->elem.label->text)
				snprintf(GB_raidnum[i][j]->elem.label->text, 9, "%u", 0);
		}
	}
	SDL_FreeSurface(GB_map->elem.image->data);
	GB_map->elem.image->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
	SDL_FreeSurface(city_overlay);
	city_overlay=render_cities(ntargs, targs);
	SDL_BlitSurface(city_overlay, NULL, GB_map->elem.image->data, NULL);
	SDL_FreeSurface(weather_overlay);
	weather_overlay=render_weather(state.weather);
	SDL_BlitSurface(weather_overlay, NULL, GB_map->elem.image->data, NULL);
	/*if(seltarg>=0)
		SDL_BlitSurface(location, NULL, GB_map->elem.image->data, &(SDL_Rect){.x=targs[seltarg].lon, .y=targs[seltarg].lat});*/
	int seltarg=-1;
	while(1)
	{
		for(unsigned int i=0;i<ntargs;i++)
		{
			if(GB_ttrow[i]&&GB_ttrow[i]->elem.box)
			{
				unsigned int traid=0;
				unsigned int raid[ntypes];
				for(unsigned int j=0;j<ntypes;j++)
					raid[j]=0;
				for(unsigned int j=0;j<state.raids[i].nbombers;j++)
				{
					traid++;
					raid[state.bombers[state.raids[i].bombers[j]].type]++;
				}
				for(unsigned int j=0;j<ntypes;j++)
				{
					if(GB_rbrow[i][j])
						GB_rbrow[i][j]->hidden=(GB_btrow[j]?GB_btrow[j]->hidden:true)||!raid[j];
				}
				GB_ttrow[i]->elem.box->bgcolour=(traid?(atg_colour){127, 103, 95, ATG_ALPHA_OPAQUE}:(atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
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
				case ATG_EV_CLICK:;
					atg_ev_click c=e.event.click;
					atg_mousebutton b=c.button;
					if(c.e)
					{
						for(unsigned int i=0;i<ntypes;i++)
						{
							if(c.e==GB_btpic[i])
							{
								if(seltarg<0)
									fprintf(stderr, "btrow %d\n", i);
								else
								{
									unsigned int amount;
									switch(b)
									{
										case ATG_MB_RIGHT:
											amount=1;
										break;
										case ATG_MB_MIDDLE:
											amount=-1;
										break;
										case ATG_MB_LEFT:
										default:
											amount=10;
									}
									for(unsigned int j=0;j<state.nbombers;j++)
									{
										if(state.bombers[j].type!=i) continue;
										if(!state.bombers[j].landed) continue;
										state.bombers[j].landed=false;;
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
						if(c.e==GB_ttl)
						{
							seltarg=-1;
							SDL_FreeSurface(GB_map->elem.image->data);
							GB_map->elem.image->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
							SDL_BlitSurface(city_overlay, NULL, GB_map->elem.image->data, NULL);
							SDL_BlitSurface(weather_overlay, NULL, GB_map->elem.image->data, NULL);
							free(GB_raid_label->elem.label->text);
							GB_raid_label->elem.label->text=strdup("Select a Target");
							GB_raid->elem.box=GB_raidbox_empty;
						}
						for(unsigned int i=0;i<ntargs;i++)
						{
							if(c.e==GB_ttrow[i])
							{
								seltarg=i;
								SDL_FreeSurface(GB_map->elem.image->data);
								GB_map->elem.image->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
								SDL_BlitSurface(city_overlay, NULL, GB_map->elem.image->data, NULL);
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
											amount=1;
										break;
										case ATG_MB_MIDDLE:
											amount=-1;
										break;
										case ATG_MB_LEFT:
										default:
											amount=10;
									}
									for(unsigned int j=0;j<state.raids[seltarg].nbombers;j++)
									{
										unsigned int k=state.raids[seltarg].bombers[j];
										if(state.bombers[k].type!=i) continue;
										state.bombers[k].landed=true;
										amount--;
										state.raids[seltarg].nbombers--;
										for(unsigned int k=j;k<state.raids[seltarg].nbombers;k++)
											state.bombers[k]=state.bombers[k+1];
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
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e)
					{
						if(trigger.e==GB_go)
						{
							goto run_raid;
						}
					}
				break;
				default:
					fprintf(stderr, "e.type %d\n", e.type);
				break;
			}
		}
		SDL_Delay(50);
		#if 0 // for testing weathersim code
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
	
	run_raid:;
	unsigned int totalraids;
	totalraids=0;
	for(unsigned int i=0;i<ntargs;i++)
		totalraids+=state.raids[i].nbombers;
	if(totalraids)
	{
		canvas->box=raidbox;
		atg_resize_canvas(canvas, 640, 480);
		SDL_FreeSurface(RB_map->elem.image->data);
		RB_map->elem.image->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
		SDL_Surface *with_city=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
		SDL_FreeSurface(city_overlay);
		city_overlay=render_cities(ntargs, targs);
		SDL_BlitSurface(city_overlay, NULL, with_city, NULL);
		SDL_Surface *with_weather=SDL_ConvertSurface(with_city, with_city->format, with_city->flags);
		SDL_FreeSurface(weather_overlay);
		weather_overlay=render_weather(state.weather);
		SDL_BlitSurface(weather_overlay, NULL, with_weather, NULL);
		SDL_BlitSurface(with_weather, NULL, RB_map->elem.image->data, NULL);
		unsigned int it=0;
		for(unsigned int i=0;i<ntargs;i++)
			for(unsigned int j=0;j<state.raids[i].nbombers;j++)
			{
				unsigned int k=state.raids[i].bombers[j], type=state.bombers[k].type;
				state.bombers[k].targ=i;
				state.bombers[k].lat=types[type].blat;
				state.bombers[k].lon=types[type].blon;
				state.bombers[k].bmb=types[type].cap;
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
				state.bombers[k].speed=(types[type].speed+irandu(16)-8)/300.0;
				state.bombers[k].fuelt=256+irandu(64);
				if(targs[i].lon>180)
				{
					state.bombers[k].fuelt+=3*(targs[i].lon-180);
					state.bombers[k].bmb*=(280-targs[i].lon)/100.0;
				}
			}
		SDL_Surface *with_ac_b=SDL_ConvertSurface(with_weather, with_weather->format, with_weather->flags);
		SDL_Surface *ac_b_overlay=render_ac_b(state);
		SDL_BlitSurface(ac_b_overlay, NULL, with_ac_b, NULL);
		SDL_BlitSurface(with_ac_b, NULL, RB_map->elem.image->data, NULL);
		unsigned int inair=totalraids, t=0;
		while(inair)
		{
			t++;
			if((!(t&1))&&(it<512))
			{
				w_iter(&state.weather, lorw);
				SDL_FreeSurface(weather_overlay);
				weather_overlay=render_weather(state.weather);
				SDL_BlitSurface(with_city, NULL, with_weather, NULL);
				SDL_BlitSurface(weather_overlay, NULL, with_weather, NULL);
				it++;
			}
			for(unsigned int i=0;i<ntargs;i++)
				for(unsigned int j=0;j<state.raids[i].nbombers;j++)
				{
					unsigned int k=state.raids[i].bombers[j], type=state.bombers[k].type;
					if(state.bombers[k].crashed||state.bombers[k].landed) continue;
					if(brandp(types[type].fail/10000.0))
					{
						state.bombers[k].failed=true;
						if(brandp(0.02))
						{
							state.bombers[k].crashed=true;
							inair--;
							continue;
						}
					}
					bool home=state.bombers[k].bombed||state.bombers[k].failed||(t>state.bombers[k].fuelt);
					int destx=home?types[type].blon:targs[i].lon,
						desty=home?types[type].blat:targs[i].lat;
					double cx=destx-(state.bombers[k].lon+state.bombers[k].navlon), cy=desty-(state.bombers[k].lat+state.bombers[k].navlat);
					double d=hypot(cx, cy);
					if(home)
					{
						if(d<0.4)
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
					}
					else if((cx<1.2)&&(state.bombers[k].idtar||!state.roe.idtar||brandp(0.005)))
					{
						state.bombers[k].bmblon=state.bombers[k].lon;
						state.bombers[k].bmblat=state.bombers[k].lat;
						state.bombers[k].bombed=true;
					}
					state.bombers[k].idtar=false;
					double dx=cx*state.bombers[k].speed/d,
						dy=cy*state.bombers[k].speed/d;
					if(d==0) dx=dy=-1;
					state.bombers[k].lon+=dx;
					state.bombers[k].lat+=dy;
					// TODO: navigation affected by sun/moon, navaids...
					double navacc=3.0/types[type].accu;
					double ex=drandu(navacc)-(navacc/2), ey=drandu(navacc)-(navacc/2);
					state.bombers[k].driftlon=state.bombers[k].driftlon*.98+ex;
					state.bombers[k].driftlat=state.bombers[k].driftlat*.98+ey;
					state.bombers[k].lon+=state.bombers[k].driftlon;
					state.bombers[k].lat+=state.bombers[k].driftlat;
					state.bombers[k].navlon-=state.bombers[k].driftlon;
					state.bombers[k].navlat-=state.bombers[k].driftlat;
					unsigned int x=state.bombers[k].lon, y=state.bombers[k].lat;
					double wea=((x<128)&&(y<128))?state.weather.p[x][y]-1000:0;
					double navp=types[type].accu*3.5/(double)(8+max(1008-wea, 0));
					if(state.bombers[k].lon<64) navp=1;
					if(brandp(navp))
					{
						state.bombers[k].navlon*=(256.0+state.bombers[k].lon)/512.0;
						state.bombers[k].navlat*=(256.0+state.bombers[k].lon)/512.0;
						if(pget(city_overlay, state.bombers[k].lon, state.bombers[k].lat).a==ATG_ALPHA_OPAQUE)
						{
							state.bombers[k].idtar=true;
							unsigned int dm=0;
							double mind=1000;
							for(unsigned int i=0;i<ntargs;i++)
							{
								double d=hypot(state.bombers[k].lon+state.bombers[k].navlon-targs[i].lon, state.bombers[k].lat+state.bombers[k].navlat-targs[i].lat);
								if(d<mind)
								{
									mind=d;
									dm=i;
								}
							}
							state.bombers[k].navlon=targs[dm].lon-state.bombers[k].lon;
							state.bombers[k].navlat=targs[dm].lat-state.bombers[k].lat;
						}
					}
				}
			SDL_FreeSurface(ac_b_overlay);
			ac_b_overlay=render_ac_b(bi, b);
			SDL_BlitSurface(with_weather, NULL, with_ac_b, NULL);
			SDL_BlitSurface(ac_b_overlay, NULL, with_ac_b, NULL);
			SDL_BlitSurface(with_ac_b, NULL, RB_map->elem.image->data, NULL);
			atg_flip(canvas);
		}
		// incorporate the results, and clear the raids ready for next cycle
		unsigned int nloss=0, nbomb=0, tbomb=0, ntarg=0, ttarg=0;
		unsigned int bntarg[ntypes], bttarg[ntypes], tntarg[ntargs], tttarg[ntargs];
		for(unsigned int i=0;i<ntargs;i++)
			tntarg[i]=tttarg[i]=0;
		for(unsigned int j=0;j<ntypes;j++)
			bntarg[j]=bttarg[j]=0;
		for(unsigned int i=0;i<ntargs;i++)
			for(unsigned int j=0;j<state.raids[i].nbombers;j++)
			{
				unsigned int k=state.raids[i].bombers[j], type=state.bombers[k].type;
				if(state.bombers[k].bombed)
				{
					nbomb++;
					tbomb+=state.bombers[k].bmb;
					for(unsigned int i=0;i<ntargs;i++)
					{
						int dx=state.bombers[k].bmblon-targs[i].lon, dy=state.bombers[k].bmblat-targs[i].lat;
						if((abs(dx)<=HALFCITY)&&(abs(dy)<=HALFCITY))
						{
							if(pget(targs[i].picture, dx+HALFCITY, dy+HALFCITY).a==ATG_ALPHA_OPAQUE)
							{
								state.dmg[i]=max(0, state.dmg[i]-state.bombers[k].bmb/100000.0);
								ntarg++;
								ttarg+=state.bombers[k].bmb;
								bntarg[type]++;
								bttarg[type]+=state.bombers[k].bmb;
								tntarg[i]++;
								tttarg[i]+=b[a].bmb;
							}
						}
					}
				}
			}
		for(unsigned int i=0;i<state.nbombers;i++)
		{
			if(state.bombers[i].crashed)
			{
				nloss++;
				state.nbombers--;
				for(unsigned int j=i;j<state.nbombers;j++)
					state.bombers[j]=state.bombers[j+1];
			}
		}
		// report on the results (TODO: GUI bit)
		fprintf(stderr, "%u dispatched\n%u lost\n%u bombed (%ulb)\n", bi, nloss, nbomb, tbomb);
		fprintf(stderr, "%u on target (%ulb)\n", ntarg, ttarg);
		fprintf(stderr, " By type:\n");
		for(unsigned int j=0;j<ntypes;j++)
		{
			if(bntarg[j])
				fprintf(stderr, "  %s %s: %u (%ulb)\n", types[j].manu, types[j].name, bntarg[j], bttarg[j]);
		}
		fprintf(stderr, " By target\n");
		for(unsigned int i=0;i<ntargs;i++)
		{
			if(tntarg[i])
				fprintf(stderr, "  %s: %u (%u lb)\n", targs[i].name, tntarg[i], tttarg[i]);
		}
		// finish the weather
		for(; it<512;it++)
			w_iter(&state.weather, lorw);
	}
	else
	{
		for(unsigned int it=0;it<512;it++)
			w_iter(&state.weather, lorw);
	}
	goto gameloop;
	
	do_exit:
	canvas->box=NULL;
	atg_free_canvas(canvas);
	atg_free_box_box(mainbox);
	atg_free_box_box(gamebox);
	SDL_FreeSurface(weather_overlay);
	return(0);
}

date readdate(const char *t, date nulldate)
{
	date d;
	if(t&&*t&&(sscanf(t, "%u-%u-%u", &d.day, &d.month, &d.year)==3)) return(d);
	return(nulldate);
}

int diffdate(date date1, date date2) // returns <0 if date1<date2, >0 if date1>date2, 0 if date1==date2
{
	int d=date1.year-date2.year;
	if(d) return(d);
	d=date1.month-date2.month;
	if(d) return(d);
	return(date1.day-date2.day);
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
	else
	{
		unsigned char s_version[3]={0,0,0};
		unsigned char version[3]={VER_MAJ,VER_MIN,VER_REV};
		while(!feof(fs))
		{
			char *line=fgetl(fs);
			if(!line) break;
			if(!*line)
			{
				free(line);
				continue;
			}
			char *tag=line, *dat=strchr(line, ':');
			if(dat) *dat++=0;
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
			else if(strcmp(tag, "TIME")==0)
			{
				f=sscanf(dat, "%u\n", &state->hour);
				if(f!=1)
				{
					fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
					e|=1;
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
					for(unsigned int i=0;i<nbombers;i++)
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
						unsigned int svble,nav;
						f=sscanf(line, "Type %u:%u,%u\n", &j, &svble, &nav);
						if(f!=3)
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
						state->bombers[i]=(ac_bomber){.type=j, .failed=!svble};
						for(unsigned int n=0;n<NNAVAIDS;n++)
							state->bombers[i].nav[n]=(nav>>n)&1;
					}
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
						unsigned int j;
						f=sscanf(line, "Targ %u:%la,%la\n", &j, &state->dmg[i], &state->flk[i]);
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
	}
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
			pset(rv, x, y, (atg_colour){180+weather.t[x>>1][y>>1]*3, 210, 255-weather.t[x>>1][y>>1]*3, cl});
		}
	}
	return(rv);
}

SDL_Surface *render_cities(unsigned int ntargs, target *targs)
{
	SDL_Surface *rv=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 256, 256, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!rv)
	{
		fprintf(stderr, "render_cities: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(NULL);
	}
	SDL_FillRect(rv, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, ATG_ALPHA_TRANSPARENT&0xff);
	for(unsigned int i=0;i<ntargs;i++)
	{
		SDL_BlitSurface(targs[i].picture, NULL, rv, &(SDL_Rect){.x=targs[i].lon-HALFCITY, .y=targs[i].lat-HALFCITY});
	}
	return(rv);
}

SDL_Surface *render_ac_b(game state)
{
	SDL_Surface *rv=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 256, 256, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!rv)
	{
		fprintf(stderr, "render_ac_b: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(NULL);
	}
	SDL_FillRect(rv, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, ATG_ALPHA_TRANSPARENT&0xff);
	for(unsigned int i=0;i<state.ntargs;i++)
		for(unsigned int j=0;j<state.raid[i].nbombers)
		{
			unsigned int k=state.raid[i].bombers[j];
			unsigned int x=floor(state.bombers[k].lon), y=floor(state.bombers[k].lat);
			if(state.bombers[k].crashed)
				pset(rv, x, y, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
			else if(state.bombers[k].failed)
				pset(rv, x, y, (atg_colour){0, 255, 255, ATG_ALPHA_OPAQUE});
			else if(state.bombers[k].bombed)
				pset(rv, x, y, (atg_colour){127, 255, 63, ATG_ALPHA_OPAQUE});
			else
				pset(rv, x, y, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
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
