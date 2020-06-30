/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	render: functions to render to maps
*/

#include "render.h"

#include <math.h>
#include <SDL_gfxBlitFunc.h>

#include "almanack.h"
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

SDL_Surface *render_sun(double showtime)
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
			double coords[2], rise, set;
			project_coords(y, x, coords);
			sun_calc(coords, todays_delta, todays_eqn, &rise, &set);
			double morning=(showtime-rise)*12.0/M_PI, evening=(set-showtime)*12.0/M_PI;
			double sunny=max(morning, evening);
			Uint8 cl=min(max(floor((sunny+1.5)*128.0), 0), 256);
			pset(rv, x, y, (atg_colour){cl, cl, 0, 64});
		}
	}
	return(rv);
}

void render_one_route(SDL_Surface *s, const game *state, unsigned int i, bool markers)
{
	if(!state->raids[i].nbombers) return;
	int latl=0, lonl=0;
	for(unsigned int stage=0;stage<8;stage++)
	{
		int lat=targs[i].route[stage][0], lon=targs[i].route[stage][1];
		if(!lat&&!lon) continue;
		if(latl||lonl)
		{
			line(s, lonl, latl, lon, lat, (atg_colour){.r=0, .g=0, .b=0, .a=192});
		}
		else
		{
			for(unsigned int j=0;j<state->raids[i].nbombers;j++)
			{
				unsigned int k=state->raids[i].bombers[j];
				if(state->bombers[k].squadron<0)
					continue;
				unsigned int b=state->squads[state->bombers[k].squadron].base;
				line(s, base_lon(bases[b]), base_lat(bases[b]), lon, lat, (atg_colour){.r=0, .g=0, .b=0, .a=12});
			}
		}
		if(markers)
		{
			for(int dx=-1;dx<2;dx++)
				for(int dy=-1;dy<2;dy++)
					pset(s, lon+dx, lat+dy, (atg_colour){.r=0, .g=0, .b=0, .a=224});
		}
		latl=lat;
		lonl=lon;
	}
	for(unsigned int j=0;j<state->raids[i].nbombers;j++)
	{
		unsigned int k=state->raids[i].bombers[j];
		if(state->bombers[k].squadron<0)
			continue;
		unsigned int b=state->squads[state->bombers[k].squadron].base;
		line(s, lonl, latl, base_lon(bases[b]), base_lat(bases[b]), (atg_colour){.r=0, .g=0, .b=255, .a=12});
	}
}

SDL_Surface *render_routes(const game *state)
{
	if(datebefore(state->now, event[EVENT_GEE])) return(NULL);
	SDL_Surface *rv=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 256, 256, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!rv)
	{
		fprintf(stderr, "render_targets: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(NULL);
	}
	SDL_FillRect(rv, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, ATG_ALPHA_TRANSPARENT&0xff);
	for(unsigned int i=0;i<ntargs;i++)
		render_one_route(rv, state, i, false);
	return(rv);
}

