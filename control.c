/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	control: the raid control screen
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "control.h"
#include "ui.h"
#include "globals.h"
#include "widgets.h"
#include "date.h"
#include "bits.h"
#include "render.h"

extern game state;

atg_box *control_box;
atg_element *GB_resize, *GB_full, *GB_exit;
atg_element *GB_map, *GB_filters;
atg_element **GB_btrow, **GB_btpc, **GB_btnew, **GB_btp, **GB_btnum, **GB_btpic, **GB_btdesc, **GB_navrow, *(*GB_navbtn)[NNAVAIDS], *(*GB_navgraph)[NNAVAIDS];
atg_element *GB_go, *GB_msgrow[MAXMSGS], *GB_save;
atg_element *GB_ttl, **GB_ttrow, **GB_ttdmg, **GB_ttflk, **GB_ttint;
atg_element ***GB_rbrow, ***GB_rbpic, ***GB_raidnum, *(**GB_raidload)[2], *GB_raid_label, *GB_raid;
atg_box **GB_raidbox, *GB_raidbox_empty;
char *GB_datestring, *GB_budget_label, *GB_confid_label, *GB_morale_label;
SDL_Surface *GB_moonimg;
int filter_nav[NNAVAIDS], filter_pff=0;

bool filter_apply(ac_bomber b, int filter_pff, int filter_nav[NNAVAIDS]);

