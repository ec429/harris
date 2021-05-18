/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details

	handle_crews: screen for training, crew assignments etc.
*/

#include <math.h>

#include "handle_crews.h"
#include "ui.h"
#include "globals.h"
#include "date.h"
#include "post_raid.h" /* for refill_students() */
#include "bits.h"
#include "render.h"
#include "widgets.h"

#define HCTBL_BG_COLOUR	(atg_colour){23, 23, 31, ATG_ALPHA_OPAQUE}
#define HC_SKILL_HHEIGHT	12
#define HC_SKILL_FHEIGHT	(HC_SKILL_HHEIGHT * 2)
#define HC_TP_BG_COLOUR	(atg_colour){23, 23, 63, ATG_ALPHA_OPAQUE}

atg_element *handle_crews_box;

atg_element *HC_cont, *HC_full;
char *HC_count[CREW_CLASSES][SHOW_STATUSES];
SDL_Surface *HC_skill[CREW_CLASSES][SHOW_STATUSES];
SDL_Surface *HC_tops[CREW_CLASSES][2];
atg_element *HC_tp_BT, *HC_tp_box[TPIPE__MAX], *HC_tp_btn[TPIPE__MAX], *HC_tp_dwell[TPIPE__MAX], *HC_tp_cont[TPIPE_LFS];
SDL_Surface *HC_tp_tops[CREW_CLASSES];
atg_element *HC_tp_text_box;
char *HC_tp_count[CREW_CLASSES], *HC_tp_ecount[CREW_CLASSES];
enum tpipe selstage=TPIPE__MAX;
int hc_filter_groups[8];
atg_element *HC_filter_groups[8];

void update_crews(game *state);

