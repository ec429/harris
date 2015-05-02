/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	ui: user interface framework
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
	SCRN_SETPGAME,
	SCRN_SETPDIFF,
	SCRN_SETPTYPS,
	SCRN_LOADGAME,
	SCRN_SAVEGAME,
	SCRN_CONTROL,
	SCRN_RUNRAID,
	SCRN_RRESULTS,
	SCRN_POSTRAID,
	SCRN_INTELBMB,
	SCRN_INTELFTR,
	SCRN_INTELTRG,
	SCRN_HCREWS,
	NUM_SCREENS,
}
screen_id;

struct screen
{
	const char *name;
	int (*create)(void);
	screen_id (*func)(atg_canvas *, game *);
	void (*free)(void);
	atg_element **box;
};

int main_menu_create(void);
int setup_game_create(void);
int setup_difficulty_create(void);
int setup_types_create(void);
int load_game_create(void);
int save_game_create(void);
int control_create(void);
int run_raid_create(void);
int raid_results_create(void);
int post_raid_create(void);
int intel_bombers_create(void);
int intel_fighters_create(void);
int intel_targets_create(void);
int handle_crews_create(void);

screen_id main_menu_screen(atg_canvas *, game *);
screen_id setup_game_screen(atg_canvas *, game *);
screen_id setup_difficulty_screen(atg_canvas *, game *);
screen_id setup_types_screen(atg_canvas *, game *);
screen_id load_game_screen(atg_canvas *, game *);
screen_id save_game_screen(atg_canvas *, game *);
screen_id control_screen(atg_canvas *, game *);
screen_id run_raid_screen(atg_canvas *, game *);
screen_id raid_results_screen(atg_canvas *, game *);
screen_id post_raid_screen(atg_canvas *, game *);
screen_id intel_bombers_screen(atg_canvas *, game *);
screen_id intel_fighters_screen(atg_canvas *, game *);
screen_id intel_targets_screen(atg_canvas *, game *);
screen_id handle_crews_screen(atg_canvas *, game *);

void main_menu_free(void);
void setup_game_free(void);
void setup_difficulty_free(void);
void setup_types_free(void);
void load_game_free(void);
void save_game_free(void);
void control_free(void);
void run_raid_free(void);
void raid_results_free(void);
void post_raid_free(void);
void intel_bombers_free(void);
void intel_fighters_free(void);
void intel_targets_free(void);
void handle_crews_free(void);

extern screen_id intel_caller;
extern SDL_Surface *ttype_icons[TCLASS_INDUSTRY+ICLASS_MIXED+1];

void update_navbtn(game state, atg_element *(*GB_navbtn)[NNAVAIDS], unsigned int i, unsigned int n, SDL_Surface *grey_overlay, SDL_Surface *yellow_overlay);
void message_box(atg_canvas *canvas, const char *titletext, const char *bodytext, const char *signtext);
int msgadd(atg_canvas *canvas, game *state, date when, const char *ref, const char *msg);
