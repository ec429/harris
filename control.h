/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	control: the raid control screen
*/

#include <atg.h>

#include "types.h"
#include "date.h"
#include "globals.h"

extern atg_element *control_box;

void game_preinit(game *state); // post-load and -setup, but pre-SCRN_CONTROL entry

void clear_crew(game *state, unsigned int i);
void clear_sqn(game *state, unsigned int i);
void fill_flights(game *state);
bool ensure_crewed(game *state, unsigned int i);
void fixup_crew_assignments(game *state, unsigned int i, bool kill, double wskill);

static inline bool is_pff(const game *state, unsigned int i)
{
	if(state->bombers[i].squadron<0)
		return(false);
	unsigned int s=state->bombers[i].squadron;
	return(base_grp(bases[state->squads[s].base])==8);
}

extern int filter_nav[NNAVAIDS], filter_elite, filter_student;
extern bool filter_marks[MAX_MARKS], filter_groups[7];
