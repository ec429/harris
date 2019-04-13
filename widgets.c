/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	widgets: defines custom libatg widgets
*/
#include "widgets.h"
#include <atg_internals.h>
#include "bits.h"
#include "date.h"
#include "globals.h"
#include "render.h"

const char *prio_labels[4]={"NONE","LOW","MED","HIGH"};
atg_colour prio_colours[4]={{31, 31, 95, 0}, {95, 31, 31, 0}, {95, 95, 15, 0}, {31, 159, 31, 0}};

SDL_Surface *priority_selector_render_callback(const struct atg_element *e);
void priority_selector_match_click_callback(struct atg_event_list *list, atg_element *element, SDL_MouseButtonEvent button, unsigned int xoff, unsigned int yoff);

atg_element *create_priority_selector(unsigned int *prio)
{
	atg_element *rv=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
	if(!rv) return(NULL);
	rv->type="priority_selector";
	rv->match_click_callback=priority_selector_match_click_callback;
	rv->render_callback=priority_selector_render_callback;
	for(unsigned int i=0;i<4;i++)
	{
		atg_colour fg=prio_colours[i];
		atg_element *btn=atg_create_element_button(prio_labels[i], fg, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
		if(!btn)
		{
			atg_free_element(rv);
			return(NULL);
		}
		if(atg_ebox_pack(rv, btn))
		{
			atg_free_element(btn);
			atg_free_element(rv);
			return(NULL);
		}
	}
	rv->userdata=prio;
	return(rv);
}

SDL_Surface *priority_selector_render_callback(const struct atg_element *e)
{
	if(!e) return(NULL);
	atg_box *b=e->elemdata;
	if(!b) return(NULL);
	if(!b->elems) return(NULL);
	for(unsigned int i=0;i<b->nelems;i++)
	{
		atg_button *button=b->elems[i]->elemdata;
		if(e->userdata)
		{
			if(*(unsigned int *)e->userdata==i)
				button->content->bgcolour=(atg_colour){159, 159, 159, ATG_ALPHA_OPAQUE};
			else
				button->content->bgcolour=(atg_colour){31, 31, 31, ATG_ALPHA_OPAQUE};
		}
		else
			button->content->bgcolour=(atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE};
	}
	return(atg_render_box(e));
}

void priority_selector_match_click_callback(struct atg_event_list *list, atg_element *element, SDL_MouseButtonEvent button, unsigned int xoff, unsigned int yoff)
{
	atg_box *b=element->elemdata;
	if(!b->elems) return;
	struct atg_event_list sub_list={.list=NULL, .last=NULL};
	for(unsigned int i=0;i<b->nelems;i++)
		atg__match_click_recursive(&sub_list, b->elems[i], button, xoff+element->display.x, yoff+element->display.y);
	unsigned int oldval=0;
	if(element->userdata) oldval=*(unsigned int *)element->userdata;
	while(sub_list.list)
	{
		atg_event event=sub_list.list->event;
		if(event.type==ATG_EV_TRIGGER)
		{
			if(event.event.trigger.button==ATG_MB_LEFT)
			{
				for(unsigned int i=0;i<b->nelems;i++)
				{
					if(event.event.trigger.e==b->elems[i])
					{
						if(element->userdata) *(unsigned int *)element->userdata=i;
					}
				}
			}
			else if(event.event.trigger.button==ATG_MB_SCROLLDN)
			{
				if(element->userdata) *(unsigned int *)element->userdata=(1+*(unsigned int *)element->userdata)%b->nelems;
			}
			else if(event.event.trigger.button==ATG_MB_SCROLLUP)
			{
				if(element->userdata) *(unsigned int *)element->userdata=(b->nelems-1+*(unsigned int *)element->userdata)%b->nelems;
			}
		}
		atg__event_list *next=sub_list.list->next;
		free(sub_list.list);
		sub_list.list=next;
	}
	if(element->userdata&&(*(unsigned int *)element->userdata!=oldval))
		atg__push_event(list, (atg_event){.type=ATG_EV_VALUE, .event.value=(atg_ev_value){.e=element, .value=*(unsigned int *)element->userdata}});
}

SDL_Surface *filter_switch_render_callback(const struct atg_element *e);
void filter_switch_match_click_callback(struct atg_event_list *list, atg_element *element, SDL_MouseButtonEvent button, unsigned int xoff, unsigned int yoff);

atg_element *create_filter_switch(SDL_Surface *icon, int *value)
{
	atg_element *rv=atg_create_element_image(icon);
	if(!rv) return(NULL);
	rv->type="filter_switch";
	rv->match_click_callback=filter_switch_match_click_callback;
	rv->render_callback=filter_switch_render_callback;
	rv->userdata=value;
	return(rv);
}

SDL_Surface *filter_switch_render_callback(const struct atg_element *e)
{
	if(!e) return(NULL);
	if(!e->userdata) return(NULL);
	atg_image *i=e->elemdata;
	if(!i) return(NULL);
	SDL_Surface *rv=atg_resize_surface(i->data, e);
	unsigned int w=rv->w, h=rv->h;
	switch(*(int *)e->userdata)
	{
		case 1:
			SDL_FillRect(rv, &(SDL_Rect){(w>>1)-(w>>2), (h>>1)-1, (w>>1), 2}, SDL_MapRGB(rv->format, 0, 255, 0));
			SDL_FillRect(rv, &(SDL_Rect){(w>>1)-1, (h>>1)-(h>>2), 2, (h>>1)}, SDL_MapRGB(rv->format, 0, 255, 0));
		break;
		case -1:
			SDL_FillRect(rv, &(SDL_Rect){(w>>1)-(w>>2), (h>>1)-1, (w>>1), 2}, SDL_MapRGB(rv->format, 255, 0, 0));
		break;
	}
	return(rv);
}

void filter_switch_match_click_callback(struct atg_event_list *list, atg_element *element, SDL_MouseButtonEvent button, __attribute__((unused)) unsigned int xoff, __attribute__((unused)) unsigned int yoff)
{
	int *u=element->userdata;
	if(!u) return;
	int old=*u;
	switch(button.button)
	{
		case ATG_MB_LEFT:
		case ATG_MB_SCROLLUP:
			*u=min(*u+1, 1);
		break;
		case ATG_MB_RIGHT:
		case ATG_MB_SCROLLDN:
			*u=max(*u-1, -1);
		break;
	}
	if(*u!=old)
		atg__push_event(list, (atg_event){.type=ATG_EV_VALUE, .event.value=(atg_ev_value){.e=element, .value=*u}});
}

SDL_Surface *bfilter_switch_render_callback(const struct atg_element *e);
void bfilter_switch_match_click_callback(struct atg_event_list *list, atg_element *element, SDL_MouseButtonEvent button, unsigned int xoff, unsigned int yoff);

void bfilter_switch_free_callback(struct atg_element *e)
{
	atg_ebox_empty(e);
	free(e->userdata);
}

atg_element *create_bfilter_switch(unsigned int count, atg_colour bgcolour, SDL_Surface **icon, bool *value)
{
	atg_element *rv=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, bgcolour);
	unsigned int i;
	if(!rv) return(NULL);
	atg_element **btns=calloc(count, sizeof(atg_element *));
	if(!btns)
	{
		atg_free_element(rv);
		return(NULL);
	}
	rv->type="bfilter_switch";
	for(i=0;i<count;i++)
	{
		btns[i]=atg_create_element_image(icon[i]);
		if(!btns[i]) goto fail;
		btns[i]->userdata=value+i;
		btns[i]->render_callback=bfilter_switch_render_callback;
		btns[i]->match_click_callback=bfilter_switch_match_click_callback;
		if(atg_ebox_pack(rv, btns[i]))
		{
			atg_free_element(btns[i]);
			goto fail;
		}
	}
	rv->userdata=btns;
	rv->free_callback=bfilter_switch_free_callback;
	return(rv);
fail:
	atg_ebox_empty(rv);
	free(btns);
	atg_free_element(rv);
	return(NULL);
}

