/*
	harris - a strategy game
	Copyright (C) 2012-2020 Edward Cree

	licensed under GPLv2 - see top of harris.c for details

	handle_manfs: screen for tasking manufacturers' R&D
*/

#include "ui.h"
#include "globals.h"
#include "bits.h"
#include "date.h"
#include "builder/data.h"
#include "builder/calc.h"
#include "builder.h"
#include "control.h"

atg_element *handle_manfs_box;

atg_element *HM_cont, *HM_full;
atg_element *HM_mbscroll, **HM_mbox;
char *HM_out_buf[OUT_ROWS];
SDL_Surface *HM_bp;
atg_element *HM_proto, *HM_tool, *HM_halt;
atg_element *HM_fresh, *HM_mark, *HM_mod;
atg_element *HM_doc, *HM_dice;

int handle_manfs_create(void)
{
	handle_manfs_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!handle_manfs_box)
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
	if(atg_ebox_pack(handle_manfs_box, leftbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *title=atg_create_element_label("HARRIS: Manufacturer Development", 12, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
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
	if(!(HM_cont=atg_create_element_button("Continue", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(top_box, HM_cont))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HM_full=atg_create_element_image(fullbtn);
	if(!HM_full)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	HM_full->clickable=true;
	if(atg_ebox_pack(top_box, HM_full))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(HM_mbox=calloc(builder->entities.nmanf, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	atg_element *mboxes=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!mboxes)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	for(unsigned int i=0;i<builder->entities.nmanf;i++)
	{
		struct manf *m=builder->entities.manf[i];
		atg_element *mbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
		if(!mbox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(mboxes, mbox))
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
		shim->w=8;
		shim->h=2;
		if(atg_ebox_pack(mbox, shim))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *mname=atg_create_element_label(m->name, 12, (atg_colour){255, 255, 223, ATG_ALPHA_OPAQUE});
		if(!mname)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(mbox, mname))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		HM_mbox[i]=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
		if(!HM_mbox[i])
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(mbox, HM_mbox[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	if(!(HM_mbscroll=atg_create_element_scroll(mboxes, SCROLL_FG_COLOUR, GAME_BG_COLOUR)))
	{
		fprintf(stderr, "atg_create_element_scroll failed\n");
		return(1);
	}
	HM_mbscroll->h=600;
	HM_mbscroll->w=leftbox->w;
	if(atg_ebox_pack(leftbox, HM_mbscroll))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *rightbox;
	int rc=builder_rightbox_create(&rightbox, HM_out_buf, &HM_bp, GAME_BG_COLOUR);
	if(rc==1)
		atg_free_element(rightbox);
	if(rc)
		return(1);
	if(atg_ebox_pack(handle_manfs_box, rightbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *actions=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!actions)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(rightbox, actions))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HM_proto=atg_create_element_button("Prototype", (atg_colour){239, 79, 79, ATG_ALPHA_OPAQUE}, GAME_BG_COLOUR);
	if(!HM_proto)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(actions, HM_proto))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HM_tool=atg_create_element_button("Tool", (atg_colour){223, 223, 79, ATG_ALPHA_OPAQUE}, GAME_BG_COLOUR);
	if(!HM_tool)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(actions, HM_tool))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HM_halt=atg_create_element_button("Halt work", (atg_colour){159, 79, 79, ATG_ALPHA_OPAQUE}, GAME_BG_COLOUR);
	if(!HM_halt)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(actions, HM_halt))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *refits=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!refits)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(rightbox, refits))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *reflbl=atg_create_element_label("Use as basis for: ", 12, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!reflbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(refits, reflbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(HM_fresh=atg_create_element_button("New design", (atg_colour){239, 239, 255, ATG_ALPHA_OPAQUE}, GAME_BG_COLOUR)))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(refits, HM_fresh))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(HM_mark=atg_create_element_button("Mark refit", (atg_colour){239, 239, 255, ATG_ALPHA_OPAQUE}, GAME_BG_COLOUR)))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	HM_mark->hidden=true;
	if(atg_ebox_pack(refits, HM_mark))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(HM_mod=atg_create_element_button("Mod refit", (atg_colour){239, 239, 255, ATG_ALPHA_OPAQUE}, GAME_BG_COLOUR)))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	HM_mod->hidden=true;
	if(atg_ebox_pack(refits, HM_mod))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *showbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!showbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(rightbox, showbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *showlbl=atg_create_element_label("Show: ", 12, (atg_colour){179, 179, 195, ATG_ALPHA_OPAQUE});
	if(!showlbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(showbox, showlbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HM_doc=atg_create_element_toggle("Current doctrine", true, (atg_colour){159, 159, 179, ATG_ALPHA_OPAQUE}, GAME_BG_COLOUR);
	if(!HM_doc)
	{
		fprintf(stderr, "atg_create_element_toggle failed\n");
		return(1);
	}
	if(atg_ebox_pack(showbox, HM_doc))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HM_dice=atg_create_element_toggle("Test results", true, (atg_colour){159, 159, 179, ATG_ALPHA_OPAQUE}, GAME_BG_COLOUR);
	if(!HM_dice)
	{
		fprintf(stderr, "atg_create_element_toggle failed\n");
		return(1);
	}
	if(atg_ebox_pack(showbox, HM_dice))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	return(0);
}

void wipe_m2v(char **outbuf, SDL_Surface *bp)
{
	for(unsigned int i=0;i<OUT_ROWS;i++)
		*outbuf[i]=0;
	atg_colour bp_bg=BB_BP_COLOUR;
	SDL_FillRect(bp, &(SDL_Rect){0, 0, bp->w, bp->h}, SDL_MapRGB(bp->format, bp_bg.r, bp_bg.g, bp_bg.b));
}

enum design_status {
	DSTA_DRAW,
	DSTA_PROTO,
	DSTA_TOOL
} design_status(const struct bomber *b)
{
	if(b->proto_work<b->tproto)
		return DSTA_DRAW;
	if(b->prod_work<b->tprod)
		return DSTA_PROTO;
	return DSTA_TOOL;
}

atg_colour dsta_colour(enum design_status dsta)
{
	switch(dsta)
	{
	case DSTA_DRAW:
		return (atg_colour){239, 79, 79, ATG_ALPHA_OPAQUE};
	case DSTA_PROTO:
		return (atg_colour){223, 223, 79, ATG_ALPHA_OPAQUE};
	case DSTA_TOOL:
		return (atg_colour){79, 223, 79, ATG_ALPHA_OPAQUE};
	default:
		return GAME_BG_COLOUR;
	}
}

const char *default_mark_names[MAX_MARKS]={"Mk I", "Mk II", "Mk III", "Mk IV"};

void realise_design(struct bomber *bb)
{
	unsigned int type=bb->slot_idx;
	unsigned int mark=bb->mark_idx;
	bombertype *bt=types+type;
	calc_bomber(bb, &bb->tn);
	struct bomber bd=*bb, *b=&bd;
	b->parent=bb;
	b->refit=REFIT_DOCTRINE;
	struct tech_numbers *tn=&builder->tn;
	calc_bomber(b, tn);
	if(bb->refit==REFIT_FRESH)
	{
		snprintf(bt->manu, 40, "%s", b->manf->name);
		// TODO prompt player for a name (and a markname?)
		snprintf(bt->name, 40, "%s", b->name);
		snprintf(GB_btname[type], 80, "%s %s", bt->manu, bt->name);
		bt->entry=b->entry;
		if(!date_before_start(bt->entry))
		{
			bt->novelty=bt->entry;
			bt->novelty.month+=4;
			if(bt->novelty.month>12)
			{
				bt->novelty.month-=12;
				bt->novelty.year++;
			}
		}
	}
	unsigned int mrcap[2];
	unsigned int mrange[2];
	for(unsigned int concrete=0;concrete<2;concrete++)
	{
		struct bomber bmr=*b; /* bomber at Max Range */
		unsigned int mptow, mts, mtg;
		int delta;
		bmr.tanks.pct=100;
		bmr.parent=b;
		bmr.refit=REFIT_DOCTRINE;
		calc_bomber(&bmr, tn);
		mts = concrete && tn->rcs ? tn->rcs : tn->rgs;
		mtg = concrete && tn->rcg ? tn->rcg : tn->rgg;
		mptow = floor(wing_lift(&bmr.wing, mts / 1.6f));
		mptow = min(mptow, mtg * 1000);
		mptow = min(mptow, bmr.mtow);
		delta = bmr.gross - mptow;
		if((int)bmr.bay.load < delta)
		{
			delta-=bmr.bay.load;
			bmr.bay.load = mrcap[concrete] = 0;
			bmr.tanks.pct=ceil(100.0f*(1.0f - delta/(bmr.tanks.hlb*100.0f)));
		}
		else
		{
			bmr.bay.load = mrcap[concrete] = min(((int)bmr.bay.load) - delta, (int)bmr.bay.cap);
		}
		calc_bomber(&bmr, tn);
		mrange[concrete]=ceil(bmr.range/1.5f);
	}
	unsigned int mprange[2];
	struct bomber bmp[2]; /* bomber at Max Payload */
	for(unsigned int concrete=0;concrete<2;concrete++)
	{
		bmp[concrete] = *b;
		unsigned int mptow, mts, mtg;
		int delta;
		bmp[concrete].tanks.pct=100;
		bmp[concrete].parent=b;
		bmp[concrete].refit=REFIT_DOCTRINE;
		calc_bomber(&bmp[concrete], tn);
		mts = concrete && tn->rcs ? tn->rcs : tn->rgs;
		mtg = concrete && tn->rcg ? tn->rcg : tn->rgg;
		mptow = floor(wing_lift(&bmp[concrete].wing, mts / 1.6f));
		mptow = min(mptow, mtg * 1000);
		mptow = min(mptow, bmp[concrete].mtow);
		delta = bmp[concrete].gross - mptow;
		bmp[concrete].tanks.pct=max(ceil(100.0*(1.0f - delta/(bmp[concrete].tanks.hlb*100.0f))), 0);
		calc_bomber(&bmp[concrete], tn);
		delta = bmp[concrete].gross - mptow;
		if((int)bmp[concrete].bay.load < delta)
		{
			bmp[concrete].bay.load = 0;
		}
		else
		{
			bmp[concrete].bay.load = min(((int)bmp[concrete].bay.load) - delta, (int)bmp[concrete].bay.cap);
		}
		calc_bomber(&bmp[concrete], tn);
		mprange[concrete]=ceil(bmp[concrete].range/1.5f);
	}
	/* use Max Payload and corresponding fuel load to calculate stats */
	b = &bmp[1];
	for(unsigned int m=mark;m<MAX_MARKS;m++)
	{
		struct bomberstats *bs=bt->mark+m;
		bs->cost=floor(b->cost);
		bs->speed=ceil(b->cruise_spd);
		bs->alt=ceil(b->ceiling*10.0f);
		bs->capwt=b->bay.load;
		bs->capbulk=b->bay.cap;
		bs->svp=ceil(b->serv*100.0f);
		bs->defn=floor(b->defn[0]);
		bs->desch=floor(b->defn[1]);
		bs->deflk=floor(b->flak_factor);
		bs->fail=floor(b->fail*100.0f);
		bs->accu=ceil(b->accu*100.0f);
		bs->range=mprange[0];
		bs->mrcap=mrcap[0];
		bs->mrange=mrange[0];
		bs->crange=mprange[1];
		bs->cmcap=mrcap[1];
		bs->cmrange=mrange[1];
		for(unsigned int c=0;c<MAX_CREW;c++)
		{
			if(c>=b->crew.n)
			{
				bs->crew[c]=CCLASS_NONE;
				continue;
			}
			bs->crew[c]=b->crew.men[c].pos;
			if(b->crew.men[c].gun)
				switch(b->crew.men[c].pos)
				{
				case CCLASS_B:
					bs->crewbg=true;
					break;
				case CCLASS_W:
					bs->crewwg=true;
					break;
				default:
					// XXX no support yet for N (crewng)
					break;
				}
		}
		for(unsigned int n=0;n<NNAVAIDS;n++)
			bs->nav[n]=b->elec.navaid[n];
		bt->markname[m]=NULL;
		// MOD refit is applicable only to a single mark
		if(bb->refit>=REFIT_MOD)
			break;
	}
	bt->noarm=b->turrets.uab;
	bt->heavy=b->engines.number>=4;
	bt->smbay=b->bay.girth<BB_COOKIE;
	bt->load[BL_ABNORMAL]=bt->load[BL_USUAL]=bt->load[BL_ARSON]=true;
	bt->load[BL_ILLUM]=!bt->heavy;
	bt->load[BL_PONLY]=b->bay.cookie&&b->bay.cap<5000;
	// XXX plumduff has special SMBAY handling that's probably not correct for anything other than a Halifax
	bt->load[BL_PLUMDUFF]=b->bay.cookie&&b->bay.cap>=5000;
	bt->load[BL_PPLUS]=b->bay.cookie&&!bt->smbay&&b->bay.cap>8000;
	bt->load[BL_MINES]=b->bay.mine;
	bt->inc=false;
	bt->extra=true;
	bt->slowgrow=false;
	bt->otub=false;
	// TODO we need rules for this
	bt->lfs=false;
	bt->load[BL_HALFHALF]=bt->load[BL_ILLUM]&&b->bay.cookie&&!bt->smbay;
	bt->train=bt->exit=(date){9999, 99, 99};
	bt->convertfrom=-1;
	bt->newmark=max(bt->newmark, mark);
	bt->markname[mark]=strdup(default_mark_names[mark]);
	return;
}

int assign_slots(game *state, struct bomber *b)
{
	unsigned int type, mark;
	switch(b->refit)
	{
	case REFIT_FRESH:
		if(state->next_custom_slot>=ntypes)
		{
			fprintf(stderr, "No slots left!\n");
			return(1);
		}
		type=state->next_custom_slot++;
		if(!type) // can't happen
			fprintf(stderr, "slot_idx zero used, bad stuff will happen!\n");
		mark=0;
		break;
	case REFIT_MARK:
		type=b->parent->slot_idx;
		if(types[type].newmark+1>=MAX_MARKS)
		{
			fprintf(stderr, "No mark slots left!\n");
			return(1);
		}
		mark=++types[type].newmark;
		break;
	case REFIT_MOD:
		type=b->parent->slot_idx;
		mark=b->parent->mark_idx;
		break;
	default:
		fprintf(stderr, "Bad refit_level, everything will catch fire!\n");
		type=mark=0;
		break;
	}
	b->slot_idx=type;
	b->mark_idx=mark;
	return(0);
}

void update_refit_buttons(const game *state, struct bomber *b)
{
	if(!b)
	{
		HM_fresh->hidden=HM_mark->hidden=HM_mod->hidden=true;
		return;
	}
	HM_fresh->hidden=state->next_custom_slot>=ntypes;
	HM_mark->hidden=design_status(b)<DSTA_TOOL||types[b->slot_idx].newmark+1>=MAX_MARKS;
	HM_mod->hidden=design_status(b)<DSTA_TOOL;
	HM_dice->hidden=design_status(b)<DSTA_PROTO;
}

void update_m2v(struct bomber *b, char **outbuf)
{
	struct bomber db, pb;
	struct randomisation pd={};
	if(b->refit)
		pd=b->parent->dice;
	calc_bomber(b, &b->tn);
	atg_event ve;
	if(atg_value_event(HM_doc, &ve)==0 && ve.type==ATG_EV_TOGGLE && ve.event.toggle.state)
	{
		db=*b;
		doc_tech(&db.tn, &builder->tn);
		b=&db;
	}
	if(atg_value_event(HM_dice, &ve)==0 && ve.type==ATG_EV_TOGGLE && !ve.event.toggle.state)
	{
		pb=*b;
		pb.dice=pd;
		b=&pb;
	}
	calc_bomber(b, &b->tn);
	builder_update_m2v(b, outbuf);
}

screen_id handle_manfs_screen(atg_canvas *canvas, game *state)
{
	screen_id rc=SCRN_CONTROL;
	atg_event e;

	unsigned int prestart=0;
	if(date_before_start(state->now))
		prestart=state->now.day;
	int seldes=-1;
	atg_element **HM_dbtn=calloc(state->ndesigns, sizeof(atg_element *));
	if(!HM_dbtn)
	{
		perror("calloc");
		return rc;
	}
	atg_element **HM_dsta=calloc(state->ndesigns, sizeof(atg_element *));
	if(!HM_dsta)
	{
		perror("calloc");
		free(HM_dbtn);
		return rc;
	}
	atg_element **HM_dpro=calloc(state->ndesigns, sizeof(atg_element *));
	if(!HM_dpro)
	{
		perror("calloc");
		free(HM_dbtn);
		return rc;
	}
	atg_element **HM_dtoo=calloc(state->ndesigns, sizeof(atg_element *));
	if(!HM_dtoo)
	{
		perror("calloc");
		free(HM_dbtn);
		return rc;
	}
	for(unsigned int i=0;i<builder->entities.nmanf;i++)
	{
		struct manf *m=builder->entities.manf[i];
		atg_element *box=HM_mbox[i];
		atg_ebox_empty(box);
		for(unsigned int j=0;j<state->ndesigns;j++)
		{
			struct bomber *b=state->designs+j;
			if(b->manf!=m) continue;
			calc_bomber(b, &b->tn);
			atg_element *row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
			if(!row)
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				break;
			}
			if(atg_ebox_pack(box, row))
			{
				perror("atg_ebox_pack");
				atg_free_element(row);
				break;
			}
			atg_element *shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
			if(!shim)
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				return(1);
			}
			shim->w=16;
			shim->h=2;
			if(atg_ebox_pack(row, shim))
			{
				perror("atg_ebox_pack");
				return(1);
			}
			HM_dbtn[j]=atg_create_element_button(b->name, (atg_colour){223, 223, 239, ATG_ALPHA_OPAQUE}, GAME_BG_COLOUR);
			if(!HM_dbtn[j])
			{
				fprintf(stderr, "atg_create_element_label failed\n");
				break;
			}
			HM_dbtn[j]->w=159;
			if(atg_ebox_pack(row, HM_dbtn[j]))
			{
				perror("atg_ebox_pack");
				break;
			}
			shim=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
			if(!shim)
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				return(1);
			}
			shim->w=2;
			shim->h=2;
			if(atg_ebox_pack(row, shim))
			{
				perror("atg_ebox_pack");
				return(1);
			}
			HM_dsta[j]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, dsta_colour(design_status(b)));
			if(!HM_dsta[j])
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				break;
			}
			HM_dsta[j]->w=17;
			HM_dsta[j]->h=17;
			if(atg_ebox_pack(row, HM_dsta[j]))
			{
				perror("atg_ebox_pack");
				break;
			}
			if(!(HM_dpro[j]=atg_create_element_image(protopic)))
			{
				fprintf(stderr, "atg_create_element_image failed\n");
				break;
			}
			HM_dpro[j]->hidden=m->proto_idx!=(int)j;
			if(atg_ebox_pack(row, HM_dpro[j]))
			{
				perror("atg_ebox_pack");
				break;
			}
			if(!(HM_dtoo[j]=atg_create_element_image(toolpic)))
			{
				fprintf(stderr, "atg_create_element_image failed\n");
				break;
			}
			HM_dtoo[j]->hidden=m->prod_idx!=(int)j;
			if(atg_ebox_pack(row, HM_dtoo[j]))
			{
				perror("atg_ebox_pack");
				break;
			}
		}
	}
	wipe_m2v(HM_out_buf, HM_bp);
	HM_proto->hidden=HM_tool->hidden=HM_halt->hidden=true;
	update_refit_buttons(state, NULL);

	while(1)
	{
		struct bomber *seldesb=NULL;
		if(seldes>=0)
			seldesb=state->designs+seldes;
		struct manf *seldesm=NULL;
		if(seldesb)
			seldesm=seldesb->manf;
		HM_mbscroll->h=canvas->surface->h-HM_mbscroll->display.y;
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
							goto out;
					}
				break;
				case ATG_EV_CLICK:;
					atg_ev_click c=e.event.click;
					if(c.e==HM_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
						break;
					}
					fprintf(stderr, "Clicked on unknown clickable!\n");
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==HM_cont)
						goto out;
					if(trigger.e==HM_proto)
					{
						if(seldes<0||!seldesb)
						{
							fprintf(stderr, "Tried to proto no design!\n");
							break;
						}
						if(seldesb->error)
						{
							fprintf(stderr, "Tried to proto invalid design!\n");
							break;
						}
						if(design_status(seldesb)>=DSTA_PROTO)
						{
							fprintf(stderr, "Design is already protoed!\n");
							break;
						}
						if(state->next_custom_slot>=ntypes)
						{
							fprintf(stderr, "No slots left for more bomber types!\n");
							break;
						}
						if(!prestart)
						{
							if(!seldesm)
							{
								fprintf(stderr, "Can't find manufacturer for design!\n");
								break;
							}
							if(seldesm->proto_idx>=0)
								HM_dpro[seldesm->proto_idx]->hidden=true;
							seldesm->proto_idx=seldes;
							HM_dpro[seldes]->hidden=false;
							HM_proto->hidden=true;
							HM_halt->hidden=false;
							break;
						}
						if(seldesb->cproto>state->cash)
						{
							fprintf(stderr, "Not enough cash to proto this design.\n");
							break;
						}
						state->cash-=seldesb->cproto;
						seldesb->proto_work=seldesb->tproto;
						// complete the design
						do_randomise(seldesb);
						calc_bomber(seldesb, &seldesb->tn);
						update_m2v(seldesb, HM_out_buf);
						HM_proto->hidden=true;
						update_refit_buttons(state, seldesb);
						if(HM_dsta[seldes])
						{
							atg_box *b=HM_dsta[seldes]->elemdata;
							b->bgcolour=dsta_colour(design_status(seldesb));
						}
						break;
					}
					if(trigger.e==HM_tool)
					{
						if(seldes<0||!seldesb)
						{
							fprintf(stderr, "Tried to tool no design!\n");
							break;
						}
						if(seldesb->error)
						{
							fprintf(stderr, "Tried to tool invalid design!\n");
							break;
						}
						if(design_status(seldesb)>=DSTA_TOOL)
						{
							fprintf(stderr, "Design is already tooled!\n");
							break;
						}
						if(state->next_custom_slot>=ntypes)
						{
							fprintf(stderr, "No slots left for more bomber types!\n");
							break;
						}
						if(!prestart)
						{
							if(!seldesm)
							{
								fprintf(stderr, "Can't find manufacturer for design!\n");
								break;
							}
							if(seldesm->prod_idx>=0)
								HM_dtoo[seldesm->prod_idx]->hidden=true;
							seldesm->prod_idx=seldes;
							HM_dtoo[seldes]->hidden=false;
							HM_tool->hidden=true;
							HM_halt->hidden=false;
							break;
						}
						// ensure dev costs updated
						calc_bomber(seldesb, &seldesb->tn);
						if(design_status(seldesb)<DSTA_PROTO)
						{
							// proto it first.  TODO refactor this
							if(seldesb->cproto>state->cash)
							{
								fprintf(stderr, "Not enough cash to proto this design.\n");
								break;
							}
							state->cash-=seldesb->cproto;
							seldesb->proto_work=seldesb->tproto;
							// complete the design
							do_randomise(seldesb);
							calc_bomber(seldesb, &seldesb->tn);
							update_m2v(seldesb, HM_out_buf);
							HM_proto->hidden=true;
						}
						// TODO handle prestart==2 differently
						if(seldesb->cprod>state->cash)
						{
							fprintf(stderr, "Not enough cash to tool this design.\n");
							break;
						}
						if(assign_slots(state, seldesb))
							break;
						seldesb->prod_work=seldesb->tprod;
						state->cash-=seldesb->cprod;
						seldesb->entry=state->now;
						realise_design(seldesb);
						bombertype *bt=types+seldesb->slot_idx;
						if(seldesb->refit==REFIT_FRESH)
						{
							bt->pc=30000;
							state->btypes[seldesb->slot_idx]=true;
						}
						HM_tool->hidden=true;
						update_refit_buttons(state, seldesb);
						if(HM_dsta[seldes])
						{
							atg_box *b=HM_dsta[seldes]->elemdata;
							b->bgcolour=dsta_colour(design_status(seldesb));
						}
						break;
					}
					if(trigger.e==HM_halt)
					{
						if(seldes<0||!seldesm)
						{
							fprintf(stderr, "Tried to tool no design!\n");
							break;
						}
						if(seldesm->proto_idx==seldes)
							seldesm->proto_idx=-1;
						HM_dpro[seldes]->hidden=true;
						HM_proto->hidden=design_status(seldesb)!=DSTA_DRAW;
						if(seldesm->prod_idx==seldes)
							seldesm->prod_idx=-1;
						HM_dtoo[seldes]->hidden=true;
						HM_tool->hidden=design_status(seldesb)==DSTA_TOOL;
						HM_halt->hidden=true;
						break;
					}
					for(i=0;i<state->ndesigns;i++)
					{
						if(trigger.e==HM_dbtn[i])
						{
							seldes=i;
							struct bomber *b=state->designs+i;
							update_m2v(b, HM_out_buf);
							HM_proto->hidden=design_status(b)!=DSTA_DRAW||b->manf->proto_idx==(int)i;
							HM_tool->hidden=design_status(b)==DSTA_TOOL||b->manf->prod_idx==(int)i;
							HM_halt->hidden=b->manf->proto_idx!=(int)i&&b->manf->prod_idx!=(int)i;
							update_refit_buttons(state, b);
							break;
						}
					}
					if(i<state->ndesigns)
						break;
					if(seldes>=0)
					{
						if(trigger.e==HM_fresh)
						{
							src_design=seldes;
							src_rfl=REFIT_FRESH;
							return(SCRN_BUILDER);
						}
						if(trigger.e==HM_mark)
						{
							src_design=seldes;
							src_rfl=REFIT_MARK;
							return(SCRN_BUILDER);
						}
						if(trigger.e==HM_mod)
						{
							src_design=seldes;
							src_rfl=REFIT_MOD;
							return(SCRN_BUILDER);
						}
					}
					fprintf(stderr, "Clicked on unknown button!\n");
				break;
				case ATG_EV_TOGGLE:;
					atg_ev_toggle toggle=e.event.toggle;
					if(toggle.e==HM_doc||toggle.e==HM_dice)
					{
						if(seldesb)
							update_m2v(seldesb, HM_out_buf);
						break;
					}
					fprintf(stderr, "Clicked on unknown toggle!\n");
				break;
				default:
				break;
			}
		}
		SDL_Delay(50);
	}
out:
	free(HM_dbtn);
	free(HM_dsta);
	free(HM_dpro);
	free(HM_dtoo);
	return(rc);
}

void handle_manfs_free(void)
{
	atg_free_element(handle_manfs_box);
}
