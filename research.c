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
char HR_name_buf[60], HR_desc_buf[80];
#define HR_TN_ROWS	12
atg_element *HR_tn_row[HR_TN_ROWS];
char HR_tn_ibuf[HR_TN_ROWS][5], HR_tn_dbuf[HR_TN_ROWS][80];
char HR_tn_nbuf[HR_TN_ROWS][6], HR_tn_obuf[HR_TN_ROWS][6];
char HR_tn_rbuf[HR_TN_ROWS][12];
atg_element *HR_r3[3];
char HR_r3_nbuf[3][40];
atg_element *HR_add, *HR_rm;

static int divider(atg_colour fgcolour, atg_element *box)
{
	atg_element *div_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!div_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	div_box->w=box->w;
	div_box->h=5;
	if(atg_ebox_pack(box, div_box))
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
	shim->h=2;
	if(atg_ebox_pack(div_box, shim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *divider=atg_create_element_box(ATG_BOX_PACK_VERTICAL, fgcolour);
	if(!divider)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	divider->w=div_box->w;
	divider->h=1;
	if(atg_ebox_pack(div_box, divider))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	return(0);
}

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
	midbox->w=500;
	if(atg_ebox_pack(research_box, midbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *current_lbl=atg_create_element_label("Currently Researching (max 3)", 12, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!current_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(midbox, current_lbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *crbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!crbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	crbox->w=midbox->w;
	crbox->h=54;
	if(atg_ebox_pack(midbox, crbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	for(unsigned int i=0;i<3;i++)
	{
		if(!(HR_r3[i]=atg_create_element_button_empty((atg_colour){239, 239, 179, ATG_ALPHA_OPAQUE}, GAME_BG_COLOUR)))
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		if(atg_ebox_pack(crbox, HR_r3[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		*HR_r3_nbuf[i]=0;
		atg_element *name=atg_create_element_label_refer(HR_r3_nbuf[i], 12, (atg_colour){239, 239, 179, ATG_ALPHA_OPAQUE});
		if(!name)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(HR_r3[i], name))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	if(divider((atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, midbox))
		return(1);
	atg_element *data_lbl=atg_create_element_label("Technology Info", 12, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!data_lbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(midbox, data_lbl))
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
	text_guard->w=midbox->w;
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
	tdesc->w=text_guard->w-4;
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
		tnd->w=340;
		if(atg_ebox_pack(HR_tn_row[i], tnd))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		*HR_tn_rbuf[i]=0;
		atg_element *tnr=atg_create_element_label_refer(HR_tn_rbuf[i], 10, (atg_colour){179, 179, 179, ATG_ALPHA_OPAQUE});
		if(!tnr)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		tnr->w=68;
		if(atg_ebox_pack(HR_tn_row[i], tnr))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	if(!(HR_add=atg_create_element_button("Add to research", (atg_colour){47, 223, 31, ATG_ALPHA_OPAQUE}, GAME_BG_COLOUR)))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	HR_add->hidden=true;
	if(atg_ebox_pack(midbox, HR_add))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(HR_rm=atg_create_element_button("Cancel research", (atg_colour){179, 31, 31, ATG_ALPHA_OPAQUE}, GAME_BG_COLOUR)))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	HR_rm->hidden=true;
	if(atg_ebox_pack(midbox, HR_rm))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	return(0);
}

int tech_time(date now, const struct tech *tech)
{
	return tech->year*12+tech->month - (now.year*12+now.month);
}

bool tech_future(date now, const struct tech *tech)
{
	return !tech->supported && tech_time(now, tech) > 0;
}

bool tech_far_off(date now, const struct tech *tech)
{
	return tech_time(now, tech)>6;
}

bool can_change(const game *state)
{
	return state->now.day==1 || !diffdate(state->now, (date){1939, 9, 3});
}

unsigned int tech_slots(const game *state)
{
	unsigned int rv=0, i;
	for(i=0;i<3;i++)
		if(!state->researching[i])
			rv++;
	return rv;
}

void update_midbox(const game *state, struct tech *t)
{
	struct tech *supported=NULL;
	for(unsigned int i=0;i<3;i++)
	{
		snprintf(HR_r3_nbuf[i], sizeof(HR_r3_nbuf[i]), "%s",
			 state->researching[i]?state->researching[i]->name:"nil");
		HR_r3[i]->hidden=!state->researching[i];
		if(state->researching[i]&&tech_future(state->now, state->researching[i]))
		{
			if(supported)
				fprintf(stderr, "Warning, multiple support\n");
			supported=state->researching[i];
		}
	}
	if(!t)
	{
		snprintf(HR_name_buf, sizeof(HR_name_buf), "No tech selected");
		*HR_desc_buf=0;
		for(unsigned int i=0;i<HR_TN_ROWS;i++)
			HR_tn_row[i]->hidden=true;
		HR_add->hidden=HR_rm->hidden=true;
	}
	else
	{
		if(t==&supporting)
			snprintf(HR_name_buf, sizeof(HR_name_buf), "%s (%s)", t->name, supported?supported->name:"unused");
		else
			snprintf(HR_name_buf, sizeof(HR_name_buf), "%s (%02u-%04u)", t->name, t->month, t->year);
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
			const char *rfl;
			switch (rfl_tn(j))
			{
			case REFIT_FRESH:
				rfl="New designs";
				break;
			case REFIT_MARK:
				rfl="Mark refits";
				break;
			case REFIT_MOD:
				rfl="Mod refits";
				break;
			case REFIT_DOCTRINE:
				rfl="Immediate";
				break;
			default:
				rfl="???";
				break;
			}
			snprintf(HR_tn_rbuf[i], sizeof(HR_tn_rbuf[i]), "%s", rfl);
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
			snprintf(HR_tn_rbuf[i], sizeof(HR_tn_rbuf[i]), "Unlocks");
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
			snprintf(HR_tn_rbuf[i], sizeof(HR_tn_rbuf[i]), "Unlocks");
			if(++i>=HR_TN_ROWS)
				goto full;
		}
		for(;i<HR_TN_ROWS;i++)
			HR_tn_row[i]->hidden=true;
full:
		if(can_change(state))
		{
			unsigned int need_slots=(tech_future(state->now, t))?2:1;
			HR_add->hidden=tech_slots(state)<need_slots;
			HR_rm->hidden=true;
			for(i=0;i<3;i++)
				if(state->researching[i]==t)
				{
					HR_add->hidden=true;
					HR_rm->hidden=(t==&supporting&&supported);
					break;
				}
		}
		else
		{
			HR_add->hidden=HR_rm->hidden=true;
		}
	}
}

screen_id research_screen(atg_canvas *canvas, game *state)
{
	struct tech *seltech=NULL;
	atg_event e;

	for(unsigned int i=0;i<builder->entities.ntech;i++)
	{
		struct tech *t=builder->entities.tech[i];
		atg_button *btn=HR_tb[i]->elemdata;

		// don't show techs we already have, or can't research yet
		HR_tb[i]->hidden=!t->have_reqs||tech_far_off(state->now, t)||t->unlocked;
		if(HR_tb[i]->hidden) continue;
		btn->fgcolour=tech_future(state->now, t)?(atg_colour){127, 127, 127, ATG_ALPHA_OPAQUE}:(atg_colour){239, 239, 179, ATG_ALPHA_OPAQUE};
	}

	update_midbox(state, seltech);

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
						seltech=t;
						update_midbox(state, seltech);
						break;
					}
					if(i<builder->entities.ntech) break;
					for(i=0;i<3;i++)
					{
						if(trigger.e!=HR_r3[i]) continue;
						if(!state->researching[i])
						{
							fprintf(stderr, "No such r3!\n");
							break;
						}
						seltech=state->researching[i];
						update_midbox(state, seltech);
						break;
					}
					if(i<3) break;
					if(trigger.e==HR_add)
					{
						if(!seltech)
						{
							fprintf(stderr, "No tech to add!\n");
							break;
						}
						if(seltech->unlocked)
						{
							fprintf(stderr, "Tech is already unlocked!\n");
							break;
						}
						if(tech_far_off(state->now, seltech))
						{
							fprintf(stderr, "Tech not available yet!\n");
							break;
						}
						if(!seltech->have_reqs)
						{
							fprintf(stderr, "Don't have reqs for tech!\n");
							break;
						}
						bool future=tech_future(state->now, seltech);
						if(tech_slots(state)<(future?2:1))
						{
							fprintf(stderr, "Not enough open slots!\n");
							break;
						}
						if(future)
						{
							for(i=0;i<3;i++)
							{
								if(!state->researching[i])
								{
									state->researching[i]=&supporting;
									update_midbox(state, seltech);
									break;
								}
							}
						}
						for(i=0;i<3;i++)
						{
							if(!state->researching[i])
							{
								state->researching[i]=seltech;
								update_midbox(state, seltech);
								break;
							}
						}
						if(i<3) break;
						fprintf(stderr, "No open slots!\n");
						break;
					}
					if(trigger.e==HR_rm)
					{
						if(!seltech)
						{
							fprintf(stderr, "No tech to remove!\n");
							break;
						}
						if(tech_future(state->now, seltech))
						{
							// remove Supporting Research
							for(i=0;i<3;i++)
								if(state->researching[i]==&supporting)
								{
									state->researching[i]=NULL;
									break;
								}
							if(i>=3)
								fprintf(stderr, "Warning, did not find support to remove!\n");
						}
						for(i=0;i<3;i++)
							if(state->researching[i]==seltech)
							{
								if(seltech==&supporting)
								{
									fprintf(stderr, "Can't remove support!\n");
									break;
								}
								state->researching[i]=NULL;
								update_midbox(state, seltech);
								break;
							}
						if(i<3) break;
						fprintf(stderr, "Tech not found to remove!\n");
						break;
					}
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
