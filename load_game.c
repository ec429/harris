/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	load_game: the "Load Game" screen
*/

#include <stdio.h>

#include "load_game.h"
#include "ui.h"
#include "globals.h"
#include "saving.h"

#define NAMEBUFLEN	256

atg_element *load_game_box;
char *LB_btext;
atg_element *LB_file, *LB_text, *LB_full, *LB_exit, *LB_load;

int load_game_create(void)
{
	load_game_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 47, ATG_ALPHA_OPAQUE});
	if(!load_game_box)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	LB_file=atg_create_element_filepicker("Load Game", NULL, (atg_colour){239, 239, 255, ATG_ALPHA_OPAQUE}, (atg_colour){31, 31, 47, ATG_ALPHA_OPAQUE});
	if(!LB_file)
	{
		fprintf(stderr, "atg_create_element_filepicker failed\n");
		return(1);
	}
	LB_file->h=mainsizey-30;
	LB_file->w=mainsizex;
	if(atg_ebox_pack(load_game_box, LB_file))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *LB_hbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE});
	if(!LB_hbox)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	else if(atg_ebox_pack(load_game_box, LB_hbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	else
	{
		LB_load=atg_create_element_button("Load", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE});
		if(!LB_load)
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		LB_load->w=34;
		if(atg_ebox_pack(LB_hbox, LB_load))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		LB_text=atg_create_element_button_empty((atg_colour){239, 255, 239, ATG_ALPHA_OPAQUE}, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE});
		if(!LB_text)
		{
			fprintf(stderr, "atg_create_element_button_empty failed\n");
			return(1);
		}
		LB_text->h=24;
		LB_text->w=mainsizex-64;
		if(atg_ebox_pack(LB_hbox, LB_text))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(LB_btext=malloc(NAMEBUFLEN)))
		{
			perror("malloc");
			return(1);
		}
		atg_element *label=atg_create_element_label_nocopy(LB_btext, 12, (atg_colour){239, 255, 239, ATG_ALPHA_OPAQUE});
		if(!label)
		{
			fprintf(stderr, "atg_create_element_label_nocopy failed\n");
			return(1);
		}
		if(atg_ebox_pack(LB_text, label))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		LB_btext[0]=0;
		LB_full=atg_create_element_image(fullbtn);
		if(!LB_full)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		LB_full->w=16;
		LB_full->clickable=true;
		if(atg_ebox_pack(LB_hbox, LB_full))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		LB_exit=atg_create_element_image(exitbtn);
		if(!LB_exit)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		LB_exit->clickable=true;
		if(atg_ebox_pack(LB_hbox, LB_exit))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	return(0);
}

screen_id load_game_screen(atg_canvas *canvas, game *state)
{
	atg_event e;
	
	atg_filepicker *lbf=LB_file->elemdata;
	if(!lbf)
	{
		fprintf(stderr, "Error: LB_file->elemdata==NULL\n");
		return(SCRN_MAINMENU);
	}
	free(lbf->value);
	lbf->value=NULL;
	while(1)
	{
		LB_file->w=mainsizex;
		LB_file->h=mainsizey-30;
		LB_text->w=mainsizex-64;
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
							return(SCRN_MAINMENU);
						case SDL_VIDEORESIZE:
							mainsizex=canvas->surface->w;
							mainsizey=canvas->surface->h;
						break;
					}
				break;
				case ATG_EV_CLICK:;
					atg_ev_click c=e.event.click;
					if(c.e==LB_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
					}
					else if (c.e==LB_exit)
						return(SCRN_MAINMENU);
					else if(c.e)
					{
						fprintf(stderr, "Clicked on unknown clickable!\n");
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==LB_load)
					{
						if(!lbf->curdir)
						{
							fprintf(stderr, "Error: lbf->curdir==NULL\n");
						}
						else if(!lbf->value)
						{
							fprintf(stderr, "Select a file first!\n");
						}
						else
						{
							char *file=malloc(strlen(lbf->curdir)+strlen(lbf->value)+1);
							sprintf(file, "%s%s", lbf->curdir, lbf->value);
							fprintf(stderr, "Loading game state from '%s'...\n", file);
							int rc=loadgame(file, state, lorw);
							free(file);
							if(!rc)
							{
								fprintf(stderr, "Game loaded\n");
								return(SCRN_CONTROL);
							}
							else
							{
								fprintf(stderr, "Failed to load from save file\n");
							}
						}
					}
					else if(trigger.e==LB_text)
					{
						LB_btext[0]=0;
					}
					else if(!trigger.e)
					{
						// internal error
					}
					else
						fprintf(stderr, "Clicked on unknown button!\n");
				break;
				case ATG_EV_VALUE:;
					atg_ev_value value=e.event.value;
					if(value.e==LB_file)
					{
						if(lbf->value)
							snprintf(LB_btext, NAMEBUFLEN, "%s", lbf->value);
						else
							LB_btext[0]=0;
					}
				break;
				default:
				break;
			}
		}
		SDL_Delay(50);
	}
}

void load_game_free(void)
{
	atg_free_element(load_game_box);
}
