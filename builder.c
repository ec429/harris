/*
	harris - a strategy game
	Copyright (C) 2012-2023 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	builder: bomber design screen
*/

#include "builder.h"
#include <stdbool.h>
#include "ui.h"
#include "globals.h"
#include "bits.h"
#include "date.h"

atg_element *builder_box;
atg_element *BB_full, *BB_cont;
/*atg_element **IB_types, **IB_namebox, *IB_side_image, *IB_text_box, *IB_stat_box, *IB_crew_box;
unsigned int IB_i, IB_showmark;
char *IB_mark_buf;
atg_element *IB_mark_box, *IB_mark_lbl, *IB_mark_prev, *IB_mark_noprev, *IB_mark_next, *IB_mark_nonext;
atg_element *IB_breakdown_box, *IB_breakdown_row[MAX_MARKS], *IB_tw_spin[MAX_MARKS];
unsigned int IB_mark_count[MAX_MARKS][2];
char *IB_mark_count_buf[MAX_MARKS];
SDL_Surface *IB_blank;*/

int builder_create(void)
{
	if(!(builder_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR)))
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	atg_element *top_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
	if(!top_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(builder_box, top_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *title=atg_create_element_label("HARRIS: Aircraft Design", 12, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!title)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(top_box, title))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(BB_cont=atg_create_element_button("Continue", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(top_box, BB_cont))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_full=atg_create_element_image(fullbtn);
	if(!BB_full)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	BB_full->clickable=true;
	if(atg_ebox_pack(top_box, BB_full))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *main_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!main_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	main_box->w=default_w;
	main_box->h=default_h-18;
	if(atg_ebox_pack(builder_box, main_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *left_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!left_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(main_box, left_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	/* TODO: M, E, E#, E*, E+, W, F, B, L, N, U, G */
	atg_element *mid_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!mid_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(main_box, mid_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	/* TODO: pic, outputs, actions */
	atg_element *right_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!right_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(main_box, right_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	/* TODO: T, T*, C, C* */
	return(0);
}

screen_id builder_screen(atg_canvas *canvas, game *state)
{
	(void)state;
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
							return(SCRN_CONTROL);
					}
				break;
				case ATG_EV_CLICK:;
					atg_ev_click c=e.event.click;
					if(c.e==BB_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
					}
					else
					{
						fprintf(stderr, "Clicked on unknown clickable!\n");
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==BB_cont)
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
				case ATG_EV_VALUE:;
					//atg_ev_value v=e.event.value;
					if(false)
					{
						//
					}
					else
					{
						fprintf(stderr, "Clicked on unknown spinner!\n");
					}
				break;
				default:
				break;
			}
		}
		SDL_Delay(50);
	}
}

void builder_free(void)
{
	atg_free_element(builder_box);
}
