/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

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
#include "history.h"
#include "render.h"
#include "routing.h"
#include "weather.h"
#include "intel_bombers.h"
#include "intel_targets.h"
#include "run_raid.h"
#include "setup_difficulty.h"

extern game state;

atg_element *control_box;
atg_element *GB_resize, *GB_full, *GB_exit;
atg_element *GB_map;
atg_element **GB_btrow, **GB_btpc, **GB_btnew, **GB_btp, **GB_btpic, **GB_btint, **GB_navrow, *(*GB_navbtn)[NNAVAIDS], *(*GB_navgraph)[NNAVAIDS];
atg_element *GB_go, *GB_msgbox, *GB_msgrow[MAXMSGS], *GB_save, *GB_fintel, *GB_hcrews, *GB_cshort[CREW_CLASSES], *GB_diff;
atg_element *GB_ttl, **GB_ttrow, **GB_ttdmg, **GB_ttflk, **GB_ttint;
atg_element *GB_zhbox, *GB_zh, **GB_rbpic, **GB_rbrow, *(*GB_raidloadbox)[2], *(*GB_raidload)[2];
char **GB_btnum, **GB_raidnum, **GB_estcap;
char *GB_datestring, *GB_budget_label, *GB_confid_label, *GB_morale_label, *GB_raid_label;
SDL_Surface *GB_moonimg;
int filter_nav[NNAVAIDS], filter_pff=0, filter_elite=0, filter_student=0;
atg_element *GB_fi_nav[NNAVAIDS], *GB_fi_pff, *GB_fi_elite, *GB_fi_student;

bool filter_apply(ac_bomber b);
bool ensure_crewed(game *state, unsigned int i);
int update_raidbox(const game *state, int seltarg);
int update_raidnums(const game *state, int seltarg);

bool shortof[CREW_CLASSES];

