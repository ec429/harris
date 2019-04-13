/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	setup_game: the "Set Up New Game" screen
*/

#include <stdio.h>
#include <unistd.h>

#include "setup_game.h"
#include "ui.h"
#include "globals.h"
#include "saving.h"
#include "intel_bombers.h"
#include "intel_fighters.h"
#include "setup_difficulty.h"
#ifdef WINDOWS
#include "bits.h" /* for strndup */
#endif

atg_element *setup_game_box;
atg_element *SG_full, *SG_exit, *SG_cont;
atg_element **SG_stitles, *SG_text_box, *SG_acbox;
atg_element **SG_btrow, **SG_btint, **SG_ftrow, **SG_ftint;
char **SG_btnum, **SG_ftnum;
char *SG_datestring;
int selstart;

#define SG_BG_COLOUR	(atg_colour){31, 31, 47, ATG_ALPHA_OPAQUE}

int setup_game_create(void)
{
	setup_game_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, SG_BG_COLOUR);
	if(!setup_game_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	atg_element *SG_topbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, SG_BG_COLOUR);
	if(!SG_topbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(setup_game_box, SG_topbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *SG_title=atg_create_element_label("Set Up New Game", 12, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
	if(!SG_title)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(SG_topbox, SG_title))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	SG_full=atg_create_element_image(fullbtn);
	if(!SG_full)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	SG_full->clickable=true;
	if(atg_ebox_pack(SG_topbox, SG_full))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	SG_exit=atg_create_element_image(exitbtn);
	if(!SG_exit)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	SG_exit->clickable=true;
	if(atg_ebox_pack(SG_topbox, SG_exit))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *SG_midbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, SG_BG_COLOUR);
	if(!SG_midbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(setup_game_box, SG_midbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *SG_selbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE});
	if(!SG_selbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(SG_midbox, SG_selbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(SG_stitles=malloc(nstarts*sizeof(atg_element *))))
	{
		perror("malloc");
		return(1);
	}
	for(unsigned int i=0;i<nstarts;i++)
	{
		if(!(SG_stitles[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		atg_element *title=atg_create_element_label(starts[i].title, 16, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE});
		if(!title)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(SG_stitles[i], title))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		SG_stitles[i]->clickable=true;
		SG_stitles[i]->w=480;
		if(atg_ebox_pack(SG_selbox, SG_stitles[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	if(!(SG_datestring=malloc(11)))
	{
		perror("malloc");
		return(1);
	}
	snprintf(SG_datestring, 11, "  -  -    ");
	atg_element *SG_date=atg_create_element_label_nocopy(SG_datestring, 24, (atg_colour){255, 255, 223, ATG_ALPHA_OPAQUE});
	if(!SG_date)
	{
		fprintf(stderr, "atg_create_element_label_nocopy failed\n");
		return(1);
	}
	if(atg_ebox_pack(SG_midbox, SG_date))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	SG_acbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, SG_BG_COLOUR);
	if(!SG_acbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(setup_game_box, SG_acbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *bomberbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, SG_BG_COLOUR);
	if(!bomberbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	bomberbox->w=360;
	if(atg_ebox_pack(SG_acbox, bomberbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *bblabel=atg_create_element_label("RAF Front Line Bombers", 16, (atg_colour){159, 239, 159, ATG_ALPHA_OPAQUE});
	if(!bblabel)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(bomberbox, bblabel))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(SG_btrow=malloc(ntypes*sizeof(atg_element *))))
	{
		perror("malloc");
		return(1);
	}
	if(!(SG_btint=malloc(ntypes*sizeof(atg_element *))))
	{
		perror("malloc");
		return(1);
	}
	if(!(SG_btnum=malloc(ntypes*sizeof(char *))))
	{
		perror("malloc");
		return(1);
	}
	for(unsigned int i=0;i<ntypes;i++)
	{
		if(!(SG_btrow[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, SG_BG_COLOUR)))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(bomberbox, SG_btrow[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *btpic=atg_create_element_image(types[i].picture);
		if(!btpic)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		btpic->w=38;
		btpic->h=42;
		btpic->cache=true;
		if(atg_ebox_pack(SG_btrow[i], btpic))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *rightbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
		if(!rightbox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		rightbox->h=42;
		if(atg_ebox_pack(SG_btrow[i], rightbox))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *nibox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
		if(!nibox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(rightbox, nibox))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(SG_btint[i]=atg_create_element_image(rawtypes[i].text?intelbtn:nointelbtn)))
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		SG_btint[i]->clickable=true;
		SG_btint[i]->w=10;
		SG_btint[i]->h=12;
		if(atg_ebox_pack(nibox, SG_btint[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(types[i].manu&&types[i].name))
		{
			fprintf(stderr, "Missing manu or name in type %u\n", i);
			return(1);
		}
		atg_element *manu=atg_create_element_label(types[i].manu, 10, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
		if(!manu)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		manu->w=191;
		manu->cache=true;
		if(atg_ebox_pack(nibox, manu))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *countbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
		if(!countbox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(rightbox, countbox))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *name=atg_create_element_label(types[i].name, 14, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
		if(!name)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		name->w=144;
		if(atg_ebox_pack(countbox, name))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(SG_btnum[i]=malloc(8)))
		{
			perror("malloc");
			return(1);
		}
		snprintf(SG_btnum[i], 8, "number");
		atg_element *btnum=atg_create_element_label_nocopy(SG_btnum[i], 16, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE});
		if(!btnum)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(countbox, btnum))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	atg_element *fighterbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, SG_BG_COLOUR);
	if(!fighterbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	fighterbox->w=360;
	if(atg_ebox_pack(SG_acbox, fighterbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *fblabel=atg_create_element_label("Luftwaffe Front Line Fighters", 16, (atg_colour){239, 159, 159, ATG_ALPHA_OPAQUE});
	if(!fblabel)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(fighterbox, fblabel))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(SG_ftrow=malloc(nftypes*sizeof(atg_element *))))
	{
		perror("malloc");
		return(1);
	}
	if(!(SG_ftint=malloc(nftypes*sizeof(atg_element *))))
	{
		perror("malloc");
		return(1);
	}
	if(!(SG_ftnum=malloc(ntypes*sizeof(char *))))
	{
		perror("malloc");
		return(1);
	}
	for(unsigned int i=0;i<nftypes;i++)
	{
		if(!(SG_ftrow[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, SG_BG_COLOUR)))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(fighterbox, SG_ftrow[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *ftpic=atg_create_element_image(ftypes[i].picture);
		if(!ftpic)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		ftpic->w=38;
		ftpic->h=42;
		ftpic->cache=true;
		if(atg_ebox_pack(SG_ftrow[i], ftpic))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *rightbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
		if(!rightbox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		rightbox->h=42;
		if(atg_ebox_pack(SG_ftrow[i], rightbox))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *nibox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
		if(!nibox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(rightbox, nibox))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(SG_ftint[i]=atg_create_element_image(ftypes[i].text?intelbtn:nointelbtn)))
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		SG_ftint[i]->clickable=true;
		SG_ftint[i]->w=10;
		SG_ftint[i]->h=12;
		if(atg_ebox_pack(nibox, SG_ftint[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(ftypes[i].manu&&ftypes[i].name))
		{
			fprintf(stderr, "Missing manu or name in ftype %u\n", i);
			return(1);
		}
		atg_element *manu=atg_create_element_label(ftypes[i].manu, 10, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
		if(!manu)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		manu->w=191;
		manu->cache=true;
		if(atg_ebox_pack(nibox, manu))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *countbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
		if(!countbox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(rightbox, countbox))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *name=atg_create_element_label(ftypes[i].name, 14, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
		if(!name)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		name->w=144;
		if(atg_ebox_pack(countbox, name))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(SG_ftnum[i]=malloc(8)))
		{
			perror("malloc");
			return(1);
		}
		snprintf(SG_ftnum[i], 8, "number");
		atg_element *ftnum=atg_create_element_label_nocopy(SG_ftnum[i], 16, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE});
		if(!ftnum)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_ebox_pack(countbox, ftnum))
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
	text_guard->w=724;
	if(atg_ebox_pack(setup_game_box, text_guard))
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
	if(!(SG_text_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	SG_text_box->w=720;
	if(atg_ebox_pack(text_guard, SG_text_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(SG_cont=atg_create_element_button("Continue", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(setup_game_box, SG_cont))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	return(0);
}

screen_id setup_game_screen(atg_canvas *canvas, game *state)
{
	atg_event e;
	int i;
	#ifndef WINDOWS
	if(chdir(localdat?cwd:DATIDIR))
	{
		perror("Failed to enter data dir: chdir");
		return(1);
	}
	#endif
	
	while(1)
	{
		SG_acbox->hidden=(selstart<0);
		for(i=0;i<(int)nstarts;i++)
		{
			atg_box *tb=SG_stitles[i]->elemdata;
			if(tb)
			{
				if(i==selstart)
					tb->bgcolour=(atg_colour){47, 63, 79, ATG_ALPHA_OPAQUE};
				else
					tb->bgcolour=(atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE};
			}
		}
		atg_ebox_empty(SG_text_box);
		if(selstart>=0)
		{
			snprintf(SG_datestring, 11, "%02u-%02u-%04u", state->now.day, state->now.month, state->now.year);
			const char *bodytext=starts[selstart].description;
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
				if(atg_ebox_pack(SG_text_box, r))
				{
					perror("atg_ebox_pack");
					atg_free_element(r);
					break;
				}
				x+=l;
				if(bodytext[x]=='\n') x++;
			}
			for(unsigned int i=0;i<ntypes;i++)
			{
				unsigned int count=0;
				for(unsigned int j=0;j<state->nbombers;j++)
					if(state->bombers[j].type==i) count++;
				if(SG_btnum[i])
					snprintf(SG_btnum[i], 8, "%u", count);
				if(SG_btrow[i])
					SG_btrow[i]->hidden=!count;
			}
			for(unsigned int i=0;i<nftypes;i++)
			{
				unsigned int count=0;
				for(unsigned int j=0;j<state->nfighters;j++)
					if(state->fighters[j].type==i) count++;
				if(SG_ftnum[i])
					snprintf(SG_ftnum[i], 8, "%u", count);
				if(SG_ftrow[i])
					SG_ftrow[i]->hidden=!count;
			}
		}
		else
		{
			snprintf(SG_datestring, 11, "  -  -  ");
		}

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
							return(SCRN_MAINMENU);
						case SDL_VIDEORESIZE:
							mainsizex=canvas->surface->w;
							mainsizey=canvas->surface->h;
						break;
					}
				break;
				case ATG_EV_CLICK:;
					atg_ev_click c=e.event.click;
					for(i=0;i<(int)nstarts;i++)
						if(c.e==SG_stitles[i]) break;
					if(i<(int)nstarts)
					{
						selstart=i;
						fprintf(stderr, "Loading game state from '%s'...\n", starts[i].filename);
						if(loadgame(starts[i].filename, state))
						{
							fprintf(stderr, "Failed to load from start file\n");
							selstart=-1;
						}
					}
					else if(c.e==SG_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
					}
					else if (c.e==SG_exit)
						return(SCRN_MAINMENU);
					else if(c.e)
					{
						unsigned int j;
						for(j=0;j<ntypes;j++)
							if(c.e==SG_btint[j]) break;
						if(j<ntypes)
						{
							IB_i=j;
							IB_showmark=types[j].newmark;
							intel_caller=SCRN_SETPGAME;
							return(SCRN_INTELBMB);
						}
						else
						{
							for(j=0;j<nftypes;j++)
								if(c.e==SG_ftint[j]) break;
							if(j<nftypes)
							{
								IF_i=j;
								intel_caller=SCRN_SETPGAME;
								return(SCRN_INTELFTR);
							}
							else
							{
								fprintf(stderr, "Clicked on unknown clickable!\n");
							}
						}
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==SG_cont)
					{
						for(unsigned int i=0;i<ntypes;i++)
							state->btypes[i]=!types[i].extra;
						difficulty_show_only=false;
						return(SCRN_SETPDIFF);
					}
					else if(!trigger.e)
					{
						// internal error
					}
					else
						fprintf(stderr, "Clicked on unknown button!\n");
				break;
				default:
				break;
			}
		}
		SDL_Delay(50);
	}
}

void setup_game_free(void)
{
	atg_free_element(setup_game_box);
}
