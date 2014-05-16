/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	ui: user interface framework
*/

#include "ui.h"

#include "globals.h"
#include "date.h"

screen_id intel_caller=SCRN_MAINMENU;

void update_navbtn(game state, atg_element *(*GB_navbtn)[NNAVAIDS], unsigned int i, unsigned int n, SDL_Surface *grey_overlay, SDL_Surface *yellow_overlay)
{
	if(GB_navbtn[i][n]&&GB_navbtn[i][n]->elemdata)
	{
		atg_image *img=GB_navbtn[i][n]->elemdata;
		SDL_Surface *pic=img->data;
		if(pic)
		{
			if(!types[i].nav[n])
				SDL_FillRect(pic, &(SDL_Rect){.x=0, .y=0, .w=16, .h=16}, SDL_MapRGBA(pic->format, 63, 63, 63, SDL_ALPHA_OPAQUE));
			else
			{
				SDL_BlitSurface(navpic[n], NULL, pic, NULL);
				if(datebefore(state.now, event[navevent[n]]))
					SDL_BlitSurface(grey_overlay, NULL, pic, NULL);
				if((state.nap[n]==(int)i) || (state.nap[n]<0))
					SDL_BlitSurface(yellow_overlay, NULL, pic, NULL);
			}
		}
	}
}

int msgadd(atg_canvas *canvas, game *state, date when, const char *ref, const char *msg)
{
	if(!state) return(1);
	if(!ref) return(2);
	if(!msg) return(2);
	size_t rl=strlen(ref);
	char *res=malloc(13+rl+strlen(msg));
	if(!res)
		return(-1);
	snprintf(res, 12, "%02u-%02u-%04u:", when.day, when.month, when.year);
	strcpy(res+11, ref);
	strcat(res+11+rl, "\n");
	strcat(res+12+rl, msg);
	unsigned int in=0, out=0;
	while(in<MAXMSGS)
	{
		if(state->msg[in])
			state->msg[out++]=state->msg[in];
		in++;
	}
	unsigned int fill=out;
	while(fill<MAXMSGS)
		state->msg[fill++]=NULL;
	if(out<MAXMSGS)
		state->msg[out]=res;
	else
	{
		for(in=1;in<MAXMSGS;in++)
			state->msg[in-1]=state->msg[in];
		state->msg[MAXMSGS-1]=res;
	}
	message_box(canvas, "To the Commander-in-Chief, Bomber Command:", res, "Air Chief Marshal C. F. A. Portal, CAS");
	return(0);
}

void message_box(atg_canvas *canvas, const char *titletext, const char *bodytext, const char *signtext)
{
	atg_element *oldbox=canvas->content;
	atg_element *msgbox=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE});
	if(!msgbox)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return;
	}
	atg_element *title=atg_create_element_label(titletext, 16, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
	if(!title)
		fprintf(stderr, "atg_create_element_label failed\n");
	else
	{
		if(atg_ebox_pack(msgbox, title))
			perror("atg_ebox_pack");
		else
		{
			size_t x=0;
			while(bodytext[x])
			{
				size_t l=strcspn(bodytext+x, "\n");
				char *t=l?strndup(bodytext+x, l):strdup(" ");
				atg_element *r=atg_create_element_label(t, 12, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
				free(t);
				if(!r)
				{
					fprintf(stderr, "atg_create_element_label failed\n");
					break;
				}
				if(atg_ebox_pack(msgbox, r))
				{
					perror("atg_ebox_pack");
					break;
				}
				x+=l;
				if(bodytext[x]=='\n') x++;
			}
			if(!bodytext[x])
			{
				atg_element *sign=atg_create_element_label(signtext, 14, (atg_colour){2, 2, 9, ATG_ALPHA_OPAQUE});
				if(!sign)
					fprintf(stderr, "atg_create_element_label failed\n");
				else
				{
					if(atg_ebox_pack(msgbox, sign))
						perror("atg_ebox_pack");
					else
					{
						atg_element *cont = atg_create_element_button("Continue", (atg_colour){31, 31, 31, ATG_ALPHA_OPAQUE}, (atg_colour){239, 239, 224, ATG_ALPHA_OPAQUE});
						if(!cont)
							fprintf(stderr, "atg_create_element_button failed\n");
						else
						{
							if(atg_ebox_pack(msgbox, cont))
								perror("atg_ebox_pack");
							else
							{
								canvas->content=msgbox;
								atg_flip(canvas);
								atg_event e;
								while(1)
								{
									if(atg_poll_event(&e, canvas))
									{
										if(e.type==ATG_EV_TRIGGER) break;
										if(e.type==ATG_EV_RAW)
										{
											SDL_Event s=e.event.raw;
											if(s.type==SDL_QUIT) break;
										}
									}
									else
										SDL_Delay(50);
									atg_flip(canvas);
								}
								canvas->content=oldbox;
							}
						}
					}
				}
			}
		}
	}
	atg_free_element(msgbox);
}
