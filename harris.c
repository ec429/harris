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

typedef struct
{
	//MANUFACTURER:NAME:COST:SPEED:CAPACITY:SVP:DEFENCE:FAILURE:ACCURACY:DD-MM-YYYY:DD-MM-YYYY
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
	SDL_Surface *picture;
}
bombertype;

typedef struct
{
	//NAME:PROD:FLAK:ESIZ:LAT:LONG:DD-MM-YYYY:DD-MM-YYYY
	unsigned int id;
	char * name;
	unsigned int prod, flak, esiz, lat, lon;
	date entry, exit;
}
target;

typedef struct
{
	date now;
	unsigned int hour;
	unsigned int ntypes;
	unsigned int *nbombers, *nsvble;
	w_state weather;
	unsigned int ntargs;
	double *dmg, *flk;
}
game;

#define VER_MAJ	0
#define VER_MIN	1
#define VER_REV	0

int loadgame(const char *fn, game *state, bool lorw[128][128]);
date readdate(const char *t, date nulldate);
int diffdate(date date1, date date2); // returns <0 if date1<date2, >0 if date1>date2, 0 if date1==date2
SDL_Surface *render_weather(w_state weather);
int pset(SDL_Surface *s, unsigned int x, unsigned int y, atg_colour c);
int ntypes=0;
int ntargs=0;
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
				// MANUFACTURER:NAME:COST:SPEED:CAPACITY:SVP:DEFENCE:FAILURE:ACCURACY:DD-MM-YYYY:DD-MM-YYYY
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
				this.exit=readdate(exit, (date){9999, 99, 99});
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
				// NAME:PROD:FLAK:ESIZ:LAT:LONG:DD-MM-YYYY:DD-MM-YYYY
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
				this.exit=readdate(exit, (date){99, 99, 9999});
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
	for(int i=0;i<ntypes;i++)
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
	atg_element *GB_map=atg_create_element_image(map);
	if(!GB_map)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	if(atg_pack_element(gamebox, GB_map))
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
	for(int i=0;i<ntargs;i++)
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
		if(!(GB_ttflk[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){159, 159, 151, ATG_ALPHA_OPAQUE})))
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
	}
	
	gameloop:
	canvas->box=gamebox;
	atg_resize_canvas(canvas, 640, 480);
	for(int i=0;i<ntypes;i++)
	{
		if(GB_btrow[i])
			GB_btrow[i]->hidden=((diffdate(types[i].entry, state.now)>0)||(diffdate(types[i].exit, state.now)<0));
		if(GB_btnum[i]&&GB_btnum[i]->elem.label&&GB_btnum[i]->elem.label->text)
			snprintf(GB_btnum[i]->elem.label->text, 12, "%u/%u", state.nsvble[i], state.nbombers[i]);
	}
	for(int i=0;i<ntargs;i++)
	{
		if(GB_ttdmg[i])
			GB_ttdmg[i]->w=floor(state.dmg[i]);
		if(GB_ttflk[i])
			GB_ttflk[i]->w=floor(state.flk[i]);
	}
	SDL_Surface *weather_overlay=render_weather(state.weather);
	SDL_BlitSurface(weather_overlay, NULL, GB_map->elem.image->data, NULL);
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
				case ATG_EV_CLICK:;
					atg_ev_click c=e.event.click;
					if(c.e)
					{
						for(int i=0;i<ntypes;i++)
						{
							if(c.e==GB_btrow[i])
							{
								fprintf(stderr, "btrow %d\n", i);
							}
						}
						for(int i=0;i<ntargs;i++)
						{
							if(c.e==GB_ttrow[i])
							{
								fprintf(stderr, "ttrow %d\n", i);
								SDL_FreeSurface(GB_map->elem.image->data);
								GB_map->elem.image->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
								SDL_BlitSurface(location, NULL, GB_map->elem.image->data, &(SDL_Rect){.x=targs[i].lon, .y=targs[i].lat});
								SDL_BlitSurface(weather_overlay, NULL, GB_map->elem.image->data, NULL);
							}
						}
					}
				break;
				default:
					fprintf(stderr, "e.type %d\n", e.type);
				break;
			}
		}
		/*for(unsigned int it=0;it<8;it++)
			w_iter(&state.weather, lorw);
		SDL_FreeSurface(GB_map->elem.image->data);
		GB_map->elem.image->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
		//SDL_FillRect(GB_map->elem.image->data, &(SDL_Rect){.x=0, .y=0, .w=256, .h=256}, SDL_MapRGB(GB_map->elem.image->data->format, 0, 0, 0));
		SDL_FreeSurface(weather_overlay);
		weather_overlay=render_weather(state.weather);
		SDL_BlitSurface(weather_overlay, NULL, GB_map->elem.image->data, NULL);*/
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
		unsigned char s_version[4]={0,0,0,0};
		unsigned char version[4]={VER_REV,VER_MIN,VER_MAJ,0};
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
				f=sscanf(dat, "%hhu.%hhu.%hhu\n", s_version+2, s_version+1, s_version);
				if(f!=3)
				{
					fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
					e|=1;
				}
				if(*(long *)s_version>*(long *)version)
				{
					fprintf(stderr, "Warning - file is newer version than program;\n may cause strange behaviour\n");
				}
			}
			else if(*(long *)s_version==0)
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
				int sntypes;
				f=sscanf(dat, "%d\n", &sntypes);
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
					for(int i=0;i<ntypes;i++)
					{
						free(line);
						line=fgetl(fs);
						if(!line)
						{
							fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
							e|=64;
							break;
						}
						int j;
						unsigned int snbombers, snsvble; // don't read 'snsvble' as 'sensible', or you'll just get confused.  It's savefile (s) number (n) serviceable (svble).
						f=sscanf(line, "Type %d:%u/%u\n", &j, &snbombers, &snsvble);
						if(f!=3)
						{
							fprintf(stderr, "1 Too few arguments to part %d of tag \"%s\"\n", i, tag);
							e|=1;
							break;
						}
						if(j!=i)
						{
							fprintf(stderr, "4 Index mismatch in part %d (%d?) of tag \"%s\"\n", i, j, tag);
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
				int sntargs;
				f=sscanf(dat, "%d\n", &sntargs);
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
					for(int i=0;i<ntargs;i++)
					{
						free(line);
						line=fgetl(fs);
						if(!line)
						{
							fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
							e|=64;
							break;
						}
						int j;
						f=sscanf(line, "Targ %d:%la,%la\n", &j, &state->dmg[i], &state->flk[i]);
						if(f!=3)
						{
							fprintf(stderr, "1 Too few arguments to part %d of tag \"%s\"\n", i, tag);
							e|=1;
							break;
						}
						if(j!=i)
						{
							fprintf(stderr, "4 Index mismatch in part %d (%d?) of tag \"%s\"\n", i, j, tag);
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
	for(unsigned int x=0;x<256;x++)
	{
		for(unsigned int y=0;y<256;y++)
		{
			Uint8 cl=min(max(floor(1016-weather.p[x>>1][y>>1])*8.0, 0), 255);
			pset(rv, x, y, (atg_colour){255, 255, 255, cl});
			/*double p=weather.p[x>>1][y>>1];
			if(p>1004)
				pset(rv, x, y, (atg_colour){0, 0, 0, ATG_ALPHA_TRANSPARENT});
			else if(p>996)
			{
				Uint8 cl=floor(255-(1004-p)*6);
				pset(rv, x, y, (atg_colour){cl, cl, cl, 255-cl});
			}
			else if(p>988)
			{
				Uint8 cl=floor(255-48-(996-p)*8);
				pset(rv, x, y, (atg_colour){cl, cl, cl, 255-48-104-(cl>>1)});
			}
			else
			{
				Uint8 cl=max(floor(255-48-64-(988-p)*3), 0);
				pset(rv, x, y, (atg_colour){cl, cl, cl, 255-cl});
			}
			*/
		}
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
