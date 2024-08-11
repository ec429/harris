/*
	harris - a strategy game
	Copyright (C) 2012-2020 Edward Cree

	licensed under GPLv2 - see top of harris.c for details

	handle_manfs: screen for tasking manufacturers' R&D
*/

#include <atg.h>

#include "types.h"

extern atg_element *handle_manfs_box;

int assign_slots(game *state, struct bomber *b);
void realise_design(const game *state, struct bomber *b);
