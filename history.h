#pragma once
/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	history: logging of game events
*/
#include <stdio.h>
#include <stddef.h>
#include "types.h"

char *hist_alloc(history *hist); // Create a new history line, return a pointer for writing
int hist_append(history *hist, const char line[HIST_LINE]); // Append a line to the history
int hist_clear(history *hist); // Empty the history (freeing all events)
int hist_save(history hist, FILE *out); // Write history out to file
int hist_load(FILE *in, size_t nents, history *hist); // Read history in from file.  Not a mirror of hist_save, since it doesn't read nents itself (this is for reasons related to how loadgame functions)

int eva_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type, const char *ev); // Append an aircraft event to the history
int ct_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type); // Append a CT (constructed) event to the history
int na_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type, unsigned int nid); // Append a NA (navaid) event to the history
int pf_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type); // Append a PF (PFF assign) event to the history
int ra_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type, unsigned int tid); // Append a RA (raid targ) event to the history
int hi_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type, unsigned int tid, unsigned int bmb); // Append a HI (hit targ) event to the history
int dmac_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type, double ddmg, double cdmg, acid src); // Append a DM AC (damaged by aircraft) event to the history
int dmfk_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type, double ddmg, double cdmg, unsigned int fid); // Append a DM FK (damaged by flak) event to the history
int dmtf_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type, double ddmg, double cdmg, unsigned int tid); // Append a DM TF (damaged by target flak) event to the history
int fa_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type, unsigned int fa); // Append a FA (failed) event to the history
int cr_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type); // Append a CR (crashed) event to the history
int ob_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type); // Append an OB (obsolete) event to the history

int evt_append(history *hist, date d, harris_time t, unsigned int tid, const char *ev); // Append a target event to the history
int tdm_append(history *hist, date d, harris_time t, unsigned int tid, double ddmg, double cdmg); // Append a DM (damaged or repaired) event to the history
int tfk_append(history *hist, date d, harris_time t, unsigned int tid, double dflk, double cflk); // Append a FK (flak-change) event to the history
int tsh_append(history *hist, date d, harris_time t, unsigned int tid); // Append a SH (ship sunk) event to the history

int evm_append(history *hist, date d, harris_time t, const char *ev); // Append a miscellaneous event to the history
int ca_append(history *hist, date d, harris_time t, unsigned int cshr, unsigned int cash); // Append a CA (cash) event to the history
int co_append(history *hist, date d, harris_time t, double confid); // Append a CO (confid) event to the history
int mo_append(history *hist, date d, harris_time t, double morale); // Append a MO (morale) event to the history
int gp_append(history *hist, date d, harris_time t, unsigned int iclass, double gprod, double dprod); // Append a GP (GProd) event to the history
