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
#include "builder/calc.h"

enum out_row {
	OUT_DIM,
	OUT_WGT,
	OUT_SPD,
	OUT_CRC, /* Ceiling, Range, Climb */
	OUT_RAN, /* Max Range condition */
	OUT_DEF,
	OUT_FSA, /* FAil, SVp, ACcuracy */
	OUT_CST,
	OUT_NOERR,
	OUT_ERR,

	OUT_ROWS=OUT_ERR+8
};

atg_element *builder_box;
atg_element *BB_full, *BB_cont;
unsigned int selmanf, seleng, selft, selgirth, selesl;
struct multi_sel selgun[LXN_COUNT];
atg_element *BB_manf, *BB_engc, *BB_egg, *BB_over, *BB_eng, *BB_wa, *BB_wr;
atg_element *BB_fuse, *BB_girth, *BB_cap, *BB_csbs, *BB_esl, *BB_na[NNAVAIDS];
atg_element *BB_fuel, *BB_fill, *BB_sst, *BB_gross, *BB_agw;
atg_element *BB_gun[LXN_COUNT], *BB_cc[CREW_CLASSES], *BB_cd[CREW_CLASSES];
char *BB_manf_buf, *BB_manf_dbuf, *BB_eng_buf, *BB_eng_dbuf, *BB_eng_obuf;
char *BB_fuse_dbuf, *BB_girth_dbuf, *BB_esl_dbuf, *BB_gun_dbuf[LXN_COUNT];
char *BB_out_buf[OUT_ROWS];
SDL_Surface *BB_bp;

const atg_colour BB_BG_COLOUR		= {81, 102, 189, ATG_ALPHA_OPAQUE},
		 BB_BP_COLOUR		= {81, 102, 189, ATG_ALPHA_OPAQUE},
		 BB_BP_BORDER		= {63, 79, 171, ATG_ALPHA_OPAQUE},
		 BB_INFG_COLOUR		= {223, 223, 79, ATG_ALPHA_OPAQUE},
		 BB_OFF_COLOUR		= {31, 31, 31, ATG_ALPHA_OPAQUE},
		 BB_PAPER_COLOUR	= {239, 239, 239, ATG_ALPHA_OPAQUE},
		 BB_INK_COLOUR		= {31, 31, 31, ATG_ALPHA_OPAQUE},
		 BB_ERR_COLOUR		= {79, 15, 15, ATG_ALPHA_OPAQUE},
		 BB_SPIN_FG_COLOUR	= {191, 191, 47, ATG_ALPHA_OPAQUE},
		 BB_SPIN_BG_COLOUR	= {15, 31, 47, ATG_ALPHA_OPAQUE};

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
		atg_element *btn=atg_create_element_button(name, BB_INFG_COLOUR, BB_BG_COLOUR);
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
	atg_element *rv=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BG_COLOUR);
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
		atg_element *btn=atg_create_element_button(ident_ft(i), BB_INFG_COLOUR, BB_BG_COLOUR);
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
		atg_element *btn=atg_create_element_button(ident_bbg(i), BB_INFG_COLOUR, BB_BG_COLOUR);
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
		atg_element *btn=atg_create_element_button(ident_esl(i), BB_INFG_COLOUR, BB_BG_COLOUR);
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

