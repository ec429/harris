/*
	harris - a strategy game
	Copyright (C) 2012-2020 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details

	handle_squadrons: screen for managing bases and squadrons
*/

#include <atg.h>

#include "types.h"

extern atg_element *handle_squadrons_box;

bool mixed_base(game *state, unsigned int b, double *eff);
bool mixed_group(game *state, int g, double *eff);
void update_sqn_list(game *state);
