/*
	harris - a strategy game
	Copyright (C) 2012-2023 Edward Cree

	licensed under GPLv2 - see top of harris.c for details
	
	builder: bomber design screen
*/

#include "builder.h"
#include <stdbool.h>
#include "ui.h"
#include "globals.h"
#include "bits.h"
#include "date.h"
#include "widgets.h"
#include "builder/data.h"

atg_element *builder_box;
atg_element *BB_full, *BB_cont;
unsigned int selmanf, seleng;
atg_element *BB_manf, *BB_engc, *BB_egg, *BB_over, *BB_eng;
char *BB_manf_buf, *BB_manf_dbuf, *BB_eng_buf, *BB_eng_dbuf, *BB_eng_obuf;
/*atg_element **IB_types, **IB_namebox, *IB_side_image, *IB_text_box, *IB_stat_box, *IB_crew_box;
unsigned int IB_i, IB_showmark;
char *IB_mark_buf;
atg_element *IB_mark_box, *IB_mark_lbl, *IB_mark_prev, *IB_mark_noprev, *IB_mark_next, *IB_mark_nonext;
atg_element *IB_breakdown_box, *IB_breakdown_row[MAX_MARKS], *IB_tw_spin[MAX_MARKS];
unsigned int IB_mark_count[MAX_MARKS][2];
char *IB_mark_count_buf[MAX_MARKS];
SDL_Surface *IB_blank;*/

atg_element *create_manf_selector(unsigned int *manf)
{
	atg_element *rv=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
	if(!rv) return(NULL);
	rv->type="selector";
	rv->match_click_callback=selector_match_click_callback;
	rv->render_callback=selector_render_callback;
	for(unsigned int i=0;i<builder->entities.nmanf;i++)
	{
		const char *name = builder->entities.manf[i]->ident;
		atg_colour fg=(atg_colour){47, 79, 223, ATG_ALPHA_OPAQUE};
		atg_element *btn=atg_create_element_button(name, fg, GAME_BG_COLOUR);
		if(!btn)
		{
			atg_free_element(rv);
			return(NULL);
		}
		if(atg_ebox_pack(rv, btn))
		{
			atg_free_element(btn);
			atg_free_element(rv);
			return(NULL);
		}
	}
	rv->userdata=manf;
	return(rv);
}

atg_element *create_eng_selector(unsigned int *eng)
{
	atg_element *rv=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!rv) return(NULL);
	rv->type="selector";
	rv->match_click_callback=selector_match_click_callback;
	rv->render_callback=selector_render_callback;
	for(unsigned int i=0;i<builder->entities.neng;i++)
	{
		const char *name = builder->entities.eng[i]->ident;
		atg_colour fg=(atg_colour){47, 79, 223, ATG_ALPHA_OPAQUE};
		atg_element *btn=atg_create_element_button(name, fg, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
		if(!btn)
		{
			atg_free_element(rv);
			return(NULL);
		}
		if(atg_ebox_pack(rv, btn))
		{
			atg_free_element(btn);
			atg_free_element(rv);
			return(NULL);
		}
	}
	rv->userdata=eng;
	return(rv);
}