atg_element *create_gun_selector(struct multi_sel *gun, enum turret_location lxn)
{
	atg_element *rv=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BG_COLOUR);
	if(!rv) return(NULL);
	rv->type="selector";
	rv->match_click_callback=multi_selector_match_click_callback;
	rv->render_callback=multi_selector_render_callback;
	atg_element *nobtn=atg_create_element_button("None", BB_INFG_COLOUR, BB_OFF_COLOUR);
	if(!nobtn)
	{
		atg_free_element(rv);
		return(NULL);
	}
	if(atg_ebox_pack(rv, nobtn))
	{
		atg_free_element(nobtn);
		atg_free_element(rv);
		return(NULL);
	}
	for(unsigned int i=0;i<builder->entities.ngun;i++)
	{
		const char *name=builder->entities.gun[i]->ident;
		if(builder->entities.gun[i]->lxn!=lxn)
			continue;
		atg_element *btn=atg_create_element_button(name, BB_INFG_COLOUR, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
		if(!btn)
		{
			atg_free_element(rv);
			return(NULL);
		}
		btn->userdata=malloc(sizeof(i));
		if(!btn->userdata)
		{
			atg_free_element(btn);
			atg_free_element(rv);
			return(NULL);
		}
		*(unsigned int *)btn->userdata=i;
		if(atg_ebox_pack(rv, btn))
		{
			atg_free_element(btn);
			atg_free_element(rv);
			return(NULL);
		}
	}
	rv->userdata=gun;
	return(rv);
}

