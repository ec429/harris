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
#include "render.h"

/*
#define HCTBL_BG_COLOUR	(atg_colour){23, 23, 31, ATG_ALPHA_OPAQUE}
#define HC_SKILL_HHEIGHT	12
#define HC_SKILL_FHEIGHT	(HC_SKILL_HHEIGHT * 2)
#define HC_TP_BG_COLOUR	(atg_colour){23, 23, 63, ATG_ALPHA_OPAQUE}
*/

atg_element *handle_squadrons_box;

atg_element *HS_cont, *HS_full;
atg_element *HS_map, *HS_stnbox, *HS_sqnbox, *HS_fltbox;
atg_element *HS_gpl, *HS_gprow[7];
#define gpnum(_i)	((_i)==6?8:(_i)+1)
atg_element *HS_stl, *HS_stpaved, *HS_stpaving, *HS_sqbtn[2], *HS_nosq;
char *HS_stname, *HS_stpavetime, *HS_stsqnum[2];
atg_element *HS_flbtn[3];
char *HS_sqname, *HS_btman, *HS_btnam, *HS_sqest;
SDL_Surface *HS_btpic;
char *HS_flname;
atg_element *HS_sll, **HS_slrow, **HS_slgl;
char **HS_slgrp, **HS_slsqn;
int selgrp=-1, selstn=-1, selsqn=-1, selflt=-1;

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
	if(atg_ebox_pack(HS_stnbox, HS_stpaved))
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
	if(atg_ebox_pack(HS_stnbox, HS_stpaving))
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
	bases[2].paved=2;
	state->paving=3;
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
	(void)state;
	if((HS_fltbox->hidden=selflt<0))
		return;
	HS_flname[0]='A'+selflt;
}

void update_sqn_info(game *state)
{
	selflt=-1;
	update_flt_info(state);
	if((HS_sqnbox->hidden=selsqn<0))
		return;
	snprintf(HS_sqname, 20, "No. %d Squadron", state->squads[selsqn].number);
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
	unsigned int nom=0, act=0, flts=state->squads[selsqn].third_flight?3:2;
	for(unsigned int f=0;f<flts;f++)
	{
		nom+=10;
		act+=state->squads[selsqn].nb[f];
		snprintf(HS_sqest, 40, " %2d/%2d I.E.", act, nom);
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
	update_sqn_info(state);
	if((HS_stnbox->hidden=selstn<0))
		return;
	snprintf(HS_stname, 48, "%s", bases[selstn].name);
	HS_stpaved->hidden=!bases[selstn].paved;
	if(!(HS_stpaving->hidden=selstn!=state->paving))
		snprintf(HS_stpavetime, 24, "Paving (%d days left)", PAVE_TIME-bases[selstn].pprog);
	update_sqn_list(state);
	HS_nosq->hidden=bases[selstn].nsqns;
	for(unsigned int i=0;i<2;i++)
		if(!(HS_sqbtn[i]->hidden=i>=bases[selstn].nsqns))
			snprintf(HS_stsqnum[i], 12, "No. %d Sqn", state->squads[bases[selstn].sqn[i]].number);
}

screen_id handle_squadrons_screen(atg_canvas *canvas, game *state)
{
	atg_event e;
	update_sqn_list(state);
	update_stn_list(state);

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
							selstn=i;
							update_stn_list(state);
							update_stn_info(state);
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
					for(i=0;i<bases[selstn].nsqns;i++)
						if(trigger.e==HS_sqbtn[i])
						{
							selsqn=bases[selstn].sqn[i];
							update_sqn_info(state);
							break;
						}
					if(i<bases[selstn].nsqns)
						break;
					for(i=0;i<3;i++)
						if(trigger.e==HS_flbtn[i])
						{
							selflt=i;
							update_flt_info(state);
							break;
						}
					if(i<3)
						break;
					if(trigger.e==HS_cont)
						return(SCRN_CONTROL);
					fprintf(stderr, "Clicked on unknown button!\n");
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
