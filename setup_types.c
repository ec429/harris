/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	setup_types: screen for new game a/c type selection
*/

#include <stdio.h>
#include <unistd.h>

#include "setup_types.h"
#include "ui.h"
#include "globals.h"
#include "control.h"
#include "intel_bombers.h"

atg_element *setup_types_box;
atg_element *ST_full, *ST_exit, *ST_back, *ST_start;
atg_element **ST_btrow, **ST_btint, **ST_btsel;

#define ST_BG_COLOUR	(atg_colour){31, 31, 47, ATG_ALPHA_OPAQUE}

int setup_types_create(void)
{
	setup_types_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, ST_BG_COLOUR);
	if(!setup_types_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	atg_element *ST_topbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, ST_BG_COLOUR);
	if(!ST_topbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(setup_types_box, ST_topbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *ST_title=atg_create_element_label("Set Up New Game: Aircraft Types Selection", 12, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
	if(!ST_title)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(ST_topbox, ST_title))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	ST_full=atg_create_element_image(fullbtn);
	if(!ST_full)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	ST_full->clickable=true;
	if(atg_ebox_pack(ST_topbox, ST_full))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	ST_exit=atg_create_element_image(exitbtn);
	if(!ST_exit)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	ST_exit->clickable=true;
	if(atg_ebox_pack(ST_topbox, ST_exit))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *bomberbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, ST_BG_COLOUR);
	if(!bomberbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(setup_types_box, bomberbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(ST_btrow=malloc(ntypes*sizeof(atg_element *))))
	{
		perror("malloc");
		return(1);
	}
	if(!(ST_btint=malloc(ntypes*sizeof(atg_element *))))
	{
		perror("malloc");
		return(1);
	}
	if(!(ST_btsel=malloc(ntypes*sizeof(atg_element *))))
	{
		perror("malloc");
		return(1);
	}
	for(unsigned int i=0;i<ntypes;i++)
	{
		atg_colour bg=(atg_colour){63, 71, 63, ATG_ALPHA_OPAQUE};
		if(types[i].extra)
			bg.r=79;
		if(!(ST_btrow[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, bg)))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(bomberbox, ST_btrow[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *btpic=atg_create_element_image(types[i].picture);
		if(!btpic)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		btpic->w=38;
		btpic->h=42;
		btpic->cache=true;
		if(atg_ebox_pack(ST_btrow[i], btpic))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *namebox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, bg);
		if(!namebox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		namebox->h=42;
		namebox->w=160;
		if(atg_ebox_pack(ST_btrow[i], namebox))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *nibox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, bg);
		if(!nibox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(namebox, nibox))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(ST_btint[i]=atg_create_element_image(types[i].text?intelbtn:nointelbtn)))
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		ST_btint[i]->clickable=true;
		ST_btint[i]->w=10;
		ST_btint[i]->h=12;
		if(atg_ebox_pack(nibox, ST_btint[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(types[i].manu&&types[i].name))
		{
			fprintf(stderr, "Missing manu or name in type %u\n", i);
			return(1);
		}
		atg_element *manu=atg_create_element_label(types[i].manu, 10, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
		if(!manu)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		manu->w=151;
		manu->cache=true;
		if(atg_ebox_pack(nibox, manu))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *name=atg_create_element_label(types[i].name, 14, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
		if(!name)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		name->w=144;
		if(atg_ebox_pack(namebox, name))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(ST_btsel[i]=atg_create_element_image(NULL)))
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		ST_btsel[i]->w=36;
		ST_btsel[i]->h=42;
		ST_btsel[i]->clickable=true;
		if(atg_ebox_pack(ST_btrow[i], ST_btsel[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	atg_element *ST_nav=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, ST_BG_COLOUR);
	if(!ST_nav)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(setup_types_box, ST_nav))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(ST_back=atg_create_element_button("Go Back", (atg_colour){239, 195, 195, ATG_ALPHA_OPAQUE}, (atg_colour){71, 63, 63, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(ST_nav, ST_back))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(ST_start=atg_create_element_button("Start Game!", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(ST_nav, ST_start))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	return(0);
}

screen_id setup_types_screen(atg_canvas *canvas, game *state)
{
	atg_event e;
	
	while(1)
	{
		for(unsigned int i=0;i<ntypes;i++)
		{
			if(!ST_btsel[i]) continue;
			atg_image *img=ST_btsel[i]->elemdata;
			if(img)
				img->data=state->btypes[i]?tick:cross;
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
					if(c.e==ST_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
					}
					else if (c.e==ST_exit)
						return(SCRN_MAINMENU);
					else if(c.e)
					{
						unsigned int i;
						for(i=0;i<ntypes;i++)
						{
							if(c.e==ST_btsel[i])
							{
								state->btypes[i]=!state->btypes[i];
								break;
							}
							if(c.e==ST_btint[i])
							{
								intel_caller=SCRN_SETPTYPS;
								IB_i=i;
								return(SCRN_INTELBMB);
							}
						}
						if(i==ntypes)
							fprintf(stderr, "Clicked on unknown clickable!\n");
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==ST_start)
					{
						game_preinit(state);
						return(SCRN_CONTROL);
					}
					else if(trigger.e==ST_back)
					{
						return(SCRN_SETPDIFF);
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

void setup_types_free(void)
{
	for(unsigned int i=0;i<ntypes;i++)
	{
		if(!ST_btsel[i]) continue;
		atg_image *img=ST_btsel[i]->elemdata;
		if(img)
			img->data=NULL;
	}
	atg_free_element(setup_types_box);
}
