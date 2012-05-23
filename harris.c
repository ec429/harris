#include <stdio.h>
#include <stdbool.h>

#include "atg/atg.h"

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

date readdate(const char *t, date nulldate);

int main(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[])
{
	atg_canvas *canvas=atg_create_canvas(120, 86, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
	if(!canvas)
	{
		fprintf(stderr, "atg_create_canvas failed\n");
		return(1);
	}
	
	// Load data files
	fprintf(stderr, "Loading data files...\n");
	FILE *typefp=fopen("dat/bombers", "r");
	bombertype *types=NULL;
	int ntypes=0;
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
				if((e=sscanf(next, "%[^:]:%[^:]:%u:%u:%u:%u:%u:%u:%u:%zn", this.name, this.manu, &this.cost, &this.speed, &this.cap, &this.svp, &this.defn, &this.fail, &this.accu, &db))!=9)
				{
					fprintf(stderr, "Malformed `bombers' line `%s'\n", next);
					fprintf(stderr, "  sscanf returned %d\n", e);
					return(1);
				}
				this.name=realloc(this.name, strlen(this.name)+1);
				this.entry=readdate(next+db, (date){0, 0, 0});
				const char *exit=strchr(next+db, ':');
				this.exit=readdate(exit, (date){99, 99, 9999});
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
	int ntargs=0;
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
	atg_element *QuickStart=atg_create_element_button("Quick Start Game", (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE});
	if(!QuickStart)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_pack_element(mainbox, QuickStart))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *NewGame=atg_create_element_button("Set Up New Game", (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE});
	if(!NewGame)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_pack_element(mainbox, NewGame))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *LoadGame=atg_create_element_button("Load Game", (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE});
	if(!LoadGame)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_pack_element(mainbox, LoadGame))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *Exit=atg_create_element_button("Exit", (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE});
	if(!Exit)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_pack_element(mainbox, Exit))
	{
		perror("atg_pack_element");
		return(1);
	}
	QuickStart->w=NewGame->w=LoadGame->w=Exit->w=canvas->surface->w;
	
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
	
	int errupt=0;
	atg_event e;
	while(!errupt)
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
							errupt++;
						break;
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==Exit)
						errupt++;
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
	
	/*while(!errupt)
	{
		//SDL_FillRect(screen, &cls, SDL_MapRGB(screen->format, 0, 0, 0));
		switch(state)
		{
			case 0: // State 0: Main Menu.  Successors: 1, 2, 3, exit.
				SDL_FillRect(screen, &cls, SDL_MapRGB(screen->format, 0, 0, 0));
				dtext(screen, (OSIZ_X-90)/2, 8, "Harris", big_font, 224, 192, 32);
				int pressed=-1, hover=-1;
				char * mainmenu[] = {"Quick Start New Game", "Set Up New Game", "Load Saved Game", "Exit"};
				if((mouse.x>=95) && (mouse.x<=705) && (mouse.y>=64) && (mouse.y<=292) && ((mouse.y-64)%59<=51)) // lots of hardcoded constants and stuff, should really fix this at some point (refactor?)
				{
					hover=(mouse.y-64)/59;
					if(drag & SDL_BUTTON_LEFT)
					{
						pressed=hover;
					}
					else if(dold & SDL_BUTTON_LEFT)
					{
						switch(hover)
						{
							case 0:
								state=1; // Quick Start New Game: go to quickstart
							break;
							case 1:
								state=2; // Set Up New Game: go to game settings page
							break;
							case 2:
								state=3; // Load Saved Game: go to file selection page
							break;
							case 3:
								errupt++; // Exit: end the main loop
							break;
							default:
								fprintf(stderr, "No such button %d in mainmenu!\n", hover);
							break;
						}
					}
				}
				dmenu(screen, 400, 64, 4, pressed, hover, mainmenu, big_font, button_u, button_p, 8, 16, 32, 16, 32, 64);
			break;
			case 1: // State 1: Load in quickstart data.  Successors: 0,4.
				{
					char *boxtextlines[] = {" Loading quickstart data", "", " Please wait..."};
					showbox(screen, box_large, boxtextlines, 3, harris_font, 12, 24, 96);
				}
				// TODO: refactor all this loading-code into a function, so we can re-use it for eg. state 3
				// CAVEAT: We may need to change the 'Weather rand:' tag handling, since it shouldn't be present in savegames unless trickery has occurred.
				state=4; // If nothing goes wrong, we'll go to main game page when we're done
				FILE *qssav = fopen("save/qstart.sav", "r");
				if(!qssav)
				{
					fprintf(stderr, "Failed to open save/qstart.sav!\n");
					perror("fopen");
					char *boxtextlines[] = {"Failed to open save/qstart.sav!"};
					okbox2(screen, box_large, boxtextlines, 1, small_button_u, small_button_p, harris_font, big_font, "OK", 24, 48, 96, 48, 96, 192);
					state=0; // drop back to main menu
				}
				else
				{
					unsigned char s_version[4]={0,0,0,0};
					unsigned char version[4]={VER_REV,VER_MIN,VER_MAJ,0};
					while((!feof(qssav)) && (state==4))
					{
						char tag[16];
						int i=0;
						while(i<15)
						{
							tag[i]=fgetc(qssav);
							if(feof(qssav))
								break;
							if(tag[i]==':')
							{
								tag[i+1]=0;
								break;
							}
							i++;
						}
						int e=0,f; // poor-man's try...
						if(i==15)
						{
							fprintf(stderr, "64 Delimiter not found or tag too long\n");
							e|=64; // ...throw...
						}
						else if(strcmp(tag, "HARR:")==0)
						{
							f=fscanf(qssav, "%hhu.%hhu.%hhu\n", s_version+2, s_version+1, s_version);
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
						else if(strcmp(tag, "DATE:")==0)
						{
							f=fscanf(qssav, "%d/%d/%d\n", &day, &month, &year);
							if(f!=3)
							{
								fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
								e|=1;
							}
						}
						else if(strcmp(tag, "TIME:")==0)
						{
							f=fscanf(qssav, "%d\n", &hour);
							if(f!=1)
							{
								fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
								e|=1;
							}
						}
						else if(strcmp(tag, "Bombers:")==0)
						{
							int sntypes;
							f=fscanf(qssav, "%d\n", &sntypes);
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
								for(i=0;i<ntypes;i++)
								{
									int j, snbombers, snsvble; // don't read 'snsvble' as 'sensible', or you'll just get confused.  It's savefile (s) number (n) serviceable (svble).
									f=fscanf(qssav, "Type %d:%u/%u\n", &j, &snbombers, &snsvble);
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
									nbombers[i]=snbombers;
									nsvble[i]=snsvble;
								}
							}
						}
						else if(strcmp(tag, "Targets:")==0)
						{
							int sntargs;
							f=fscanf(qssav, "%d\n", &sntargs);
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
								for(i=0;i<ntargs;i++)
								{
									int j;
									char sdmg[17], sflk[17]; // extra byte for the null in each case
									f=fscanf(qssav, "Targ %d:%16s,%16s\n", &j, sdmg, sflk);
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
									targs[i].dmg=decdbl(sdmg);
									targs[i].flk=decdbl(sflk);
								}
							}
						}
						else if(strcmp(tag, "Weather rand:")==0)
						{
							unsigned int seed;
							f=fscanf(qssav, "%u\n", &seed);
							if(f!=1)
							{
								fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
								e|=1;
							}
							srand(seed);
							w_init(&weather, 16);
						}
						else
						{
							fprintf(stderr, "128 Bad tag \"%s\"\n", tag);
							e|=128;
						}
						if(e!=0) // ...catch
						{
							char *boxtextlines[3];
							boxtextlines[0] = (char *)malloc(80); // should be plenty for now, since %d can't expand to very many characters - but keep an eye on this
							sprintf(boxtextlines[0], "Error (%d) reading from", e);
							boxtextlines[1]=" save/qstart.sav!";
							boxtextlines[2]="Dropping back to main menu";
							fprintf(stderr, "%s%s\n", boxtextlines[0], boxtextlines[1]);
							okbox2(screen, box_large, boxtextlines, 3, small_button_u, small_button_p, harris_font, big_font, "OK", 24, 48, 96, 48, 96, 192);
							state=0; // drop back to main menu
							fclose(qssav);
						}
					}
				}
			break;
			default:
				fprintf(stderr, "harris: (fatal) Invalid state %d in main loop!\n", state);
				errupt++;
			break;
		}
	}
	*/
	return(0);
}

date readdate(const char *t, date nulldate)
{
	date d;
	if(t&&*t&&(sscanf(t, "%u-%u-%u", &d.day, &d.month, &d.year)==3)) return(d);
	return(nulldate);
}