SDL_Surface *bfilter_switch_render_callback(const struct atg_element *e)
{
	if(!e) return(NULL);
	if(!e->userdata) return(NULL);
	atg_image *i=e->elemdata;
	if(!i) return(NULL);
	SDL_Surface *rv=atg_resize_surface(i->data, e);
	unsigned int w=rv->w, h=rv->h;
	if(*(bool *)e->userdata)
	{
		SDL_FillRect(rv, &(SDL_Rect){(w>>1)-(w>>2), (h>>1)-1, (w>>1), 2}, SDL_MapRGB(rv->format, 0, 255, 0));
		SDL_FillRect(rv, &(SDL_Rect){(w>>1)-1, (h>>1)-(h>>2), 2, (h>>1)}, SDL_MapRGB(rv->format, 0, 255, 0));
	}
	return(rv);
}

void bfilter_switch_match_click_callback(struct atg_event_list *list, atg_element *element, SDL_MouseButtonEvent button, __attribute__((unused)) unsigned int xoff, __attribute__((unused)) unsigned int yoff)
{
	bool *u=element->userdata;
	if(!u) return;
	switch(button.button)
	{
		case ATG_MB_LEFT:
		case ATG_MB_RIGHT:
			*u=!*u;
		break;
	}
	atg__push_event(list, (atg_event){.type=ATG_EV_VALUE, .event.value=(atg_ev_value){.e=element, .value=*u}});
}
/**/

