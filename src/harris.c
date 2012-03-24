#include <stdio.h>
//#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h> // tells us what endianism we are

#include "../inc/draw.h"
#include "../inc/dialogs.h"
#include "../inc/weather.h"

typedef struct
{
	int year;
	int month;
	int day;
}
date;

typedef struct
{
	//ID:MANUFACTURER:NAME:COST:SPEED:CAPACITY:SVP:DEFENCE:FAILURE:ACCURACY:DD-MM-YYYY:DD-MM-YYYY
	int id;
	char * manu;
	char * name;
	int cost;
	int speed;
	int cap;
	int svp;
	int defn;
	int fail;
	int accu;
	date entry;
	date exit;
}
bombertype;

typedef struct
{
	//ID:NAME:PROD:FLAK:ESIZ:LAT:LONG:DD-MM-YYYY:DD-MM-YYYY
	int id;
	char * name;
	int prod, flak, esiz, lat, lon;
	date entry, exit;
	double dmg;
	double flk;
}
target;

#define VER_MAJ	0
#define VER_MIN	1
#define VER_REV	0

bool little_endian;

char * slurp(FILE *);
double decdbl(char *);
void test_endian(void);

int main(void)
{
	test_endian();
	// init some GUI stuff
	TTF_Init();
	atexit(TTF_Quit);
	TTF_Font *harris_font=TTF_OpenFont(FONT_FILE, 12);
	TTF_Font *big_font=TTF_OpenFont(FONT_FILE, 36);
	SDL_Surface * screen = gf_init(OSIZ_X, OSIZ_Y);
	SDL_Surface * arthurharris = IMG_Load("img/arthurharris.jpg");
	if(!arthurharris)
	{
		fprintf(stderr, "Failed to read splash screen image!\n");
		fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
		dtext(screen, 8, 8, "Harris", harris_font, 192, 192, 192);
		dtext(screen, 8, 20, "loading datafiles...", harris_font, 192, 192, 192);
		SDL_Flip(screen);
	}
	else
	{
		// Draws the image on the screen:
		SDL_Rect rcDest = {(OSIZ_X-491)/2, 0, 0, 0}; // 491x600
		SDL_BlitSurface(arthurharris, NULL, screen, &rcDest);
		SDL_FreeSurface(arthurharris);
		dtext(screen, (OSIZ_X-90)/2, 8, "Harris", big_font, 224, 192, 32);
		dtext(screen, (OSIZ_X-128)/2, 44, "Bomber Command WWII", harris_font, 224, 192, 0);
		SDL_Flip(screen);
		sleep(1);
	}
	
	SDL_Surface * button_u = IMG_Load("img/button_u.png");
	SDL_Surface * button_p = IMG_Load("img/button_p.png");
	SDL_Surface * small_button_u = IMG_Load("img/small_button_u.png");
	SDL_Surface * small_button_p = IMG_Load("img/small_button_p.png");
	SDL_Surface * box_small = IMG_Load("img/box.png");
	SDL_Surface * box_large = IMG_Load("img/box_large.png");
	if(!button_u || !button_p || !small_button_u || !small_button_p || !box_small || !box_large)
	{
		fprintf(stderr, "Failed to read images!\n");
		fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	
	// Load data files
	FILE * typefp = fopen("dat/bombers", "r");
	bombertype * types = NULL;
	int ntypes=0;
	if(typefp==NULL)
	{
		fprintf(stderr, "Failed to open data file 'bombers'!\n");
		char *boxtextlines[] = {"Failed to open data file 'bombers'!", "Harris will now exit."};
		okbox2(screen, box_large, boxtextlines, 2, small_button_u, small_button_p, harris_font, big_font, "OK", 24, 48, 96, 48, 96, 192);
		return(1);
	}
	else
	{
		char * typefile = slurp(typefp);
		char * next = strtok(typefile, "\n");
		while(next!=NULL)
		{
			if(*next!='#')
			{
				bombertype this;
				char entry[80], exit[80];
				// ID:MANUFACTURER:NAME:COST:SPEED:CAPACITY:SVP:DEFENCE:FAILURE:ACCURACY:DD-MM-YYYY:DD-MM-YYYY
				this.name=(char *)malloc(80); // this is bad, because it could be crashed by a long name.  However, if no-one fiddles with the data files it's ok; it will do as an interim
				this.manu=(char *)malloc(80);
				sscanf(next, "%u:%s:%s:%u:%u:%u:%u:%u:%u:%u:%79s:%79s", &this.id, this.name, this.manu, &this.cost, &this.speed, &this.cap, &this.svp, &this.defn, &this.fail, &this.accu, entry, exit);
				if(*entry==0)
				{
					this.entry.day=this.entry.month=this.entry.year=0;
				}
				else
				{
					sscanf(entry, "%u-%u-%u", &this.entry.day, &this.entry.month, &this.entry.year);
				}
				if(*exit==0)
				{
					this.exit.day=this.exit.month=this.exit.year=9999;
				}
				else
				{
					sscanf(entry, "%u-%u-%u", &this.exit.day, &this.exit.month, &this.exit.year);
				}
				types = (bombertype *)realloc(types, (ntypes+1)*sizeof(bombertype));
				types[ntypes]=this;
				ntypes++;
			}
			next=strtok(NULL, "\n");
		}
		free(typefile);
		fprintf(stderr, "Loaded %u bomber types\n", ntypes);
	}
	
	FILE * targfp = fopen("dat/targets", "r");
	target * targs = NULL;
	int ntargs=0;
	if(targfp==NULL)
	{
		fprintf(stderr, "Failed to open data file 'targets'!\n");
		char *boxtextlines[] = {"Failed to open data file 'targets'!", "Harris will now exit."};
		okbox2(screen, box_large, boxtextlines, 2, small_button_u, small_button_p, harris_font, big_font, "OK", 24, 48, 96, 48, 96, 192);
		return(1);
	}
	else
	{
		char * targfile = slurp(targfp);
		char * next = strtok(targfile, "\n");
		while(next!=NULL)
		{
			if(*next!='#')
			{
				target this;
				char entry[80], exit[80];
				// ID:NAME:PROD:FLAK:ESIZ:LAT:LONG:DD-MM-YYYY:DD-MM-YYYY
				this.name=(char *)malloc(80); // this is bad, because it could be crashed by a long name.  However, if no-one fiddles with the data files it's ok; it will do as an interim
				sscanf(next, "%u:%s:%u:%u:%u:%u:%u:%79s:%79s", &this.id, this.name, &this.prod, &this.flak, &this.esiz, &this.lat, &this.lon, entry, exit);
				if(*entry==0)
				{
					this.entry.day=this.entry.month=this.entry.year=0;
				}
				else
				{
					sscanf(entry, "%u-%u-%u", &this.entry.day, &this.entry.month, &this.entry.year);
				}
				if(*exit==0)
				{
					this.exit.day=this.exit.month=this.exit.year=9999;
				}
				else
				{
					sscanf(entry, "%u-%u-%u", &this.exit.day, &this.exit.month, &this.exit.year);
				}
				this.dmg=100;
				this.flk=this.flak;
				targs = (target *)realloc(targs, (ntargs+1)*sizeof(target));
				targs[ntargs]=this;
				ntargs++;
			}
			next=strtok(NULL, "\n");
		}
		free(targfile);
		fprintf(stderr, "Loaded %u targets\n", ntargs);
	}
	
	// okbox(screen, box_small, "Test", small_button_u, small_button_p, harris_font, big_font, "OK", 24, 48, 96, 48, 96, 192);
	
	// SDL stuff
	SDL_WM_SetCaption("Harris", "Harris");
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_Event event;
	SDL_Rect cls;
	cls.x=0;
	cls.y=0;
	cls.w=OSIZ_X;
	cls.h=OSIZ_Y;
	int errupt = 0;
	
	char button;
	pos mouse;
	char drag=0, dold=drag;
	
	int state=0; // main menu
	
	/* game state vars */
	int day,month,year;
	int hour;
	int nbombers[ntypes], nsvble[ntypes];
	w_state weather;
	// targets state is in targs[].dmg, .flk
	
	while(!errupt)
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
		SDL_Flip(screen);
		
		dold=drag;
		
		while(SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					errupt++;
				break;
				case SDL_KEYDOWN:
					if(event.key.type==SDL_KEYDOWN)
					{
						SDL_keysym key=event.key.keysym;
						if(key.sym==SDLK_q)
						{
							errupt++;
						}
						/*
						the ascii character is:
						if ((key.unicode & 0xFF80) == 0)
						{
							// it's (char)keysym.unicode & 0x7F;
						}
						else
						{
							// it's not [low] ASCII
						}
						*/
					}
				break;
				case SDL_MOUSEMOTION:
					mouse.x=event.motion.x;
					mouse.y=event.motion.y;
				break;
				case SDL_MOUSEBUTTONDOWN:
					mouse.x=event.button.x;
					mouse.y=event.button.y;
					button=event.button.button;
					drag|=button;
					switch(button)
					{
						case SDL_BUTTON_LEFT:
						break;
						case SDL_BUTTON_RIGHT:
						break;
						case SDL_BUTTON_WHEELUP:
						break;
						case SDL_BUTTON_WHEELDOWN:
						break;
					}
				break;
				case SDL_MOUSEBUTTONUP:
					mouse.x=event.button.x;
					mouse.y=event.button.y;
					button=event.button.button;
					drag&=~button;
					switch(button)
					{
						case SDL_BUTTON_LEFT:
						break;
						case SDL_BUTTON_RIGHT:
						break;
						case SDL_BUTTON_WHEELUP:
						break;
						case SDL_BUTTON_WHEELDOWN:
						break;
					}
				break;
			}
		}
		SDL_Delay(18);
	}

	// clean up
	TTF_CloseFont(harris_font);
	if(SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);
	return(0);
}

