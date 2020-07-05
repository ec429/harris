/*
	harris - a strategy game
	Copyright (C) 2012-2020 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details

	handle_squadrons: screen for managing bases and squadrons
*/

#include <math.h>

#include "handle_crews.h"
#include "ui.h"
#include "globals.h"
#include "date.h"
#include "bits.h"
#include "control.h"
#include "rand.h"
#include "render.h"
#include "run_raid.h" // for crewman_skill

#define CREW_BG_COLOUR	(atg_colour){23, 23, 31, ATG_ALPHA_OPAQUE}

atg_element *handle_squadrons_box;

atg_element *HS_cont, *HS_full;
atg_element *HS_map, *HS_stnbox, *HS_sqnbox, *HS_fltbox;
atg_element *HS_gpl, *HS_gprow[7], *HS_mgw[7];
char *HS_mge[7];
#define gpnum(_i)	((_i)==6?8:(_i)+1)
atg_element **HS_btrow, **HS_btcvt;
char **HS_btnum;
atg_element *HS_stl, *HS_stpaved, *HS_stpaving, *HS_stpavebtn, *HS_sqbtn[2], *HS_nosq, *HS_mbw;
char *HS_stname, *HS_stpavetime, *HS_stsqnum[2], *HS_mbe;
atg_element *HS_sqncr[CREW_CLASSES], *HS_rtimer, *HS_remust, *HS_grow, *HS_split, *HS_sqdis, *HS_flbtn[3];
char *HS_sqname, *HS_btman, *HS_btnam, *HS_sqest, *HS_sqnc[CREW_CLASSES], *HS_rtime;
SDL_Surface *HS_btpic;
atg_element **HS_flcr;
char *HS_flname, *HS_flest, *HS_flsta, *HS_flmark, **HS_flcc, **HS_flcrew;
atg_element *HS_sll, **HS_slrow, **HS_slgl;
char **HS_slgrp, **HS_slsqn;
int selgrp=-1, selstn=-1, selsqn=-1, selflt=-1;
bool remustering=false;

static void generate_snums(game *state)
{
	unsigned int avail=min(SNUM_DEPTH/2, 699-state->nsquads);
	unsigned int shufbuf[699];
	for(unsigned int i=0;i<699;i++)
		shufbuf[i]=i;
	for(unsigned int s=0;s<state->nsquads;s++)
	{
		unsigned int n=state->squads[s].number;
		unsigned int i;
		for(i=0;i<699-s;i++)
			if(shufbuf[i]==n)
				break;
		for(;i<698-s;i++)
			shufbuf[i]=shufbuf[i+1];
	}
	for(unsigned int i=0;i<avail;i++)
	{
		unsigned int j=i+irandu(699-i);
		state->snums[i]=shufbuf[j];
		shufbuf[j]=shufbuf[i];
	}
	state->nsnums=avail;
}

static unsigned int pick_snum(game *state)
{
	if(!state->nsnums)
		generate_snums(state);
	if(!state->nsnums)
		return 0;
	return state->snums[--state->nsnums];
}

static void push_snum(game *state, unsigned int number)
{
	if(state->nsnums>=SNUM_DEPTH-1)
		for(unsigned int i=0;i<SNUM_DEPTH-1;i++)
			state->snums[i]=state->snums[i+1];
	else
		state->nsnums++;
	state->snums[state->nsnums-1]=number;
}

bool mixed_base(game *state, unsigned int b, double *eff)
{
	if(bases[b].nsqns<2)
		return false;
	unsigned int sa=bases[b].sqn[0], sb=bases[b].sqn[1];
	if(state->squads[sa].btype==state->squads[sb].btype)
		return false;
	if(eff)
	{
		unsigned int na=0, nb=0;
		for(unsigned int f=0;f<3;f++)
			na+=state->squads[sa].nb[f];
		for(unsigned int f=0;f<3;f++)
			nb+=state->squads[sb].nb[f];
		*eff=sqrt(max(na, nb)/(double)(na+nb));
	}
	return true;
}

bool mixed_group(game *state, int g, double *eff)
{
	unsigned int nb[ntypes], mnb=0, total=0;
	memset(nb, 0, sizeof(nb));
	for(unsigned int i=0;i<state->nbombers;i++)
	{
		if(state->bombers[i].squadron<0)
			continue;
		unsigned int s=state->bombers[i].squadron;
		if(base_grp(bases[state->squads[s].base])!=gpnum(g))
			continue;
		unsigned int n=++nb[state->bombers[i].type];
		mnb=max(mnb, n);
		total++;
	}
	if(mnb==total)
		return false;
	if(eff)
		*eff=sqrt(mnb/(double)total);
	return true;
}