SDL_Surface *load_selector_render_callback(const struct atg_element *e);
void load_selector_match_click_callback(struct atg_event_list *list, atg_element *element, SDL_MouseButtonEvent button, unsigned int xoff, unsigned int yoff);

struct load_selector_userdata
{
	bombertype *type;
	unsigned int *load;
};

atg_element *create_load_selector(bombertype *type, unsigned int *load)
{
	atg_element *rv=atg_create_element_image(NULL);
	if(!rv) return(NULL);
	rv->type="load_selector";
	rv->match_click_callback=load_selector_match_click_callback;
	rv->render_callback=load_selector_render_callback;
	struct load_selector_userdata *userdata=malloc(sizeof(struct load_selector_userdata));
	if(!userdata)
	{
		perror("malloc");
		atg_free_element(rv);
		return(NULL);
	}
	userdata->type=type;
	userdata->load=load;
	rv->userdata=userdata;
	return(rv);
}

SDL_Surface *load_selector_render_callback(const struct atg_element *e)
{
	if(!e) return(NULL);
	if(!e->userdata) return(NULL);
	struct load_selector_userdata *u=e->userdata;
	if(*u->load>=NBOMBLOADS) return(NULL);
	SDL_Surface *rv=bombloads[*u->load].pic;
	if(rv) rv->refcount++;
	return(rv);
}

void load_selector_match_click_callback(struct atg_event_list *list, atg_element *element, SDL_MouseButtonEvent button, __attribute__((unused)) unsigned int xoff, __attribute__((unused)) unsigned int yoff)
{
	struct load_selector_userdata *u=element->userdata;
	if(!u) return;
	bombertype *t=u->type;
	if(!t) return;
	unsigned int old=*u->load;
	int limit=0;
	switch(button.button)
	{
		case ATG_MB_LEFT:
		case ATG_MB_SCROLLUP:
			do
			{
				if(limit++>NBOMBLOADS) return;
				*u->load=(*u->load+1)%NBOMBLOADS;
			}
			while(!t->load[*u->load]);
		break;
		case ATG_MB_RIGHT:
		case ATG_MB_SCROLLDN:
			do
			{
				if(limit++>NBOMBLOADS) return;
				*u->load=(*u->load+NBOMBLOADS-1)%NBOMBLOADS;
			}
			while(!t->load[*u->load]);
		break;
	}
	if(*u->load!=old)
		atg__push_event(list, (atg_event){.type=ATG_EV_VALUE, .event.value=(atg_ev_value){.e=element, .value=*u->load}});
}

SDL_Surface *time_spinner_render_callback(const atg_element *e)
{
	if(!e) return(NULL);
	atg_spinner *s=e->elemdata;
	if(!s) return(NULL);
	harris_time t=maketime(s->value);
	snprintf(s->val, 6, s->fmt, t.hour, t.minute); // it is assumed that 6 <= VALUE_LEN (from atg:w_spinner.c)
	SDL_Surface *content=atg_render_box(&(atg_element){.w=e->w, .h=e->h, .elemdata=s->content, .clickable=false, .userdata=NULL});
	if(!content) return(NULL);
	SDL_Surface *rv=SDL_CreateRGBSurface(SDL_HWSURFACE, content->w, content->h, content->format->BitsPerPixel, content->format->Rmask, content->format->Gmask, content->format->Bmask, content->format->Amask);
	if(!rv)
	{
		SDL_FreeSurface(content);
		return(NULL);
	}
	SDL_FillRect(rv, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, SDL_MapRGBA(rv->format, s->content->bgcolour.r, s->content->bgcolour.g, s->content->bgcolour.b, s->content->bgcolour.a));
	SDL_BlitSurface(content, NULL, rv, &(SDL_Rect){.x=0, .y=0});
	SDL_FreeSurface(content);
	return(rv);
}

atg_element *create_time_spinner(Uint8 flags, int minval, int maxval, int step, int initvalue, const char *fmt, atg_colour fgcolour, atg_colour bgcolour)
{
	atg_element *rv=atg_create_element_spinner(flags, minval, maxval, step, initvalue, fmt, fgcolour, bgcolour);
	if(!rv) return(NULL);
	rv->type="time_spinner";
	rv->render_callback=time_spinner_render_callback;
	return(rv);
}
