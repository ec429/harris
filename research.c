/*
	harris - a strategy game
	Copyright (C) 2012-2024 Edward Cree

	licensed under GPLv2 - see top of harris.c for details

	research: screen for research priorities
*/

#include "research.h"
#include "ui.h"
#include "globals.h"
#include "date.h"
#include "bits.h"
#include "builder.h"
#include "builder/data.h"

atg_element *research_box;

atg_element *HR_cont, *HR_full;
atg_element **HR_tb;
char HR_name_buf[40], HR_desc_buf[80];
#define HR_TN_ROWS	12
atg_element *HR_tn_row[HR_TN_ROWS];
char HR_tn_ibuf[HR_TN_ROWS][5], HR_tn_dbuf[HR_TN_ROWS][80];
char HR_tn_nbuf[HR_TN_ROWS][6], HR_tn_obuf[HR_TN_ROWS][6];

int research_create(void)
{
	research_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!research_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	atg_element *leftbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!leftbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	leftbox->w=239;
	if(atg_ebox_pack(research_box, leftbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *title=atg_create_element_label("HARRIS: Research & Development", 12, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!title)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(leftbox, title))
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
	if(atg_ebox_pack(leftbox, top_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(HR_cont=atg_create_element_button("Continue", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(top_box, HR_cont))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HR_full=atg_create_element_image(fullbtn);
	if(!HR_full)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	HR_full->clickable=true;
	if(atg_ebox_pack(top_box, HR_full))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(HR_tb=calloc(builder->entities.ntech, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	for(unsigned int i=0;i<builder->entities.ntech;i++)
	{
		struct tech *t=builder->entities.tech[i];

		atg_element *trow=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
		if(!trow)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(leftbox, trow))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(HR_tb[i]=atg_create_element_button(t->name, (atg_colour){239, 239, 179, ATG_ALPHA_OPAQUE}, GAME_BG_COLOUR)))
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		HR_tb[i]->w=200;
		if(atg_ebox_pack(trow, HR_tb[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	atg_element *midbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!midbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(research_box, midbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	*HR_name_buf=0;
	atg_element *tname=atg_create_element_label_refer(HR_name_buf, 12, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!tname)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(midbox, tname))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *text_guard=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, BB_PAPER_COLOUR);
	if(!text_guard)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	text_guard->w=400;
	if(atg_ebox_pack(midbox, text_guard))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, BB_PAPER_COLOUR);
	if(!shim)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	shim->w=2;
	shim->h=11;
	if(atg_ebox_pack(text_guard, shim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	*HR_desc_buf=0;
	atg_element *tdesc=atg_create_element_label_refer(HR_desc_buf, 9, BB_INK_COLOUR);
	if(!tdesc)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	tdesc->w=396;
	if(atg_ebox_pack(text_guard, tdesc))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	for(unsigned int i=0;i<HR_TN_ROWS;i++)
	{
		if(!(HR_tn_row[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR)))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		HR_tn_row[i]->hidden=true;
		if(atg_ebox_pack(midbox, HR_tn_row[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		*HR_tn_ibuf[i]=0;
		atg_element *tni=atg_create_element_label_refer(HR_tn_ibuf[i], 7, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE});
		if(!tni)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		tni->w=20;
		if(atg_ebox_pack(HR_tn_row[i], tni))
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
		shim->h=12;
		if(atg_ebox_pack(HR_tn_row[i], shim))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		*HR_tn_nbuf[i]=0;
		atg_element *tnn=atg_create_element_label_refer(HR_tn_nbuf[i], 10, (atg_colour){47, 223, 31, ATG_ALPHA_OPAQUE});
		if(!tnn)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		tnn->w=30;
		if(atg_ebox_pack(HR_tn_row[i], tnn))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		*HR_tn_obuf[i]=0;
		atg_element *tno=atg_create_element_label_refer(HR_tn_obuf[i], 10, (atg_colour){179, 31, 31, ATG_ALPHA_OPAQUE});
		if(!tno)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		tno->w=40;
		if(atg_ebox_pack(HR_tn_row[i], tno))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		*HR_tn_dbuf[i]=0;
		atg_element *tnd=atg_create_element_label_refer(HR_tn_dbuf[i], 10, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE});
		if(!tnd)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(HR_tn_row[i], tnd))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	return(0);
}

int tech_time(date now, const struct tech *tech)
{
	return tech->year*12+tech->month - (now.year*12+now.month);
}

bool tech_far_off(date now, const struct tech *tech)
{
	return tech_time(now, tech)>6;
}

void update_midbox(int seltech)
{
	if(seltech<0)
	{
		snprintf(HR_name_buf, sizeof(HR_name_buf), "No tech selected");
		*HR_desc_buf=0;
		for(unsigned int i=0;i<HR_TN_ROWS;i++)
			HR_tn_row[i]->hidden=true;
	}
	else
	{
		struct tech *t=builder->entities.tech[seltech];
		snprintf(HR_name_buf, sizeof(HR_name_buf), "%s", t->name);
		snprintf(HR_desc_buf, sizeof(HR_desc_buf), "%s", t->desc);
		unsigned int i=0;
		for(unsigned int j=0;j<sizeof(t->num);j+=sizeof(unsigned int))
		{
			char *p=((char *)&t->num)+j;
			unsigned int *v=(unsigned int *)p;
			if(!*v)
				continue;
			HR_tn_row[i]->hidden=false;
			snprintf(HR_tn_ibuf[i], sizeof(HR_tn_ibuf[i]), "%s", ident_tn(j));
			snprintf(HR_tn_dbuf[i], sizeof(HR_tn_dbuf[i]), "%s", describe_tn(j));
			snprintf(HR_tn_nbuf[i], sizeof(HR_tn_nbuf[i]), "%u", *v);
			unsigned int *o=(unsigned int *)(((char *)&builder->tn)+j);
			snprintf(HR_tn_obuf[i], sizeof(HR_tn_obuf[i]), "(%u)", *o);
			if(++i>=HR_TN_ROWS)
				goto full;
		}
		for(unsigned int j=0;j<ARRAY_SIZE(t->eng);j++)
		{
			if(!t->eng[j])
				continue;
			HR_tn_row[i]->hidden=false;
			snprintf(HR_tn_ibuf[i], sizeof(HR_tn_ibuf[i]), "%s", t->eng[j]->ident);
			snprintf(HR_tn_dbuf[i], sizeof(HR_tn_dbuf[i]), "%s %s", t->eng[j]->manu, t->eng[j]->name);
			snprintf(HR_tn_nbuf[i], sizeof(HR_tn_nbuf[i]), "%u", t->eng[j]->bhp);
			snprintf(HR_tn_obuf[i], sizeof(HR_tn_obuf[i]), "hp");
			if(++i>=HR_TN_ROWS)
				goto full;
		}
		for(unsigned int j=0;j<ARRAY_SIZE(t->gun);j++)
		{
			if(!t->gun[j])
				continue;
			HR_tn_row[i]->hidden=false;
			snprintf(HR_tn_ibuf[i], sizeof(HR_tn_ibuf[i]), "%s", t->gun[j]->ident);
			snprintf(HR_tn_dbuf[i], sizeof(HR_tn_dbuf[i]), "%s", t->gun[j]->name);
			unsigned int n=t->gun[j]->gun,c=303;
			if(n>4)
			{
				n/=2;
				c=50;
			}
			snprintf(HR_tn_nbuf[i], sizeof(HR_tn_nbuf[i]), "%ux", n);
			snprintf(HR_tn_obuf[i], sizeof(HR_tn_obuf[i]), ".%u", c);
			if(++i>=HR_TN_ROWS)
				goto full;
		}
		for(;i<HR_TN_ROWS;i++)
			HR_tn_row[i]->hidden=true;
full:
	}
}

screen_id research_screen(atg_canvas *canvas, game *state)
{
	int seltech=-1;
	atg_event e;

	for(unsigned int i=0;i<builder->entities.ntech;i++)
	{
		struct tech *t=builder->entities.tech[i];
		atg_button *btn=HR_tb[i]->elemdata;

		// don't show techs we already have, or can't research yet
		HR_tb[i]->hidden=!t->have_reqs||tech_far_off(state->now, t)||t->unlocked;
		if(HR_tb[i]->hidden) continue;
		btn->fgcolour=tech_time(state->now, t)>0?(atg_colour){127, 127, 127, ATG_ALPHA_OPAQUE}:(atg_colour){239, 239, 179, ATG_ALPHA_OPAQUE};
	}

	update_midbox(seltech);

	while(1)
	{
		atg_flip(canvas);
		while(atg_poll_event(&e, canvas))
		{
			unsigned int i;
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
					if(c.e==HR_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
						break;
					}
					fprintf(stderr, "Clicked on unknown clickable!\n");
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==HR_cont)
						return(SCRN_CONTROL);
					for(i=0;i<builder->entities.ntech;i++)
					{
						if(trigger.e!=HR_tb[i]) continue;
						struct tech *t=builder->entities.tech[i];
						if(!t->have_reqs)
						{
							fprintf(stderr, "Don't have reqs for tech!\n");
							break;
						}
						if(tech_far_off(state->now, t))
						{
							fprintf(stderr, "Tech not available yet!\n");
							break;
						}
						if(t->unlocked)
						{
							fprintf(stderr, "Tech is already unlocked!\n");
							break;
						}
						seltech=i;
						update_midbox(seltech);
						break;
					}
					if(i<builder->entities.ntech) break;
					fprintf(stderr, "Clicked on unknown button!\n");
				break;
				case ATG_EV_TOGGLE:;
					//atg_ev_toggle toggle=e.event.toggle;
					fprintf(stderr, "Clicked on unknown toggle!\n");
				break;
				default:
				break;
			}
		}
		SDL_Delay(50);
	}
}

void research_free(void)
{
	atg_free_element(research_box);
}
