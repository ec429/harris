/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	setup_difficulty: screen for new game difficulty settings
*/

#include <stdio.h>
#include <unistd.h>

#include "setup_difficulty.h"
#include "ui.h"
#include "globals.h"

atg_element *setup_difficulty_box;
atg_element *SD_full, *SD_exit, *SD_back, *SD_cont;
atg_element *SD_diff[DIFFICULTY_CLASSES][3];

#define SD_BG_COLOUR	(atg_colour){31, 31, 47, ATG_ALPHA_OPAQUE}

int setup_difficulty_create(void)
{
	setup_difficulty_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, SD_BG_COLOUR);
	if(!setup_difficulty_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	atg_element *SD_topbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, SD_BG_COLOUR);
	if(!SD_topbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(setup_difficulty_box, SD_topbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *SD_title=atg_create_element_label("Set Up New Game: Difficulty Settings", 12, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
	if(!SD_title)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(SD_topbox, SD_title))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	SD_full=atg_create_element_image(fullbtn);
	if(!SD_full)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	SD_full->clickable=true;
	if(atg_ebox_pack(SD_topbox, SD_full))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	SD_exit=atg_create_element_image(exitbtn);
	if(!SD_exit)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	SD_exit->clickable=true;
	if(atg_ebox_pack(SD_topbox, SD_exit))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *SD_headrow=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, SD_BG_COLOUR);
	if(!SD_headrow)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(setup_difficulty_box, SD_headrow))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *head_name=atg_create_element_label("Category", 12, (atg_colour){247, 247, 247, ATG_ALPHA_OPAQUE});
	if(!head_name)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	head_name->w=160;
	if(atg_ebox_pack(SD_headrow, head_name))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *head_desc=atg_create_element_label("Description", 12, (atg_colour){247, 247, 247, ATG_ALPHA_OPAQUE});
	if(!head_desc)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	head_desc->w=480;
	if(atg_ebox_pack(SD_headrow, head_desc))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *head_level=atg_create_element_label("Difficulty setting", 12, (atg_colour){247, 247, 247, ATG_ALPHA_OPAQUE});
	if(!head_level)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	head_level->w=48*3;
	if(atg_ebox_pack(SD_headrow, head_level))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	char *level_names[3]={"Easy", "Medium", "Hard"};
	atg_colour level_colours[3]={{127, 247, 127, ATG_ALPHA_OPAQUE}, {223, 223, 127, ATG_ALPHA_OPAQUE}, {247, 127, 127, ATG_ALPHA_OPAQUE}};
	for(unsigned int d=0;d<DIFFICULTY_CLASSES;d++)
	{
		atg_element *SD_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, SD_BG_COLOUR);
		if(!SD_row)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(setup_difficulty_box, SD_row))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *row_name=atg_create_element_label(dclasses[d].name, 11, (atg_colour){239, 239, 223, ATG_ALPHA_OPAQUE});
		if(!row_name)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		row_name->w=160;
		if(atg_ebox_pack(SD_row, row_name))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *row_desc=atg_create_element_label(dclasses[d].desc, 11, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE});
		if(!row_desc)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		row_desc->w=480;
		if(atg_ebox_pack(SD_row, row_desc))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		for(unsigned int i=0;i<3;i++)
		{
			if(!(SD_diff[d][i]=atg_create_element_toggle(level_names[i], false, level_colours[i], SD_BG_COLOUR)))
			{
				fprintf(stderr, "atg_create_element_toggle failed\n");
				return(1);
			}
			SD_diff[d][i]->w=48;
			if(atg_ebox_pack(SD_row, SD_diff[d][i]))
			{
				perror("atg_ebox_pack");
				return(1);
			}
		}
	}
	atg_element *SD_nav=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, SD_BG_COLOUR);
	if(!SD_nav)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(setup_difficulty_box, SD_nav))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(SD_back=atg_create_element_button("Go Back", (atg_colour){239, 195, 195, ATG_ALPHA_OPAQUE}, (atg_colour){71, 63, 63, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(SD_nav, SD_back))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(SD_cont=atg_create_element_button("Continue", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(SD_nav, SD_cont))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	return(0);
}

screen_id setup_difficulty_screen(atg_canvas *canvas, game *state)
{
	atg_event e;
	bool mixed_difficulty=false;
	for(unsigned int d=1;d<DIFFICULTY_CLASSES;d++)
		if(state->difficulty[d]!=state->difficulty[0])
		{
			mixed_difficulty=true;
			break;
		}
	
	while(1)
	{
		for(unsigned int d=0;d<DIFFICULTY_CLASSES;d++)
		{
			unsigned int dv=state->difficulty[d];
			for(unsigned int i=0;i<3;i++)
			{
				atg_toggle *t=SD_diff[d][i]->elemdata;
				t->state=(dv==i)&&(d||!mixed_difficulty);
			}
		}
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
					if(c.e==SD_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
					}
					else if (c.e==SD_exit)
						return(SCRN_MAINMENU);
					else if(c.e)
					{
						fprintf(stderr, "Clicked on unknown clickable!\n");
					}
				break;
				case ATG_EV_TOGGLE:;
					atg_ev_toggle toggle=e.event.toggle;
					for(unsigned int d=0;d<DIFFICULTY_CLASSES;d++)
						for(unsigned int i=0;i<3;i++)
							if(toggle.e==SD_diff[d][i])
							{
								state->difficulty[d]=i;
								mixed_difficulty=d;
								if(!d)
									for(unsigned int d=1;d<DIFFICULTY_CLASSES;d++)
										state->difficulty[d]=i;
								goto toggle_found;
							}
					fprintf(stderr, "Clicked on unknown toggle!\n");
					toggle_found:
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==SD_cont)
					{
						return(SCRN_SETPTYPS);
					}
					else if(trigger.e==SD_back)
					{
						return(SCRN_SETPGAME);
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

void setup_difficulty_free(void)
{
	atg_free_element(setup_difficulty_box);
}
