/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	load_data: load game entity data
*/

#include "load_data.h"

#include <math.h>
#include <atg.h>
#include <SDL_image.h>

#include "globals.h"
#include "bits.h"
#include "date.h"
#include "events.h"
#include "render.h"
#include "ui.h"
#include "widgets.h"

#ifdef WINDOWS /* I hate having to put in these ugly warts */
#define ssize_t	int
#define zn	"%n"
#else
#define zn	"%zn"
#endif

int load_bombers(void)
{
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
				if((e=sscanf(next, "%[^:]:%[^:]:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:"zn, this.manu, this.name, &this.cost, &this.speed, &this.alt, &this.cap, &this.svp, &this.defn, &this.fail, &this.accu, &this.range, &this.blat, &this.blon, &db))!=13)
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
				this.extra=strstr(nav, "EXTRA");
				for(unsigned int l=0;l<NBOMBLOADS;l++)
					this.load[l]=strstr(nav, bombloads[l].name);
				char pn[256];
				strcpy(pn, "art/bombers/");
				for(size_t p=0;p<nlen;p++)
				{
					if(12+p>=255)
					{
						pn[12+p]=0;
						break;
					}
					pn[12+p]=tolower(this.name[p]);
				}
				strncat(pn, ".png", 256);
				if(!(this.picture=IMG_Load(pn))&&!this.extra)
				{
					fprintf(stderr, "Failed to load %s: %s\n", pn, IMG_GetError());
					return(1);
				}
				char sn[256];
				strcpy(sn, "art/large/bombers/");
				for(size_t p=0;p<nlen;p++)
				{
					if(18+p>=255)
					{
						pn[18+p]=0;
						break;
					}
					sn[18+p]=tolower(this.name[p]);
				}
				strncat(sn, "-side.png", 256);
				if(!(this.side_image=IMG_Load(sn))&&!this.extra)
				{
					fprintf(stderr, "Failed to load %s: %s\n", sn, IMG_GetError());
					return(1);
				}
				this.prio=2;
				this.pribuf=0;
				this.pcbuf=0;
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
	return(0);
}

int load_fighters(void)
{
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
				if((e=sscanf(next, "%[^:]:%[^:]:%u:%u:%u:%u:%hhu:"zn, this.manu, this.name, &this.cost, &this.speed, &this.arm, &this.mnv, &this.radpri, &db))!=7)
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
				char pn[256];
				strcpy(pn, "art/fighters/");
				for(size_t p=0;p<nlen;p++)
				{
					if(13+p>=255)
					{
						pn[13+p]=0;
						break;
					}
					pn[13+p]=tolower(this.name[p]);
				}
				strncat(pn, ".png", 256);
				if(!(this.picture=IMG_Load(pn)))
				{
					fprintf(stderr, "Failed to load %s: %s\n", pn, IMG_GetError());
					return(1);
				}
				char sn[256];
				strcpy(sn, "art/large/fighters/");
				for(size_t p=0;p<nlen;p++)
				{
					if(19+p>=255)
					{
						pn[19+p]=0;
						break;
					}
					sn[19+p]=tolower(this.name[p]);
				}
				strncat(sn, "-side.png", 256);
				if(!(this.side_image=IMG_Load(sn)))
				{
					fprintf(stderr, "Failed to load %s: %s\n", sn, IMG_GetError());
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
	return(0);
}

int load_ftrbases(void)
{
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
				if((e=sscanf(next, "%u:%u:"zn, &this.lat, &this.lon, &db))!=2)
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
	return(0);
}

int load_locations(void)
{
	FILE *locfp=fopen("dat/locations", "r");
	if(!locfp)
	{
		fprintf(stderr, "Failed to open data file `locations'!\n");
		return(1);
	}
	else
	{
		char *locfile=slurp(locfp);
		fclose(locfp);
		char *next=locfile?strtok(locfile, "\n"):NULL;
		while(next)
		{
			if(*next!='#')
			{
				locxn this;
				// NAME:LAT:LONG:RADIUS
				this.name=(char *)malloc(strcspn(next, ":")+1);
				int e;
				if((e=sscanf(next, "%[^:]:%u:%u:%u", this.name, &this.lat, &this.lon, &this.radius))!=4)
				{
					fprintf(stderr, "Malformed `locations' line `%s'\n", next);
					fprintf(stderr, "  sscanf returned %d\n", e);
					return(1);
				}
				locs=(locxn *)realloc(locs, (nlocs+1)*sizeof(locxn));
				locs[nlocs]=this;
				nlocs++;
			}
			next=strtok(NULL, "\n");
		}
		free(locfile);
		fprintf(stderr, "Loaded %u locations\n", nlocs);
	}
	return(0);
}

int load_targets(void)
{
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
				if((e=sscanf(next, "%[^:]:%u:%u:%u:%u:%u:"zn, this.name, &this.prod, &this.flak, &this.esiz, &this.lat, &this.lon, &db))!=6)
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
				targs=(target *)realloc(targs, (ntargs+1)*sizeof(target));
				targs[ntargs]=this;
				ntargs++;
				if(this.class==TCLASS_CITY)
				{
					locs=(locxn *)realloc(locs, (nlocs+1)*sizeof(locxn));
					locs[nlocs]=(locxn){.name=strdup(this.name), .lat=this.lat, .lon=this.lon, .radius=8+sqrt(this.esiz)+sqrt(this.prod)};
					nlocs++;
				}
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
					if(targs[t].city<0)
					{
						fprintf(stderr, "Error: TCLASS_LEAFLET not linked to its CITY\n");
						fprintf(stderr, "\t(targs[%u].name == \"%s\")\n", t, targs[t].name);
						return(1);
					}
					(targs[t].picture=targs[targs[t].city].picture)->refcount++;
					targs[t].psiz=targs[targs[t].city].psiz;
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
	return(0);
}

int load_flaksites(void)
{
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
				if((e=sscanf(next, "%u:%u:%u:"zn, &this.strength, &this.lat, &this.lon, &db))!=3)
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
	return(0);
}

int load_events(void)
{
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
	return(0);
}

int load_texts(void)
{
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
		free(txfile);
	}
	return(0);
}

