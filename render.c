/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	render: functions to render to maps
*/

#include "render.h"

#include <math.h>
#include <SDL_gfxBlitFunc.h>

#include "bits.h"
#include "globals.h"
#include "date.h"
#include "widgets.h"

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
				pset(rv, x, y, (atg_colour){255, 127, 63, ATG_ALPHA_OPAQUE});
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
		else if(state.fighters[i].radar)
			pset(rv, x, y, (atg_colour){255, 191, 127, ATG_ALPHA_OPAQUE});
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
