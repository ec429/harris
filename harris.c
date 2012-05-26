#include <stdio.h>
#include <stdbool.h>

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

int loadgame(const char *fn, game *state);
date readdate(const char *t, date nulldate);
int ntypes=0;
int ntargs=0;

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
	
	atg_box *gamebox=atg_create_box(ATG_BOX_PACK_VERTICAL, (atg_colour){63, 47, 0, ATG_ALPHA_OPAQUE});
	if(!gamebox)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	atg_box *GB_btrow[ntypes];
	for(int i=0;i<ntypes;i++)
	{
		atg_element *e;
		if(!(e=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_pack_element(gamebox, e))
		{
			perror("atg_pack_element");
			return(1);
		}
		e->clickable=true;
		e->w=120;
		if(!(GB_btrow[i]=e->elem.box))
		{
			fprintf(stderr, "GB_btrow[%d] is NULL\n", i);
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
		if(atg_pack_element(GB_btrow[i], picture))
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
						if(!loadgame("save/qstart.sav", &state))
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
					if(c.e&&(c.e->type==ATG_BOX))
					{
						atg_box *b=c.e->elem.box;
						if(b)
						{
							for(int i=0;i<ntypes;i++)
							{
								if(b==GB_btrow[i])
								{
									fprintf(stderr, "btrow %d\n", i);
								}
							}
						}
					}
				break;
				default:
					fprintf(stderr, "e.type %d\n", e.type);
				break;
			}
		}
	}
	
	do_exit:
	canvas->box=NULL;
	atg_free_canvas(canvas);
	atg_free_box(mainbox);
	atg_free_box(gamebox);
	return(0);
}

date readdate(const char *t, date nulldate)
{
	date d;
	if(t&&*t&&(sscanf(t, "%u-%u-%u", &d.day, &d.month, &d.year)==3)) return(d);
	return(nulldate);
}

int loadgame(const char *fn, game *state)
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
				w_init(&state->weather, 16);
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