int handle_crews_create(void)
{
	handle_crews_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!handle_crews_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	atg_element *title=atg_create_element_label("HARRIS: Crews & Training", 12, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!title)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(handle_crews_box, title))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *top_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!top_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(handle_crews_box, top_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(HC_cont=atg_create_element_button("Continue", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(top_box, HC_cont))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HC_full=atg_create_element_image(fullbtn);
	if(!HC_full)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	HC_full->clickable=true;
	if(atg_ebox_pack(top_box, HC_full))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *head_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, HCTBL_BG_COLOUR);
	if(!head_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(handle_crews_box, head_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *class_header=atg_create_element_label("Position", 12, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE});
	if(!class_header)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	class_header->w=80;
	if(atg_ebox_pack(head_box, class_header))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	for(unsigned int status=0;status<SHOW_STATUSES;status++)
	{
		atg_element *header=atg_create_element_label(cstatuses[status], 12, (atg_colour){191, 191, 191, ATG_ALPHA_OPAQUE});
		if(!header)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		header->w=240;
		if(atg_ebox_pack(head_box, header))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	for(unsigned int i=0;i<CREW_CLASSES;i++)
	{
		atg_element *row_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, HCTBL_BG_COLOUR);
		if(!row_box)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(handle_crews_box, row_box))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *class_label=atg_create_element_label(cclasses[i].name, 12, (atg_colour){207, 207, 207, ATG_ALPHA_OPAQUE});
		if(!class_label)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		class_label->w=80;
		class_label->h=20;
		if(atg_ebox_pack(row_box, class_label))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		for(unsigned int status=0;status<SHOW_STATUSES;status++)
		{
			atg_element *cell_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, HCTBL_BG_COLOUR);
			if(!cell_box)
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				return(1);
			}
			cell_box->w=240;
			if(atg_ebox_pack(row_box, cell_box))
			{
				perror("atg_ebox_pack");
				return(1);
			}
			HC_count[i][status]=malloc(32);
			if(!HC_count[i][status])
			{
				perror("malloc");
				return(1);
			}
			strcpy(HC_count[i][status], "count");
			atg_element *count=atg_create_element_label_nocopy(HC_count[i][status], 12, (atg_colour){191, 191, 159, ATG_ALPHA_OPAQUE});
			if(!count)
			{
				fprintf(stderr, "atg_create_element_label failed\n");
				return(1);
			}
			count->w=80;
			if(atg_ebox_pack(cell_box, count))
			{
				perror("atg_ebox_pack");
				return(1);
			}
			HC_skill[i][status]=SDL_CreateRGBSurface(SDL_HWSURFACE, 101, HC_SKILL_FHEIGHT, 32,  0xff000000, 0xff0000, 0xff00, 0xff);
			if(!HC_skill[i][status])
			{
				fprintf(stderr, "HC_skill[][]=SDL_CreateRGBSurface: %s\n", SDL_GetError());
				return(1);
			}
			atg_element *skill=atg_create_element_image(HC_skill[i][status]);
			if(!skill)
			{
				fprintf(stderr, "atg_create_element_image failed\n");
				return(1);
			}
			if(atg_ebox_pack(cell_box, skill))
			{
				perror("atg_ebox_pack");
				return(1);
			}
			if(status!=CSTATUS_STUDENT)
			{
				atg_element *shim=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, HCTBL_BG_COLOUR);
				if(!shim)
				{
					fprintf(stderr, "atg_create_element_box failed\n");
					return(1);
				}
				shim->w=10;
				if(atg_ebox_pack(cell_box, shim))
				{
					perror("atg_ebox_pack");
					return(1);
				}
				unsigned int sta=status?1:0;
				HC_tops[i][sta]=SDL_CreateRGBSurface(SDL_HWSURFACE, 31, HC_SKILL_FHEIGHT, 32,  0xff000000, 0xff0000, 0xff00, 0xff);
				if(!HC_tops[i][sta])
				{
					fprintf(stderr, "HC_tops[][]=SDL_CreateRGBSurface: %s\n", SDL_GetError());
					return(1);
				}
				atg_element *tops=atg_create_element_image(HC_tops[i][sta]);
				if(!tops)
				{
					fprintf(stderr, "atg_create_element_image failed\n");
					return(1);
				}
				if(atg_ebox_pack(cell_box, tops))
				{
					perror("atg_ebox_pack");
					return(1);
				}
			}
		}
		atg_element *cd_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, HCTBL_BG_COLOUR);
		if(!cd_box)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		cd_box->w=800;
		cd_box->h=20;
		if(atg_ebox_pack(handle_crews_box, cd_box))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *shim=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, HCTBL_BG_COLOUR);
		if(!shim)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		shim->w=40;
		if(atg_ebox_pack(cd_box, shim))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *class_desc=atg_create_element_label(cclasses[i].desc, 8, (atg_colour){183, 183, 183, ATG_ALPHA_OPAQUE});
		if(!class_desc)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(cd_box, class_desc))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	atg_element *gfilters=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!gfilters)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(handle_crews_box, gfilters))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *gft=atg_create_element_label("Groups:", 12, (atg_colour){239, 239, 0, ATG_ALPHA_OPAQUE});
	if(!gft)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(gfilters, gft))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	for(unsigned int g=0;g<8;g++)
	{
		hc_filter_groups[g]=0;
		if(!(HC_filter_groups[g]=create_filter_switch(grouppic[g], hc_filter_groups+g)))
		{
			fprintf(stderr, "create_filter_switch failed\n");
			return(1);
		}
		if(atg_ebox_pack(gfilters, HC_filter_groups[g]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	atg_element *shim2=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!shim2)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	shim2->h=24;
	if(atg_ebox_pack(handle_crews_box, shim2))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *tpipe_header=atg_create_element_label("Training pipeline", 12, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE});
	if(!tpipe_header)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(handle_crews_box, tpipe_header))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *tp_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!tp_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(handle_crews_box, tp_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(HC_tp_BT=atg_create_element_button("BT", (atg_colour){223, 223, 31, ATG_ALPHA_OPAQUE}, HC_TP_BG_COLOUR)))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(tp_box, HC_tp_BT))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	
	for(enum tpipe stage=0;stage<TPIPE__MAX;stage++)
	{
		if(!(HC_tp_box[stage]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR)))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(tp_box, HC_tp_box[stage]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *arrow=atg_create_element_label("→", 16, (atg_colour){159, 159, 159, ATG_ALPHA_OPAQUE});
		if(!arrow)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(HC_tp_box[stage], arrow))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *st_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, HC_TP_BG_COLOUR);
		if(!st_box)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(HC_tp_box[stage], st_box))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(HC_tp_btn[stage]=atg_create_element_button(tpipe_names[stage], (atg_colour){223, 223, 31, ATG_ALPHA_OPAQUE}, HC_TP_BG_COLOUR)))
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		if(atg_ebox_pack(st_box, HC_tp_btn[stage]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *dwell_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, HC_TP_BG_COLOUR);
		if(!dwell_box)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(st_box, dwell_box))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *dwell_lbl=atg_create_element_label("Days: ", 8, (atg_colour){191, 191, 191, ATG_ALPHA_OPAQUE});
		if(!dwell_lbl)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(dwell_box, dwell_lbl))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(HC_tp_dwell[stage]=atg_create_element_spinner(0, 0, max_dwell[stage], 1, 0, "%03u", (atg_colour){191, 191, 191, ATG_ALPHA_OPAQUE}, HC_TP_BG_COLOUR)))
		{
			fprintf(stderr, "atg_create_element_spinner failed\n");
			return(1);
		}
		if(atg_ebox_pack(dwell_box, HC_tp_dwell[stage]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(stage>=TPIPE_LFS)
			continue;
		atg_element *cont_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, HC_TP_BG_COLOUR);
		if(!cont_box)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(st_box, cont_box))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *cont_lbl=atg_create_element_label("Cont: ", 8, (atg_colour){191, 191, 191, ATG_ALPHA_OPAQUE});
		if(!cont_lbl)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(cont_box, cont_lbl))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(HC_tp_cont[stage]=atg_create_element_spinner(0, 0, 100, 2, 0, "%03u%%", (atg_colour){191, 191, 191, ATG_ALPHA_OPAQUE}, HC_TP_BG_COLOUR)))
		{
			fprintf(stderr, "atg_create_element_spinner failed\n");
			return(1);
		}
		if(atg_ebox_pack(cont_box, HC_tp_cont[stage]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	atg_element *shim3=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!shim3)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	shim3->h=16;
	if(atg_ebox_pack(handle_crews_box, shim3))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *sel_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!sel_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(handle_crews_box, sel_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *tp_data_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, HC_TP_BG_COLOUR);
	if (!tp_data_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(sel_box, tp_data_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *tp_data_header=atg_create_element_label("Position", 12, (atg_colour){223, 223, 31, ATG_ALPHA_OPAQUE});
	if(!tp_data_header)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(tp_data_box, tp_data_header))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	for(enum cclass i=0;i<CREW_CLASSES;i++)
	{
		atg_element *tp_data_row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, HC_TP_BG_COLOUR);
		if (!tp_data_row)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(tp_data_box, tp_data_row))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *pos=atg_create_element_label(cclasses[i].name, 10, (atg_colour){207, 207, 31, ATG_ALPHA_OPAQUE});
		if(!pos)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		pos->w=68;
		pos->h=16;
		if(atg_ebox_pack(tp_data_row, pos))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(HC_tp_count[i]=malloc(32)))
		{
			perror("malloc");
			return(1);
		}
		strcpy(HC_tp_count[i], "count");
		atg_element *count=atg_create_element_label_nocopy(HC_tp_count[i], 10, (atg_colour){159, 159, 159, ATG_ALPHA_OPAQUE});
		if(!count)
		{
			fprintf(stderr, "atg_create_element_label_nocopy failed\n");
			return(1);
		}
		if(atg_ebox_pack(tp_data_row, count))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(HC_tp_ecount[i]=malloc(32)))
		{
			perror("malloc");
			return(1);
		}
		strcpy(HC_tp_ecount[i], "ecount");
		atg_element *ecount=atg_create_element_label_nocopy(HC_tp_ecount[i], 10, (atg_colour){159, 159, 159, ATG_ALPHA_OPAQUE});
		if(!ecount)
		{
			fprintf(stderr, "atg_create_element_label_nocopy failed\n");
			return(1);
		}
		if(atg_ebox_pack(tp_data_row, ecount))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *shim=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){159, 159, 159, ATG_ALPHA_OPAQUE});
		if(!shim)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		shim->w=10;
		if(atg_ebox_pack(tp_data_row, shim))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(HC_tp_tops[i]=SDL_CreateRGBSurface(SDL_HWSURFACE, 30, HC_SKILL_HHEIGHT, 32,  0xff000000, 0xff0000, 0xff00, 0xff)))
		{
			fprintf(stderr, "HC_tp_tops[]=SDL_CreateRGBSurface: %s\n", SDL_GetError());
			return(1);
		}
		atg_element *tops=atg_create_element_image(HC_tp_tops[i]);
		if(!tops)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		if(atg_ebox_pack(tp_data_row, tops))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	atg_element *text_guard=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!text_guard)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	text_guard->w=414;
	if(atg_ebox_pack(sel_box, text_guard))
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
	if(!(HC_tp_text_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	HC_tp_text_box->w=410;
	if(atg_ebox_pack(text_guard, HC_tp_text_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	return(0);
}

void update_selstage(unsigned int *counts, unsigned int *ecounts, unsigned int *acounts, enum tpipe stage)
{
	atg_ebox_empty(HC_tp_text_box);
	const char *bodytext=stage>=TPIPE__MAX ? tpipe_bt_desc : tpipe_descs[stage];
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
		if(atg_ebox_pack(HC_tp_text_box, r))
		{
			perror("atg_ebox_pack");
			atg_free_element(r);
			break;
		}
		x+=l;
		if(bodytext[x]=='\n') x++;
	}
	for(enum cclass i=0;i<CREW_CLASSES;i++)
	{
		if(stage<TPIPE__MAX)
			snprintf(HC_tp_count[i], 32, "%4u/%4u", acounts[i], counts[i]);
		else
			snprintf(HC_tp_count[i], 32, "         ");
		snprintf(HC_tp_ecount[i], 32, "-%3u", ecounts[i]);
	}
}

void recalc_tp_tops(game *state, enum tpipe stage)
{
	for(enum cclass c=0;c<CREW_CLASSES;c++)
	{
		unsigned int dens[30]={0}, mxd=1, dw=state->tpipe[stage].dwell, sc=max((dw+29)/30, 1);
		if(!HC_tp_tops[c]) continue;
		SDL_FillRect(HC_tp_tops[c], &(SDL_Rect){0, 0, HC_tp_tops[c]->w, HC_tp_tops[c]->h}, SDL_MapRGB(HC_tp_tops[c]->format, 7, 7, 15));
		if(stage>=TPIPE__MAX)
			continue;
		for(unsigned int i=0;i<state->ncrews;i++)
		{
			if(state->crews[i].status != CSTATUS_STUDENT)
				continue;
			if(state->crews[i].class != c)
				continue;
			if(state->crews[i].training != stage)
				continue;
			unsigned int x=state->crews[i].tour_ops/sc;
			if(x>=30)
				continue;
			dens[x]++;
			if(x)
				mxd=max(dens[x], mxd);
		}
		unsigned int h=HC_tp_tops[c]->h-1;
		for(unsigned int x=0;x<30;x++)
		{
			double frac=min(dens[x]/(double)mxd, 1.0);
			unsigned int br=63+ceil(frac*192);
			unsigned int y=h-ceil(frac*h);
			bool end=x==dw/sc;
			if(end)
				line(HC_tp_tops[c], x, 0, x, y-1, (atg_colour){47, 47, 15, ATG_ALPHA_OPAQUE});
			line(HC_tp_tops[c], x, y, x, h+1, (atg_colour){br, end ? br : 0, end ? 15 : 0, ATG_ALPHA_OPAQUE});
		}
	}
}

void recalc_ecounts(game *state, unsigned int *ecounts, unsigned int *acounts, enum tpipe stage)
{
	memset(ecounts, 0, sizeof(unsigned int[CREW_CLASSES]));
	memset(acounts, 0, sizeof(unsigned int[CREW_CLASSES]));
	for(unsigned int i=0;i<state->ncrews;i++)
	{
		if(state->crews[i].status != CSTATUS_STUDENT)
			continue;
		if(state->crews[i].training != stage)
			continue;
		if(state->crews[i].assignment < 0 && !(state->crews[i].class==CCLASS_E && stage==TPIPE_OTU))
			continue;
		acounts[state->crews[i].class]++;
		if((int)state->crews[i].tour_ops>=state->tpipe[stage].dwell)
			ecounts[state->crews[i].class]++;
	}
}

screen_id handle_crews_screen(atg_canvas *canvas, game *state)
{
	/* We might have been given more aircraft.  So fill 'em up with students. */
	refill_students(state, false);
	atg_event e;
	unsigned int counts[TPIPE__MAX+1][CREW_CLASSES]={0};
	unsigned int acounts[TPIPE__MAX][CREW_CLASSES]={0};
	unsigned int ecounts[TPIPE__MAX+1][CREW_CLASSES]={0};
	if((HC_filter_groups[6]->hidden=datebefore(state->now, event[EVENT_PFF])))
		hc_filter_groups[6]=0;
	for(unsigned int i=0;i<state->ncrews;i++)
	{
		if(state->crews[i].status == CSTATUS_INSTRUC)
		{
			enum cclass j=state->crews[i].class;
			counts[TPIPE__MAX][j]+=cclasses[j].pupils;
			if(cclasses[j].extra_pupil != CCLASS_NONE)
				counts[TPIPE__MAX][cclasses[j].extra_pupil]++;
		}
		if(state->crews[i].status != CSTATUS_STUDENT)
			continue;
		counts[state->crews[i].training][state->crews[i].class]++;
		if(state->crews[i].assignment < 0 && !(state->crews[i].class==CCLASS_E && state->crews[i].training==TPIPE_OTU))
			continue;
		acounts[state->crews[i].training][state->crews[i].class]++;
		if((int)state->crews[i].tour_ops>=state->tpipe[state->crews[i].training].dwell)
			ecounts[state->crews[i].training][state->crews[i].class]++;
	}
	for(enum cclass j=0;j<CREW_CLASSES;j++)
	{
		unsigned int scount=0;
		for(enum tpipe stage=0;stage<TPIPE__MAX;stage++)
			scount+=counts[stage][j];
		unsigned int pool=(counts[TPIPE__MAX][j]+cclasses[j].initpool)/GET_DC(state, TPOOL);
		if(scount<pool)
			ecounts[TPIPE__MAX][j]=min(pool-scount, max((pool-scount)/16, 1));
	}

	for(enum tpipe stage=0; stage<TPIPE__MAX; stage++)
	{
		if((HC_tp_box[stage]->hidden=state->tpipe[stage].dwell<0))
			continue;
		atg_spinner *s;
		if(HC_tp_dwell[stage])
		{
			s=HC_tp_dwell[stage]->elemdata;
			s->value=state->tpipe[stage].dwell;
		}
		if(stage<TPIPE_LFS && HC_tp_cont[stage])
		{
			s=HC_tp_cont[stage]->elemdata;
			s->value=state->tpipe[stage].cont;
		}
	}
	recalc_tp_tops(state, selstage);

	while(1)
	{
		update_selstage(counts[selstage], ecounts[selstage], acounts[selstage], selstage);
		update_crews(state);
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
					if(c.e==HC_full)
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
					enum tpipe stage;
					if(trigger.e==HC_cont)
						return(SCRN_CONTROL);
					if(trigger.e==HC_tp_BT)
					{
						selstage=TPIPE__MAX;
						recalc_tp_tops(state, selstage);
						break;
					}
					for(stage=0;stage<TPIPE__MAX;stage++)
						if(trigger.e==HC_tp_btn[stage])
						{
							selstage=stage;
							recalc_tp_tops(state, selstage);
							break;
						}
					if(stage<TPIPE__MAX)
						break;
					fprintf(stderr, "Clicked on unknown button!\n");
				break;
				case ATG_EV_VALUE:
					for(enum tpipe stage=0;stage<TPIPE__MAX;stage++)
					{
						if(e.event.value.e==HC_tp_dwell[stage]) {
							state->tpipe[stage].dwell=e.event.value.value;
							recalc_ecounts(state, ecounts[stage], acounts[stage], stage);
							recalc_tp_tops(state, selstage);
						}
						if(stage<TPIPE_LFS && e.event.value.e==HC_tp_cont[stage]) {
							state->tpipe[stage].cont=e.event.value.value;
							recalc_ecounts(state, ecounts[stage], acounts[stage], stage);
						}
					}
				break;
				default:
				break;
			}
		}
		SDL_Delay(50);
	}
}