int control_create(void)
{
	control_box=atg_create_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!control_box)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	atg_element *GB_bt=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!GB_bt)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(control_box, GB_bt))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_box *GB_btb=GB_bt->elem.box;
	if(!GB_btb)
	{
		fprintf(stderr, "GB_bt->elem.box==NULL\n");
		return(1);
	}
	GB_datestring=malloc(11);
	if(!GB_datestring)
	{
		perror("malloc");
		return(1);
	}
	atg_element *GB_datetimebox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!GB_datetimebox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(GB_btb, GB_datetimebox))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_box *GB_dtb=GB_datetimebox->elem.box;
	if(!GB_dtb)
	{
		fprintf(stderr, "GB_datetimebox->elem.box==NULL\n");
		return(1);
	}
	atg_element *GB_date=atg_create_element_label("", 12, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
	if(!GB_date)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	GB_date->w=80;
	if(atg_pack_element(GB_dtb, GB_date))
	{
		perror("atg_pack_element");
		return(1);
	}
	else
	{
		atg_label *l=GB_date->elem.label;
		if(l)
			l->text=GB_datestring;
		else
		{
			fprintf(stderr, "GB_date->elem.label==NULL\n");
			return(1);
		}
	}
	GB_moonimg=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 14, 14, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!GB_moonimg)
	{
		fprintf(stderr, "moonimg: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(1);
	}
	atg_element *GB_moonpic=atg_create_element_image(NULL);
	if(!GB_moonpic)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	GB_moonpic->w=18;
	if(atg_pack_element(GB_dtb, GB_moonpic))
	{
		perror("atg_pack_element");
		return(1);
	}
	else
	{
		atg_image *i=GB_moonpic->elem.image;
		if(!i)
		{
			fprintf(stderr, "GB_moonpic->elem.image==NULL\n");
			return(1);
		}
		i->data=GB_moonimg;
	}
	GB_resize=atg_create_element_image(resizebtn);
	if(!GB_resize)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	GB_resize->w=16;
	GB_resize->clickable=true;
	if(atg_pack_element(GB_dtb, GB_resize))
	{
		perror("atg_pack_element");
		return(1);
	}
	GB_full=atg_create_element_image(fullbtn);
	if(!GB_full)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	GB_full->w=16;
	GB_full->clickable=true;
	if(atg_pack_element(GB_dtb, GB_full))
	{
		perror("atg_pack_element");
		return(1);
	}
	GB_exit=atg_create_element_image(exitbtn);
	if(!GB_exit)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	GB_exit->clickable=true;
	if(atg_pack_element(GB_dtb, GB_exit))
	{
		perror("atg_pack_element");
		return(1);
	}
	if(!(GB_btrow=calloc(ntypes, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_btpc=calloc(ntypes, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_btnew=calloc(ntypes, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_btp=calloc(ntypes, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_btnum=calloc(ntypes, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_btpic=calloc(ntypes, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_btdesc=calloc(ntypes, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_navrow=calloc(ntypes, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_navbtn=calloc(ntypes, sizeof(atg_element * [NNAVAIDS]))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_navgraph=calloc(ntypes, sizeof(atg_element * [NNAVAIDS]))))
	{
		perror("calloc");
		return(1);
	}
	for(unsigned int i=0;i<ntypes;i++)
	{
		if(!(GB_btrow[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_pack_element(GB_btb, GB_btrow[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
		GB_btrow[i]->w=239;
		atg_box *b=GB_btrow[i]->elem.box;
		if(!b)
		{
			fprintf(stderr, "GB_btrow[i]->elem.box==NULL\n");
			return(1);
		}
		SDL_Surface *pic=SDL_CreateRGBSurface(SDL_HWSURFACE, 36, 40, types[i].picture->format->BitsPerPixel, types[i].picture->format->Rmask, types[i].picture->format->Gmask, types[i].picture->format->Bmask, types[i].picture->format->Amask);
		if(!pic)
		{
			fprintf(stderr, "pic=SDL_CreateRGBSurface: %s\n", SDL_GetError());
			return(1);
		}
		SDL_FillRect(pic, &(SDL_Rect){0, 0, pic->w, pic->h}, SDL_MapRGB(pic->format, 0, 0, 0));
		SDL_BlitSurface(types[i].picture, NULL, pic, &(SDL_Rect){(36-types[i].picture->w)>>1, (40-types[i].picture->h)>>1, 0, 0});
		GB_btpic[i]=atg_create_element_image(pic);
		SDL_FreeSurface(pic);
		if(!GB_btpic[i])
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		GB_btpic[i]->w=38;
		GB_btpic[i]->clickable=true;
		if(atg_pack_element(b, GB_btpic[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_element *vbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!vbox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_pack_element(b, vbox))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_box *vb=vbox->elem.box;
		if(!vb)
		{
			fprintf(stderr, "vbox->elem.box==NULL\n");
			return(1);
		}
		if(types[i].manu&&types[i].name)
		{
			size_t len=strlen(types[i].manu)+strlen(types[i].name)+2;
			char *fullname=malloc(len);
			if(fullname)
			{
				snprintf(fullname, len, "%s %s", types[i].manu, types[i].name);
				if(!(GB_btdesc[i]=atg_create_element_label(fullname, 10, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE})))
				{
					fprintf(stderr, "atg_create_element_label failed\n");
					return(1);
				}
				GB_btdesc[i]->w=201;
				GB_btdesc[i]->cache=true;
				GB_btdesc[i]->clickable=true;
				if(atg_pack_element(vb, GB_btdesc[i]))
				{
					perror("atg_pack_element");
					return(1);
				}
			}
			else
			{
				perror("malloc");
				return(1);
			}
			free(fullname);
		}
		else
		{
			fprintf(stderr, "Missing manu or name in type %u\n", i);
			return(1);
		}
		if(!(GB_btnum[i]=atg_create_element_label("svble/total", 12, (atg_colour){159, 191, 255, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_pack_element(vb, GB_btnum[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_element *hbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!hbox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_pack_element(vb, hbox))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_box *hb=hbox->elem.box;
		if(!hb)
		{
			fprintf(stderr, "hbox->elem.box==NULL\n");
			return(1);
		}
		if(atg_pack_element(hb, types[i].prio_selector))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_element *pcbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 79, 31, ATG_ALPHA_OPAQUE});
		if(!pcbox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		pcbox->w=3;
		pcbox->h=18;
		if(atg_pack_element(hb, pcbox))
		{
			perror("atg_pack_element");
			return(1);
		}
		if(!(GB_btpc[i]=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		GB_btpc[i]->w=3;
		if(atg_ebox_pack(pcbox, GB_btpc[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(GB_btnew[i]=atg_create_element_label("N", 12, (atg_colour){159, 191, 63, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_pack_element(hb, GB_btnew[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
		if(!(GB_btp[i]=atg_create_element_label("P!", 12, (atg_colour){191, 159, 31, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_pack_element(hb, GB_btp[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
		GB_navrow[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!GB_navrow[i])
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_pack_element(vb, GB_navrow[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
		for(unsigned int n=0;n<NNAVAIDS;n++)
		{
			SDL_Surface *pic=SDL_CreateRGBSurface(SDL_HWSURFACE, 16, 16, navpic[n]->format->BitsPerPixel, navpic[n]->format->Rmask, navpic[n]->format->Gmask, navpic[n]->format->Bmask, navpic[n]->format->Amask);
			if(!pic)
			{
				fprintf(stderr, "pic=SDL_CreateRGBSurface: %s\n", SDL_GetError());
				return(1);
			}
			SDL_FillRect(pic, &(SDL_Rect){.x=0, .y=0, .w=16, .h=16}, SDL_MapRGBA(pic->format, 0, 0, 0, SDL_ALPHA_TRANSPARENT));
			SDL_BlitSurface(navpic[n], NULL, pic, NULL);
			if(!(GB_navbtn[i][n]=atg_create_element_image(pic)))
			{
				fprintf(stderr, "atg_create_element_image failed\n");
				return(1);
			}
			GB_navbtn[i][n]->w=GB_navbtn[i][n]->h=16;
			GB_navbtn[i][n]->clickable=true;
			if(!types[i].nav[n])
				SDL_FillRect(pic, &(SDL_Rect){.x=0, .y=0, .w=16, .h=16}, SDL_MapRGBA(pic->format, 63, 63, 63, SDL_ALPHA_OPAQUE));
			if(atg_ebox_pack(GB_navrow[i], GB_navbtn[i][n]))
			{
				perror("atg_ebox_pack");
				return(1);
			}
			atg_element *graphbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 79, 31, ATG_ALPHA_OPAQUE});
			if(!graphbox)
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				return(1);
			}
			graphbox->w=3;
			graphbox->h=16;
			if(atg_ebox_pack(GB_navrow[i], graphbox))
			{
				perror("atg_ebox_pack");
				return(1);
			}
			if(!(GB_navgraph[i][n]=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE})))
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				return(1);
			}
			GB_navgraph[i][n]->w=3;
			if(atg_ebox_pack(graphbox, GB_navgraph[i][n]))
			{
				perror("atg_ebox_pack");
				return(1);
			}
		}
	}
	GB_go=atg_create_element_button("Run tonight's raids", (atg_colour){159, 191, 255, ATG_ALPHA_OPAQUE}, (atg_colour){31, 63, 31, ATG_ALPHA_OPAQUE});
	if(!GB_go)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	GB_go->w=159;
	if(atg_pack_element(GB_btb, GB_go))
	{
		perror("atg_pack_element");
		return(1);
	}
	GB_save=atg_create_element_button("Save game", (atg_colour){159, 255, 191, ATG_ALPHA_OPAQUE}, (atg_colour){31, 63, 31, ATG_ALPHA_OPAQUE});
	if(!GB_save)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	GB_save->w=159;
	if(atg_pack_element(GB_btb, GB_save))
	{
		perror("atg_pack_element");
		return(1);
	}
	GB_budget_label=malloc(32);
	if(!GB_budget_label)
	{
		perror("malloc");
		return(1);
	}
	atg_element *GB_budget=atg_create_element_label(NULL, 12, (atg_colour){191, 255, 191, ATG_ALPHA_OPAQUE});
	if(!GB_budget)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	else
	{
		atg_label *l=GB_budget->elem.label;
		if(!l)
		{
			fprintf(stderr, "GB_budget->elem.label==NULL\n");
			return(1);
		}
		*(l->text=GB_budget_label)=0;
		GB_budget->w=159;
		if(atg_pack_element(GB_btb, GB_budget))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	GB_confid_label=malloc(32);
	if(!GB_confid_label)
	{
		perror("malloc");
		return(1);
	}
	atg_element *GB_confid=atg_create_element_label(NULL, 12, (atg_colour){191, 255, 191, ATG_ALPHA_OPAQUE});
	if(!GB_confid)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	else
	{
		atg_label *l=GB_confid->elem.label;
		if(!l)
		{
			fprintf(stderr, "GB_confid->elem.label==NULL\n");
			return(1);
		}
		*(l->text=GB_confid_label)=0;
		GB_confid->w=159;
		if(atg_pack_element(GB_btb, GB_confid))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	GB_morale_label=malloc(32);
	if(!GB_morale_label)
	{
		perror("malloc");
		return(1);
	}
	atg_element *GB_morale=atg_create_element_label(NULL, 12, (atg_colour){191, 255, 191, ATG_ALPHA_OPAQUE});
	if(!GB_morale)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	else
	{
		atg_label *l=GB_morale->elem.label;
		if(!l)
		{
			fprintf(stderr, "GB_morale->elem.label==NULL\n");
			return(1);
		}
		*(l->text=GB_morale_label)=0;
		GB_morale->w=159;
		if(atg_pack_element(GB_btb, GB_morale))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	atg_element *GB_msglbl=atg_create_element_label("Messages:", 12, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
	if(!GB_msglbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_pack_element(GB_btb, GB_msglbl))
	{
		perror("atg_pack_element");
		return(1);
	}
	for(unsigned int i=0;i<MAXMSGS;i++)
	{
		if(!(GB_msgrow[i]=atg_create_element_button(NULL, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE}, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		GB_msgrow[i]->hidden=true;
		GB_msgrow[i]->w=GB_btrow[0]->w;
		if(atg_pack_element(GB_btb, GB_msgrow[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	GB_filters=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!GB_filters)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(GB_btb, GB_filters))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_box *GB_fb=GB_filters->elem.box;
	if(!GB_fb)
	{
		fprintf(stderr, "GB_filters->elem.box==NULL\n");
		return(1);
	}
	atg_element *GB_pad=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!GB_pad)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(GB_fb, GB_pad))
	{
		perror("atg_pack_element");
		return(1);
	}
	GB_pad->h=12;
	atg_element *GB_ft=atg_create_element_label("Filters:", 12, (atg_colour){239, 239, 0, ATG_ALPHA_OPAQUE});
	if(!GB_ft)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_pack_element(GB_fb, GB_ft))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *GB_fi=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){23, 23, 31, ATG_ALPHA_OPAQUE});
	if(!GB_fi)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(GB_fb, GB_fi))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_box *GB_fib=GB_fi->elem.box;
	if(!GB_fib)
	{
		fprintf(stderr, "GB_fi->elem.box==NULL\n");
		return(1);
	}
	atg_element *GB_fi_nav[NNAVAIDS];
	for(unsigned int n=0;n<NNAVAIDS;n++)
	{
		filter_nav[n]=0;
		if(!(GB_fi_nav[n]=create_filter_switch(navpic[n], filter_nav+n)))
		{
			fprintf(stderr, "create_filter_switch failed\n");
			return(1);
		}
		if(atg_pack_element(GB_fib, GB_fi_nav[n]))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	atg_element *GB_fi_pff=create_filter_switch(pffpic, &filter_pff);
	if(!GB_fi_pff)
	{
		fprintf(stderr, "create_filter_switch failed\n");
		return(1);
	}
	if(atg_pack_element(GB_fib, GB_fi_pff))
	{
		perror("atg_pack_element");
		return(1);
	}
	SDL_Surface *map=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
	if(!map)
	{
		fprintf(stderr, "map: SDL_ConvertSurface: %s\n", SDL_GetError());
		return(1);
	}
	atg_element *GB_middle=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!GB_middle)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(control_box, GB_middle))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_box *GB_mb=GB_middle->elem.box;
	if(!GB_mb)
	{
		fprintf(stderr, "GB_middle->elem.box==NULL\n");
		return(1);
	}
	GB_map=atg_create_element_image(map);
	if(!GB_map)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	GB_map->h=map->h+2;
	if(atg_pack_element(GB_mb, GB_map))
	{
		perror("atg_pack_element");
		return(1);
	}
	GB_raid_label=atg_create_element_label("Select a Target", 12, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE});
	if(!GB_raid_label)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_pack_element(GB_mb, GB_raid_label))
	{
		perror("atg_pack_element");
		return(1);
	}
	GB_raid=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!GB_raid)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(GB_mb, GB_raid))
	{
		perror("atg_pack_element");
		return(1);
	}
	GB_raidbox_empty=GB_raid->elem.box;
	if(!(GB_raidbox=calloc(ntargs, sizeof(atg_box *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_rbrow=malloc(ntargs*sizeof(atg_element **))))
	{
		perror("malloc");
		return(1);
	}
	if(!(GB_rbpic=malloc(ntargs*sizeof(atg_element **))))
	{
		perror("malloc");
		return(1);
	}
	if(!(GB_raidnum=malloc(ntargs*sizeof(atg_element **))))
	{
		perror("malloc");
		return(1);
	}
	if(!(GB_raidload=malloc(ntargs*sizeof(atg_element *(**)[2]))))
	{
		perror("malloc");
		return(1);
	}
	for(unsigned int i=0;i<ntargs;i++)
	{
		if(!(GB_rbrow[i]=calloc(ntypes, sizeof(atg_element *))))
		{
			perror("calloc");
			return(1);
		}
		if(!(GB_rbpic[i]=calloc(ntypes, sizeof(atg_element *))))
		{
			perror("calloc");
			return(1);
		}
		if(!(GB_raidnum[i]=calloc(ntypes, sizeof(atg_element *))))
		{
			perror("calloc");
			return(1);
		}
		if(!(GB_raidload[i]=calloc(ntypes, sizeof(atg_element *(*)[2]))))
		{
			perror("calloc");
			return(1);
		}
		GB_raidbox[i]=atg_create_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
		if(!GB_raidbox[i])
		{
			fprintf(stderr, "atg_create_box failed\n");
			return(1);
		}
		for(unsigned int j=0;j<ntypes;j++)
		{
			if(!(GB_rbrow[i][j]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE})))
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				return(1);
			}
			if(atg_pack_element(GB_raidbox[i], GB_rbrow[i][j]))
			{
				perror("atg_pack_element");
				return(1);
			}
			GB_rbrow[i][j]->w=256;
			atg_box *b=GB_rbrow[i][j]->elem.box;
			if(!b)
			{
				fprintf(stderr, "GB_rbrow[i][j]->elem.box==NULL\n");
				return(1);
			}
			SDL_Surface *pic=SDL_CreateRGBSurface(SDL_HWSURFACE, 36, 40, types[j].picture->format->BitsPerPixel, types[j].picture->format->Rmask, types[j].picture->format->Gmask, types[j].picture->format->Bmask, types[j].picture->format->Amask);
			if(!pic)
			{
				fprintf(stderr, "pic=SDL_CreateRGBSurface: %s\n", SDL_GetError());
				return(1);
			}
			SDL_FillRect(pic, &(SDL_Rect){0, 0, pic->w, pic->h}, SDL_MapRGB(pic->format, 0, 0, 0));
			SDL_BlitSurface(types[j].picture, NULL, pic, &(SDL_Rect){(36-types[j].picture->w)>>1, (40-types[j].picture->h)>>1, 0, 0});
			atg_element *picture=atg_create_element_image(pic);
			SDL_FreeSurface(pic);
			if(!picture)
			{
				fprintf(stderr, "atg_create_element_image failed\n");
				return(1);
			}
			picture->w=38;
			picture->cache=true;
			(GB_rbpic[i][j]=picture)->clickable=true;
			if(atg_pack_element(b, picture))
			{
				perror("atg_pack_element");
				return(1);
			}
			atg_element *vbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
			if(!vbox)
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				return(1);
			}
			vbox->w=186;
			if(atg_pack_element(b, vbox))
			{
				perror("atg_pack_element");
				return(1);
			}
			atg_box *vb=vbox->elem.box;
			if(!vb)
			{
				fprintf(stderr, "vbox->elem.box==NULL\n");
				return(1);
			}
			if(types[j].manu&&types[j].name)
			{
				size_t len=strlen(types[j].manu)+strlen(types[j].name)+2;
				char *fullname=malloc(len);
				if(fullname)
				{
					snprintf(fullname, len, "%s %s", types[j].manu, types[j].name);
					atg_element *name=atg_create_element_label(fullname, 10, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
					if(!name)
					{
						fprintf(stderr, "atg_create_element_label failed\n");
						return(1);
					}
					name->cache=true;
					name->w=184;
					if(atg_pack_element(vb, name))
					{
						perror("atg_pack_element");
						return(1);
					}
				}
				else
				{
					perror("malloc");
					return(1);
				}
				free(fullname);
			}
			else
			{
				fprintf(stderr, "Missing manu or name in type %u\n", j);
				return(1);
			}
			if(!(GB_raidnum[i][j]=atg_create_element_label("assigned", 12, (atg_colour){159, 191, 255, ATG_ALPHA_OPAQUE})))
			{
				fprintf(stderr, "atg_create_element_label failed\n");
				return(1);
			}
			if(atg_pack_element(vb, GB_raidnum[i][j]))
			{
				perror("atg_pack_element");
				return(1);
			}
			if(targs[i].class==TCLASS_CITY)
			{
				atg_element *pffloadbox = atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE}),
							*mainloadbox = atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
				if(!pffloadbox||!mainloadbox)
				{
					fprintf(stderr, "atg_create_element_box failed\n");
					return(1);
				}
				pffloadbox->w=mainloadbox->w=16;
				if(atg_pack_element(b, pffloadbox))
				{
					perror("atg_pack_element");
					return(1);
				}
				if(atg_pack_element(b, mainloadbox))
				{
					perror("atg_pack_element");
					return(1);
				}
				if(!(GB_raidload[i][j][1]=create_load_selector(&types[j], &state.raids[i].pffloads[j])))
				{
					fprintf(stderr, "create_load_selector failed\n");
					return(1);
				}
				if(atg_pack_element(pffloadbox->elem.box, GB_raidload[i][j][1]))
				{
					perror("atg_pack_element");
					return(1);
				}
				if(!(GB_raidload[i][j][0]=create_load_selector(&types[j], &state.raids[i].loads[j])))
				{
					fprintf(stderr, "create_load_selector failed\n");
					return(1);
				}
				if(atg_pack_element(mainloadbox->elem.box, GB_raidload[i][j][0]))
				{
					perror("atg_pack_element");
					return(1);
				}
			}
			else
			{
				GB_raidload[i][j][0]=NULL;
				GB_raidload[i][j][1]=NULL;
			}
		}
	}
	atg_element *GB_tt=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
	if(!GB_tt)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_pack_element(control_box, GB_tt))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_box *GB_ttb=GB_tt->elem.box;
	if(!GB_ttb)
	{
		fprintf(stderr, "GB_tt->elem.box==NULL\n");
		return(1);
	}
	GB_ttl=atg_create_element_label("Targets", 12, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE});
	if(!GB_ttl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	GB_ttl->clickable=true;
	if(atg_pack_element(GB_ttb, GB_ttl))
	{
		perror("atg_pack_element");
		return(1);
	}
	if(!(GB_ttrow=calloc(ntargs, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_ttdmg=calloc(ntargs, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_ttflk=calloc(ntargs, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_ttint=calloc(ntargs, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	for(unsigned int i=0;i<ntargs;i++)
	{
		GB_ttrow[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
		if(!GB_ttrow[i])
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		GB_ttrow[i]->w=305;
		GB_ttrow[i]->h=12;
		GB_ttrow[i]->clickable=true;
		if(atg_pack_element(GB_ttb, GB_ttrow[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_box *b=GB_ttrow[i]->elem.box;
		if(!b)
		{
			fprintf(stderr, "GB_ttrow[i]->elem.box==NULL\n");
			return(1);
		}
		atg_element *item;
		if(targs[i].p_intel)
		{
			GB_ttint[i]=atg_create_element_image(intelbtn);
			if(!GB_ttint[i])
			{
				fprintf(stderr, "atg_create_element_image failed\n");
				return(1);
			}
			GB_ttint[i]->clickable=true;
			item=GB_ttint[i];
		}
		else
		{
			GB_ttint[i]=NULL;
			item=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
			if(!item)
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				return(1);
			}
		}
		item->w=8;
		item->h=12;
		if(atg_pack_element(b, item))
		{
			perror("atg_pack_element");
			return(1);
		}
		item=atg_create_element_label(targs[i].name, 10, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE});
		if(!item)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		item->w=192;
		item->cache=true;
		if(atg_pack_element(b, item))
		{
			perror("atg_pack_element");
			return(1);
		}
		item=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
		if(!item)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		item->w=105;
		if(atg_pack_element(b, item))
		{
			perror("atg_pack_element");
			return(1);
		}
		b=item->elem.box;
		if(!b)
		{
			fprintf(stderr, "item->elem.box==NULL\n");
			return(1);
		}
		if(!(GB_ttdmg[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){0, 0, 255, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		GB_ttdmg[i]->w=1;
		GB_ttdmg[i]->h=6;
		if(atg_pack_element(b, GB_ttdmg[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
		if(!(GB_ttflk[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){183, 183, 199, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		GB_ttflk[i]->w=1;
		GB_ttflk[i]->h=6;
		if(atg_pack_element(b, GB_ttflk[i]))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	return(0);
}

screen_id control_screen(atg_canvas *canvas, game *state)
{
	atg_event e;
	
	atg_resize_canvas(canvas, mainsizex, mainsizey);
	snprintf(GB_datestring, 11, "%02u-%02u-%04u\n", state->now.day, state->now.month, state->now.year);
	snprintf(GB_budget_label, 32, "Budget: £%u/day", state->cshr);
	snprintf(GB_confid_label, 32, "Confidence: %u%%", (unsigned int)floor(state->confid+0.5));
	snprintf(GB_morale_label, 32, "Morale: %u%%", (unsigned int)floor(state->morale+0.5));
	double moonphase=pom(state->now);
	drawmoon(GB_moonimg, moonphase);
	for(unsigned int j=0;j<state->nbombers;j++)
	{
		state->bombers[j].landed=true;
		state->bombers[j].damage=0;
		state->bombers[j].ldf=false;
	}
	bool shownav=false;
	filter_pff=0;
	for(unsigned int n=0;n<NNAVAIDS;n++)
	{
		if(!datebefore(state->now, event[navevent[n]])) shownav=true;
		filter_nav[n]=0;
	}
	if(GB_filters)
		GB_filters->hidden=!(shownav||!datebefore(state->now, event[EVENT_PFF]));
	for(unsigned int i=0;i<ntypes;i++)
	{
		if(GB_btrow[i])
			GB_btrow[i]->hidden=!datewithin(state->now, types[i].entry, types[i].exit);
		if(GB_btpc[i])
			GB_btpc[i]->h=18-min(types[i].pcbuf/10000, 18);
		if(GB_btnew[i])
			GB_btnew[i]->hidden=!datebefore(state->now, types[i].novelty);
		if(GB_btp[i])
			GB_btp[i]->hidden=(types[i].pribuf<8)||(state->cash<types[i].cost)||(types[i].pcbuf>=types[i].cost);
		if(GB_btnum[i]&&GB_btnum[i]->elem.label&&GB_btnum[i]->elem.label->text)
		{
			unsigned int svble=0,total=0;
			types[i].count=0;
			for(unsigned int n=0;n<NNAVAIDS;n++)
				types[i].navcount[n]=0;
			for(unsigned int j=0;j<state->nbombers;j++)
				if(state->bombers[j].type==i)
				{
					total++;
					if(!state->bombers[j].failed) svble++;
					types[i].count++;
					for(unsigned int n=0;n<NNAVAIDS;n++)
						if(state->bombers[j].nav[n]) types[i].navcount[n]++;
				}
			snprintf(GB_btnum[i]->elem.label->text, 12, "%u/%u", svble, total);
		}
		GB_navrow[i]->hidden=!shownav;
		for(unsigned int n=0;n<NNAVAIDS;n++)
		{
			update_navbtn(*state, GB_navbtn, i, n, grey_overlay, yellow_overlay);
			if(GB_navgraph[i][n])
				GB_navgraph[i][n]->h=16-(types[i].count?(types[i].navcount[n]*16)/types[i].count:0);
		}
	}
	if(GB_go&&GB_go->elem.button&&GB_go->elem.button->content)
		GB_go->elem.button->content->bgcolour=(atg_colour){31, 63, 31, ATG_ALPHA_OPAQUE};
	for(unsigned int i=0;i<MAXMSGS;i++)
	{
		if(!GB_msgrow[i]) continue;
		GB_msgrow[i]->hidden=!state->msg[i];
		if(state->msg[i])
			if(GB_msgrow[i]->elem.button&&GB_msgrow[i]->elem.button->content)
			{
				atg_box *c=GB_msgrow[i]->elem.button->content;
				if(c->nelems&&c->elems[0]&&(c->elems[0]->type==ATG_LABEL)&&c->elems[0]->elem.label)
				{
					atg_label *l=c->elems[0]->elem.label;
					free(l->text);
					l->text=strndup(state->msg[i], min(strcspn(state->msg[i], "\n"), 33));
				}
			}
	}
	for(unsigned int i=0;i<ntargs;i++)
	{
		if(GB_ttrow[i])
			GB_ttrow[i]->hidden=!datewithin(state->now, targs[i].entry, targs[i].exit);
		if(GB_ttdmg[i])
			switch(targs[i].class)
			{
				case TCLASS_CITY:
				case TCLASS_LEAFLET: // for LEAFLET shows morale rather than damage
				case TCLASS_MINING: // for MINING shows how thoroughly mined the lane is
				case TCLASS_BRIDGE:
				case TCLASS_INDUSTRY:
				case TCLASS_AIRFIELD:
				case TCLASS_ROAD:
					GB_ttdmg[i]->w=floor(state->dmg[i]);
					GB_ttdmg[i]->hidden=false;
				break;
				case TCLASS_SHIPPING:
					GB_ttdmg[i]->w=0;
				break;
				default: // shouldn't ever get here
					fprintf(stderr, "Bad targs[%d].class = %d\n", i, targs[i].class);
					return(1);
				break;
			}
		if(GB_ttflk[i])
			GB_ttflk[i]->w=floor(state->flk[i]);
		bool pff=!datebefore(state->now, event[EVENT_PFF]);
		for(unsigned int j=0;j<ntypes;j++)
		{
			if(GB_rbrow[i][j])
				GB_rbrow[i][j]->hidden=GB_btrow[j]?GB_btrow[j]->hidden:true;
			if(GB_raidload[i][j][0])
				GB_raidload[i][j][0]->hidden=pff&&types[j].noarm;
			if(GB_raidload[i][j][1])
				GB_raidload[i][j][1]->hidden=!pff||!types[j].pff;
			if(GB_raidnum[i][j]&&GB_raidnum[i][j]->elem.label&&GB_raidnum[i][j]->elem.label->text)
			{
				unsigned int count=0;
				for(unsigned int k=0;k<state->raids[i].nbombers;k++)
					if(state->bombers[state->raids[i].bombers[k]].type==j) count++;
				snprintf(GB_raidnum[i][j]->elem.label->text, 9, "%u", count);
			}
		}
		for(unsigned int j=0;j<state->raids[i].nbombers;j++)
			state->bombers[state->raids[i].bombers[j]].landed=false;
	}
	SDL_FreeSurface(GB_map->elem.image->data);
	GB_map->elem.image->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
	SDL_FreeSurface(flak_overlay);
	flak_overlay=render_flak(state->now);
	SDL_BlitSurface(flak_overlay, NULL, GB_map->elem.image->data, NULL);
	SDL_FreeSurface(target_overlay);
	target_overlay=render_targets(state->now);
	SDL_BlitSurface(target_overlay, NULL, GB_map->elem.image->data, NULL);
	SDL_FreeSurface(weather_overlay);
	weather_overlay=render_weather(state->weather);
	SDL_BlitSurface(weather_overlay, NULL, GB_map->elem.image->data, NULL);
	int seltarg=-1;
	xhair_overlay=render_xhairs(*state, seltarg);
	SDL_BlitSurface(xhair_overlay, NULL, GB_map->elem.image->data, NULL);
	free(GB_raid_label->elem.label->text);
	GB_raid_label->elem.label->text=strdup("Select a Target");
	GB_raid->elem.box=GB_raidbox_empty;
	bool rfsh=true;
	while(1)
	{
		if(rfsh)
		{
			for(unsigned int i=0;i<ntargs;i++)
			{
				if(GB_ttrow[i]&&GB_ttrow[i]->elem.box)
				{
					unsigned int raid[ntypes];
					for(unsigned int j=0;j<ntypes;j++)
						raid[j]=0;
					for(unsigned int j=0;j<state->raids[i].nbombers;j++)
						raid[state->bombers[state->raids[i].bombers[j]].type]++;
					for(unsigned int j=0;j<ntypes;j++)
						if(GB_rbrow[i][j])
							GB_rbrow[i][j]->hidden=(GB_btrow[j]?GB_btrow[j]->hidden:true)||!raid[j];
					GB_ttrow[i]->elem.box->bgcolour=(state->raids[i].nbombers?(atg_colour){127, 103, 95, ATG_ALPHA_OPAQUE}:(atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
					if((int)i==seltarg)
					{
						GB_ttrow[i]->elem.box->bgcolour.r=GB_ttrow[i]->elem.box->bgcolour.g=GB_ttrow[i]->elem.box->bgcolour.r+64;
					}
					if(GB_ttrow[i]->elem.box->nelems>1)
					{
						if(GB_ttrow[i]->elem.box->elems[1]&&(GB_ttrow[i]->elem.box->elems[1]->type==ATG_BOX)&&GB_ttrow[i]->elem.box->elems[1]->elem.box)
						{
							GB_ttrow[i]->elem.box->elems[1]->elem.box->bgcolour=GB_ttrow[i]->elem.box->bgcolour;
						}
					}
				}
			}
			GB_raid->elem.box=(seltarg<0)?GB_raidbox_empty:GB_raidbox[seltarg];
			for(unsigned int i=0;i<ntypes;i++)
			{
				if(!GB_btrow[i]->hidden&&GB_btpic[i])
				{
					atg_image *img=GB_btpic[i]->elem.image;
					if(img)
					{
						SDL_Surface *pic=img->data;
						SDL_FillRect(pic, &(SDL_Rect){0, 0, pic->w, pic->h}, SDL_MapRGB(pic->format, 0, 0, 0));
						SDL_BlitSurface(types[i].picture, NULL, pic, &(SDL_Rect){(36-types[i].picture->w)>>1, (40-types[i].picture->h)>>1, 0, 0});
						if(seltarg>=0)
						{
							double dist=hypot((signed)types[i].blat-(signed)targs[seltarg].lat, (signed)types[i].blon-(signed)targs[seltarg].lon)*1.6;
							if(types[i].range<dist)
								SDL_BlitSurface(grey_overlay, NULL, pic, NULL);
						}
					}
				}
			}
			SDL_FreeSurface(GB_map->elem.image->data);
			GB_map->elem.image->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
			SDL_BlitSurface(flak_overlay, NULL, GB_map->elem.image->data, NULL);
			SDL_BlitSurface(target_overlay, NULL, GB_map->elem.image->data, NULL);
			SDL_BlitSurface(weather_overlay, NULL, GB_map->elem.image->data, NULL);
			SDL_FreeSurface(xhair_overlay);
			xhair_overlay=render_xhairs(*state, seltarg);
			SDL_BlitSurface(xhair_overlay, NULL, GB_map->elem.image->data, NULL);
			atg_flip(canvas);
			rfsh=false;
		}
		while(atg_poll_event(&e, canvas))
		{
			if(e.type!=ATG_EV_RAW) rfsh=true;
			switch(e.type)
			{
				case ATG_EV_RAW:;
					SDL_Event s=e.event.raw;
					switch(s.type)
					{
						case SDL_QUIT:
							do_quit:
							free(state->hist.ents);
							state->hist.ents=NULL;
							state->hist.nents=state->hist.nalloc=0;
							for(unsigned int i=0;i<ntargs;i++)
							{
								state->raids[i].nbombers=0;
								free(state->raids[i].bombers);
								state->raids[i].bombers=NULL;
							}
							mainsizex=canvas->surface->w;
							mainsizey=canvas->surface->h;
							return(SCRN_MAINMENU);
						break;
						case SDL_VIDEORESIZE:
							rfsh=true;
						break;
					}
				break;
				case ATG_EV_CLICK:;
					atg_ev_click c=e.event.click;
					atg_mousebutton b=c.button;
					SDLMod m = SDL_GetModState();
					if(c.e)
					{
						for(unsigned int i=0;i<ntypes;i++)
						{
							if(!datewithin(state->now, types[i].entry, types[i].exit)) continue;
							for(unsigned int n=0;n<NNAVAIDS;n++)
							{
								if(c.e==GB_navbtn[i][n])
								{
									if(b==ATG_MB_LEFT)
									{
										if(m&KMOD_CTRL)
											state->nap[n]=-1;
										else if((m&KMOD_ALT)&&(m&KMOD_SHIFT))
											state->nap[n]=ntypes;
										else if(types[i].nav[n]&&!datebefore(state->now, event[navevent[n]]))
											state->nap[n]=i;
									}
									else if(b==ATG_MB_RIGHT)
										fprintf(stderr, "%ux %s in %ux %s %s\n", types[i].navcount[n], navaids[n], types[i].count, types[i].manu, types[i].name);
									else if(b==ATG_MB_MIDDLE)
										state->nap[n]=-1;
									for(unsigned int j=0;j<ntypes;j++)
										update_navbtn(*state, GB_navbtn, j, n, grey_overlay, yellow_overlay);
								}
							}
							if(c.e==GB_btpic[i])
							{
								if(seltarg<0)
									fprintf(stderr, "btrow %d\n", i);
								else
								{
									double dist=hypot((signed)types[i].blat-(signed)targs[seltarg].lat, (signed)types[i].blon-(signed)targs[seltarg].lon)*1.6;
									if(types[i].range<dist)
										fprintf(stderr, "insufficient range: %u<%g\n", types[i].range, dist);
									else
									{
										unsigned int amount;
										switch(b)
										{
											case ATG_MB_RIGHT:
											case ATG_MB_SCROLLDN:
												amount=1;
											break;
											case ATG_MB_MIDDLE:
												amount=-1;
											break;
											case ATG_MB_LEFT:
												if(m&KMOD_CTRL)
												{
													amount=-1;
													break;
												}
												/* else fallthrough */
											case ATG_MB_SCROLLUP:
											default:
												amount=10;
										}
										for(unsigned int j=0;j<state->nbombers;j++)
										{
											if(state->bombers[j].type!=i) continue;
											if(state->bombers[j].failed) continue;
											if(!state->bombers[j].landed) continue;
											if(!filter_apply(state->bombers[j], filter_pff, filter_nav)) continue;
											state->bombers[j].landed=false;
											amount--;
											unsigned int n=state->raids[seltarg].nbombers++;
											unsigned int *new=realloc(state->raids[seltarg].bombers, state->raids[seltarg].nbombers*sizeof(unsigned int));
											if(!new)
											{
												perror("realloc");
												state->raids[seltarg].nbombers=n;
												break;
											}
											(state->raids[seltarg].bombers=new)[n]=j;
											if(!amount) break;
										}
										if(GB_raidnum[seltarg][i]&&GB_raidnum[seltarg][i]->elem.label&&GB_raidnum[seltarg][i]->elem.label->text)
										{
											unsigned int count=0;
											for(unsigned int j=0;j<state->raids[seltarg].nbombers;j++)
												if(state->bombers[state->raids[seltarg].bombers[j]].type==i) count++;
											snprintf(GB_raidnum[seltarg][i]->elem.label->text, 9, "%u", count);
										}
									}
								}
							}
							if((c.e==GB_btdesc[i])&&types[i].text)
							{
								message_box(canvas, "From the Bomber Command files:", types[i].text, "R.H.M.S. Saundby, SASO");
							}
						}
						if(c.e==GB_ttl)
						{
							seltarg=-1;
							free(GB_raid_label->elem.label->text);
							GB_raid_label->elem.label->text=strdup("Select a Target");
							GB_raid->elem.box=GB_raidbox_empty;
						}
						for(unsigned int i=0;i<ntargs;i++)
						{
							if(!datewithin(state->now, targs[i].entry, targs[i].exit)) continue;
							if(c.e==GB_ttint[i])
							{
								intel *ti=targs[i].p_intel;
								if(ti&&ti->ident&&ti->text)
								{
									size_t il=strlen(ti->ident), ml=il+32;
									char *msg=malloc(ml);
									snprintf(msg, ml, "Target Intelligence File: %s", ti->ident);
									message_box(canvas, msg, ti->text, "Wg Cdr Fawssett, OC Targets");
								}
							}
							if(c.e==GB_ttrow[i])
							{
								seltarg=i;
								free(GB_raid_label->elem.label->text);
								size_t ll=9+strlen(targs[i].name);
								GB_raid_label->elem.label->text=malloc(ll);
								snprintf(GB_raid_label->elem.label->text, ll, "Raid on %s", targs[i].name);
								GB_raid->elem.box=GB_raidbox[i];
							}
							for(unsigned int j=0;j<ntypes;j++)
							{
								if(c.e==GB_rbpic[i][j])
								{
									unsigned int amount;
									switch(b)
									{
										case ATG_MB_RIGHT:
										case ATG_MB_SCROLLDN:
											amount=1;
										break;
										case ATG_MB_MIDDLE:
											amount=-1;
										break;
										case ATG_MB_LEFT:
											if(m&KMOD_CTRL)
											{
												amount=-1;
												break;
											}
											/* else fallthrough */
										case ATG_MB_SCROLLUP:
										default:
											amount=10;
									}
									for(unsigned int l=0;l<state->raids[i].nbombers;l++)
									{
										unsigned int k=state->raids[i].bombers[l];
										if(state->bombers[k].type!=j) continue;
										if(!filter_apply(state->bombers[k], filter_pff, filter_nav)) continue;
										state->bombers[k].landed=true;
										amount--;
										state->raids[i].nbombers--;
										for(unsigned int k=l;k<state->raids[i].nbombers;k++)
											state->raids[i].bombers[k]=state->raids[i].bombers[k+1];
										if(!amount) break;
										l--;
									}
									if(GB_raidnum[i][j]&&GB_raidnum[i][j]->elem.label&&GB_raidnum[i][j]->elem.label->text)
									{
										unsigned int count=0;
										for(unsigned int l=0;l<state->raids[i].nbombers;l++)
											if(state->bombers[state->raids[i].bombers[l]].type==j) count++;
										snprintf(GB_raidnum[i][j]->elem.label->text, 9, "%u", count);
									}
								}
							}
						}
						if(c.e==GB_resize)
						{
							mainsizex=default_w;
							mainsizey=default_h;
							atg_resize_canvas(canvas, mainsizex, mainsizey);
						}
						if(c.e==GB_full)
						{
							fullscreen=!fullscreen;
							atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
							rfsh=true;
						}
						if(c.e==GB_exit)
							goto do_quit;
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e)
					{
						if(trigger.e==GB_go)
						{
							if(GB_go&&GB_go->elem.button&&GB_go->elem.button->content)
								GB_go->elem.button->content->bgcolour=(atg_colour){55, 55, 55, ATG_ALPHA_OPAQUE};
							atg_flip(canvas);
							return(SCRN_RUNRAID);
						}
						else if(trigger.e==GB_save)
						{
							mainsizex=canvas->surface->w;
							mainsizey=canvas->surface->h;
							return(SCRN_SAVEGAME);
						}
						for(unsigned int i=0;i<MAXMSGS;i++)
							if((trigger.e==GB_msgrow[i])&&state->msg[i])
							{
								message_box(canvas, "To the Commander-in-Chief, Bomber Command:", state->msg[i], "Air Chief Marshal C. F. A. Portal, CAS");
							}
					}
				break;
				case ATG_EV_VALUE:
					// ignore
				break;
				default:
					fprintf(stderr, "e.type %d\n", e.type);
				break;
			}
		}
		SDL_Delay(50);
		#if 0 // for testing weathersim code
		rfsh=true;
		for(unsigned int i=0;i<16;i++)
			w_iter(&state->weather, lorw);
		SDL_FreeSurface(GB_map->elem.image->data);
		GB_map->elem.image->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
		SDL_FillRect(GB_map->elem.image->data, &(SDL_Rect){.x=0, .y=0, .w=terrain->w, .h=terrain->h}, SDL_MapRGB(terrain->format, 0, 0, 0));
		SDL_FreeSurface(weather_overlay);
		weather_overlay=render_weather(state->weather);
		SDL_BlitSurface(weather_overlay, NULL, GB_map->elem.image->data, NULL);
		SDL_FreeSurface(xhair_overlay);
		xhair_overlay=render_xhairs(state, seltarg);
		SDL_BlitSurface(xhair_overlay, NULL, GB_map->elem.image->data, NULL);
		#endif
	}
}

void control_free(void)
{
	atg_free_box_box(control_box);
}

bool filter_apply(ac_bomber b, int filter_pff, int filter_nav[NNAVAIDS])
{
	if(filter_pff>0&&!b.pff) return(false);
	if(filter_pff<0&&b.pff) return(false);
	for(unsigned int n=0;n<NNAVAIDS;n++)
	{
		if(filter_nav[n]>0&&!b.nav[n]) return(false);
		if(filter_nav[n]<0&&b.nav[n]) return(false);
	}
	return(true);
}
