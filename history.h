#pragma once
/*
	harris - a strategy game
	Copyright (C) 2012-2013 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	history: logging of game events
*/
#include <stdio.h>
#include <stddef.h>
#include "bits.h"
#include "date.h"

typedef struct
{
	size_t nents;
	size_t nalloc;
	char (*ents)[80];
}
history;

char *hist_alloc(history *hist); // Create a new history line, return a pointer for writing
int hist_append(history *hist, const char line[80]); // Append a line to the history
int hist_save(history hist, FILE *out); // Write history out to file
int hist_load(FILE *in, size_t nents, history *hist); // Read history in from file.  Not a mirror of hist_save, since it doesn't read nents itself (this is for reasons related to how loadgame functions)

int ev_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type, const char *ev); // Append an unspecified event to the history
int ct_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type); // Append a CT (constructed) event to the history
int na_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type, unsigned int nid); // Append a NA (navaid) event to the history
int pf_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type); // Append a PF (PFF assign) event to the history
int ra_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type, unsigned int tid); // Append a RA (raid targ) event to the history
int hi_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type, unsigned int bmb); // Append a HI (hit targ) event to the history
int dmac_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type, double ddmg, acid src); // Append a DM AC (damaged by aircraft) event to the history
int dmfk_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type, double ddmg, unsigned int fid); // Append a DM FK (damaged by flak) event to the history
int fa_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type); // Append a FA (failed) event to the history
int cr_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type); // Append a CR (crashed) event to the history
