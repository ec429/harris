/*
	harris - a strategy game
	Copyright (C) 2012-2023 Edward Cree

	licensed under GPLv2 - see top of harris.c for details
	
	builder: bomber design screen
*/

#include <atg.h>

enum out_row {
	OUT_DIM,
	OUT_WGT,
	OUT_SPD,
	OUT_CRC, /* Ceiling, Range, Climb */
	OUT_RAN, /* Max Range condition */
	OUT_DEF,
	OUT_FSA, /* FAil, SVp, ACcuracy */
	OUT_CST,
	OUT_PROTO,
	OUT_TOOL,
	OUT_NOERR,
	OUT_ERR,

	OUT_ROWS=OUT_ERR+8
};

extern const atg_colour BB_BP_COLOUR, BB_PAPER_COLOUR, BB_INK_COLOUR;

extern atg_element *builder_box;

int builder_rightbox_create(atg_element **ret, char **outbuf, SDL_Surface **bp, atg_colour bgcolour);
struct bomber;
void builder_update_m2v(const struct bomber *b, char **outbuf);
