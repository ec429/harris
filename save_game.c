/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	save_game: the "Save Game" screen
*/

#include <stdio.h>

#include "save_game.h"
#include "ui.h"
#include "globals.h"
#include "saving.h"

#define NAMEBUFLEN	256

atg_element *save_game_box;
char *SA_btext;
atg_element *SA_file, *SA_text, *SA_full, *SA_exit, *SA_save;

int save_game_create(void)
{
	save_game_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 47, ATG_ALPHA_OPAQUE});
	if(!save_game_box)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	SA_file=atg_create_element_filepicker("Save Game", NULL, (atg_colour){239, 239, 255, ATG_ALPHA_OPAQUE}, (atg_colour){31, 31, 47, ATG_ALPHA_OPAQUE});
	if(!SA_file)
	{
		fprintf(stderr, "atg_create_element_filepicker failed\n");
		return(1);
	}
	SA_file->h=mainsizey-30;
	SA_file->w=mainsizex;
	if(atg_ebox_pack(save_game_box, SA_file))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *SA_hbox=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE});
	if(!SA_hbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	else if(atg_ebox_pack(save_game_box, SA_hbox))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	else
	{
		SA_save=atg_create_element_button("Save", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE});
		if(!SA_save)
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		SA_save->w=34;
		if(atg_ebox_pack(SA_hbox, SA_save))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		SA_text=atg_create_element_button_empty((atg_colour){239, 255, 239, ATG_ALPHA_OPAQUE}, (atg_colour){39, 39, 55, ATG_ALPHA_OPAQUE});
		if(!SA_text)
		{
			fprintf(stderr, "atg_create_element_button_empty failed\n");
			return(1);
		}
		SA_text->h=24;
		SA_text->w=mainsizex-64;
		if(atg_ebox_pack(SA_hbox, SA_text))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(!(SA_btext=malloc(NAMEBUFLEN)))
		{
			perror("malloc");
			return(1);
		}
		atg_element *label=atg_create_element_label_nocopy(SA_btext, 12, (atg_colour){239, 255, 239, ATG_ALPHA_OPAQUE});
		if(!label)
		{
			fprintf(stderr, "atg_create_element_label_nocopy failed\n");
			return(1);
		}
		if(atg_ebox_pack(SA_text, label))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		SA_btext[0]=0;
		SA_full=atg_create_element_image(fullbtn);
		if(!SA_full)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		SA_full->w=16;
		SA_full->clickable=true;
		if(atg_ebox_pack(SA_hbox, SA_full))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		SA_exit=atg_create_element_image(exitbtn);
		if(!SA_exit)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		SA_exit->clickable=true;
		if(atg_ebox_pack(SA_hbox, SA_exit))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	return(0);
}

screen_id save_game_screen(atg_canvas *canvas, game *state)
{
	atg_event e;
	atg_filepicker *f=SA_file->elemdata;

	if(!f)
	{
		fprintf(stderr, "Error: SA_file->elemdata==NULL\n");
		return(SCRN_CONTROL);
	}
	while(1)
	{
		SA_file->w=mainsizex;
		SA_file->h=mainsizey-30;
		SA_text->w=mainsizex-64;
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
							return(SCRN_CONTROL);
						case SDL_VIDEORESIZE:
							mainsizex=canvas->surface->w;
							mainsizey=canvas->surface->h;
						break;
						case SDL_KEYDOWN:;
							switch(s.key.keysym.sym)
							{
								case SDLK_BACKSPACE:
									if(SA_btext)
									{
										size_t l=strlen(SA_btext);
										if(l)
											SA_btext[l-1]=0;
									}
								break;
								case SDLK_RETURN:
									goto do_save;
								default:
									if((s.key.keysym.unicode&0xFF80)==0) // TODO handle UTF-8 in filenames?
									{
										char what=s.key.keysym.unicode&0x7F;
										if(what)
										{
											if(f->value)
											{
												size_t l=strlen(f->value);
												char *n=realloc(f->value, l+2);
												if(n)
												{
													(f->value=n)[l]=what;
													n[l+1]=0;
													if(SA_btext)
													{
														if(f->value)
															snprintf(SA_btext, NAMEBUFLEN, "%s", f->value);
														else
															SA_btext[0]=0;
													}
												}
												else
													perror("realloc");
											}
											else
											{
												f->value=malloc(2);
												if(f->value)
												{
													f->value[0]=what;
													f->value[1]=0;
													if(SA_btext)
													{
														if(f->value)
															snprintf(SA_btext, NAMEBUFLEN, "%s", f->value);
														else
															SA_btext[0]=0;
													}
												}
												else
													perror("malloc");
											}
										}
									}
								break;
							}
						break;
					}
				break;
				case ATG_EV_CLICK:;
					atg_ev_click c=e.event.click;
					if(c.e==SA_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
					}
					else if (c.e==SA_exit)
						return(SCRN_CONTROL);
					else if(c.e)
					{
						fprintf(stderr, "Clicked on unknown clickable!\n");
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==SA_save)
					{
						do_save:;
						if(!f->curdir)
						{
							fprintf(stderr, "Error: f->curdir==NULL\n");
						}
						else if(!f->value)
						{
							fprintf(stderr, "Select/enter a filename first!\n");
						}
						else
						{
							char *file=malloc(strlen(f->curdir)+strlen(f->value)+1);
							sprintf(file, "%s%s", f->curdir, f->value);
							fprintf(stderr, "Saving game state to '%s'...\n", file);
							if(!savegame(file, *state))
							{
								fprintf(stderr, "Game saved\n");
								return(SCRN_CONTROL);
							}
							else
							{
								fprintf(stderr, "Failed to save to file\n");
							}
						}
					}
					else if(trigger.e==SA_text)
					{
						free(f->value);
						f->value=NULL;
						if(SA_btext)
							SA_btext[0]=0;
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
					if(value.e==SA_file)
					{
						if(SA_btext)
						{
							if(f->value)
								snprintf(SA_btext, NAMEBUFLEN, "%s", f->value);
							else
								SA_btext[0]=0;
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

void save_game_free(void)
{
	atg_free_element(save_game_box);
}