SDL_Surface *render_current_route(const game *state, int seltarg)
{
	if(seltarg<0) return(NULL);
	if(datebefore(state->now, event[EVENT_GEE])) return(NULL);
	SDL_Surface *rv=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 256, 256, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!rv)
	{
		fprintf(stderr, "render_targets: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(NULL);
	}
	SDL_FillRect(rv, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, ATG_ALPHA_TRANSPARENT&0xff);
	render_one_route(rv, state, seltarg, true);
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
				// nothing; they're in render_cities (and we do nothing for LEAFLET anyway)
			break;
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

SDL_Surface *render_cities(void)
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
		if(targs[i].class!=TCLASS_CITY) continue;
		SDL_gfxBlitRGBA(targs[i].picture, NULL, rv, &(SDL_Rect){.x=targs[i].lon-targs[i].picture->w/2, .y=targs[i].lat-targs[i].picture->h/2});
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
		if(!flaks[i].mapped) continue;
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

SDL_Surface *render_ac(const game *state)
{
	SDL_Surface *rv=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 256, 256, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!rv)
	{
		fprintf(stderr, "render_ac: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(NULL);
	}
	SDL_FillRect(rv, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, ATG_ALPHA_TRANSPARENT&0xff);
	for(unsigned int i=0;i<state->ntargs;i++)
		for(unsigned int j=0;j<state->raids[i].nbombers;j++)
		{
			unsigned int k=state->raids[i].bombers[j];
			unsigned int x=floor(state->bombers[k].lon), y=floor(state->bombers[k].lat);
			if(state->bombers[k].crashed)
				pset(rv, x, y, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
			else if(state->bombers[k].damage>6)
				pset(rv, x, y, (atg_colour){255, 127, 63, ATG_ALPHA_OPAQUE});
			else if(state->bombers[k].failed)
				pset(rv, x, y, (atg_colour){0, 255, 255, ATG_ALPHA_OPAQUE});
			else if(state->bombers[k].bombed)
				pset(rv, x, y, (atg_colour){127, 255, 63, ATG_ALPHA_OPAQUE});
			else
				pset(rv, x, y, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
		}
	for(unsigned int i=0;i<state->nfighters;i++)
	{
		unsigned int x=floor(state->fighters[i].lon), y=floor(state->fighters[i].lat);
		if(state->fighters[i].crashed)
			pset(rv, x, y, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
		else if(state->fighters[i].landed)
			pset(rv, x, y, (atg_colour){127, 0, 0, ATG_ALPHA_OPAQUE});
		else if(state->fighters[i].radar)
			pset(rv, x, y, (atg_colour){255, 191, 127, ATG_ALPHA_OPAQUE});
		else
			pset(rv, x, y, (atg_colour){255, 0, 0, ATG_ALPHA_OPAQUE});
	}
	return(rv);
}

SDL_Surface *render_xhairs(const game *state)
{
	SDL_Surface *rv=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 256, 256, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!rv)
	{
		fprintf(stderr, "render_xhairs: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(NULL);
	}
	SDL_FillRect(rv, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, ATG_ALPHA_TRANSPARENT&0xff);
	for(unsigned int i=0;i<state->ntargs;i++)
		if(state->raids[i].nbombers)
		{
			SDL_gfxBlitRGBA(yellowhair, NULL, rv, &(SDL_Rect){.x=targs[i].lon-3, .y=targs[i].lat-3});
		}
	return(rv);
}

SDL_Surface *render_seltarg(int seltarg)
{
	SDL_Surface *rv=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 256, 256, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!rv)
	{
		fprintf(stderr, "render_seltarg: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(NULL);
	}
	SDL_FillRect(rv, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, ATG_ALPHA_TRANSPARENT&0xff);
	if(seltarg>=0)
	{
		SDL_gfxBlitRGBA(location, NULL, rv, &(SDL_Rect){.x=targs[seltarg].lon-3, .y=targs[seltarg].lat-3});
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
	atg_colour c;
	switch(s->format->BytesPerPixel)
	{
		case 1:
		{
			uint8_t pixval = *(uint8_t *)((char *)s->pixels + s_off);
			SDL_GetRGBA(pixval, s->format, &c.r, &c.g, &c.b, &c.a);
		}
		break;			
		case 2:
		{
			uint16_t pixval = *(uint16_t *)((char *)s->pixels + s_off);
			SDL_GetRGBA(pixval, s->format, &c.r, &c.g, &c.b, &c.a);
		}
		break;
		case 4:
		{
			uint32_t pixval = *(uint32_t *)((char *)s->pixels + s_off);
			SDL_GetRGBA(pixval, s->format, &c.r, &c.g, &c.b, &c.a);
		}
		break;
		default:
			fprintf(stderr, "Bad BPP %d, failing!\n", s->format->BytesPerPixel);
			exit(1);
	}
	return(c);
}