int handle_squadrons_create(void)
{
	handle_squadrons_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!handle_squadrons_box)
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
	if(atg_ebox_pack(handle_squadrons_box, leftbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *title=atg_create_element_label("HARRIS: Stations & Squadrons", 12, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
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
	if(!(HS_cont=atg_create_element_button("Continue", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(top_box, HS_cont))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_full=atg_create_element_image(fullbtn);
	if(!HS_full)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	HS_full->clickable=true;
	if(atg_ebox_pack(top_box, HS_full))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_gpl=atg_create_element_label("Bomber Groups", 18, (atg_colour){255, 223, 239, ATG_ALPHA_OPAQUE});
	if(!HS_gpl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	HS_gpl->w=238;
	HS_gpl->clickable=true;
	if(atg_ebox_pack(leftbox, HS_gpl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	for(unsigned int i=0;i<7;i++)
	{
		HS_gprow[i]=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!HS_gprow[i])
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		HS_gprow[i]->w=238;
		HS_gprow[i]->clickable=true;
		if(atg_ebox_pack(leftbox, HS_gprow[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		char gpname[12];
		snprintf(gpname, sizeof(gpname), "No. %d Group", gpnum(i));
		atg_element *item=atg_create_element_label(gpname, 16, (atg_colour){159, 143, 143, ATG_ALPHA_OPAQUE});
		if(!item)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		item->cache=true;
		if(atg_ebox_pack(HS_gprow[i], item))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		HS_mge[i]=malloc(40);
		if(!HS_mge[i])
		{
			perror("malloc");
			return(1);
		}
		snprintf(HS_mge[i], 40, " ");
		HS_mgw[i]=atg_create_element_label_nocopy(HS_mge[i], 10, (atg_colour){191, 111, 111, ATG_ALPHA_OPAQUE});
		if(!HS_mgw[i])
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(HS_gprow[i], HS_mgw[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	atg_element *sparc=atg_create_element_label("Pool airframes:", 12, (atg_colour){143, 143, 143, ATG_ALPHA_OPAQUE});
	if(!sparc)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(leftbox, sparc))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(HS_btrow=calloc(ntypes, sizeof(*HS_btrow))))
	{
		perror("calloc");
		return(1);
	}
	if(!(HS_btnum=calloc(ntypes, sizeof(*HS_btnum))))
	{
		perror("calloc");
		return(1);
	}
	if(!(HS_btcvt=calloc(ntypes, sizeof(*HS_btcvt))))
	{
		perror("calloc");
		return(1);
	}
	for(unsigned int i=0;i<ntypes;i++)
	{
		HS_btrow[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!HS_btrow[i])
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		HS_btrow[i]->w=238;
		if(atg_ebox_pack(leftbox, HS_btrow[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *pic=atg_create_element_image(types[i].picture);
		if(!pic)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		pic->w=38;
		if(atg_ebox_pack(HS_btrow[i], pic))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *vbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!vbox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		vbox->w=200;
		vbox->h=38;
		if(atg_ebox_pack(HS_btrow[i], vbox))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		HS_btnum[i]=malloc(8);
		if(!HS_btnum[i])
		{
			perror("malloc");
			return(1);
		}
		snprintf(HS_btnum[i], 8, " ");
		atg_element *btnum=atg_create_element_label_nocopy(HS_btnum[i], 16, (atg_colour){127, 127, 143, ATG_ALPHA_OPAQUE});
		if(!btnum)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(vbox, btnum))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		HS_btcvt[i]=atg_create_element_button("Convert squadron", (atg_colour){191, 191, 47, ATG_ALPHA_OPAQUE}, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!HS_btcvt[i])
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		HS_btcvt[i]->hidden=true;
		if(atg_ebox_pack(vbox, HS_btcvt[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	atg_element *midbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!midbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(handle_squadrons_box, midbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_map=atg_create_element_image(england);
	if(!HS_map)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	HS_map->h=england->h+2;
	HS_map->clickable=true;
	if(atg_ebox_pack(midbox, HS_map))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_stnbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!HS_stnbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	HS_stnbox->hidden=true;
	if(atg_ebox_pack(midbox, HS_stnbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_stname=malloc(48);
	if(!HS_stname)
	{
		perror("malloc");
		return(1);
	}
	HS_stname[0]=0;
	HS_stl=atg_create_element_label_nocopy(HS_stname, 12, (atg_colour){159, 159, 159, ATG_ALPHA_OPAQUE});
	if(!HS_stl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(HS_stnbox, HS_stl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *stnstate=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 47, 55, ATG_ALPHA_OPAQUE});
	if(!stnstate)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(HS_stnbox, stnstate))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_stpaved=atg_create_element_label("Paved", 10, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE});
	if(!HS_stpaved)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(stnstate, HS_stpaved))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_stpavetime=malloc(24);
	if(!HS_stpavetime)
	{
		perror("malloc");
		return(1);
	}
	HS_stpavetime[0]=0;
	HS_stpaving=atg_create_element_label_nocopy(HS_stpavetime, 10, (atg_colour){191, 159, 127, ATG_ALPHA_OPAQUE});
	if(!HS_stpaving)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(stnstate, HS_stpaving))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_stpavebtn=atg_create_element_button("Pave this airfield", (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 55, ATG_ALPHA_OPAQUE});
	if(!HS_stpavebtn)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	HS_stpavebtn->hidden=true;
	if(atg_ebox_pack(stnstate, HS_stpavebtn))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_mbe=malloc(40);
	if(!HS_mbe)
	{
		perror("malloc");
		return(1);
	}
	snprintf(HS_mbe, 40, " ");
	HS_mbw=atg_create_element_label_nocopy(HS_mbe, 10, (atg_colour){191, 111, 111, ATG_ALPHA_OPAQUE});
	if(!HS_mbw)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(stnstate, HS_mbw))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *sqnlist=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!sqnlist)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	sqnlist->w=255;
	if(atg_ebox_pack(HS_stnbox, sqnlist))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	for(unsigned int i=0;i<2;i++)
	{
		HS_sqbtn[i]=atg_create_element_button_empty((atg_colour){191, 191, 79, ATG_ALPHA_OPAQUE}, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
		if(!HS_sqbtn[i])
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		if(atg_ebox_pack(sqnlist, HS_sqbtn[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		HS_sqbtn[i]->hidden=true;
		HS_stsqnum[i]=malloc(12);
		if(!HS_stsqnum[i])
		{
			perror("malloc");
			return(1);
		}
		HS_stsqnum[i][0]=0;
		atg_element *text=atg_create_element_label_nocopy(HS_stsqnum[i], 11, (atg_colour){191, 191, 79, ATG_ALPHA_OPAQUE});
		if(!text)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(HS_sqbtn[i], text))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	HS_nosq=atg_create_element_label("This station has no Squadrons", 11, (atg_colour){95, 95, 95, ATG_ALPHA_OPAQUE});
	if(!HS_nosq)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	HS_nosq->hidden=true;
	if(atg_ebox_pack(sqnlist, HS_nosq))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *separator=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){63, 63, 79, ATG_ALPHA_OPAQUE});
	if(!separator)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	separator->w=256;
	separator->h=1;
	if(atg_ebox_pack(midbox, separator))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_sqnbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!HS_sqnbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(midbox, HS_sqnbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_sqnbox->hidden=true;
	HS_sqname=malloc(20);
	if(!HS_sqname)
	{
		perror("malloc");
		return(1);
	}
	HS_sqname[0]=0;
	atg_element *sqname=atg_create_element_label_nocopy(HS_sqname, 12, (atg_colour){191, 191, 191, ATG_ALPHA_OPAQUE});
	if(!sqname)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(HS_sqnbox, sqname))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_rtime=malloc(24);
	if(!HS_rtime)
	{
		perror("malloc");
		return(1);
	}
	snprintf(HS_rtime, 24, " ");
	HS_rtimer=atg_create_element_label_nocopy(HS_rtime, 12, (atg_colour){191, 63, 63, ATG_ALPHA_OPAQUE});
	if(!HS_rtimer)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	HS_rtimer->hidden=true;
	if(atg_ebox_pack(HS_sqnbox, HS_rtimer))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *rebox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!rebox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(HS_sqnbox, rebox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_remust=atg_create_element_toggle("Remuster", false, (atg_colour){191, 111, 47, ATG_ALPHA_OPAQUE}, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!HS_remust)
	{
		fprintf(stderr, "atg_create_element_toggle failed\n");
		return(1);
	}
	if(atg_ebox_pack(rebox, HS_remust))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_grow=atg_create_element_button("Grow to 3 flights", (atg_colour){191, 191, 47, ATG_ALPHA_OPAQUE}, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!HS_grow)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	HS_grow->hidden=true;
	if(atg_ebox_pack(rebox, HS_grow))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_split=atg_create_element_button("Split C flight", (atg_colour){191, 191, 47, ATG_ALPHA_OPAQUE}, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!HS_split)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	HS_split->hidden=true;
	if(atg_ebox_pack(rebox, HS_split))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_sqdis=atg_create_element_button("Disband!", (atg_colour){255, 31, 15, ATG_ALPHA_OPAQUE}, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!HS_sqdis)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(rebox, HS_sqdis))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *btrow=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
	if(!btrow)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(HS_sqnbox, btrow))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	// Clone the first type's picture, to get an image of the correct size
	HS_btpic=SDL_ConvertSurface(types[0].picture, types[0].picture->format, SDL_HWSURFACE);
	if(!HS_btpic)
	{
		fprintf(stderr, "SDL_ConvertSurface: %s\n", SDL_GetError());
		return(1);
	}
	atg_element *btpic=atg_create_element_image(HS_btpic);
	if(!btpic)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	HS_btpic->w=38;
	if(atg_ebox_pack(btrow, btpic))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *vbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
	if(!vbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(btrow, vbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_btman=malloc(40);
	if(!HS_btman)
	{
		perror("malloc");
		return(1);
	}
	snprintf(HS_btman, 40, " ");
	atg_element *btman=atg_create_element_label_nocopy(HS_btman, 10, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
	if(!btman)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(vbox, btman))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_btnam=malloc(40);
	if(!HS_btnam)
	{
		perror("malloc");
		return(1);
	}
	snprintf(HS_btnam, 40, " ");
	atg_element *btnam=atg_create_element_label_nocopy(HS_btnam, 14, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
	if(!btnam)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(vbox, btnam))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_sqest=malloc(40);
	if(!HS_sqest)
	{
		perror("malloc");
		return(1);
	}
	snprintf(HS_sqest, 40, " ");
	atg_element *sqest=atg_create_element_label_nocopy(HS_sqest, 9, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
	if(!sqest)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(vbox, sqest))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *spare=atg_create_element_label("Spare bods:", 10, (atg_colour){143, 143, 143, ATG_ALPHA_OPAQUE});
	if(!spare)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(HS_sqnbox, spare))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	for(unsigned int c=0;c<CREW_CLASSES;c++)
	{
		HS_sqncr[c]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
		if(!HS_sqncr[c])
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(HS_sqnbox, HS_sqncr[c]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *ccl=atg_create_element_label(cclasses[c].name, 9, (atg_colour){127, 127, 143, ATG_ALPHA_OPAQUE});
		if(!ccl)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		ccl->w=80;
		if(atg_ebox_pack(HS_sqncr[c], ccl))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		HS_sqnc[c]=malloc(6);
		if(!HS_sqnc[c])
		{
			perror("malloc");
			return(1);
		}
		snprintf(HS_sqnc[c], 6, " ");
		atg_element *sqnc=atg_create_element_label_nocopy(HS_sqnc[c], 9, (atg_colour){127, 127, 127, ATG_ALPHA_OPAQUE});
		if(!sqnc)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(HS_sqncr[c], sqnc))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	atg_element *fltlist=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!fltlist)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	fltlist->w=255;
	if(atg_ebox_pack(HS_sqnbox, fltlist))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *fll=atg_create_element_label("Flights: ", 11, (atg_colour){191, 191, 159, ATG_ALPHA_OPAQUE});
	if(!fll)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(fltlist, fll))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	for(unsigned int i=0;i<3;i++)
	{
		char fltname[4]={' ', 'A'+i, ' ', 0};
		HS_flbtn[i]=atg_create_element_button(fltname, (atg_colour){191, 191, 79, ATG_ALPHA_OPAQUE}, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
		if(!HS_flbtn[i])
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		if(atg_ebox_pack(fltlist, HS_flbtn[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	separator=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){63, 63, 79, ATG_ALPHA_OPAQUE});
	if(!separator)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	separator->w=256;
	separator->h=1;
	if(atg_ebox_pack(midbox, separator))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_fltbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!HS_fltbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(midbox, HS_fltbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_fltbox->hidden=true;
	HS_flname=malloc(12);
	if(!HS_flname)
	{
		perror("malloc");
		return(1);
	}
	snprintf(HS_flname, 12, "* Flight");
	atg_element *flname=atg_create_element_label_nocopy(HS_flname, 12, (atg_colour){191, 191, 191, ATG_ALPHA_OPAQUE});
	if(!flname)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(HS_fltbox, flname))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_flest=malloc(12);
	if(!HS_flest)
	{
		perror("malloc");
		return(1);
	}
	snprintf(HS_flest, 12, " ");
	atg_element *flest=atg_create_element_label_nocopy(HS_flest, 11, (atg_colour){159, 159, 159, ATG_ALPHA_OPAQUE});
	if(!flest)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(HS_fltbox, flest))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *flstab=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, CREW_BG_COLOUR);
	if(!flstab)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(HS_fltbox, flstab))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *flstal=atg_create_element_label("Status:", 11, (atg_colour){207, 207, 207, ATG_ALPHA_OPAQUE});
	if(!flstal)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	flstal->w=80;
	flstal->h=16;
	if(atg_ebox_pack(flstab, flstal))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_flsta=malloc(12);
	if(!HS_flsta)
	{
		perror("malloc");
		return(1);
	}
	snprintf(HS_flsta, 12, " ");
	atg_element *flstl=atg_create_element_label_nocopy(HS_flsta, 10, (atg_colour){191, 159, 127, ATG_ALPHA_OPAQUE});
	if(!flstl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(flstab, flstl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *flmab=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, CREW_BG_COLOUR);
	if(!flmab)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(HS_fltbox, flmab))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *flmal=atg_create_element_label("Mark:", 11, (atg_colour){191, 207, 191, ATG_ALPHA_OPAQUE});
	if(!flmal)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	flmal->w=80;
	flmal->h=16;
	if(atg_ebox_pack(flmab, flmal))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_flmark=malloc(12);
	if(!HS_flmark)
	{
		perror("malloc");
		return(1);
	}
	snprintf(HS_flmark, 12, " ");
	atg_element *flmkl=atg_create_element_label_nocopy(HS_flmark, 10, (atg_colour){191, 159, 127, ATG_ALPHA_OPAQUE});
	if(!flmkl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(flmab, flmkl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(HS_flcr=calloc(MAX_CREW, sizeof(*HS_flcr))))
	{
		perror("calloc");
		return(1);
	}
	if(!(HS_flcc=calloc(MAX_CREW, sizeof(*HS_flcc))))
	{
		perror("calloc");
		return(1);
	}
	if(!(HS_flcrew=calloc(MAX_CREW, sizeof(*HS_flcrew))))
	{
		perror("calloc");
		return(1);
	}
	for(unsigned int i=0;i<MAX_CREW;i++)
	{
		HS_flcr[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, CREW_BG_COLOUR);
		if(!HS_flcr[i])
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(HS_fltbox, HS_flcr[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		HS_flcc[i]=malloc(16);
		if(!HS_flcc[i])
		{
			perror("malloc");
			return(1);
		}
		snprintf(HS_flcc[i], 16, " ");
		atg_element *class_label=atg_create_element_label_nocopy(HS_flcc[i], 11, (atg_colour){207, 207, 207, ATG_ALPHA_OPAQUE});
		if(!class_label)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		class_label->w=80;
		class_label->h=16;
		if(atg_ebox_pack(HS_flcr[i], class_label))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		HS_flcrew[i]=malloc(12);
		if(!HS_flcrew[i])
		{
			perror("malloc");
			return(1);
		}
		snprintf(HS_flcrew[i], 12, " ");
		atg_element *crew_label=atg_create_element_label_nocopy(HS_flcrew[i], 10, (atg_colour){191, 159, 127, ATG_ALPHA_OPAQUE});
		if(!crew_label)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(HS_flcr[i], crew_label))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	atg_element *rightbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
	if(!rightbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(handle_squadrons_box, rightbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	HS_sll=atg_create_element_label("Stations", 12, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE});
	if(!HS_sll)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	HS_sll->w=305;
	HS_sll->clickable=true;
	if(atg_ebox_pack(rightbox, HS_sll))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(HS_slrow=calloc(nbases, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(HS_slgl=calloc(nbases, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(HS_slgrp=calloc(nbases, sizeof(*HS_slgrp))))
	{
		perror("calloc");
		return(1);
	}
	if(!(HS_slsqn=calloc(nbases, sizeof(*HS_slsqn))))
	{
		perror("calloc");
		return(1);
	}
	for(unsigned int i=0;i<nbases;i++)
	{
		HS_slrow[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
		if(!HS_slrow[i])
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		HS_slrow[i]->w=305;
		HS_slrow[i]->h=12;
		HS_slrow[i]->clickable=true;
		if(atg_ebox_pack(rightbox, HS_slrow[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		HS_slgrp[i]=malloc(4);
		if(!HS_slgrp[i])
		{
			perror("malloc");
			return(1);
		}
		snprintf(HS_slgrp[i], 4, " ");
		atg_element *item=atg_create_element_label_nocopy(HS_slgrp[i], 10, (atg_colour){247, 247, 191, ATG_ALPHA_OPAQUE});
		if(!item)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		item->w=21;
		if(atg_ebox_pack(HS_slrow[i], item))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		HS_slgl[i]=item;
		item=atg_create_element_label(bases[i].name, 10, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE});
		if(!item)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		item->w=192;
		item->cache=true;
		if(atg_ebox_pack(HS_slrow[i], item))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		HS_slsqn[i]=malloc(12);
		if(!HS_slsqn[i])
		{
			perror("malloc");
			return(1);
		}
		snprintf(HS_slsqn[i], 12, " ");
		item=atg_create_element_label_nocopy(HS_slsqn[i], 10, (atg_colour){247, 247, 191, ATG_ALPHA_OPAQUE});
		if(!item)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		item->w=54;
		if(atg_ebox_pack(HS_slrow[i], item))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	return(0);
}

void update_stn_list(game *state)
{
	/* Also updates group list, map */
	for(int i=0;i<7;i++)
	{
		atg_box *b=HS_gprow[i]->elemdata;
		if(gpnum(i)==selgrp)
			b->bgcolour=(atg_colour){63, 47, 31, ATG_ALPHA_OPAQUE};
		else
			b->bgcolour=(atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE};
	}
	HS_gprow[6]->hidden=datebefore(state->now, event[EVENT_PFF]);
	atg_image *map_img=HS_map->elemdata;
	SDL_FreeSurface(map_img->data);
	map_img->data=SDL_ConvertSurface(england, england->format, england->flags);
	for(int i=0;i<(int)nbases;i++)
	{
		atg_box *b=HS_slrow[i]->elemdata;
		int hilight=(i==selstn?64:0)+(base_grp(bases[i])==selgrp?40:0);
		char br,bg,bb,hr=0,hg=0,hb=0;
		if(bases[i].paved) {
			b->bgcolour=(atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE};
			br=bg=bb=127;
			hr=hilight;
			hb=hilight;
		}
		else if(i==state->paving)
		{
			b->bgcolour=(atg_colour){63, 47, 23, ATG_ALPHA_OPAQUE};
			br=63;
			bg=47;
			bb=23;
			hr=hilight;
			hb=hilight;
		}
		else
		{
			b->bgcolour=(atg_colour){47, 63, 31, ATG_ALPHA_OPAQUE};
			br=47;
			bg=63;
			bb=31;
			hg=hilight;
		}
		SDL_FillRect(map_img->data, &(SDL_Rect){.x=bases[i].lon-2, .y=bases[i].lat-2, .w=5, .h=5}, SDL_MapRGBA(map_img->data->format, br+hr, bg+hg, bb+hb, ATG_ALPHA_OPAQUE));
		SDL_FillRect(map_img->data, &(SDL_Rect){.x=bases[i].lon-1, .y=bases[i].lat-1, .w=3, .h=3}, SDL_MapRGBA(map_img->data->format, br, bg, bb, ATG_ALPHA_OPAQUE));
		if(i==selstn)
		{
			b->bgcolour.r+=23;
			b->bgcolour.g+=23;
		}
		switch(bases[i].nsqns)
		{
			case 1:
				line(map_img->data, bases[i].lon, bases[i].lat-state->squads[bases[i].sqn[0]].third_flight, bases[i].lon, bases[i].lat+2, (atg_colour){143, 95, 0, ATG_ALPHA_OPAQUE});
				break;
			case 2:
				line(map_img->data, bases[i].lon-1, bases[i].lat, bases[i].lon-1, bases[i].lat+2, (atg_colour){143, 95, 0, ATG_ALPHA_OPAQUE});
				line(map_img->data, bases[i].lon+1, bases[i].lat, bases[i].lon+1, bases[i].lat+2, (atg_colour){143, 95, 0, ATG_ALPHA_OPAQUE});
				break;
			case 0:
			default:
				break;
		}
		atg_label *l=HS_slgl[i]->elemdata;
		if(base_grp(bases[i])==selgrp)
			l->colour=(atg_colour){255, 191, 159, ATG_ALPHA_OPAQUE};
		else
			l->colour=(atg_colour){247, 247, 191, ATG_ALPHA_OPAQUE};
		snprintf(HS_slgrp[i], 4, "[%d]", base_grp(bases[i]));
		if(bases[i].nsqns==2)
			snprintf(HS_slsqn[i], 12, "[AB][AB]");
		else if(bases[i].nsqns==1)
		{
			if(state->squads[bases[i].sqn[0]].third_flight)
				snprintf(HS_slsqn[i], 12, "[ABC]");
			else
				snprintf(HS_slsqn[i], 12, "[AB]");
		}
		else
			snprintf(HS_slsqn[i], 12, " ");
	}
}

void update_flt_info(game *state)
{
	if((HS_fltbox->hidden=selflt<0))
		return;
	if(selsqn<0) /* can't happen */
	{
		fprintf(stderr, "selsqn<0 in %s\n", __func__);
		return;
	}
	unsigned int type=state->squads[selsqn].btype;
	HS_flname[0]='A'+selflt;
	snprintf(HS_flest, 40, "%d a/c", state->squads[selsqn].nb[selflt]);
	snprintf(HS_flsta, 12, " ");
	snprintf(HS_flmark, 12, " ");
	for(unsigned int i=0;i<MAX_CREW;i++)
	{
		if((HS_flcr[i]->hidden=types[type].crew[i]>=CCLASS_NONE))
			snprintf(HS_flcc[i], 16, " ");
		else
			snprintf(HS_flcc[i], 16, cclasses[types[type].crew[i]].name);
		snprintf(HS_flcrew[i], 12, " ");
	}
	unsigned int nb=0;
	for(unsigned int i=0;i<state->nbombers;i++)
		if(state->bombers[i].squadron==selsqn && state->bombers[i].flight==selflt)
		{
			snprintf(HS_flsta+nb, 12-nb, "%c",
				 state->bombers[i].failed?'F':
				 !state->bombers[i].landed?'A':
				 '-');
			snprintf(HS_flmark+nb, 12-nb, "%c", 'a'+state->bombers[i].mark);
			for(unsigned int j=0;j<MAX_CREW;j++)
			{
				if(types[type].crew[j]>=CCLASS_NONE)
					continue;
				int k=state->bombers[i].crew[j];
				if(k<0)
					snprintf(HS_flcrew[j]+nb, 12-nb, "-");
				else
				{
					int sk=min(crewman_skill(state->crews+k, type), 90)/10;
					snprintf(HS_flcrew[j]+nb, 12-nb, "%d", sk);
				}
			}
			if(++nb>=10)
				break;
		}
}

void update_sqn_info(game *state)
{
	atg_toggle *t=HS_remust->elemdata;
	t->state=remustering=false;
	selflt=-1;
	bool active=false;
	for(unsigned int i=0;i<state->nbombers;i++)
		if(state->bombers[i].squadron==selsqn)
			if(!state->bombers[i].landed)
			{
				active=true;
				break;
			}
	HS_sqdis->hidden=HS_remust->hidden=active;
	update_flt_info(state);
	for(unsigned int i=0;i<ntypes;i++)
		HS_btcvt[i]->hidden=selsqn<0||state->squads[selsqn].btype==i||active;
	if((HS_sqnbox->hidden=selsqn<0))
		return;
	snprintf(HS_sqname, 20, "No. %d Squadron", state->squads[selsqn].number);
	snprintf(HS_rtime, 24, "Operational in %d days", state->squads[selsqn].rtime);
	HS_rtimer->hidden=!state->squads[selsqn].rtime;
	if (state->squads[selsqn].third_flight)
	{
		HS_grow->hidden=true;
		HS_split->hidden=state->squads[selsqn].nb[2]<8;
	}
	else
	{
		HS_grow->hidden=bases[state->squads[selsqn].base].nsqns>=2;
		HS_split->hidden=true;
	}
	unsigned int type=state->squads[selsqn].btype;
	if(type>=ntypes) /* error */
	{
		snprintf(HS_btman, 40, "internal");
		snprintf(HS_btnam, 40, "Error");
	}
	else
	{
		SDL_FillRect(HS_btpic, &(SDL_Rect){0, 0, HS_btpic->w, HS_btpic->h}, SDL_MapRGB(HS_btpic->format, 0, 0, 0));
		SDL_BlitSurface(types[type].picture, NULL, HS_btpic, NULL);
		snprintf(HS_btman, 40, types[type].manu);
		snprintf(HS_btnam, 40, types[type].name);
	}
	unsigned int sv=0, nom=0, act=0, flts=state->squads[selsqn].third_flight?3:2;
	for(unsigned int i=0;i<state->nbombers;i++)
		if(state->bombers[i].squadron==selsqn)
			if(!state->bombers[i].failed)
				sv++;
	for(unsigned int f=0;f<flts;f++)
	{
		nom+=10;
		act+=state->squads[selsqn].nb[f];
		snprintf(HS_sqest, 40, " %2d/%2d/%2d I.E.", sv, act, nom);
	}
	for(unsigned int c=0;c<CREW_CLASSES;c++)
	{
		snprintf(HS_sqnc[c], 6, "%2d", state->squads[selsqn].nc[c]);
		HS_sqncr[c]->hidden=!state->squads[selsqn].nc[c];
	}
	HS_flbtn[2]->hidden=!state->squads[selsqn].third_flight;
}

void update_sqn_list(game *state)
{
	for(int i=0;i<(int)nbases;i++)
		bases[i].nsqns=0;
	for(unsigned int s=0;s<state->nsquads;s++)
	{
		unsigned int b=state->squads[s].base;
		if(bases[b].nsqns>=2)
		{
			fprintf(stderr, "More than two sqns at base %u\n", b);
			continue;
		}
		bases[b].sqn[bases[b].nsqns++]=s;
	}
}

void update_stn_info(game *state)
{
	selsqn=-1;
	if((HS_stnbox->hidden=selstn<0))
		return update_sqn_info(state);
	if(bases[selstn].nsqns==1)
		selsqn=bases[selstn].sqn[0];
	update_sqn_info(state);
	snprintf(HS_stname, 48, "%s", bases[selstn].name);
	HS_stpaved->hidden=!bases[selstn].paved;
	if(!(HS_stpaving->hidden=selstn!=state->paving))
		snprintf(HS_stpavetime, 24, "Paving (%d days left)", PAVE_TIME-state->pprog);
	HS_stpavebtn->hidden=datebefore(state->now, event[EVENT_PAVING])||bases[selstn].paved||state->paving>=0||bases[selstn].nsqns;
	double eff;
	if(!(HS_mbw->hidden=!mixed_base(state, selstn, &eff)))
		snprintf(HS_mbe, 40, "Mixed types (%.1f%% efficiency)", eff*100.0);
	update_sqn_list(state);
	HS_nosq->hidden=bases[selstn].nsqns;
	for(unsigned int i=0;i<2;i++)
		if(!(HS_sqbtn[i]->hidden=i>=bases[selstn].nsqns))
			snprintf(HS_stsqnum[i], 12, "No. %d Sqn", state->squads[bases[selstn].sqn[i]].number);
}

void update_group_info(game *state)
{
	for(int i=0;i<7;i++)
	{
		double eff;
		if(!(HS_mgw[i]->hidden=!mixed_group(state, i, &eff)))
			snprintf(HS_mge[i], 40, "Mixed types (%.1f%% efficiency)", eff*100.0);
	}
}

void update_pool_info(game *state)
{
	unsigned int count[ntypes];
	memset(count, 0, sizeof(count));
	for(unsigned int i=0;i<state->nbombers;i++)
		if(state->bombers[i].squadron<0 && !state->bombers[i].train)
			count[state->bombers[i].type]++;
	for(unsigned int i=0;i<ntypes;i++)
	{
		HS_btrow[i]->hidden=!(count[i] && datewithin(state->now, types[i].entry, types[i].train) && state->btypes[i]);
		snprintf(HS_btnum[i], 8, "%d", count[i]);
	}
}

screen_id handle_squadrons_screen(atg_canvas *canvas, game *state)
{
	atg_event e;
	remustering=false;
	update_group_info(state);
	update_sqn_list(state);
	update_stn_info(state);
	update_stn_list(state);
	update_pool_info(state);

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
					for(i=0;i<7;i++)
						if(c.e==HS_gprow[i])
						{
							selgrp=gpnum(i);
							update_stn_list(state);
							break;
						}
					if(i<7)
						break;
					for(i=0;i<nbases;i++)
						if(c.e==HS_slrow[i])
						{
							int resqn=-1;
							if(remustering)
							{
								resqn=selsqn;
								bool room;
								switch(bases[i].nsqns)
								{
									case 0:
										room=true;
										break;
									case 1:
										room=!state->squads[bases[i].sqn[0]].third_flight;
										break;
									case 2:
									default:
										room=false;
										break;
								}
								bool active=false;
								for(unsigned int i=0;i<state->nbombers;i++)
									if(state->bombers[i].squadron==selsqn)
										if(!state->bombers[i].landed)
										{
											active=true;
											break;
										}
								if(selsqn>=0 && room && (int)i!=state->paving && !active)
								{
									unsigned int b=state->squads[selsqn].base;
									bool sg=base_grp(bases[b])==base_grp(bases[i]);
									state->squads[selsqn].rtime+=sg?7:14;
									state->squads[selsqn].base=i;
									// update group in case we moved to a new one
									for(unsigned int j=0;j<state->ncrews;j++)
										if(state->crews[j].squadron==selsqn)
											state->crews[j].group=base_grp(bases[i]);
									update_group_info(state);
									update_sqn_list(state);
									selstn=i;
								}
								atg_toggle *t=HS_remust->elemdata;
								t->state=remustering=false;
							}
							else
								selstn=i;
							update_stn_list(state);
							update_stn_info(state);
							if(resqn>=0)
								selsqn=resqn;
							update_sqn_info(state);
							break;
						}
					if(i<nbases)
						break;
					if(c.e==HS_gpl)
					{
						selgrp=-1;
						update_stn_list(state);
						break;
					}
					if(c.e==HS_sll)
					{
						selstn=-1;
						update_stn_list(state);
						update_stn_info(state);
						break;
					}
					if(c.e==HS_map)
					{
						unsigned int mind=(unsigned)-1, best=0;
						for(i=0;i<nbases;i++)
						{
							int dx=bases[i].lon-c.pos.x, dy=bases[i].lat-c.pos.y;
							unsigned int d=dx*dx+dy*dy;
							if(d<mind)
							{
								mind=d;
								best=i;
							}
						}
						if(mind<=35)
							selstn=best;
						else
							selstn=-1;
						update_stn_list(state);
						update_stn_info(state);
						update_sqn_info(state);
						break;
					}
					if(c.e==HS_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
						break;
					}
					fprintf(stderr, "Clicked on unknown clickable!\n");
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==HS_stpavebtn)
					{
						if(datebefore(state->now, event[EVENT_PAVING]))
							break;
						if(state->paving>=0)
							break;
						if(selstn<0)
							break;
						if(bases[selstn].nsqns)
							break;
						state->paving=selstn;
						state->pprog=0;
						update_stn_list(state);
						update_stn_info(state);
						break;
					}
					for(i=0;i<bases[selstn].nsqns;i++)
						if(trigger.e==HS_sqbtn[i])
						{
							selsqn=bases[selstn].sqn[i];
							update_sqn_info(state);
							break;
						}
					if(i<bases[selstn].nsqns)
						break;
					if(trigger.e==HS_grow)
					{
						if(selsqn<0)
							break;
						if(state->squads[selsqn].third_flight)
							break;
						unsigned int b=state->squads[selsqn].base;
						if(bases[b].nsqns>=2)
							break;
						state->squads[selsqn].third_flight=true;
						if(state->squads[selsqn].nb[2])
						{
							fprintf(stderr, "Warning: grow nb[2]=%u\n", state->squads[selsqn].nb[2]);
							state->squads[selsqn].nb[2]=0;
						}
						/* Try to fill the new C flight */
						fill_flights(state);
						update_group_info(state);
						update_pool_info(state);
						update_sqn_list(state);
						update_stn_list(state);
						int resqn=selsqn;
						update_stn_info(state);
						selsqn=resqn;
						update_sqn_info(state);
						break;
					}
					if(trigger.e==HS_split)
					{
						if(selsqn<0)
							break;
						if(!state->squads[selsqn].third_flight)
							break;
						if(state->squads[selsqn].nb[2]<8)
							break;
						unsigned int b=state->squads[selsqn].base;
						if(bases[b].nsqns>=2)
							break;
						/* Form a new squadron from C flight of selsqn */
						squadron *n=realloc(state->squads, (state->nsquads+1)*sizeof(*n));
						if(!n)
						{
							perror("realloc");
							break;
						}
						unsigned int newsqn=state->nsquads++;
						(state->squads=n)[newsqn]=(squadron){
							.number=pick_snum(state),
							.base=b,
							.btype=state->squads[selsqn].btype,
							.rtime=state->squads[selsqn].rtime+7,
							.third_flight=false,
							.nb[0]=state->squads[selsqn].nb[2],
							.nc={0},
						};
						state->squads[selsqn].third_flight=false;
						state->squads[selsqn].nb[2]=0;
						for(unsigned int i=0;i<state->nbombers;i++)
							if(state->bombers[i].squadron==selsqn&&state->bombers[i].flight==2)
							{
								state->bombers[i].squadron=newsqn;
								state->bombers[i].flight=0;
								for(unsigned int j=0;j<MAX_CREW;j++)
									if(state->bombers[i].crew[j]>=0)
									{
										unsigned int k=state->bombers[i].crew[j];
										state->crews[k].squadron=newsqn;
									}
							}
						/* Try to fill the new B flight */
						fill_flights(state);
						update_group_info(state);
						update_pool_info(state);
						update_sqn_list(state);
						update_stn_list(state);
						update_stn_info(state);
						selsqn=newsqn;
						update_sqn_info(state);
						break;
					}
					if(trigger.e==HS_sqdis)
					{
						if(selsqn<0)
							break;
						bool active=false;
						for(unsigned int i=0;i<state->nbombers;i++)
							if(state->bombers[i].squadron==selsqn)
								if(!state->bombers[i].landed)
								{
									active=true;
									break;
								}
						if(active)
							break;
						for(unsigned int i=0;i<state->nbombers;i++)
							if(state->bombers[i].squadron==selsqn)
							{
								clear_sqn(state, i);
								clear_crew(state, i);
							}
							else if(state->bombers[i].squadron>selsqn)
								state->bombers[i].squadron--;
						for(unsigned int i=0;i<state->ncrews;i++)
							if(state->crews[i].squadron==selsqn)
								state->crews[i].squadron=-1;
							else if(state->crews[i].squadron>selsqn)
								state->crews[i].squadron--;
						push_snum(state, state->squads[selsqn].number);
						state->nsquads--;
						for(unsigned int i=selsqn;i<state->nsquads;i++)
							state->squads[i]=state->squads[i+1];
						/* Aircraft and crews may have been freed up */
						fill_flights(state);
						for(unsigned int j=0;j<state->nbombers;j++)
						{
							if(!state->bombers[j].train && state->bombers[j].squadron>=0)
								ensure_crewed(state, j);
						}
						update_pool_info(state);
						update_group_info(state);
						update_sqn_list(state);
						update_stn_list(state);
						update_stn_info(state);
						break;
					}
					for(i=0;i<3;i++)
						if(trigger.e==HS_flbtn[i])
						{
							selflt=i;
							update_flt_info(state);
							break;
						}
					if(i<3)
						break;
					for(i=0;i<ntypes;i++)
						if(trigger.e==HS_btcvt[i])
						{
							if(selsqn<0)
								break;
							if(!state->btypes[i])
								break;
							if(!datewithin(state->now, types[i].entry, types[i].train))
								break;
							/* Begin converting selsqn to the new type */
							bool related=types[i].convertfrom==(int)state->squads[selsqn].btype;
							state->squads[selsqn].btype=i;
							state->squads[selsqn].rtime+=related?14:types[i].heavy?28:21;
							/* Remove existing aircraft from sqn */
							for(unsigned int j=0;j<state->nbombers;j++)
								if(state->bombers[j].squadron==selsqn)
								{
									clear_sqn(state, j);
									clear_crew(state, j);
								}
							for(unsigned int f=0;f<3;f++)
								if(state->squads[selsqn].nb[f])
								{
									fprintf(stderr, "Warning: leftover nb[%u]=%u\n", f, state->squads[selsqn].nb[f]);
									state->squads[selsqn].nb[f]=0;
								}
							/* Try to add some new ones */
							fill_flights(state);
							update_pool_info(state);
							update_group_info(state);
							update_sqn_list(state);
							update_stn_list(state);
							int resqn=selsqn;
							update_stn_info(state);
							selsqn=resqn;
							update_sqn_info(state);
							break;
						}
					if(i<ntypes)
						break;
					if(trigger.e==HS_cont)
						return(SCRN_CONTROL);
					fprintf(stderr, "Clicked on unknown button!\n");
				break;
				case ATG_EV_TOGGLE:;
					atg_ev_toggle toggle=e.event.toggle;
					if(toggle.e==HS_remust)
					{
						bool active=false;
						for(unsigned int i=0;i<state->nbombers;i++)
							if(state->bombers[i].squadron==selsqn)
								if(!state->bombers[i].landed)
								{
									active=true;
									break;
								}
						if(active)
							toggle.state=false;
						remustering=toggle.state;
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
}

void handle_squadrons_free(void)
{
	atg_free_element(handle_squadrons_box);
}
