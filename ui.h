/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	ui: header for the user interface
*/

#include "types.h"

#define GAME_BG_COLOUR	(atg_colour){31, 31, 15, ATG_ALPHA_OPAQUE}

#define default_w		800
#define default_h		720

#define RS_cell_w		112
#define RS_firstcol_w	128
#define RS_cell_h		56
#define RS_lastrow_h	100

typedef enum
{
	SCRN_MAINMENU,
	SCRN_LOADGAME,
	SCRN_SAVEGAME,
	SCRN_CONTROL,
	SCRN_RUNRAID,
	SCRN_RRESULTS,
	NUM_SCREENS,
}
screen_id;

struct screen
{
	const char *name;
	int (*create)(void);
	screen_id (*func)(atg_canvas *, game *);
	void (*free)(void);
	atg_box *box;
};

int main_menu_create(void);
int load_game_create(void);
int save_game_create(void);
int control_create(void);
int run_raid_create(void);
int raid_results_create(void);

screen_id main_menu_screen(atg_canvas *, game *);
screen_id load_game_screen(atg_canvas *, game *);
screen_id save_game_screen(atg_canvas *, game *);
screen_id control_screen(atg_canvas *, game *);
screen_id run_raid_screen(atg_canvas *, game *);
screen_id raid_results_screen(atg_canvas *, game *);

void main_menu_free(void);
void load_game_free(void);
void save_game_free(void);
void control_free(void);
void run_raid_free(void);
void raid_results_free(void);

extern atg_box *main_menu_box, *load_game_box, *save_game_box, *control_box, *run_raid_box, *raid_results_box;

void update_navbtn(game state, atg_element *(*GB_navbtn)[NNAVAIDS], unsigned int i, unsigned int n, SDL_Surface *grey_overlay, SDL_Surface *yellow_overlay);
void message_box(atg_canvas *canvas, const char *titletext, const char *bodytext, const char *signtext);