char * slurp(FILE *fp)
{
	fseek(fp, 0, SEEK_SET);
	// slurps a filefull of textual data, {re}alloc()ing as it goes, so you don't need to make a buffer for it, nor must thee fret thyself about overruns!
	char * lout = (char *)malloc(80);
	int i=0;
	int c;
	while(!feof(fp))
	{
		c = fgetc(fp);
		if(c == EOF)
			break;
		if (c != 0)
			lout[i++]=(char)c;
		if ((i%80) == 0)
		{
			if ((lout = (char *)realloc(lout, i+80))==NULL)
			{
				free(lout);
				fprintf(stderr, "\nNot enough memory to store input!\n");
				return(NULL);
			}
		}
	}
	lout[i]=0;
	lout=(char *)realloc(lout, i+1);
	return(lout);
}

double decdbl(char * ptr)
{
	if(ptr==NULL)
	{
		return(nan(""));
	}
	char conv[8];
	int i;
	for(i=0;i<8;i++)
	{
		if(!(ptr[i*2]&&ptr[i*2+1]))
			return(nan(""));
		char c[3];
		c[0]=ptr[i*2];
		c[1]=ptr[i*2+1];
		c[2]=0;
		unsigned char h;
		sscanf(c, "%hhx", &h);
		if(little_endian)
			conv[7-i]=h;
		else
			conv[i]=h;
	}
	double rv = *(double *)conv;
	return(rv);
}

void test_endian(void)
{
	#ifdef __BYTE_ORDER
	# if __BYTE_ORDER == __LITTLE_ENDIAN
		little_endian=true;
		return;
	# else
	#  if __BYTE_ORDER == __BIG_ENDIAN
		little_endian=false;
		return;
	#  endif
	# endif
	#endif /* __BYTE_ORDER */
	long one=1;
	little_endian=(*((char *)(&one)));
}
