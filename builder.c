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
unsigned int selmanf, seleng, selft, selgirth, selesl;
atg_element *BB_manf, *BB_engc, *BB_egg, *BB_over, *BB_eng, *BB_wa, *BB_wr;
atg_element *BB_fuse, *BB_girth, *BB_cap, *BB_csbs, *BB_esl;
char *BB_manf_buf, *BB_manf_dbuf, *BB_eng_buf, *BB_eng_dbuf, *BB_eng_obuf;
char *BB_fuse_dbuf, *BB_girth_dbuf, *BB_esl_dbuf;
/*atg_element **IB_types, **IB_namebox, *IB_side_image, *IB_text_box, *IB_stat_box, *IB_crew_box;
unsigned int IB_i, IB_showmark;
char *IB_mark_buf;
atg_element *IB_mark_box, *IB_mark_lbl, *IB_mark_prev, *IB_mark_noprev, *IB_mark_next, *IB_mark_nonext;
atg_element *IB_breakdown_box, *IB_breakdown_row[MAX_MARKS], *IB_tw_spin[MAX_MARKS];
unsigned int IB_mark_count[MAX_MARKS][2];
char *IB_mark_count_buf[MAX_MARKS];
SDL_Surface *IB_blank;*/

const atg_colour BB_INFG_COLOUR		= {47, 79, 223, ATG_ALPHA_OPAQUE},
		 BB_PAPER_COLOUR	= {239, 239, 239, ATG_ALPHA_OPAQUE},
		 BB_INK_COLOUR		= {31, 31, 31, ATG_ALPHA_OPAQUE},
		 BB_SPIN_FG_COLOUR	= {0, 115, 223, ATG_ALPHA_OPAQUE},
		 BB_SPIN_BG_COLOUR	= {31, 15, 15, ATG_ALPHA_OPAQUE};

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
		atg_element *btn=atg_create_element_button(name, BB_INFG_COLOUR, GAME_BG_COLOUR);
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
		atg_element *btn=atg_create_element_button(name, BB_INFG_COLOUR, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
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

atg_element *create_fuse_selector(unsigned int *ft)
{
	atg_element *rv=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
	if(!rv) return(NULL);
	rv->type="selector";
	rv->match_click_callback=selector_match_click_callback;
	rv->render_callback=selector_render_callback;
	for(enum fuse_type i=0;i<FT_COUNT;i++)
	{
		atg_element *btn=atg_create_element_button(ident_ft(i), BB_INFG_COLOUR, GAME_BG_COLOUR);
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
	rv->userdata=ft;
	return(rv);
}

atg_element *create_bbg_selector(unsigned int *girth)
{
	atg_element *rv=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
	if(!rv) return(NULL);
	rv->type="selector";
	rv->match_click_callback=selector_match_click_callback;
	rv->render_callback=selector_render_callback;
	for(enum bb_girth i=0;i<BB_COUNT;i++)
	{
		atg_element *btn=atg_create_element_button(ident_bbg(i), BB_INFG_COLOUR, GAME_BG_COLOUR);
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
	rv->userdata=girth;
	return(rv);
}

atg_element *create_esl_selector(unsigned int *esl)
{
	atg_element *rv=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
	if(!rv) return(NULL);
	rv->type="selector";
	rv->match_click_callback=selector_match_click_callback;
	rv->render_callback=selector_render_callback;
	for(enum elec_level i=0;i<ESL_COUNT;i++)
	{
		atg_element *btn=atg_create_element_button(ident_esl(i), BB_INFG_COLOUR, GAME_BG_COLOUR);
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
	rv->userdata=esl;
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
	manf_box->h=48;
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
	atg_element *manf_lbl=atg_create_element_label("Manufacturer: ", 14, BB_INFG_COLOUR);
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
	atg_element *manf_tg=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_PAPER_COLOUR);
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
	shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_PAPER_COLOUR);
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
	atg_element *manf_tb=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_PAPER_COLOUR);
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
	atg_element *manf_name=atg_create_element_label_nocopy(BB_manf_buf, 11, BB_INK_COLOUR);
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
	atg_element *manf_desc=atg_create_element_label_nocopy(BB_manf_dbuf, 9, BB_INK_COLOUR);
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
	eng_box->h=93;
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
	atg_element *eng_lbl=atg_create_element_label("Engines: ", 14, BB_INFG_COLOUR);
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
	BB_engc=atg_create_element_spinner(ATG_SPINNER_RIGHTCLICK_STEP10, 1, 8, 1, 1, "%u", BB_SPIN_FG_COLOUR, BB_SPIN_BG_COLOUR);
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
	BB_egg=atg_create_element_toggle("Power Egg", false, BB_INFG_COLOUR, GAME_BG_COLOUR);
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
	BB_over=atg_create_element_toggle("Overbuild mounts", false, BB_INFG_COLOUR, GAME_BG_COLOUR);
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
	BB_eng->w=left_box->w;
	if(atg_ebox_pack(eng_box, BB_eng))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *eng_tg=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_PAPER_COLOUR);
	if(!eng_tg)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	eng_tg->w=left_box->w;
	if(atg_ebox_pack(eng_box, eng_tg))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_PAPER_COLOUR);
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
	atg_element *eng_tb=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_PAPER_COLOUR);
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
	atg_element *eng_name=atg_create_element_label_nocopy(BB_eng_buf, 11, BB_INK_COLOUR);
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
	atg_element *eng_desc=atg_create_element_label_nocopy(BB_eng_dbuf, 9, BB_INK_COLOUR);
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
	atg_element *eng_over=atg_create_element_label_nocopy(BB_eng_obuf, 9, BB_INK_COLOUR);
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
	atg_element *wing_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!wing_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	wing_box->h=20;
	if(atg_ebox_pack(left_box, wing_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *wa_lbl=atg_create_element_label("Wing Area: ", 14, BB_INFG_COLOUR);
	if(!wa_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(wing_box, wa_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_wa=atg_create_element_spinner(ATG_SPINNER_RIGHTCLICK_STEP10, 200, 2400, 10, 240, "%04u", BB_SPIN_FG_COLOUR, BB_SPIN_BG_COLOUR);
	if(!BB_wa)
	{
		fprintf(stderr, "atg_create_element_spinner failed\n");
		return(1);
	}
	if(atg_ebox_pack(wing_box, BB_wa))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *ws_lbl=atg_create_element_label("sq.ft. ", 11, BB_INFG_COLOUR);
	if(!ws_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(wing_box, ws_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *wr_lbl=atg_create_element_label("Aspect: ", 14, BB_INFG_COLOUR);
	if(!wr_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(wing_box, wr_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_wr=atg_create_element_spinner(ATG_SPINNER_RIGHTCLICK_STEP10, 10, 150, 1, 70, "%03u", BB_SPIN_FG_COLOUR, BB_SPIN_BG_COLOUR);
	if(!BB_wr)
	{
		fprintf(stderr, "atg_create_element_spinner failed\n");
		return(1);
	}
	if(atg_ebox_pack(wing_box, BB_wr))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *wrt_lbl=atg_create_element_label("/10", 11, BB_INFG_COLOUR);
	if(!wrt_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(wing_box, wrt_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *fuse_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!fuse_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	fuse_box->h=34;
	if(atg_ebox_pack(left_box, fuse_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *fuse_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!fuse_row)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(fuse_box, fuse_row))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *fuse_lbl=atg_create_element_label("Fuselage: ", 14, BB_INFG_COLOUR);
	if(!fuse_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(fuse_row, fuse_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_fuse=create_fuse_selector(&selft);
	if(!BB_fuse)
	{
		fprintf(stderr, "create_fuse_selector failed\n");
		return(1);
	}
	if(atg_ebox_pack(fuse_row, BB_fuse))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *fuse_tg=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_PAPER_COLOUR);
	if(!fuse_tg)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	fuse_tg->w=left_box->w;
	if(atg_ebox_pack(fuse_box, fuse_tg))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_PAPER_COLOUR);
	if(!shim)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	shim->w=2;
	shim->h=8;
	if(atg_ebox_pack(fuse_tg, shim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(BB_fuse_dbuf=malloc(64)))
	{
		perror("malloc");
		return(1);
	}
	atg_element *fuse_desc=atg_create_element_label_nocopy(BB_fuse_dbuf, 9, BB_INK_COLOUR);
	if(!fuse_desc)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(fuse_tg, fuse_desc))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *bomb_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!bomb_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	bomb_box->h=50;
	if(atg_ebox_pack(left_box, bomb_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *bomb_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!bomb_row)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(bomb_box, bomb_row))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *bbg_lbl=atg_create_element_label("Bombbay girth: ", 14, BB_INFG_COLOUR);
	if(!bbg_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(bomb_row, bbg_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_girth=create_bbg_selector(&selgirth);
	if(!BB_girth)
	{
		fprintf(stderr, "create_bbg_selector failed\n");
		return(1);
	}
	if(atg_ebox_pack(bomb_row, BB_girth))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *bomb_tg=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_PAPER_COLOUR);
	if(!bomb_tg)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	bomb_tg->w=left_box->w;
	if(atg_ebox_pack(bomb_box, bomb_tg))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_PAPER_COLOUR);
	if(!shim)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	shim->w=2;
	shim->h=8;
	if(atg_ebox_pack(bomb_tg, shim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(BB_girth_dbuf=malloc(64)))
	{
		perror("malloc");
		return(1);
	}
	atg_element *bomb_desc=atg_create_element_label_nocopy(BB_girth_dbuf, 9, BB_INK_COLOUR);
	if(!bomb_desc)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(bomb_tg, bomb_desc))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *load_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!load_row)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(bomb_box, load_row))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *cap_lbl=atg_create_element_label("Capacity: ", 14, BB_INFG_COLOUR);
	if(!cap_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(load_row, cap_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_cap=atg_create_element_spinner(ATG_SPINNER_RIGHTCLICK_STEP10, 500, 25000, 100, 1000, "%05u", BB_SPIN_FG_COLOUR, BB_SPIN_BG_COLOUR);
	if(!BB_cap)
	{
		fprintf(stderr, "atg_create_element_spinner failed\n");
		return(1);
	}
	if(atg_ebox_pack(load_row, BB_cap))
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
	if(atg_ebox_pack(load_row, shim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_csbs=atg_create_element_toggle("CSBS", false, BB_INFG_COLOUR, GAME_BG_COLOUR);
	if(!BB_csbs)
	{
		fprintf(stderr, "atg_create_element_spinner failed\n");
		return(1);
	}
	if(atg_ebox_pack(load_row, BB_csbs))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *elec_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!elec_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	elec_box->h=34;
	if(atg_ebox_pack(left_box, elec_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *elec_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!elec_row)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(elec_box, elec_row))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *esl_lbl=atg_create_element_label("Electrics: ", 14, BB_INFG_COLOUR);
	if(!esl_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(elec_row, esl_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_esl=create_esl_selector(&selesl);
	if(!BB_esl)
	{
		fprintf(stderr, "create_bbg_selector failed\n");
		return(1);
	}
	if(atg_ebox_pack(elec_row, BB_esl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *elec_tg=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_PAPER_COLOUR);
	if(!elec_tg)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	elec_tg->w=left_box->w;
	if(atg_ebox_pack(elec_box, elec_tg))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_PAPER_COLOUR);
	if(!shim)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	shim->w=2;
	shim->h=8;
	if(atg_ebox_pack(elec_tg, shim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(BB_esl_dbuf=malloc(64)))
	{
		perror("malloc");
		return(1);
	}
	atg_element *elec_desc=atg_create_element_label_nocopy(BB_esl_dbuf, 9, BB_INK_COLOUR);
	if(!elec_desc)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(elec_tg, elec_desc))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	/* TODO: LN, U, G */
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
	/* Hide stuff for which tech is not unlocked yet */
	for(unsigned int i=0;i<builder->entities.neng;i++)
	{
		struct engine *eng=builder->entities.eng[i];
		atg_box *b=BB_eng->elemdata;
		b->elems[i]->hidden=!eng->unlocked;
	}
	BB_egg->hidden=!builder->tn.ees;
	(void)state;
	atg_event e;
	while(1)
	{
		const char *manf_name="", *manf_desc="";
		unsigned int manf_geo=0;
		if(selmanf<builder->entities.nmanf)
		{
			struct manf *manf=builder->entities.manf[selmanf];
			manf_name=manf->name;
			manf_desc=manf->desc;
			manf_geo=manf->geo;
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
		((atg_box *)BB_fuse->elemdata)->elems[FT_GEODETIC]->hidden=!manf_geo;
		strncpy(BB_fuse_dbuf, describe_ft(selft), 64);
		strncpy(BB_girth_dbuf, describe_bbg_long(selgirth), 64);
		strncpy(BB_esl_dbuf, describe_esl_long(selesl), 64);
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