int builder_create(void)
{
	if(!(builder_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR)))
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
	atg_element *title=atg_create_element_label("HARRIS: Aircraft Design  ", 12, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
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
	atg_element *main_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BG_COLOUR);
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
	atg_element *shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
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
	atg_element *left_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
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
	atg_element *manf_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
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
	atg_element *manf_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BG_COLOUR);
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
	atg_element *eng_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
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
	atg_element *engc_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BG_COLOUR);
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
	shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
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
	BB_egg=atg_create_element_toggle("Power Egg", false, BB_INFG_COLOUR, BB_OFF_COLOUR);
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
	BB_over=atg_create_element_toggle("Overbuild mounts", false, BB_INFG_COLOUR, BB_OFF_COLOUR);
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
	atg_element *wing_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BG_COLOUR);
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
	atg_element *fuse_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
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
	atg_element *fuse_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BG_COLOUR);
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
	atg_element *bomb_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
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
	atg_element *bomb_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BG_COLOUR);
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
	atg_element *load_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BG_COLOUR);
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
	atg_element *blb_lbl=atg_create_element_label("lb", 11, BB_INFG_COLOUR);
	if(!blb_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(load_row, blb_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
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
	BB_csbs=atg_create_element_toggle("CSBS", false, BB_INFG_COLOUR, BB_OFF_COLOUR);
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
	atg_element *guns_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
	if(!guns_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	guns_box->h=222;
	if(atg_ebox_pack(left_box, guns_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *guns_title=atg_create_element_label("Turrets: ", 14, BB_INFG_COLOUR);
	if(!guns_title)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(guns_box, guns_title))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	for(enum turret_location i=0;i<LXN_COUNT;i++)
	{
		atg_element *guns_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BG_COLOUR);
		if(!guns_row)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(guns_box, guns_row))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		BB_gun[i]=create_gun_selector(selgun+i, i);
		if(!BB_gun[i])
		{
			fprintf(stderr, "create_gun_selector failed\n");
			return(1);
		}
		if(atg_ebox_pack(guns_box, BB_gun[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *gun_tg=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_PAPER_COLOUR);
		if(!gun_tg)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		gun_tg->w=left_box->w;
		if(atg_ebox_pack(guns_box, gun_tg))
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
		if(atg_ebox_pack(gun_tg, shim))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(BB_gun_dbuf[i]=malloc(64)))
		{
			perror("malloc");
			return(1);
		}
		atg_element *gun_desc=atg_create_element_label_nocopy(BB_gun_dbuf[i], 9, BB_INK_COLOUR);
		if(!gun_desc)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(gun_tg, gun_desc))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	atg_element *crew_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
	if(!crew_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	crew_box->h=128;
	if(atg_ebox_pack(left_box, crew_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *crew_title_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BG_COLOUR);
	if(!crew_title_row)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(crew_box, crew_title_row))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *crew_lbl=atg_create_element_label("Crew:", 14, BB_INFG_COLOUR);
	if(!crew_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	crew_lbl->w=120;
	if(atg_ebox_pack(crew_title_row, crew_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *dual_lbl=atg_create_element_label("Dual-role as gunner:", 14, BB_INFG_COLOUR);
	if(!dual_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(crew_title_row, dual_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	for(enum cclass i=0;i<CREW_CLASSES;i++)
	{
		atg_element *crew_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BG_COLOUR);
		if(!crew_row)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(crew_box, crew_row))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *crew_left=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BG_COLOUR);
		if(!crew_left)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		crew_left->w=crew_lbl->w;
		if(atg_ebox_pack(crew_row, crew_left))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		char label[16];
		snprintf(label, sizeof(label), "%s: ", cclasses[i].name);
		atg_element *ccls_lbl=atg_create_element_label(label, 14, BB_INFG_COLOUR);
		if(!ccls_lbl)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		ccls_lbl->w=100;
		if(atg_ebox_pack(crew_left, ccls_lbl))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		BB_cc[i]=atg_create_element_spinner(ATG_SPINNER_RIGHTCLICK_STEP10, 0, 7, 1, 0, "%u", BB_SPIN_FG_COLOUR, BB_SPIN_BG_COLOUR);
		if(!BB_cc[i])
		{
			fprintf(stderr, "atg_create_element_spinner failed\n");
			return(1);
		}
		if(atg_ebox_pack(crew_left, BB_cc[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		// Engineers and Gunners can't be dual-role.
		// There's currently no OCP=1 turret (only OCP=2 LXN_FIXED guns)
		if(i==CCLASS_P||i==CCLASS_E||i==CCLASS_G)
			continue;
		BB_cd[i]=atg_create_element_spinner(ATG_SPINNER_RIGHTCLICK_STEP10, 0, 7, 1, 0, "%u", BB_SPIN_FG_COLOUR, BB_SPIN_BG_COLOUR);
		if(!BB_cd[i])
		{
			fprintf(stderr, "atg_create_element_spinner failed\n");
			return(1);
		}
		if(atg_ebox_pack(crew_row, BB_cd[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	atg_element *elec_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
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
	atg_element *elec_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BG_COLOUR);
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
		fprintf(stderr, "create_esl_selector failed\n");
		return(1);
	}
	if(atg_ebox_pack(elec_row, BB_esl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *nav_lbl=atg_create_element_label(" Nav: ", 14, BB_INFG_COLOUR);
	if(!nav_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(elec_row, nav_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	for(enum nav_aid i=0;i<NNAVAIDS;i++)
	{
		BB_na[i]=atg_create_element_toggle(describe_navaid(i), false, BB_INFG_COLOUR, BB_OFF_COLOUR);
		if(!(BB_na[i]))
		{
			fprintf(stderr, "atg_create_element_toggle failed\n");
			return(1);
		}
		if(atg_ebox_pack(elec_row, BB_na[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
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
	atg_element *fuel_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
	if(!fuel_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	fuel_box->h=20;
	if(atg_ebox_pack(left_box, fuel_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *fuel_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BG_COLOUR);
	if(!fuel_row)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(fuel_box, fuel_row))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *fuel_lbl=atg_create_element_label("Fuel: ", 14, BB_INFG_COLOUR);
	if(!fuel_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(fuel_row, fuel_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_fuel=atg_create_element_spinner(ATG_SPINNER_RIGHTCLICK_STEP10, 1000, 30000, 100, 1900, "%05u", BB_SPIN_FG_COLOUR, BB_SPIN_BG_COLOUR);
	if(!BB_fuel)
	{
		fprintf(stderr, "atg_create_element_spinner failed\n");
		return(1);
	}
	if(atg_ebox_pack(fuel_row, BB_fuel))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *flb_lbl=atg_create_element_label("lb", 11, BB_INFG_COLOUR);
	if(!flb_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(fuel_row, flb_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *fill_lbl=atg_create_element_label(" Fill level: ", 14, BB_INFG_COLOUR);
	if(!fill_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(fuel_row, fill_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_fill=atg_create_element_spinner(ATG_SPINNER_RIGHTCLICK_STEP10, 10, 100, 1, 80, "%03u", BB_SPIN_FG_COLOUR, BB_SPIN_BG_COLOUR);
	if(!BB_fill)
	{
		fprintf(stderr, "atg_create_element_spinner failed\n");
		return(1);
	}
	if(atg_ebox_pack(fuel_row, BB_fill))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *fill_pct=atg_create_element_label("%", 11, BB_INFG_COLOUR);
	if(!fill_pct)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(fuel_row, fill_pct))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
	if(!shim)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	shim->w=2;
	shim->h=8;
	if(atg_ebox_pack(fuel_row, shim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_sst=atg_create_element_toggle("SST", false, BB_INFG_COLOUR, BB_OFF_COLOUR);
	if(!BB_sst)
	{
		fprintf(stderr, "atg_create_element_spinner failed\n");
		return(1);
	}
	if(atg_ebox_pack(fuel_row, BB_sst))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *gross_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
	if(!gross_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	gross_box->h=20;
	if(atg_ebox_pack(left_box, gross_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *gross_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BG_COLOUR);
	if(!gross_row)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(gross_box, gross_row))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *gross_lbl=atg_create_element_label("M.g.t.o.w.: ", 14, BB_INFG_COLOUR);
	if(!gross_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(gross_row, gross_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_gross=atg_create_element_spinner(ATG_SPINNER_RIGHTCLICK_STEP10, 1, 900, 1, 72, "%03u", BB_SPIN_FG_COLOUR, BB_SPIN_BG_COLOUR);
	if(!BB_gross)
	{
		fprintf(stderr, "atg_create_element_spinner failed\n");
		return(1);
	}
	if(atg_ebox_pack(gross_row, BB_gross))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *glb_lbl=atg_create_element_label("x100lb", 11, BB_INFG_COLOUR);
	if(!glb_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(gross_row, glb_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
	if(!shim)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	shim->w=2;
	shim->h=8;
	if(atg_ebox_pack(gross_row, shim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_agw=atg_create_element_toggle("Auto", true, BB_INFG_COLOUR, BB_OFF_COLOUR);
	if(!BB_agw)
	{
		fprintf(stderr, "atg_create_element_spinner failed\n");
		return(1);
	}
	if(atg_ebox_pack(gross_row, BB_agw))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *gutter=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
	if(!gutter)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	gutter->w=24;
	gutter->h=2;
	if(atg_ebox_pack(main_box, gutter))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *right_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BG_COLOUR);
	if(!right_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	right_box->w=418;
	if(atg_ebox_pack(main_box, right_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *bp_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_BP_BORDER);
	if(!bp_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	bp_box->w=418;
	bp_box->h=154;
	if(atg_ebox_pack(right_box, bp_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BP_BORDER);
	if(!shim)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	shim->w=418;
	shim->h=2;
	if(atg_ebox_pack(bp_box, shim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_BP_BORDER);
	if(!shim)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	shim->w=2;
	shim->h=152;
	if(atg_ebox_pack(bp_box, shim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	BB_bp=SDL_CreateRGBSurface(SDL_HWSURFACE, 414, 150, 24, 0xff0000, 0xff00, 0xff, 0);
	if(!BB_bp)
	{
		fprintf(stderr, "BB_bp: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(1);
	}
	atg_colour bp_bg=BB_BP_COLOUR;
	SDL_FillRect(BB_bp, &(SDL_Rect){0, 0, BB_bp->w, BB_bp->h}, SDL_MapRGB(BB_bp->format, bp_bg.r, bp_bg.g, bp_bg.b));
	atg_element *blueprint=atg_create_element_image(BB_bp);
	if(!blueprint)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	blueprint->w=414;
	blueprint->h=150;
	if(atg_ebox_pack(bp_box, blueprint))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *out_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_PAPER_COLOUR);
	if(!out_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	out_box->w=right_box->w;
	if(atg_ebox_pack(right_box, out_box))
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
	if(atg_ebox_pack(out_box, shim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *out_tg=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_PAPER_COLOUR);
	if(!out_tg)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(out_box, out_tg))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	for(enum out_row i=0;i<OUT_ROWS;i++)
	{
		if(!(BB_out_buf[i]=malloc(80)))
		{
			perror("malloc");
			return(1);
		}
		atg_colour fgcolour=BB_INK_COLOUR;
		if(i>=OUT_ERR)
			fgcolour=BB_ERR_COLOUR;
		atg_element *outtext=atg_create_element_label_nocopy(BB_out_buf[i], 9, fgcolour);
		if(!outtext)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(out_tg, outtext))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	/* TODO: outputs, actions */
	return(0);
}

/* Update the Controller from the Model state.
 * Used on e.g. New or Load.
 */
void builder_update_m2c(const struct bomber *b)
{
	for(unsigned int i=0;i<builder->entities.nmanf;i++)
		if(b->manf==builder->entities.manf[i])
		{
			selmanf = i;
			break;
		}
	for(unsigned int i=0;i<builder->entities.neng;i++)
		if(b->engines.typ==builder->entities.eng[i])
		{
			seleng = i;
			break;
		}
	selft=b->fuse.typ;
	selgirth=b->bay.girth;
	selesl=b->elec.esl;
	for(unsigned int i=0;i<LXN_COUNT;i++)
	{
		if(!b->turrets.typ[i])
			selgun[i].sel=0;
		if(!b->turrets.mou[i])
			selgun[i].semi=0;
		if(!b->turrets.typ[i]&&!b->turrets.mou[i])
			continue;
		if(!BB_gun[i]) continue;
		atg_box *box=BB_gun[i]->elemdata;
		if(!box) continue;
		unsigned int sel=builder->entities.ngun;
		unsigned int semi=builder->entities.ngun;
		for(unsigned int j=0;j<builder->entities.ngun;j++)
		{
			if(b->turrets.typ[i]==builder->entities.gun[j])
				sel=j;
			if(b->turrets.mou[i]==builder->entities.gun[j])
				semi=j;
		}
		for(unsigned int j=0;j<box->nelems;j++)
		{
			unsigned int *k=box->elems[j]->userdata;
			if(!k) continue;
			if(*k==sel)
				selgun[i].sel=j;
			if(*k==semi)
				selgun[i].semi=j;
		}
	}
	if(BB_engc)
	{
		atg_spinner *spin=BB_engc->elemdata;
		if(spin) spin->value=b->engines.number;
	}
	if(BB_egg)
	{
		atg_toggle *tog=BB_egg->elemdata;
		if(tog) tog->state=b->engines.egg;
	}
	if(BB_over)
	{
		atg_toggle *tog=BB_over->elemdata;
		if(tog) tog->state=b->engines.mou!=b->engines.typ;
	}
	if(BB_wa)
	{
		atg_spinner *spin=BB_wa->elemdata;
		if(spin) spin->value=b->wing.area;
	}
	if(BB_wr)
	{
		atg_spinner *spin=BB_wr->elemdata;
		if(spin) spin->value=b->wing.art;
	}
	if(BB_cap)
	{
		atg_spinner *spin=BB_cap->elemdata;
		if(spin) spin->value=b->bay.cap;
	}
	if(BB_csbs)
	{
		atg_toggle *tog=BB_csbs->elemdata;
		if(tog) tog->state=b->bay.csbs;
	}
	for(unsigned int i=0;i<NNAVAIDS;i++)
		if(BB_na[i])
		{
			atg_toggle *tog=BB_na[i]->elemdata;
			if(tog) tog->state=b->elec.navaid[i];
		}
	if(BB_fuel)
	{
		atg_spinner *spin=BB_fuel->elemdata;
		if(spin) spin->value=b->tanks.hlb*100;
	}
	if(BB_fill)
	{
		atg_spinner *spin=BB_fill->elemdata;
		if(spin) spin->value=b->tanks.pct;
	}
	if(BB_sst)
	{
		atg_toggle *tog=BB_sst->elemdata;
		if(tog) tog->state=b->tanks.sst;
	}
	if(BB_gross)
	{
		atg_spinner *spin=BB_gross->elemdata;
		if(spin) spin->value=ceil(b->mtow/100.0);
	}
	if(BB_agw)
	{
		atg_toggle *tog=BB_agw->elemdata;
		if(tog) tog->state=!b->user_mtow;
	}
	unsigned int dcount[CREW_CLASSES];
	unsigned int count[CREW_CLASSES];
	count_crew(&b->crew, count);
	count_dcrew(&b->crew, dcount);
	for(unsigned int i=0;i<CREW_CLASSES;i++)
	{
		if(BB_cc[i])
		{
			atg_spinner *spin=BB_cc[i]->elemdata;
			if(spin) spin->value=count[i];
		}
		if(BB_cd[i])
		{
			atg_spinner *spin=BB_cd[i]->elemdata;
			if(spin) spin->value=dcount[i];
		}
	}
}


/* Update the View from the Model state */
void builder_update_m2v(const struct bomber *b)
{
	const struct tech_numbers *tn=&builder->tn;
	struct bomber bmr=*b; /* bomber at Max Range */
	unsigned int mptow, mts, mtg;
	bool concrete = tn->rcs;
	int delta;
	bmr.tanks.pct=100;
	bmr.user_mtow=true; /* force it to use *b's mtow */
	calc_bomber(&bmr, tn);
	mts = concrete ? tn->rcs : tn->rgs;
	mtg = concrete ? tn->rcg : tn->rgg;
	mptow = floor(wing_lift(&bmr.wing, mts / 1.6f));
	mptow = min(mptow, mtg * 1000);
	mptow = min(mptow, bmr.mtow);
	delta = bmr.gross - mptow;
	if((int)bmr.bay.load < delta)
		bmr.bay.load = 0;
	else
		bmr.bay.load = min(((int)bmr.bay.load) - delta, (int)bmr.bay.cap);
	snprintf(BB_out_buf[OUT_DIM], 80,
		 "Dimensions: span %.1fft, chord %.1fft",
		 b->wing.span, b->wing.chord);
	snprintf(BB_out_buf[OUT_WGT], 80,
		 "Weights: tare %.0flb, gross %.0flb; wing loading %.1flb/sq ft, L/D %.1f",
		 b->tare, b->gross, b->wing.wl, b->wing.ld);
	snprintf(BB_out_buf[OUT_SPD], 80,
		 "Speeds: take-off %.1fmph, max %.1fmph, cruise %.1fmph at %.0fft",
		 b->takeoff_spd, b->deck_spd, b->cruise_spd, b->cruise_alt * 1000.0f);
	snprintf(BB_out_buf[OUT_CRC], 80,
		 "Service ceiling: %.0fft; range: %.0fmi (%.1fhr); initial climb %.0ffpm",
		 b->ceiling * 1000.0f, b->range, b->tanks.hours, b->init_climb);
	snprintf(BB_out_buf[OUT_RAN], 80,
		 "Max range: %.0fmi with %ulb bombs",
		 bmr.range, bmr.bay.load);
	snprintf(BB_out_buf[OUT_DEF], 80,
		 "Defence: %.1f/%.1f (flak %.1f): manu %.1f, evade %.1f, vuln %.2f (fr %.2f)",
		 b->defn[0], b->defn[1], b->flak_factor, b->manu_pen,
		 b->evade_factor, b->vuln, b->tanks.ratio);
	snprintf(BB_out_buf[OUT_FSA], 80,
		 "Failure: %.1f; Serviceability: %.1f; Accuracy: %.1f",
		 b->fail * 100.0f, b->serv * 100.0f, b->accu * 100.0f);
	snprintf(BB_out_buf[OUT_CST], 80,
		 "Cost: %.0f funds",
		 b->cost);
	if(b->new)
		*BB_out_buf[OUT_NOERR]=0;
	else
		snprintf(BB_out_buf[OUT_NOERR], 80, "No errors or warnings.");
	for(unsigned int i=0;i+OUT_ERR<OUT_ROWS;i++)
		if(i<b->new)
			snprintf(BB_out_buf[i+OUT_ERR], 80, b->ew[i]);
		else
			*BB_out_buf[i+OUT_ERR]=0;
}

/* Update the Model's crew from the Controller state */
void update_crew_c2m(struct bomber *b)
{
	b->crew.n=0;
	for(enum cclass i=0;i<CREW_CLASSES;i++)
	{
		atg_spinner *cspin=NULL, *dspin=NULL;
		unsigned int cc=0, cd=0;
		if(BB_cc[i])
			cspin=BB_cc[i]->elemdata;
		if(BB_cd[i])
			dspin=BB_cd[i]->elemdata;
		if(cspin)
			cc=cspin->value;
		if(dspin)
			cd=dspin->value;
		for(unsigned int j=0;j<cc;j++)
		{
			if(b->crew.n<MAX_CREW)
				b->crew.men[b->crew.n++]=(struct crewman){
					.pos=i,
					.gun=j<cd,
				};
		}
	}
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
	for(enum bb_girth i=0;i<BB_COUNT;i++)
	{
		atg_box *b=BB_girth->elemdata;
		b->elems[i]->hidden=!builder->tn.bt[i];
	}
	BB_csbs->hidden=!builder->tn.csb;
	for(enum turret_location i=0;i<LXN_COUNT;i++)
	{
		atg_box *b=BB_gun[i]->elemdata;
		for(unsigned int j=1;j<b->nelems;j++)
		{
			unsigned int *k=b->elems[j]->userdata;
			if(!k) continue;
			atg_button *btn=b->elems[j]->elemdata;
			struct turret *gun=builder->entities.gun[*k];
			btn->fgcolour=gun->unlocked?BB_INFG_COLOUR:BB_OFF_COLOUR;
		}
	}
	for(enum elec_level i=0;i<ESL_COUNT;i++)
	{
		atg_box *b=BB_esl->elemdata;
		b->elems[i]->hidden=i>builder->tn.esl;
	}
	for(enum nav_aid i=0;i<NNAVAIDS;i++)
		BB_na[i]->hidden=!builder->tn.na[i];
	BB_sst->hidden=!builder->tn.sft;
	struct bomber b;
	init_bomber(&b, builder->entities.manf[0], builder->entities.eng[0]);
	calc_bomber(&b, &builder->tn);
	builder_update_m2c(&b);
	builder_update_m2v(&b);
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
		for(enum turret_location i=0;i<LXN_COUNT;i++)
		{
			atg_box *b=BB_gun[i]->elemdata;
			if(selgun[i].sel>=b->nelems)
				*BB_gun_dbuf[i]=0;
			unsigned int *g=b->elems[selgun[i].sel]->userdata;
			if(!g)
				strncpy(BB_gun_dbuf[i], "No turret in this position.", 64);
			else if(*g>=builder->entities.ngun)
				strncpy(BB_gun_dbuf[i], "Unknown turret type.", 64);
			else
				strncpy(BB_gun_dbuf[i], builder->entities.gun[*g]->desc, 64);
		}
		atg_flip(canvas);
		bool changed=false;
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
					atg_ev_toggle t=e.event.toggle;
					if(t.e==BB_over)
					{
						if (t.state)
							b.engines.mou=b.engines.typ->u;
						else
							b.engines.mou=b.engines.typ;
						changed=true;
					}
					else if (t.e==BB_egg)
					{
						b.engines.egg=t.state;
						changed=true;
					}
					else if (t.e==BB_csbs)
					{
						b.bay.csbs=t.state;
						changed=true;
					}
					else if (t.e==BB_sst)
					{
						b.tanks.sst=t.state;
						changed=true;
					}
					else if (t.e==BB_agw)
					{
						b.mtow=ceil(b.gross);
						if((b.user_mtow=!t.state))
						{
							atg_spinner *spin=BB_gross?BB_gross->elemdata:NULL;
							if(spin)
								b.mtow=spin->value*100;
						}
						changed=true;
					}
					else
					{
						unsigned int i;
						for(i=0;i<NNAVAIDS;i++)
							if(t.e==BB_na[i])
							{
								b.elec.navaid[i]=t.state;
								changed=true;
								break;
							}
						if(i<NNAVAIDS)
							break;
						fprintf(stderr, "Clicked on unknown toggle!\n");
					}
				break;
				case ATG_EV_VALUE:;
					atg_ev_value v=e.event.value;
					if(v.e==BB_manf)
					{
						b.manf=builder->entities.manf[selmanf];
						changed=true;
					}
					else if(v.e==BB_engc)
					{
						b.engines.number=v.value;
						changed=true;
					}
					else if(v.e==BB_eng)
					{
						atg_toggle *tog=BB_over->elemdata;
						b.engines.typ=builder->entities.eng[seleng];
						if (tog->state)
							b.engines.mou=b.engines.typ->u;
						else
							b.engines.mou=b.engines.typ;
						changed=true;
					}
					else if(v.e==BB_wa)
					{
						b.wing.area=v.value;
						changed=true;
					}
					else if(v.e==BB_wr)
					{
						b.wing.art=v.value;
						changed=true;
					}
					else if(v.e==BB_fuse)
					{
						b.fuse.typ=selft;
						changed=true;
					}
					else if(v.e==BB_girth)
					{
						b.bay.girth=selgirth;
						changed=true;
					}
					else if(v.e==BB_cap)
					{
						b.bay.load=b.bay.cap=v.value;
						changed=true;
					}
					else if(v.e==BB_esl)
					{
						b.elec.esl=selesl;
						changed=true;
					}
					else if(v.e==BB_fuel)
					{
						b.tanks.hlb=(v.value+99)/100;
						changed=true;
					}
					else if(v.e==BB_fill)
					{
						b.tanks.pct=v.value;
						changed=true;
					}
					else if(v.e==BB_gross)
					{
						b.mtow=v.value*100;
						if(BB_agw)
						{
							atg_toggle *tog=BB_agw->elemdata;
							if(tog) tog->state=false;
						}
						b.user_mtow=true;
						changed=true;
					}
					else
					{
						unsigned int i;
						for(i=0;i<LXN_COUNT;i++)
							if(v.e==BB_gun[i])
							{
								atg_box *box=BB_gun[i]->elemdata;
								if(!box) break;
								if(selgun[i].sel && selgun[i].sel<box->nelems)
								{
									unsigned int *j=box->elems[selgun[i].sel]->userdata;
									if(j&&*j<builder->entities.ngun)
										b.turrets.typ[i]=builder->entities.gun[*j];
								}
								else
									b.turrets.typ[i]=NULL;
								if(selgun[i].semi && selgun[i].semi<box->nelems)
								{
									unsigned int *j=box->elems[selgun[i].semi]->userdata;
									if(j&&*j<builder->entities.ngun)
										b.turrets.mou[i]=builder->entities.gun[*j];
								}
								else
									b.turrets.mou[i]=NULL;
								changed=true;
								break;
							}
						if(i<LXN_COUNT)
							break;
						for(i=0;i<CREW_CLASSES;i++)
						{
							if(v.e==BB_cc[i])
							{
								update_crew_c2m(&b);
								changed=true;
								break;
							}
							if(v.e==BB_cd[i])
							{
								update_crew_c2m(&b);
								changed=true;
								break;
							}
						}
						if(i<CREW_CLASSES)
							break;
						fprintf(stderr, "Clicked on unknown spinner!\n");
					}
				break;
				default:
				break;
			}
		}
		if(changed)
		{
			calc_bomber(&b, &builder->tn);
			/* Don't call m2c for the very few things calc can
			 * change; just handle them ourselves in open code.
			 */
			if(BB_gross)
			{
				atg_spinner *spin=BB_gross->elemdata;
				spin->value=ceil(b.mtow/100.0);
			}
			builder_update_m2v(&b);
		}
		SDL_Delay(50);
	}
}

void builder_free(void)
{
	atg_free_element(builder_box);
}
