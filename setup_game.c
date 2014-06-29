/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	setup_game: the "Set Up New Game" screen
*/

#include <stdio.h>
#include <unistd.h>

#include "setup_game.h"
#include "ui.h"
#include "globals.h"
#include "saving.h"

atg_element *setup_game_box;
atg_element *SG_full, *SG_exit, *SG_start;
atg_element **SG_stitles, *SG_text_box;
char *SG_datestring;

#define SG_BG_COLOUR	(atg_colour){31, 31, 47, ATG_ALPHA_OPAQUE}

int setup_game_create(void)
{
	setup_game_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, SG_BG_COLOUR);
	if(!setup_game_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	atg_element *SG_topbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, SG_BG_COLOUR);
	if(!SG_topbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(setup_game_box, SG_topbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *SG_title=atg_create_element_label("Set Up New Game", 12, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
	if(!SG_title)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(SG_topbox, SG_title))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	SG_full=atg_create_element_image(fullbtn);
	if(!SG_full)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	SG_full->clickable=true;
	if(atg_ebox_pack(SG_topbox, SG_full))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	SG_exit=atg_create_element_image(exitbtn);
	if(!SG_exit)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	SG_exit->clickable=true;
	if(atg_ebox_pack(SG_topbox, SG_exit))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *SG_midbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, SG_BG_COLOUR);
	if(!SG_midbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(setup_game_box, SG_midbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *SG_selbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE});
	if(!SG_selbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(SG_midbox, SG_selbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(SG_stitles=malloc(nstarts*sizeof(atg_element *))))
	{
		perror("malloc");
		return(1);
	}
	for(unsigned int i=0;i<nstarts;i++)
	{
		if(!(SG_stitles[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		atg_element *title=atg_create_element_label(starts[i].title, 16, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE});
		if(!title)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(SG_stitles[i], title))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		SG_stitles[i]->clickable=true;
		SG_stitles[i]->w=480;
		if(atg_ebox_pack(SG_selbox, SG_stitles[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	if(!(SG_datestring=malloc(11)))
	{
		perror("malloc");
		return(1);
	}
	snprintf(SG_datestring, 11, "  -  -    ");
	atg_element *SG_date=atg_create_element_label_nocopy(SG_datestring, 24, (atg_colour){255, 255, 223, ATG_ALPHA_OPAQUE});
	if(!SG_date)
	{
		fprintf(stderr, "atg_create_element_label_nocopy failed\n");
		return(1);
	}
	if(atg_ebox_pack(SG_midbox, SG_date))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *text_guard=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!text_guard)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	text_guard->w=724;
	if(atg_ebox_pack(setup_game_box, text_guard))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *text_shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!text_shim)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	text_shim->w=2;
	text_shim->h=8;
	if(atg_ebox_pack(text_guard, text_shim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(SG_text_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	SG_text_box->w=720;
	if(atg_ebox_pack(text_guard, SG_text_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(SG_start=atg_create_element_button("Start Game!", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(setup_game_box, SG_start))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	return(0);
}

screen_id setup_game_screen(atg_canvas *canvas, game *state)
{
	atg_event e;
	int selstart=-1, i;
	(void)state;
	if(chdir(localdat?cwd:DATIDIR))
	{
		perror("Failed to enter data dir: chdir");
		return(1);
	}
	
	while(1)
	{
		for(i=0;i<(int)nstarts;i++)
		{
			atg_box *tb=SG_stitles[i]->elemdata;
			if(tb)
			{
				if(i==selstart)
					tb->bgcolour=(atg_colour){47, 63, 79, ATG_ALPHA_OPAQUE};
				else
					tb->bgcolour=(atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE};
			}
		}
		atg_ebox_empty(SG_text_box);
		if(selstart>=0)
		{
			snprintf(SG_datestring, 11, "%02u-%02u-%04u", state->now.day, state->now.month, state->now.year);
			const char *bodytext=starts[selstart].description;
			size_t x=0;
			while(bodytext && bodytext[x])
			{
				size_t l=strcspn(bodytext+x, "\n");
				if(!l) break;
				char *t=strndup(bodytext+x, l);
				atg_element *r=atg_create_element_label(t, 10, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
				free(t);
				if(!r)
				{
					fprintf(stderr, "atg_create_element_label failed\n");
					break;
				}
				if(atg_ebox_pack(SG_text_box, r))
				{
					perror("atg_ebox_pack");
					atg_free_element(r);
					break;
				}
				x+=l;
				if(bodytext[x]=='\n') x++;
			}
		}
		else
		{
			snprintf(SG_datestring, 11, "  -  -  ");
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
					for(i=0;i<(int)nstarts;i++)
						if(c.e==SG_stitles[i]) break;
					if(i<(int)nstarts)
					{
						selstart=i;
						fprintf(stderr, "Loading game state from '%s'...\n", starts[i].filename);
						if(loadgame(starts[i].filename, state, lorw))
						{
							fprintf(stderr, "Failed to load from start file\n");
							selstart=-1;
						}
					}
					else if(c.e==SG_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
					}
					else if (c.e==SG_exit)
						return(SCRN_MAINMENU);
					else if(c.e)
					{
						fprintf(stderr, "Clicked on unknown clickable!\n");
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==SG_start)
					{
						return(SCRN_CONTROL);
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

void setup_game_free(void)
{
	atg_free_element(setup_game_box);
}