int control_create(void)
{
	control_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!control_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	atg_element *GB_bt=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!GB_bt)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(control_box, GB_bt))
	{
		perror("atg_ebox_pack");
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
	if(atg_ebox_pack(GB_bt, GB_datetimebox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *GB_date=atg_create_element_label_nocopy(GB_datestring, 12, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
	if(!GB_date)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	GB_date->w=80;
	if(atg_ebox_pack(GB_datetimebox, GB_date))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	GB_moonimg=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 14, 14, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	if(!GB_moonimg)
	{
		fprintf(stderr, "moonimg: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(1);
	}
	atg_element *GB_moonpic=atg_create_element_image(GB_moonimg);
	if(!GB_moonpic)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	GB_moonpic->w=18;
	if(atg_ebox_pack(GB_datetimebox, GB_moonpic))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	GB_resize=atg_create_element_image(resizebtn);
	if(!GB_resize)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	GB_resize->w=16;
	GB_resize->clickable=true;
	if(atg_ebox_pack(GB_datetimebox, GB_resize))
	{
		perror("atg_ebox_pack");
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
	if(atg_ebox_pack(GB_datetimebox, GB_full))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	GB_exit=atg_create_element_image(exitbtn);
	if(!GB_exit)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	GB_exit->clickable=true;
	if(atg_ebox_pack(GB_datetimebox, GB_exit))
	{
		perror("atg_ebox_pack");
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
	if(!(GB_btnum=calloc(ntypes, sizeof(char *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_btpic=calloc(ntypes, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_btint=calloc(ntypes, sizeof(atg_element *))))
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
		if(atg_ebox_pack(GB_bt, GB_btrow[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		GB_btrow[i]->w=239;
		// Clone the picture, as we're going to write to it (grey overlay if target out of range)
		SDL_Surface *pic=SDL_ConvertSurface(types[i].picture, types[i].picture->format, SDL_HWSURFACE);
		if(!pic)
		{
			fprintf(stderr, "SDL_ConvertSurface: %s\n", SDL_GetError());
			return(1);
		}
		GB_btpic[i]=atg_create_element_image(pic);
		if(!GB_btpic[i])
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		GB_btpic[i]->w=38;
		GB_btpic[i]->clickable=true;
		if(atg_ebox_pack(GB_btrow[i], GB_btpic[i]))
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
		if(atg_ebox_pack(GB_btrow[i], vbox))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *nibox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!nibox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(vbox, nibox))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		GB_btint[i]=atg_create_element_image(rawtypes[i].text?intelbtn:nointelbtn);
		if(!GB_btint[i])
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		GB_btint[i]->clickable=true;
		GB_btint[i]->w=10;
		GB_btint[i]->h=12;
		if(atg_ebox_pack(nibox, GB_btint[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(types[i].manu&&types[i].name)
		{
			size_t len=strlen(types[i].manu)+strlen(types[i].name)+2;
			char *fullname=malloc(len);
			if(fullname)
			{
				snprintf(fullname, len, "%s %s", types[i].manu, types[i].name);
				atg_element *btname=atg_create_element_label(fullname, 10, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
				if(!btname)
				{
					fprintf(stderr, "atg_create_element_label failed\n");
					return(1);
				}
				btname->w=191;
				btname->cache=true;
				if(atg_ebox_pack(nibox, btname))
				{
					perror("atg_ebox_pack");
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
		if(!(GB_btnum[i]=malloc(12)))
		{
			perror("malloc");
			return(1);
		}
		atg_element *btnum=atg_create_element_label_nocopy(GB_btnum[i], 12, (atg_colour){159, 191, 255, ATG_ALPHA_OPAQUE});
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
		atg_element *hbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!hbox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(vbox, hbox))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(atg_ebox_pack(hbox, types[i].prio_selector))
		{
			perror("atg_ebox_pack");
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
		if(atg_ebox_pack(hbox, pcbox))
		{
			perror("atg_ebox_pack");
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
		if(atg_ebox_pack(hbox, GB_btnew[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(GB_btp[i]=atg_create_element_label("P!", 12, (atg_colour){191, 159, 31, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(hbox, GB_btp[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		GB_navrow[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!GB_navrow[i])
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(vbox, GB_navrow[i]))
		{
			perror("atg_ebox_pack");
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
			SDL_FreeSurface(pic); // Drop the extra reference
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
	GB_fintel=atg_create_element_button("Intel: Enemy Fighters", (atg_colour){127, 159, 223, ATG_ALPHA_OPAQUE}, (atg_colour){63, 31, 31, ATG_ALPHA_OPAQUE});
	if(!GB_fintel)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	GB_fintel->w=159;
	if(atg_ebox_pack(GB_bt, GB_fintel))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	GB_hcrews=atg_create_element_button("Crews & Training", (atg_colour){127, 223, 159, ATG_ALPHA_OPAQUE}, (atg_colour){31, 31, 63, ATG_ALPHA_OPAQUE});
	if(!GB_hcrews)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	GB_hcrews->w=159;
	if(atg_ebox_pack(GB_bt, GB_hcrews))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *csbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!csbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	csbox->w=159;
	if(atg_ebox_pack(GB_bt, csbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *csshim=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!csshim)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	csshim->w=4;
	if(atg_ebox_pack(csbox, csshim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *cslbl=atg_create_element_label("Crew req.: ", 12, (atg_colour){191, 127, 127, ATG_ALPHA_OPAQUE});
	if(!cslbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	cslbl->w=155-10*CREW_CLASSES;
	if(atg_ebox_pack(csbox, cslbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	for(unsigned int c=0;c<CREW_CLASSES;c++)
	{
		char text[2]={cclasses[c].letter, 0};
		GB_cshort[c]=atg_create_element_label(text, 12, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
		if(!GB_cshort[c])
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		GB_cshort[c]->w=10;
		if(atg_ebox_pack(csbox, GB_cshort[c]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	GB_diff=atg_create_element_button("Show Difficulty", (atg_colour){159, 159, 159, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 31, ATG_ALPHA_OPAQUE});
	if(!GB_diff)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	GB_diff->w=159;
	if(atg_ebox_pack(GB_bt, GB_diff))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	GB_go=atg_create_element_button("Run tonight's raids", (atg_colour){159, 191, 255, ATG_ALPHA_OPAQUE}, (atg_colour){31, 63, 31, ATG_ALPHA_OPAQUE});
	if(!GB_go)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	GB_go->w=159;
	if(atg_ebox_pack(GB_bt, GB_go))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	GB_save=atg_create_element_button("Save game", (atg_colour){159, 255, 191, ATG_ALPHA_OPAQUE}, (atg_colour){31, 63, 31, ATG_ALPHA_OPAQUE});
	if(!GB_save)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	GB_save->w=159;
	if(atg_ebox_pack(GB_bt, GB_save))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	GB_budget_label=malloc(32);
	if(!GB_budget_label)
	{
		perror("malloc");
		return(1);
	}
	atg_element *GB_budget=atg_create_element_label_nocopy(GB_budget_label, 12, (atg_colour){191, 255, 191, ATG_ALPHA_OPAQUE});
	if(!GB_budget)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	GB_budget->w=159;
	if(atg_ebox_pack(GB_bt, GB_budget))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	GB_confid_label=malloc(32);
	if(!GB_confid_label)
	{
		perror("malloc");
		return(1);
	}
	atg_element *GB_confid=atg_create_element_label_nocopy(GB_confid_label, 12, (atg_colour){191, 255, 191, ATG_ALPHA_OPAQUE});
	if(!GB_confid)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	GB_confid->w=159;
	if(atg_ebox_pack(GB_bt, GB_confid))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	GB_morale_label=malloc(32);
	if(!GB_morale_label)
	{
		perror("malloc");
		return(1);
	}
	atg_element *GB_morale=atg_create_element_label_nocopy(GB_morale_label, 12, (atg_colour){191, 255, 191, ATG_ALPHA_OPAQUE});
	if(!GB_morale)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	GB_morale->w=159;
	if(atg_ebox_pack(GB_bt, GB_morale))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *GB_msglbl=atg_create_element_label("Messages:", 12, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
	if(!GB_msglbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(GB_bt, GB_msglbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(GB_msgbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR)))
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(GB_bt, GB_msgbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *GB_mpad=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!GB_mpad)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(GB_bt, GB_mpad))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	GB_mpad->h=4;
	atg_element *GB_filters=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!GB_filters)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(GB_bt, GB_filters))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *GB_ft=atg_create_element_label("Filters:", 12, (atg_colour){239, 239, 0, ATG_ALPHA_OPAQUE});
	if(!GB_ft)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(GB_filters, GB_ft))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *GB_pad=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!GB_pad)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(GB_filters, GB_pad))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	GB_pad->w=4;
	for(unsigned int n=0;n<NNAVAIDS;n++)
	{
		filter_nav[n]=0;
		if(!(GB_fi_nav[n]=create_filter_switch(navpic[n], filter_nav+n)))
		{
			fprintf(stderr, "create_filter_switch failed\n");
			return(1);
		}
		if(atg_ebox_pack(GB_filters, GB_fi_nav[n]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	if(!(GB_fi_pff=create_filter_switch(pffpic, &filter_pff)))
	{
		fprintf(stderr, "create_filter_switch failed\n");
		return(1);
	}
	if(atg_ebox_pack(GB_filters, GB_fi_pff))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(GB_fi_elite=create_filter_switch(elitepic, &filter_elite)))
	{
		fprintf(stderr, "create_filter_switch failed\n");
		return(1);
	}
	if(atg_ebox_pack(GB_filters, GB_fi_elite))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(GB_fi_student=create_filter_switch(studentpic, &filter_student)))
	{
		fprintf(stderr, "create_filter_switch failed\n");
		return(1);
	}
	if(atg_ebox_pack(GB_filters, GB_fi_student))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *GB_middle=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!GB_middle)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(control_box, GB_middle))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	GB_map=atg_create_element_image(terrain);
	if(!GB_map)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	GB_map->h=terrain->h+2;
	GB_map->clickable=true;
	if(atg_ebox_pack(GB_middle, GB_map))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(GB_raid_label=malloc(48)))
	{
		perror("malloc");
		return(1);
	}
	snprintf(GB_raid_label, 48, "Select a Target");
	atg_element *raid_label=atg_create_element_label_nocopy(GB_raid_label, 12, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE});
	if(!raid_label)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(GB_middle, raid_label))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *GB_raid=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!GB_raid)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(GB_middle, GB_raid))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(GB_rbrow=calloc(ntypes, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_rbpic=calloc(ntypes, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_raidnum=calloc(ntypes, sizeof(char *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_raidload=calloc(ntypes, sizeof(atg_element *[2]))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_raidloadbox=calloc(ntypes, sizeof(atg_element *[2]))))
	{
		perror("calloc");
		return(1);
	}
	if(!(GB_estcap=calloc(ntypes, sizeof(char *))))
	{
		perror("calloc");
		return(1);
	}
	for(unsigned int i=0;i<ntypes;i++)
	{
		if(!(GB_rbrow[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(GB_raid, GB_rbrow[i]))
		{
			perror("atg_ebox_pack");
			atg_free_element(GB_rbrow[i]);
			return(1);
		}
		GB_rbrow[i]->w=256;
		atg_element *picture=atg_create_element_image(types[i].picture);
		if(!picture)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		picture->w=38;
		picture->cache=true;
		(GB_rbpic[i]=picture)->clickable=true;
		if(atg_ebox_pack(GB_rbrow[i], picture))
		{
			perror("atg_ebox_pack");
			atg_free_element(picture);
			return(1);
		}
		atg_element *vbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
		if(!vbox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		vbox->w=186;
		if(atg_ebox_pack(GB_rbrow[i], vbox))
		{
			perror("atg_ebox_pack");
			atg_free_element(vbox);
			return(1);
		}
		if(types[i].manu&&types[i].name)
		{
			char fullname[96];
			snprintf(fullname, 96, "%s %s", types[i].manu, types[i].name);
			atg_element *name=atg_create_element_label(fullname, 10, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
			if(!name)
			{
				fprintf(stderr, "atg_create_element_label failed\n");
				return(1);
			}
			name->cache=true;
			name->w=184;
			if(atg_ebox_pack(vbox, name))
			{
				perror("atg_ebox_pack");
				atg_free_element(name);
				return(1);
			}
		}
		else
		{
			fprintf(stderr, "Missing manu or name in type %u\n", i);
		}
		if(!(GB_raidnum[i]=malloc(32)))
		{
			perror("malloc");
			return(1);
		}
		atg_element *raidnum=atg_create_element_label_nocopy(GB_raidnum[i], 12, (atg_colour){159, 191, 255, ATG_ALPHA_OPAQUE});
		if(!raidnum)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(vbox, raidnum))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		for(int j=1;j>=0;j--)
		{
			if(!(GB_raidloadbox[i][j]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE})))
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				return(1);
			}
			GB_raidloadbox[i][j]->w=16;
			if(atg_ebox_pack(GB_rbrow[i], GB_raidloadbox[i][j]))
			{
				perror("atg_ebox_pack");
				return(1);
			}
			GB_raidload[i][j]=NULL;
		}
		if(!(GB_estcap[i]=malloc(24)))
		{
			perror("malloc");
			return(1);
		}
		atg_element *estcap=atg_create_element_label_nocopy(GB_estcap[i], 9, (atg_colour){127, 127, 159, ATG_ALPHA_OPAQUE});
		if(!estcap)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(vbox, estcap))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	if(!(GB_zhbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(GB_middle, GB_zhbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *shim=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){31, 31, 39, ATG_ALPHA_OPAQUE});
	if(!shim)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	shim->w=108;
	if(atg_ebox_pack(GB_zhbox, shim))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *zhlbl=atg_create_element_label("Zero Hour: ", 15, (atg_colour){127, 127, 127, ATG_ALPHA_OPAQUE});
	if(!zhlbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(GB_zhbox, zhlbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(GB_zh=create_time_spinner(0, RRT(18,0), RRT(6,0), 12, RRT(1,0), "%02u:%02u", (atg_colour){224, 224, 239, ATG_ALPHA_OPAQUE}, (atg_colour){39, 39, 47, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "create_time_spinner failed\n");
		return(1);
	}
	if(atg_ebox_pack(GB_zhbox, GB_zh))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *GB_tt=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
	if(!GB_tt)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(control_box, GB_tt))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	GB_ttl=atg_create_element_label("Targets", 12, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE});
	if(!GB_ttl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	GB_ttl->clickable=true;
	if(atg_ebox_pack(GB_tt, GB_ttl))
	{
		perror("atg_ebox_pack");
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
		if(atg_ebox_pack(GB_tt, GB_ttrow[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		GB_ttint[i]=atg_create_element_image(targs[i].p_intel?intelbtn:nointelbtn);
		if(!GB_ttint[i])
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		GB_ttint[i]->clickable=true;
		GB_ttint[i]->w=8;
		GB_ttint[i]->h=12;
		if(atg_ebox_pack(GB_ttrow[i], GB_ttint[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *item=atg_create_element_label(targs[i].name, 10, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE});
		if(!item)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		item->w=192;
		item->cache=true;
		if(atg_ebox_pack(GB_ttrow[i], item))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		item=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
		if(!item)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		item->w=105;
		if(atg_ebox_pack(GB_ttrow[i], item))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(GB_ttdmg[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){0, 0, 255, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		GB_ttdmg[i]->w=1;
		GB_ttdmg[i]->h=6;
		if(atg_ebox_pack(item, GB_ttdmg[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(GB_ttflk[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){183, 183, 199, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		GB_ttflk[i]->w=1;
		GB_ttflk[i]->h=6;
		if(atg_ebox_pack(item, GB_ttflk[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	return(0);
}

screen_id control_screen(atg_canvas *canvas, game *state)
{
	atg_event e;
	
	if(state->weather.seed)
	{
		srand(state->weather.seed);
		w_init(&state->weather, 256, lorw);
		state->weather.seed=0;
	}
	
	snprintf(GB_datestring, 11, "%02u-%02u-%04u", state->now.day, state->now.month, state->now.year);
	snprintf(GB_budget_label, 32, "Budget: £%u/day", state->cshr);
	snprintf(GB_confid_label, 32, "Confidence: %u%%", (unsigned int)floor(state->confid+0.5));
	snprintf(GB_morale_label, 32, "Morale: %u%%", (unsigned int)floor(state->morale+0.5));
	double moonphase=pom(state->now);
	drawmoon(GB_moonimg, moonphase);
	for(unsigned int j=0;j<state->nbombers;j++)
	{
		state->bombers[j].landed=true;
		state->bombers[j].damage=0;
		state->bombers[j].ld.ds=DS_NONE;
	}
	bool shownav=false;
	GB_fi_pff->hidden=datebefore(state->now, event[EVENT_PFF]);
	for(unsigned int n=0;n<NNAVAIDS;n++)
	{
		GB_fi_nav[n]->hidden=datebefore(state->now, event[navevent[n]]);
		if(!datebefore(state->now, event[navevent[n]])) shownav=true;
	}
	for(unsigned int i=0;i<ntypes;i++)
	{
		if(GB_btrow[i])
			GB_btrow[i]->hidden=!datewithin(state->now, types[i].entry, types[i].exit)||!state->btypes[i];
		if(GB_btpc[i])
			GB_btpc[i]->h=18-min(types[i].pcbuf/10000, 18);
		if(GB_btnew[i])
			GB_btnew[i]->hidden=!datebefore(state->now, types[i].novelty);
		if(GB_btp[i])
			GB_btp[i]->hidden=(types[i].pribuf<8)||(state->cash<types[i].cost)||(types[i].pcbuf>=types[i].cost);
		if(GB_btnum[i])
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
			snprintf(GB_btnum[i], 12, "%u/%u", svble, total);
		}
		GB_navrow[i]->hidden=!shownav;
		for(unsigned int n=0;n<NNAVAIDS;n++)
		{
			update_navbtn(*state, GB_navbtn, i, n, grey_overlay, yellow_overlay);
			if(GB_navgraph[i][n])
				GB_navgraph[i][n]->h=16-(types[i].count?(types[i].navcount[n]*16)/types[i].count:0);
		}
	}
	if(GB_go&&GB_go->elemdata&&((atg_button *)GB_go->elemdata)->content)
		((atg_button *)GB_go->elemdata)->content->bgcolour=(atg_colour){31, 63, 31, ATG_ALPHA_OPAQUE};
	if(GB_msgbox)
	{
		atg_ebox_empty(GB_msgbox);
		for(unsigned int i=0;i<MAXMSGS;i++)
		{
			if(state->msg[i])
			{
				char msgbuf[48];
				size_t j;
				for(j=0;state->msg[i][j]&&(state->msg[i][j]!='\n')&&j<47;j++)
					msgbuf[j]=state->msg[i][j];
				msgbuf[j]=0;
				if(!(GB_msgrow[i]=atg_create_element_button(msgbuf, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE}, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE})))
					continue; // nothing we can do about it
				GB_msgrow[i]->w=GB_btrow[0]->w;
				if(atg_ebox_pack(GB_msgbox, GB_msgrow[i])==0)
					continue; // we're good
			}
			GB_msgrow[i]=NULL;
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
					GB_ttdmg[i]->w=0;
				break;
			}
		if(GB_ttflk[i])
			GB_ttflk[i]->w=floor(state->flk[i]);
		for(unsigned int j=0;j<state->raids[i].nbombers;j++)
			state->bombers[state->raids[i].bombers[j]].landed=false;
	}
	atg_image *map_img=GB_map->elemdata;
	SDL_FreeSurface(map_img->data);
	map_img->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
	SDL_FreeSurface(flak_overlay);
	flak_overlay=render_flak(state->now);
	SDL_BlitSurface(flak_overlay, NULL, map_img->data, NULL);
	SDL_FreeSurface(target_overlay);
	target_overlay=render_targets(state->now);
	SDL_BlitSurface(target_overlay, NULL, map_img->data, NULL);
	SDL_FreeSurface(weather_overlay);
	weather_overlay=render_weather(state->weather);
	SDL_BlitSurface(weather_overlay, NULL, map_img->data, NULL);
	SDL_FreeSurface(xhair_overlay);
	xhair_overlay=render_xhairs(state);
	SDL_BlitSurface(xhair_overlay, NULL, map_img->data, NULL);
	SDL_FreeSurface(seltarg_overlay);
	int seltarg=-1;
	int routegrab=-1;
	seltarg_overlay=render_seltarg(seltarg);
	SDL_BlitSurface(seltarg_overlay, NULL, map_img->data, NULL);
	update_raidbox(state, seltarg);
	bool rfsh=true;
	while(1)
	{
		for(unsigned int c=0;c<CREW_CLASSES;c++)
		{
			atg_label *l=(atg_label *)GB_cshort[c]->elemdata;
			l->colour=shortof[c]?(atg_colour){231, 95, 95, ATG_ALPHA_OPAQUE}:(atg_colour){15, 31, 15, ATG_ALPHA_OPAQUE};
		}
		if(rfsh)
		{
			for(unsigned int i=0;i<ntargs;i++)
			{
				if(GB_ttrow[i]&&GB_ttrow[i]->elemdata)
				{
					unsigned int raid[ntypes];
					for(unsigned int j=0;j<ntypes;j++)
						raid[j]=0;
					for(unsigned int j=0;j<state->raids[i].nbombers;j++)
						raid[state->bombers[state->raids[i].bombers[j]].type]++;
					if((int)i==seltarg)
					{
						for(unsigned int j=0;j<ntypes;j++)
							if(GB_rbrow[j])
								GB_rbrow[j]->hidden=(GB_btrow[j]?GB_btrow[j]->hidden:true)||!raid[j];
					}
					atg_box *b=GB_ttrow[i]->elemdata;
					b->bgcolour=(state->raids[i].nbombers?(atg_colour){127, 103, 95, ATG_ALPHA_OPAQUE}:(atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
					if((int)i==seltarg)
					{
						b->bgcolour.r=b->bgcolour.g=b->bgcolour.r+64;
					}
					if(b->nelems>1)
					{
						if(b->elems[1]&&!strcmp(b->elems[1]->type, "__builtin_box")&&b->elems[1]->elemdata)
						{
							((atg_box *)b->elems[1]->elemdata)->bgcolour=b->bgcolour;
						}
					}
				}
			}
			update_raidnums(state, seltarg);
			for(unsigned int i=0;i<ntypes;i++)
			{
				if(!GB_btrow[i]->hidden&&GB_btpic[i])
				{
					atg_image *img=GB_btpic[i]->elemdata;
					if(img)
					{
						SDL_Surface *pic=img->data;
						SDL_FillRect(pic, &(SDL_Rect){0, 0, pic->w, pic->h}, SDL_MapRGB(pic->format, 0, 0, 0));
						SDL_BlitSurface(types[i].picture, NULL, pic, NULL);
						if(seltarg>=0)
						{
							double dist=hypot((signed)types[i].blat-(signed)targs[seltarg].lat, (signed)types[i].blon-(signed)targs[seltarg].lon)*(types[i].ovltank?1.4:1.6);
							if(types[i].range<dist)
								SDL_BlitSurface(grey_overlay, NULL, pic, NULL);
						}
					}
				}
			}
			SDL_FreeSurface(map_img->data);
			map_img->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
			SDL_BlitSurface(flak_overlay, NULL, map_img->data, NULL);
			SDL_BlitSurface(target_overlay, NULL, map_img->data, NULL);
			SDL_BlitSurface(weather_overlay, NULL, map_img->data, NULL);
			SDL_FreeSurface(xhair_overlay);
			xhair_overlay=render_xhairs(state);
			SDL_BlitSurface(xhair_overlay, NULL, map_img->data, NULL);
			SDL_FreeSurface(seltarg_overlay);
			seltarg_overlay=render_seltarg(seltarg);
			SDL_BlitSurface(seltarg_overlay, NULL, map_img->data, NULL);
			route_overlay=render_current_route(state, seltarg);
			SDL_BlitSurface(route_overlay, NULL, map_img->data, NULL);
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
							clear_raids(state);
							return(SCRN_MAINMENU);
						break;
						case SDL_MOUSEBUTTONUP:
							routegrab=-1;
						break;
						case SDL_MOUSEMOTION:
							if(routegrab>=0)
							{
								SDL_MouseMotionEvent me=s.motion;
								int newx=targs[seltarg].route[routegrab][1]+me.xrel;
								clamp(newx, 0, 256);
								targs[seltarg].route[routegrab][1]=newx;
								int newy=targs[seltarg].route[routegrab][0]+me.yrel;
								clamp(newy, 0, 256);
								targs[seltarg].route[routegrab][0]=newy;
								rfsh=true;
							}
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
							if(seltarg>=0)
							{
								if(c.e==GB_btpic[i])
								{
									double dist=hypot((signed)types[i].blat-(signed)targs[seltarg].lat, (signed)types[i].blon-(signed)targs[seltarg].lon)*(types[i].ovltank?1.4:1.6);
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
										memset(shortof, 0, sizeof(shortof));
										for(unsigned int j=0;j<state->nbombers;j++)
										{
											if(state->bombers[j].type!=i) continue;
											if(state->bombers[j].failed) continue;
											if(!state->bombers[j].landed) continue;
											if(!filter_apply(state->bombers[j])) continue;
											if(!ensure_crewed(state, j)) continue;
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
										if(GB_raidnum[i])
										{
											unsigned int count=0;
											for(unsigned int j=0;j<state->raids[seltarg].nbombers;j++)
												if(state->bombers[state->raids[seltarg].bombers[j]].type==i) count++;
											snprintf(GB_raidnum[i], 32, "%u", count);
										}
										if(state->raids[seltarg].nbombers&&!datebefore(state->now, event[EVENT_GEE])&&!state->raids[seltarg].routed)
										{
											genroute((unsigned int [2]){0, 0}, seltarg, targs[seltarg].route, state, 10000);
											state->raids[seltarg].routed=true;
										}
									}
								}
								if(c.e==GB_rbpic[i])
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
									for(unsigned int l=0;l<state->raids[seltarg].nbombers;l++)
									{
										unsigned int k=state->raids[seltarg].bombers[l];
										if(state->bombers[k].type!=i) continue;
										if(!filter_apply(state->bombers[k])) continue;
										state->bombers[k].landed=true;
										amount--;
										state->raids[seltarg].nbombers--;
										for(unsigned int k=l;k<state->raids[seltarg].nbombers;k++)
											state->raids[seltarg].bombers[k]=state->raids[seltarg].bombers[k+1];
										if(!amount) break;
										l--;
									}
									if(GB_raidnum[i])
									{
										unsigned int count=0;
										for(unsigned int l=0;l<state->raids[seltarg].nbombers;l++)
											if(state->bombers[state->raids[seltarg].bombers[l]].type==i) count++;
										snprintf(GB_raidnum[i], 32, "%u", count);
									}
								}
							}
							if(c.e==GB_btint[i])
							{
								IB_i=i;
								intel_caller=SCRN_CONTROL;
								return(SCRN_INTELBMB);
							}
						}
						if(c.e==GB_ttl)
						{
							seltarg=-1;
							update_raidbox(state, seltarg);
						}
						for(unsigned int i=0;i<ntargs;i++)
						{
							if(!datewithin(state->now, targs[i].entry, targs[i].exit)) continue;
							if(c.e==GB_ttint[i])
							{
								IT_i=i;
								intel_caller=SCRN_CONTROL;
								return(SCRN_INTELTRG);
							}
							if(c.e==GB_ttrow[i])
							{
								seltarg=i;
								update_raidbox(state, seltarg);
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
						if(c.e==GB_map&&seltarg>=0&&state->raids[seltarg].routed)
						{
							double mind=26;
							int mins=-1;
							for(unsigned int stage=0;stage<8;stage++)
							{
								int dx=c.pos.x-targs[seltarg].route[stage][1],
								    dy=c.pos.y-targs[seltarg].route[stage][0];
								if(dx*dx+dy*dy<mind)
								{
									mind=dx*dx+dy*dy;
									mins=stage;
								}
							}
							if((routegrab=mins)==4) // don't let them move the A.P.
								routegrab=-1;
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
							if(GB_go&&GB_go->elemdata&&((atg_button *)GB_go->elemdata)->content)
								((atg_button *)GB_go->elemdata)->content->bgcolour=(atg_colour){55, 55, 55, ATG_ALPHA_OPAQUE};
							atg_flip(canvas);
							return(SCRN_RUNRAID);
						}
						else if(trigger.e==GB_fintel)
						{
							intel_caller=SCRN_CONTROL;
							return(SCRN_INTELFTR);
						}
						else if(trigger.e==GB_hcrews)
						{
							return(SCRN_HCREWS);
						}
						else if(trigger.e==GB_diff)
						{
							difficulty_show_only=true;
							return(SCRN_SETPDIFF);
						}
						else if(trigger.e==GB_save)
						{
							return(SCRN_SAVEGAME);
						}
						for(unsigned int i=0;i<MAXMSGS;i++)
							if((trigger.e==GB_msgrow[i])&&state->msg[i])
							{
								message_box(canvas, "To the Commander-in-Chief, Bomber Command:", state->msg[i], "Air Chief Marshal C. F. A. Portal, CAS");
							}
					}
				break;
				case ATG_EV_VALUE:;
					atg_ev_value value=e.event.value;
					if(value.e)
					{
						if(value.e==GB_zh)
						{
							if(seltarg<0)
								fprintf(stderr, "Ignored zerohour change (to %d) with bad seltarg %d\n", value.value, seltarg);
							else
								state->raids[seltarg].zerohour=value.value;
						}
					}
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
		SDL_FreeSurface(map_img->data);
		map_img->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
		SDL_FillRect(map_img->data, &(SDL_Rect){.x=0, .y=0, .w=terrain->w, .h=terrain->h}, SDL_MapRGB(terrain->format, 0, 0, 0));
		SDL_FreeSurface(weather_overlay);
		weather_overlay=render_weather(state->weather);
		SDL_BlitSurface(weather_overlay, NULL, map_img->data, NULL);
		SDL_FreeSurface(xhair_overlay);
		xhair_overlay=render_xhairs(state);
		SDL_BlitSurface(xhair_overlay, NULL, map_img->data, NULL);
		seltarg_overlay=render_seltarg(seltarg);
		SDL_BlitSurface(seltarg_overlay, NULL, map_img->data, NULL);
		#endif
	}
}

void control_free(void)
{
	SDL_FreeSurface(GB_moonimg);
	atg_free_element(control_box);
}

bool filter_apply(ac_bomber b)
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

#define CREWOPS(k)	(state->crews[(k)].tour_ops + 30 * state->crews[(k)].full_tours) // TODO second tours should only be 20 ops

bool ensure_crewed(game *state, unsigned int i)
{
	unsigned int type=state->bombers[i].type;
	bool ok=true;
	memset(shortof, 0, sizeof(shortof));
	for(unsigned int j=0;j<MAX_CREW;j++)
	{
		if(types[type].crew[j]==CCLASS_NONE)
			continue;
		int k=state->bombers[i].crew[j];
		// 0. Deassign current crewman if unsuitable
		if(k>=0 && ((state->crews[k].skill*filter_elite<50*filter_elite) || // 'elite' filter
		            (state->bombers[i].pff&&(CREWOPS(k)<(types[type].noarm?30:15))) || // PFF requirements
		            ((filter_student==1)&&CREWOPS(k)) || // 'student' filter
		            state->crews[k].class!=types[type].crew[j])) // cclass changed under us
		{
			state->crews[k].assignment=-1;
			state->bombers[i].crew[j]=-1;
		}
		if((state->bombers[i].crew[j]<0)&&(filter_student!=1))
		{
			// 1. Look for an available CREWMAN
			// either unassigned, or assigned to a bomber who's not on a raid (yet)
			for(unsigned int k=0;k<state->ncrews;k++)
				if(state->crews[k].class==types[type].crew[j])
					if(state->crews[k].status==CSTATUS_CREWMAN)
					{
						if(state->crews[k].skill*filter_elite<50*filter_elite) continue;
						if(state->bombers[i].pff&&(CREWOPS(k)<(types[type].noarm?30:15))) continue;
						if(state->crews[k].assignment<0)
						{
							state->crews[k].assignment=i;
							state->bombers[i].crew[j]=k;
							break;
						}
						else if(state->bombers[state->crews[k].assignment].landed)
						{
							unsigned int l=state->crews[k].assignment, m;
							if(l==i) continue; // don't steal from ourselves
							if(state->bombers[l].pff&&!state->bombers[i].pff) continue; // don't pull out of PFF either
							for(m=0;m<MAX_CREW;m++)
								if(state->bombers[l].crew[m]==(int)k)
								{
									state->bombers[l].crew[m]=-1;
									break;
								}
							if(m==MAX_CREW) // XXX should never happen
							{
								fprintf(stderr, "Warning: crew linkage error b%u c%u\n", l, k);
								continue;
							}
							state->crews[k].assignment=i;
							state->bombers[i].crew[j]=k;
							break;
						}
					}
		}
		if((state->bombers[i].crew[j]<0)&&(filter_student!=-1))
		{
			// 2. Look for a matching STUDENT and promote them to CREWMAN
			for(unsigned int k=0;k<state->ncrews;k++)
				if(state->crews[k].class==types[type].crew[j])
					if(state->crews[k].status==CSTATUS_STUDENT)
					{
						if(state->crews[k].skill*filter_elite<50*filter_elite) continue;
						if(state->bombers[i].pff) continue; // never promote STUDENTs to PFF, that's silly
						state->crews[k].status=CSTATUS_CREWMAN;
						st_append(&state->hist, state->now, (harris_time){12, 00}, state->crews[k].id, state->crews[k].class, state->crews[k].status);
						state->crews[k].tour_ops=0;
						state->crews[k].assignment=i;
						state->bombers[i].crew[j]=k;
						break;
					}
		}
		if(state->bombers[i].crew[j]<0)
		{
			// 3. Give up, and don't allow assigning this bomber to any raid
			shortof[types[type].crew[j]]=true;
			ok=false;
		}
	}
	return(ok);
}

// Relink crew assignments after a bomber is destroyed, obsoleted or otherwise removed from the list
// This would be so much easier if everything was in linked-lists... ah well
void fixup_crew_assignments(game *state, unsigned int i, bool kill)
{
	for(unsigned int j=0;j<state->ncrews;j++)
		if(state->crews[j].status==CSTATUS_CREWMAN)
		{
			if(state->crews[j].assignment>(int)i)
			{
				state->crews[j].assignment--;
			}
			else if(state->crews[j].assignment==(int)i)
			{
				if(kill)
				{
					de_append(&state->hist, state->now, (harris_time){11, 00}, state->crews[j].id, state->crews[j].class);
					state->ncrews--;
					for(unsigned int k=j;k<state->ncrews;k++)
						state->crews[k]=state->crews[k+1];
					for(unsigned int k=0;k<state->nbombers;k++)
						for(unsigned int l=0;l<MAX_CREW;l++)
							if(state->bombers[k].crew[l]>(int)j)
								state->bombers[k].crew[l]--;
					j--;
				}
				else
					state->crews[j].assignment=-1;
			}
		}
}

int update_raidbox(const game *state, int seltarg)
{
	if(GB_raid_label)
	{
		if(seltarg>=0)
			snprintf(GB_raid_label, 48, "Raid on %s", targs[seltarg].name);
		else
			snprintf(GB_raid_label, 48, "Select a Target");
	}
	if(GB_zhbox)
	{
		bool stream=!datebefore(state->now, event[EVENT_GEE]);
		if(!(GB_zhbox->hidden=(seltarg<0)||!stream))
			if(GB_zh)
			{
				atg_spinner *s=GB_zh->elemdata;
				s->value=state->raids[seltarg].zerohour;
			}
	}
	bool pff=!datebefore(state->now, event[EVENT_PFF]);
	for(unsigned int i=0;i<ntypes;i++)
	{
		for(unsigned int j=0;j<2;j++)
		{
			atg_ebox_empty(GB_raidloadbox[i][j]);
			GB_raidload[i][j]=NULL;
		}
		if(seltarg>=0 && targs[seltarg].class==TCLASS_CITY)
		{
			if(!(GB_raidload[i][0]=create_load_selector(&types[i], &state->raids[seltarg].loads[i])))
			{
				fprintf(stderr, "create_load_selector failed\n");
				return(1);
			}
			if(atg_ebox_pack(GB_raidloadbox[i][0], GB_raidload[i][0]))
			{
				perror("atg_ebox_pack");
				return(1);
			}
			GB_raidload[i][0]->hidden=pff&&types[i].noarm;
			if(!(GB_raidload[i][1]=create_load_selector(&types[i], &state->raids[seltarg].pffloads[i])))
			{
				fprintf(stderr, "create_load_selector failed\n");
				return(1);
			}
			if(atg_ebox_pack(GB_raidloadbox[i][1], GB_raidload[i][1]))
			{
				perror("atg_ebox_pack");
				return(1);
			}
			GB_raidload[i][1]->hidden=!pff||!types[i].pff;
		}
	}
	return(update_raidnums(state, seltarg));
}

int update_raidnums(const game *state, int seltarg)
{
	bool stream=!datebefore(state->now, event[EVENT_GEE]);
	for(unsigned int i=0;i<ntypes;i++)
	{
		unsigned int count=0;
		if(seltarg>=0)
			for(unsigned int k=0;k<state->raids[seltarg].nbombers;k++)
				if(state->bombers[state->raids[seltarg].bombers[k]].type==i) count++;
		if(GB_raidnum[i])
			snprintf(GB_raidnum[i], 32, "%u", count);
		if(GB_rbrow[i])
			GB_rbrow[i]->hidden=!count||(GB_btrow[i]?GB_btrow[i]->hidden:true);
		if(count&&GB_estcap[i])
		{
			if(targs[seltarg].class==TCLASS_LEAFLET)
			{
				GB_estcap[i][0]=0;
			}
			else
			{
				unsigned int cap=types[i].capwt;
				double dist;
				if(stream)
				{
					dist=hypot((signed)types[i].blat-(signed)targs[seltarg].route[0][0], (signed)types[i].blon-(signed)targs[seltarg].route[0][1]);
					for(unsigned int l=0;l<4;l++)
					{
						double d=hypot((signed)targs[seltarg].route[l+1][0]-(signed)targs[seltarg].route[l][0], (signed)targs[seltarg].route[l+1][1]-(signed)targs[seltarg].route[l][1]);
						dist+=d;
					}
				}
				else
				{
					dist=hypot((signed)types[i].blat-(signed)targs[seltarg].lat, (signed)types[i].blon-(signed)targs[seltarg].lon)*1.07;
				}
				unsigned int fuelt=types[i].range*0.6/(types[i].speed/450.0);
				unsigned int estt=dist*1.1/(types[i].speed/450.0)+12;
				if(!stream) estt+=36;
				if(estt>fuelt)
				{
					unsigned int fu=estt-fuelt;
					cap*=120.0/(120.0+fu);
				}
				cap=(cap/10)*10;
				snprintf(GB_estcap[i], 24, "%12ulb est. max", cap);
			}
		}
	}
	return(0);
}

void game_preinit(game *state)
{
	// delete any bombers of deselected types
	unsigned int stripped=0;
	for(unsigned int i=0;i<state->nbombers;i++)
	{
		unsigned int type=state->bombers[i].type;
		if(!state->btypes[type])
		{
			state->nbombers--;
			for(unsigned int j=i;j<state->nbombers;j++)
				state->bombers[j]=state->bombers[j+1];
			fixup_crew_assignments(state, i, false);
			i--;
			stripped++;
			continue;
		}
	}
	if(stripped)
		fprintf(stderr, "preinit: stripped %u bombers (deselected types)\n", stripped);
}
