/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	load_game: the "Load Game" screen
*/

#include <stdio.h>
#include <atg.h>

#include "load_game.h"
#include "ui.h"
#include "globals.h"
#include "saving.h"

atg_box *load_game_box;
char **LB_btext;
atg_element *LB_file, *LB_text, *LB_full, *LB_exit, *LB_load;

int load_game_create(void)
{
	load_game_box=atg_create_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 47, ATG_ALPHA_OPAQUE});
	if(!load_game_box)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	LB_file=atg_create_element_filepicker("Load Game", NULL, (atg_colour){239, 239, 255, ATG_ALPHA_OPAQUE}, (atg_colour){31, 31, 47, ATG_ALPHA_OPAQUE});
	if(!LB_file)
	{
		fprintf(stderr, "atg_create_element_filepicker failed\n");
		return(1);
	}
	LB_file->h=mainsizey-30;
	LB_file->w=mainsizex;
	if(atg_pack_element(load_game_box, LB_file))
	{
		perror("atg_pack_element");
		return(1);
	}
	atg_element *LB_hbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE});
	LB_btext=NULL;
	if(!LB_hbox)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	else if(atg_pack_element(load_game_box, LB_hbox))
	{
		perror("atg_pack_element");
		return(1);
	}
	else
	{
		atg_box *b=LB_hbox->elem.box;
		if(!b)
		{
			fprintf(stderr, "LB_hbox->elem.box==NULL\n");
			return(1);
		}
		LB_load=atg_create_element_button("Load", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE});
		if(!LB_load)
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		LB_load->w=34;
		if(atg_pack_element(b, LB_load))
		{
			perror("atg_pack_element");
			return(1);
		}
		LB_text=atg_create_element_button(NULL, (atg_colour){239, 255, 239, ATG_ALPHA_OPAQUE}, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE});
		if(!LB_text)
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		LB_text->h=24;
		LB_text->w=mainsizex-64;
		if(atg_pack_element(b, LB_text))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_button *tb=LB_text->elem.button;
		if(!tb)
		{
			fprintf(stderr, "LB_text->elem.button==NULL\n");
			return(1);
		}
		atg_box *t=tb->content;
		if(!t)
		{
			fprintf(stderr, "tb->content==NULL\n");
			return(1);
		}
		if(t->nelems&&t->elems&&t->elems[0]->type==ATG_LABEL)
		{
			atg_label *l=t->elems[0]->elem.label;
			if(!l)
			{
				fprintf(stderr, "t->elems[0]->elem.label==NULL\n");
				return(1);
			}
			LB_btext=&l->text;
		}
		else
		{
			fprintf(stderr, "LB_text has wrong content\n");
			return(1);
		}
		LB_full=atg_create_element_image(fullbtn);
		if(!LB_full)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		LB_full->w=16;
		LB_full->clickable=true;
		if(atg_pack_element(b, LB_full))
		{
			perror("atg_pack_element");
			return(1);
		}
		LB_exit=atg_create_element_image(exitbtn);
		if(!LB_exit)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		LB_exit->clickable=true;
		if(atg_pack_element(b, LB_exit))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	return(0);
}

screen_id load_game_screen(atg_canvas *canvas, game *state)
{
	atg_event e;
	
	*LB_btext=NULL;
	free(LB_file->elem.filepicker->value);
	LB_file->elem.filepicker->value=NULL;
	atg_resize_canvas(canvas, mainsizex, mainsizey);
	while(1)
	{
		LB_file->w=mainsizex;
		LB_file->h=mainsizey-30;
		LB_text->w=mainsizex-64;
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
					if(c.e==LB_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
					}
					else if (c.e==LB_exit)
						return(SCRN_MAINMENU);
					else if(c.e)
					{
						fprintf(stderr, "Clicked on unknown clickable!\n");
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==LB_load)
					{
						atg_filepicker *f=LB_file->elem.filepicker;
						if(!f)
						{
							fprintf(stderr, "Error: LB_file->elem.filepicker==NULL\n");
						}
						else if(!f->curdir)
						{
							fprintf(stderr, "Error: f->curdir==NULL\n");
						}
						else if(!f->value)
						{
							fprintf(stderr, "Select a file first!\n");
						}
						else
						{
							char *file=malloc(strlen(f->curdir)+strlen(f->value)+1);
							sprintf(file, "%s%s", f->curdir, f->value);
							fprintf(stderr, "Loading game state from '%s'...\n", file);
							if(!loadgame(file, state, lorw))
							{
								fprintf(stderr, "Game loaded\n");
								mainsizex=canvas->surface->w;
								mainsizey=canvas->surface->h;
								return(SCRN_CONTROL);
							}
							else
							{
								fprintf(stderr, "Failed to load from save file\n");
							}
						}
					}
					else if(trigger.e==LB_text)
					{
						if(LB_btext&&*LB_btext)
							**LB_btext=0;
					}
					else if(!trigger.e)
					{
						// internal error
					}
					else
						fprintf(stderr, "Clicked on unknown button!\n");
				break;
				case ATG_EV_VALUE:;
					atg_ev_value value=e.event.value;
					if(value.e==LB_file)
					{
						atg_filepicker *f=LB_file->elem.filepicker;
						if(!f)
						{
							fprintf(stderr, "Error: LB_file->elem.filepicker==NULL\n");
						}
						else
						{
							if(LB_btext) *LB_btext=f->value;
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

void load_game_free(void)
{
	if(LB_btext) *LB_btext=NULL;
	atg_free_box_box(load_game_box);
}