void handle_crews_free(void)
{
	atg_free_element(handle_crews_box);
}

void update_crews(game *state)
{
	unsigned int count[CREW_CLASSES][SHOW_STATUSES];
	unsigned int dens[CREW_CLASSES][SHOW_STATUSES][101];
	unsigned int tops[CREW_CLASSES][2][31];
	unsigned int need[CREW_CLASSES];
	unsigned int pool[CREW_CLASSES];
	for(unsigned int i=0;i<CREW_CLASSES;i++)
	{
		for(unsigned int j=0;j<SHOW_STATUSES;j++)
		{
			count[i][j]=0;
			for(unsigned int k=0;k<101;k++)
				dens[i][j][k]=0;
			if(j<2)
				for(unsigned int k=0;k<31;k++)
					tops[i][j][k]=0;
		}
		need[i]=0;
		pool[i]=cclasses[i].initpool;
	}
	bool agroup=false;
	bool eats=!datebefore(state->now, event[EVENT_EATS]);
	for(unsigned int g=0;g<8;g++)
		if(hc_filter_groups[g]>0)
			agroup=true;
	for(unsigned int i=0;i<state->ncrews;i++)
	{
		unsigned int cls=state->crews[i].class, sta=state->crews[i].status;
		if(sta>=SHOW_STATUSES)
			continue;
		if(sta==CSTATUS_CREWMAN)
		{
			int g=state->crews[i].group-1;
			if(g>=7)
				g=6;
			if(g<0)
				g=7;
			if(hc_filter_groups[g]<(agroup?1:0))
				continue;
		}
		count[cls][sta]++;
		unsigned int skill=floor(state->crews[i].skill);
		dens[cls][sta][min(skill, 100)]++;
		if(sta==CSTATUS_CREWMAN)
		{
			unsigned int top=state->crews[i].tour_ops;
			tops[cls][0][min(top, 30)]++;
		}
		else if(sta==CSTATUS_INSTRUC)
		{
			unsigned int top=state->crews[i].tour_ops;
			tops[cls][1][min(top/6, 30)]++;
			pool[cls]+=cclasses[cls].pupils;
			if(eats&&cclasses[cls].eats)
				pool[cls]++;
			if(cclasses[cls].extra_pupil!=CCLASS_NONE)
				pool[cclasses[cls].extra_pupil]++;
		}
	}
	for(unsigned int i=0;i<state->nbombers;i++)
		if(!state->bombers[i].train)
		{
			int s=state->bombers[i].squadron, g;
			if(s<0)
				g=7;
			else
			{
				g=base_grp(bases[state->squads[s].base])-1;
				if(g>=7)
					g=6;
			}
			if(hc_filter_groups[g]<(agroup?1:0))
				continue;
			for(unsigned int j=0;j<MAX_CREW;j++)
			{
				enum cclass ct=types[state->bombers[i].type].crew[j];
				if(ct!=CCLASS_NONE)
					need[ct]++;
			}
		}
	if(datebefore(state->now, event[EVENT_FLT_ENG]))
		pool[CCLASS_E]=0;
	for(unsigned int i=0;i<CREW_CLASSES;i++)
		for(unsigned int j=0;j<SHOW_STATUSES;j++)
		{
			unsigned int mxd=0;
			for(unsigned int k=0;k<101;k++)
				mxd=max(dens[i][j][k], mxd);
			switch(j)
			{
				case CSTATUS_CREWMAN:
					snprintf(HC_count[i][j], 32, "%4u/%4u", count[i][j], need[i]);
				break;
				case CSTATUS_STUDENT:
					snprintf(HC_count[i][j], 32, "%4u/%4u", count[i][j], pool[i]/GET_DC(state, TPOOL));
				break;
				case CSTATUS_INSTRUC:
				default:
					snprintf(HC_count[i][j], 32, "%u", count[i][j]);
				break;
			}
			SDL_FillRect(HC_skill[i][j], &(SDL_Rect){0, 0, HC_skill[i][j]->w, HC_skill[i][j]->h}, SDL_MapRGB(HC_skill[i][j]->format, 7, 7, 15));
			line(HC_skill[i][j], 50, 0, 50, HC_SKILL_FHEIGHT, (atg_colour){23, 23, 23, ATG_ALPHA_OPAQUE});
			line(HC_skill[i][j], 25, 0, 25, HC_SKILL_FHEIGHT, (atg_colour){15, 15, 23, ATG_ALPHA_OPAQUE});
			line(HC_skill[i][j], 75, 0, 75, HC_SKILL_FHEIGHT, (atg_colour){15, 15, 23, ATG_ALPHA_OPAQUE});
			if(j<2)
				SDL_FillRect(HC_tops[i][j], &(SDL_Rect){0, 0, HC_tops[i][j]->w, HC_tops[i][j]->h}, SDL_MapRGB(HC_tops[i][j]->format, 7, 7, 15));
			if(count[i][j])
				for(unsigned int k=0;k<101;k++)
				{
					double frac=dens[i][j][k]/(double)mxd;
					unsigned int br=63+ceil(frac*192);
					line(HC_skill[i][j], k, HC_SKILL_HHEIGHT-ceil(frac*HC_SKILL_HHEIGHT), k, HC_SKILL_HHEIGHT+ceil(frac*HC_SKILL_HHEIGHT), (atg_colour){br, br, br, ATG_ALPHA_OPAQUE});
				}
			if(j<2 && count[i][j*2])
			{
				unsigned int mxt=0;
				for(unsigned int k=0;k<31;k++)
					mxt=max(tops[i][j][k], mxt);
				line(HC_tops[i][j], 10, 0, 10, HC_SKILL_FHEIGHT, (atg_colour){15, 15, 23, ATG_ALPHA_OPAQUE});
				line(HC_tops[i][j], 20, 0, 20, HC_SKILL_FHEIGHT, (atg_colour){15, 15, 23, ATG_ALPHA_OPAQUE});
				for(unsigned int k=0;k<31;k++)
				{
					double frac=tops[i][j][k]/(double)mxt;
					unsigned int br=63+ceil(frac*63);
					line(HC_tops[i][j], k, HC_SKILL_HHEIGHT-ceil(frac*HC_SKILL_HHEIGHT), k, HC_SKILL_HHEIGHT+ceil(frac*HC_SKILL_HHEIGHT), (atg_colour){br, 7, 15, ATG_ALPHA_OPAQUE});
				}
			}
		}
}