int builder_create(void)
{
	if(!(builder_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR)))
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	atg_element *title=atg_create_element_label("HARRIS: Aircraft Design", 12, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!title)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(builder_box, title))
	{
		perror("atg_ebox_pack");
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
	if(atg_ebox_pack(builder_box, main_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!shim)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	shim->w=2;
	if(atg_ebox_pack(main_box, shim))
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
	left_box->w=298;
	if(atg_ebox_pack(main_box, left_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *manf_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!manf_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	manf_box->h=50;
	if(atg_ebox_pack(left_box, manf_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *manf_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!manf_row)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(manf_box, manf_row))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *manf_lbl=atg_create_element_label("Manufacturer: ", 14, (atg_colour){47, 79, 223, ATG_ALPHA_OPAQUE});
	if(!manf_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(manf_row, manf_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_manf=create_manf_selector(&selmanf);
	if(!BB_manf)
	{
		fprintf(stderr, "create_manf_selector failed\n");
		return(1);
	}
	if(atg_ebox_pack(manf_row, BB_manf))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *manf_tg=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!manf_tg)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	manf_tg->w=left_box->w;
	if(atg_ebox_pack(manf_box, manf_tg))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!shim)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	shim->w=2;
	shim->h=8;
	if(atg_ebox_pack(manf_tg, shim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *manf_tb=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!manf_tb)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	manf_tb->w=manf_tg->w-4;
	if(atg_ebox_pack(manf_tg, manf_tb))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(BB_manf_buf=malloc(32)))
	{
		perror("malloc");
		return(1);
	}
	atg_element *manf_name=atg_create_element_label_nocopy(BB_manf_buf, 11, (atg_colour){31, 31, 31, ATG_ALPHA_OPAQUE});
	if(!manf_name)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(manf_tb, manf_name))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(BB_manf_dbuf=malloc(64)))
	{
		perror("malloc");
		return(1);
	}
	atg_element *manf_desc=atg_create_element_label_nocopy(BB_manf_dbuf, 9, (atg_colour){31, 31, 31, ATG_ALPHA_OPAQUE});
	if(!manf_desc)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(manf_tb, manf_desc))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *eng_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!eng_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	eng_box->h=90;
	if(atg_ebox_pack(left_box, eng_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *engc_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!engc_row)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(eng_box, engc_row))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *eng_lbl=atg_create_element_label("Engines: ", 14, (atg_colour){47, 79, 223, ATG_ALPHA_OPAQUE});
	if(!eng_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(engc_row, eng_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_engc=atg_create_element_spinner(ATG_SPINNER_RIGHTCLICK_STEP10, 1, 8, 1, 1, "%u", (atg_colour){0, 115, 223, ATG_ALPHA_OPAQUE}, (atg_colour){31, 15, 15, ATG_ALPHA_OPAQUE});
	if(!BB_engc)
	{
		fprintf(stderr, "atg_create_element_spinner failed\n");
		return(1);
	}
	if(atg_ebox_pack(engc_row, BB_engc))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!shim)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	shim->w=2;
	shim->h=8;
	if(atg_ebox_pack(engc_row, shim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_egg=atg_create_element_toggle("Power Egg", false, (atg_colour){47, 79, 223, ATG_ALPHA_OPAQUE}, GAME_BG_COLOUR);
	if(!BB_egg)
	{
		fprintf(stderr, "atg_create_element_toggle failed\n");
		return(1);
	}
	if(atg_ebox_pack(engc_row, BB_egg))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_over=atg_create_element_toggle("Overbuild mounts", false, (atg_colour){47, 79, 223, ATG_ALPHA_OPAQUE}, GAME_BG_COLOUR);
	if(!BB_over)
	{
		fprintf(stderr, "atg_create_element_toggle failed\n");
		return(1);
	}
	if(atg_ebox_pack(engc_row, BB_over))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_eng=create_eng_selector(&seleng);
	if(!BB_eng)
	{
		fprintf(stderr, "create_eng_selector failed\n");
		return(1);
	}
	BB_eng->w=300;
	if(atg_ebox_pack(eng_box, BB_eng))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *eng_tg=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!eng_tg)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	eng_tg->w=300;
	if(atg_ebox_pack(eng_box, eng_tg))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!shim)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	shim->w=2;
	shim->h=8;
	if(atg_ebox_pack(eng_tg, shim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *eng_tb=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!eng_tb)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	eng_tb->w=eng_tg->w-4;
	if(atg_ebox_pack(eng_tg, eng_tb))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(BB_eng_buf=malloc(64)))
	{
		perror("malloc");
		return(1);
	}
	atg_element *eng_name=atg_create_element_label_nocopy(BB_eng_buf, 11, (atg_colour){31, 31, 31, ATG_ALPHA_OPAQUE});
	if(!eng_name)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(eng_tb, eng_name))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(BB_eng_dbuf=malloc(64)))
	{
		perror("malloc");
		return(1);
	}
	atg_element *eng_desc=atg_create_element_label_nocopy(BB_eng_dbuf, 9, (atg_colour){31, 31, 31, ATG_ALPHA_OPAQUE});
	if(!eng_desc)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(eng_tb, eng_desc))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(BB_eng_obuf=malloc(64)))
	{
		perror("malloc");
		return(1);
	}
	atg_element *eng_over=atg_create_element_label_nocopy(BB_eng_obuf, 9, (atg_colour){31, 31, 31, ATG_ALPHA_OPAQUE});
	if(!eng_over)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(eng_tb, eng_over))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	/* TODO: W, F, B, L, N, U, G */
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
		const char *manf_name="", *manf_desc="";
		if(selmanf<builder->entities.nmanf)
		{
			manf_name=builder->entities.manf[selmanf]->name;
			manf_desc=builder->entities.manf[selmanf]->desc;
		}
		strncpy(BB_manf_buf, manf_name, 32);
		strncpy(BB_manf_dbuf, manf_desc, 64);
		const char *eng_name="", *eng_manf="", *eng_desc="", *eng_over=NULL;
		if(seleng<builder->entities.neng)
		{
			struct engine *eng=builder->entities.eng[seleng];
			eng_name=eng->name;
			eng_manf=eng->manu;
			eng_desc=eng->desc;
			if(eng->u)
				eng_over=eng->u->name;
		}
		snprintf(BB_eng_buf, 64, "%s %s", eng_manf, eng_name);
		strncpy(BB_eng_dbuf, eng_desc, 64);
		if(eng_over&&((atg_toggle *)BB_over->elemdata)->state)
			snprintf(BB_eng_obuf, 64, "Mounts overbuilt for %s.", eng_over);
		else
			*BB_eng_obuf=0;
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
				case ATG_EV_TOGGLE:;
					//atg_ev_toggle v=e.event.toggle;
					if(false)
					{
						//
					}
					else
					{
						fprintf(stderr, "Clicked on unknown toggle!\n");
					}
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