int load_intel(void)
{
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
		free(intfile);
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
	return(0);
}

int load_images(void)
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
	if(!(nointelbtn=IMG_Load("art/no-intel.png")))
	{
		fprintf(stderr, "Greyed-out Intel icon: IMG_Load: %s\n", IMG_GetError());
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
	if(!(tick=IMG_Load("art/tick.png")))
	{
		fprintf(stderr, "Tick icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(cross=IMG_Load("art/cross.png")))
	{
		fprintf(stderr, "Cross icon: IMG_Load: %s\n", IMG_GetError());
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
	if(!(ttype_icons[TCLASS_CITY]=IMG_Load("art/tclass/city.png")))
	{
		fprintf(stderr, "TClass City icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(ttype_icons[TCLASS_SHIPPING]=IMG_Load("art/tclass/shipping.png")))
	{
		fprintf(stderr, "TClass Shipping icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(ttype_icons[TCLASS_MINING]=IMG_Load("art/tclass/mining.png")))
	{
		fprintf(stderr, "TClass Mining icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(ttype_icons[TCLASS_LEAFLET]=IMG_Load("art/tclass/leaflet.png")))
	{
		fprintf(stderr, "TClass Leaflet icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(ttype_icons[TCLASS_AIRFIELD]=IMG_Load("art/tclass/airfield.png")))
	{
		fprintf(stderr, "TClass Airfield icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(ttype_icons[TCLASS_BRIDGE]=IMG_Load("art/tclass/bridge.png")))
	{
		fprintf(stderr, "TClass Bridge icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(ttype_icons[TCLASS_ROAD]=IMG_Load("art/tclass/road.png")))
	{
		fprintf(stderr, "TClass Road icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(ttype_icons[TCLASS_INDUSTRY+ICLASS_BB]=IMG_Load("art/tclass/bb.png")))
	{
		fprintf(stderr, "IClass Ball-Bearings icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(ttype_icons[TCLASS_INDUSTRY+ICLASS_OIL]=IMG_Load("art/tclass/oil.png")))
	{
		fprintf(stderr, "IClass Oil icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(ttype_icons[TCLASS_INDUSTRY+ICLASS_RAIL]=IMG_Load("art/tclass/rail.png")))
	{
		fprintf(stderr, "IClass Rail icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(ttype_icons[TCLASS_INDUSTRY+ICLASS_UBOOT]=IMG_Load("art/tclass/uboot.png")))
	{
		fprintf(stderr, "IClass U-boats icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(ttype_icons[TCLASS_INDUSTRY+ICLASS_ARM]=IMG_Load("art/tclass/arm.png")))
	{
		fprintf(stderr, "IClass Armament icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(ttype_icons[TCLASS_INDUSTRY+ICLASS_STEEL]=IMG_Load("art/tclass/steel.png")))
	{
		fprintf(stderr, "IClass Steel icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(ttype_icons[TCLASS_INDUSTRY+ICLASS_AC]=IMG_Load("art/tclass/aircraft.png")))
	{
		fprintf(stderr, "IClass Aircraft icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(ttype_icons[TCLASS_INDUSTRY+ICLASS_RADAR]=IMG_Load("art/tclass/radar.png")))
	{
		fprintf(stderr, "IClass Radar icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
	}
	if(!(ttype_icons[TCLASS_INDUSTRY+ICLASS_MIXED]=IMG_Load("art/tclass/mixed.png")))
	{
		fprintf(stderr, "IClass Mixed icon: IMG_Load: %s\n", IMG_GetError());
		return(1);
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
	
	return(0);
}

int load_starts(void)
{
	FILE *stfp=fopen("dat/starts", "r");
	if(!stfp)
	{
		fprintf(stderr, "Failed to open data file `starts'!\n");
		return(1);
	}
	else
	{
		char *line;
		unsigned int n, state=0;
		string description=null_string();
		while((line=fgetl(stfp)))
		{
			if(*line!='#')
			{
				if(strncmp(line, "==", 2)==0)
				{
					if(state==4)
					{
						starts[n].description=description.buf;
						description=null_string();
					}
					else if(state)
					{
						fprintf(stderr, "Malformed `starts' line `%s'\n", line);
						fprintf(stderr, "  Unexpected ==start, state=%u\n", state);
						return(1);
					}
					n=nstarts;
					if(!(starts=realloc(starts, ++nstarts*sizeof(startpoint))))
						return(1);
					starts[n].filename=strdup(line+2);
					starts[n].title=NULL;
					starts[n].description=NULL;
					state=1;
				}
				switch(state)
				{
					case 1:
						state=2;
					break;
					case 2:
						starts[n].title=strdup(line);
						state=3;
					break;
					case 3:
						state=4;
						description=init_string();
						if(!*line) continue;
						/* fallthrough */
					case 4:
						append_str(&description, line);
						append_char(&description, '\n');
					break;
				}
			}
			free(line);
		}
		if(state==4)
		{
			starts[n].description=description.buf;
			description=null_string();
		}
		fclose(stfp);
	}
	fprintf(stderr, "Loaded %u startpoints\n", nstarts);
	return(0);
}
