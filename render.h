/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	render: functions to render to maps
*/

#include <SDL.h>
#include "types.h"

SDL_Surface *render_weather(w_state weather);
SDL_Surface *render_targets(date now);
SDL_Surface *render_flak(date now);
SDL_Surface *render_ac(const game *state);
SDL_Surface *render_xhairs(const game *state, int seltarg);
