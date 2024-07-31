/*
	harris - a strategy game
	Copyright (C) 2012-2020 Edward Cree

	licensed under GPLv2 - see top of harris.c for details

	handle_manfs: screen for tasking manufacturers' R&D
*/

#include "ui.h"
#include "globals.h"
#include "bits.h"
#include "builder/data.h"
#include "builder/calc.h"
#include "builder.h"

atg_element *handle_manfs_box;

atg_element *HM_cont, *HM_full;
atg_element **HM_mbox;
char *HM_out_buf[OUT_ROWS];
SDL_Surface *HM_bp;

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
	for(unsigned int i=0;i<builder->entities.nmanf;i++)
	{
		struct manf *m=builder->entities.manf[i];
		atg_element *mbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
		if(!mbox)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_ebox_pack(leftbox, mbox))
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
	atg_element *rightbox;
	int rc=builder_rightbox_create(&rightbox, HM_out_buf, &HM_bp);
	if(rc==1)
		atg_free_element(rightbox);
	if(rc)
		return(1);
	if(atg_ebox_pack(handle_manfs_box, rightbox))
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

screen_id handle_manfs_screen(atg_canvas *canvas, game *state)
{
	screen_id rc=SCRN_CONTROL;
	atg_event e;

	//int seldes=-1;
	atg_element **HM_dbtn=calloc(state->ndesigns, sizeof(atg_element *));
	for(unsigned int i=0;i<builder->entities.nmanf;i++)
	{
		struct manf *m=builder->entities.manf[i];
		atg_element *box=HM_mbox[i];
		atg_ebox_empty(box);
		for(unsigned int j=0;j<state->ndesigns;j++)
		{
			struct bomber *b=state->designs+j;
			if(b->manf!=m) continue;
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
				atg_free_element(row);
				break;
			}
		}
	}
	wipe_m2v(HM_out_buf, HM_bp);

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
					for(i=0;i<state->ndesigns;i++)
					{
						if(trigger.e==HM_dbtn[i])
						{
							//seldes=i;
							struct bomber *b=state->designs+i;
							calc_bomber(b, &b->tn);
							builder_update_m2v(b, HM_out_buf);
							break;
						}
					}
					if(i<state->ndesigns)
						break;
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
out:
	free(HM_dbtn);
	return(rc);
}

void handle_manfs_free(void)
{
	atg_free_element(handle_manfs_box);
}
