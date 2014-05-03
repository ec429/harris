/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	widgets: defines custom libatg widgets
*/
#include <atg.h>
#include "types.h"

atg_element *create_priority_selector(unsigned int *prio); // create a priority selector widget.  prio is put in userdata
atg_element *create_filter_switch(SDL_Surface *icon, int *value); // create a filter switch widget.  value is put in userdata
atg_element *create_load_selector(bombertype *type, unsigned int *load); // create a bombload selector widget.  load is put in userdata
