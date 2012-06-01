#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "atg/atg.h"
#include <SDL_image.h>

#include "bits.h"
#include "weather.h"

typedef struct
{
	unsigned int year;
	unsigned int month;
	unsigned int day;
}
date;

#define NNAVAIDS	4
const char * const navaids[NNAVAIDS]={"GEE","H2S","OBOE","GH"};

typedef struct
{
	//MANUFACTURER:NAME:COST:SPEED:CAPACITY:SVP:DEFENCE:FAILURE:ACCURACY:DD-MM-YYYY:DD-MM-YYYY:NAVAIDS
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
	SDL_Surface *picture;
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
	unsigned int *nbombers;
	// TODO routing
}
raid;

typedef struct
{
	date now;
	unsigned int hour;
	unsigned int ntypes;
	unsigned int *nbombers, *nsvble;
	w_state weather;
	unsigned int ntargs;
	double *dmg, *flk;
	raid *raids;
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
int pset(SDL_Surface *s, unsigned int x, unsigned int y, atg_colour c);
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
				// MANUFACTURER:NAME:COST:SPEED:CAPACITY:SVP:DEFENCE:FAILURE:ACCURACY:DD-MM-YYYY:DD-MM-YYYY:NAVAIDS
				this.name=strdup(next); // guarantees that enough memory will be allocated
				this.manu=(char *)malloc(strcspn(next, ":")+1);
				ssize_t db;
				int e;
				if((e=sscanf(next, "%[^:]:%[^:]:%u:%u:%u:%u:%u:%u:%u:%zn", this.manu, this.name, &this.cost, &this.speed, &this.cap, &this.svp, &this.defn, &this.fail, &this.accu, &db))!=9)
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
				char pn[4+nlen+4];
				strcpy(pn, "art/");
				for(size_t p=0;p<=nlen;p++) pn[4+p]=tolower(this.name[p]);
				strcat(pn, ".png");
				if(!(this.picture=IMG_Load(pn)))
				{
					fprintf(stderr, "Failed to load %s: %s\n", pn, IMG_GetError());
					return(1);
				}
				types=(bombertype *)realloc(types, (ntypes+1)*sizeof(bombertype));
				types[ntypes]=this;
				ntypes++;
			}
			next=strtok(NULL, "\n");
		}
		free(typefile);
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
	atg_element *GB_btrow[ntypes], *GB_btnum[ntypes];
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
		GB_btrow[i]->clickable=true;
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
			fprintf(stderr, "Missing manu or name in type %d\n", i);
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
	atg_element *GB_raid=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){63, 63, 67, ATG_ALPHA_OPAQUE});
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
	
	game state;
	state.ntypes=ntypes;
	if(!(state.nbombers=malloc(ntypes*sizeof(*state.nbombers))))
	{
		perror("malloc");
		return(1);
	}
	if(!(state.nsvble=malloc(ntypes*sizeof(*state.nsvble))))
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
		if(!(state.raids[i].nbombers=malloc(ntypes*sizeof(*state.raids[i].nbombers))))
		{
			perror("malloc");
			return(1);
		}
		for(unsigned int j=0;j<ntypes;j++)
			state.raids[i].nbombers[j]=0;
	}
	
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
	for(unsigned int i=0;i<ntypes;i++)
	{
		if(GB_btrow[i])
			GB_btrow[i]->hidden=((diffdate(types[i].entry, state.now)>0)||(diffdate(types[i].exit, state.now)<0));
		if(GB_btnum[i]&&GB_btnum[i]->elem.label&&GB_btnum[i]->elem.label->text)
			snprintf(GB_btnum[i]->elem.label->text, 12, "%u/%u", state.nsvble[i], state.nbombers[i]);
	}
	for(unsigned int i=0;i<ntargs;i++)
	{
		if(GB_ttdmg[i])
			GB_ttdmg[i]->w=floor(state.dmg[i]);
		if(GB_ttflk[i])
			GB_ttflk[i]->w=floor(state.flk[i]);
	}
	SDL_FreeSurface(city_overlay);
	city_overlay=render_cities(ntargs, targs);
	SDL_BlitSurface(city_overlay, NULL, GB_map->elem.image->data, NULL);
	SDL_FreeSurface(weather_overlay);
	weather_overlay=render_weather(state.weather);
	SDL_BlitSurface(weather_overlay, NULL, GB_map->elem.image->data, NULL);
	int seltarg=-1;
	while(1)
	{
		for(unsigned int i=0;i<ntargs;i++)
		{
			if(GB_ttrow[i]&&GB_ttrow[i]->elem.box)
			{
				GB_ttrow[i]->elem.box->bgcolour=(false?(atg_colour){191, 103, 95, ATG_ALPHA_OPAQUE}:(atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
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
					if(c.e)
					{
						for(unsigned int i=0;i<ntypes;i++)
						{
							if(c.e==GB_btrow[i])
							{
								fprintf(stderr, "btrow %d\n", i);
							}
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
							}
						}
					}
				break;
				default:
					fprintf(stderr, "e.type %d\n", e.type);
				break;
			}
		}
		SDL_Delay(50);
		/*for(unsigned int it=0;it<8;it++)
			w_iter(&state.weather, lorw);
		SDL_FreeSurface(GB_map->elem.image->data);
		GB_map->elem.image->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
		//SDL_FillRect(GB_map->elem.image->data, &(SDL_Rect){.x=0, .y=0, .w=256, .h=256}, SDL_MapRGB(GB_map->elem.image->data->format, 0, 0, 0));
		SDL_FreeSurface(weather_overlay);
		weather_overlay=render_weather(state.weather);
		SDL_BlitSurface(weather_overlay, NULL, GB_map->elem.image->data, NULL);
		if(seltarg>=0)
			SDL_BlitSurface(location, NULL, GB_map->elem.image->data, &(SDL_Rect){.x=targs[seltarg].lon, .y=targs[seltarg].lat});
		*/
	}
	
	do_exit:
	canvas->box=NULL;
	atg_free_canvas(canvas);
	atg_free_box(mainbox);
	atg_free_box(gamebox);
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
						unsigned int j;
						unsigned int snbombers, snsvble; // don't read 'snsvble' as 'sensible', or you'll just get confused.  It's savefile (s) number (n) serviceable (svble).
						f=sscanf(line, "Type %u:%u/%u\n", &j, &snbombers, &snsvble);
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
						state->nbombers[i]=snbombers;
						state->nsvble[i]=snsvble;
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
			pset(rv, x, y, (atg_colour){255, 255, 255, cl});
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
