/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	main_menu: the main menu screen
*/

#include <stdio.h>

#include "main_menu.h"
#include "ui.h"
#include "globals.h"
#include "saving.h"

atg_box *main_menu_box;
atg_element *MM_full, *MM_Exit, *MM_QuickStart, *MM_LoadGame;

int main_menu_create(void)
{
	if(!(main_menu_box=atg_create_box(ATG_BOX_PACK_VERTICAL, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	atg_element *MM_topbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
	if(!MM_topbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(main_menu_box, MM_topbox))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_box *tb=MM_topbox->elem.box;
	if(!tb)
	{
		fprintf(stderr, "MM_topbox->elem.box==NULL\n");
		return(1);
	}
	atg_element *MM_title=atg_create_element_label("HARRIS: Main Menu", 12, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
	if(!MM_title)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_pack_element(tb, MM_title))
	{
		perror("atg_pack_element");
		return(1);
	}
	MM_full=atg_create_element_image(fullbtn);
	if(!MM_full)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	MM_full->clickable=true;
	if(atg_pack_element(tb, MM_full))
	{
		perror("atg_pack_element");
		return(1);
	}
	MM_QuickStart=atg_create_element_button("Quick Start Game", (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE});
	if(!MM_QuickStart)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_pack_element(main_menu_box, MM_QuickStart))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *MM_NewGame=atg_create_element_button("Set Up New Game", (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE});
	if(!MM_NewGame)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_pack_element(main_menu_box, MM_NewGame))
	{
		perror("atg_pack_element");
		return(1);
	}
	MM_LoadGame=atg_create_element_button("Load Game", (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE});
	if(!MM_LoadGame)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_pack_element(main_menu_box, MM_LoadGame))
	{
		perror("atg_pack_element");
		return(1);
	}
	MM_Exit=atg_create_element_button("Exit", (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE});
	if(!MM_Exit)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_pack_element(main_menu_box, MM_Exit))
	{
		perror("atg_pack_element");
		return(1);
	}
	MM_QuickStart->w=MM_NewGame->w=MM_LoadGame->w=MM_Exit->w=136;
	return(0);
}

screen_id main_menu_screen(atg_canvas *canvas, game *state)
{
	atg_resize_canvas(canvas, 136, 86);
	atg_event e;
	while(1)
	{
		atg_flip(canvas);
		while(atg_poll_event(&e, canvas))
		{
			switch(e.type)
			{
				case ATG_EV_RAW:;
					SDL_Event s=e.event.raw;
					switch(s.type)
					{
						case SDL_QUIT:
							return(NUM_SCREENS);
					}
				break;
				case ATG_EV_CLICK:;
					atg_ev_click c=e.event.click;
					if(c.e==MM_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
					}
					else if(c.e)
					{
						fprintf(stderr, "Clicked on unknown clickable!\n");
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==MM_Exit)
						return(NUM_SCREENS);
					else if(trigger.e==MM_QuickStart)
					{
						fprintf(stderr, "Loading game state from Quick Start file...\n");
						if(!loadgame("save/qstart.sav", state, lorw))
						{
							fprintf(stderr, "Quick Start Game loaded\n");
							return(SCRN_CONTROL);
						}
						else
						{
							fprintf(stderr, "Failed to load Quick Start save file\n");
						}
					}
					else if(trigger.e==MM_LoadGame)
					{
						return(SCRN_LOADGAME);
					}
					else if(!trigger.e)
					{
						// internal error
					}
					else
						fprintf(stderr, "Clicked on unknown button!\n");
				break;
				default:
				break;
			}
		}
		SDL_Delay(50);
	}
}

void main_menu_free(void)
{
	atg_free_box_box(main_menu_box);
}
