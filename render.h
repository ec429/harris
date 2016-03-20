/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	render: functions to render to maps
*/

#include <SDL.h>
#include "types.h"

SDL_Surface *render_weather(w_state weather);
SDL_Surface *render_sun(double showtime);
SDL_Surface *render_routes(const game *state);
SDL_Surface *render_cities(void);
SDL_Surface *render_targets(date now);
SDL_Surface *render_flak(date now);
SDL_Surface *render_ac(const game *state);
SDL_Surface *render_xhairs(const game *state);
SDL_Surface *render_seltarg(int seltarg);
SDL_Surface *render_current_route(const game *state, int seltarg);

/* Functions for drawing on an SDL_Surface */
int pset(SDL_Surface *s, unsigned int x, unsigned int y, atg_colour c);
int line(SDL_Surface *s, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, atg_colour c);
atg_colour pget(SDL_Surface *s, unsigned int x, unsigned int y);
