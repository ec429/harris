/*
	harris - a strategy game
	Copyright (C) 2012-2013 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	widgets: defines custom libatg widgets
*/
#include "widgets.h"
#include <atg_internals.h>

const char *prio_labels[4]={"NONE","LOW","MED","HIGH"};
atg_colour prio_colours[4]={{31, 31, 95, 0}, {95, 31, 31, 0}, {95, 95, 15, 0}, {31, 159, 31, 0}};

SDL_Surface *priority_selector_render_callback(const struct atg_element *e);
void priority_selector_match_click_callback(struct atg_event_list *list, atg_element *element, SDL_MouseButtonEvent button, unsigned int xoff, unsigned int yoff);

atg_element *create_priority_selector(unsigned int *prio)
{
	atg_element *rv=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
	if(!rv) return(NULL);
	rv->type=ATG_CUSTOM;
	rv->match_click_callback=priority_selector_match_click_callback;
	rv->render_callback=priority_selector_render_callback;
	atg_box *b=rv->elem.box;
	if(!b)
	{
		atg_free_element(rv);
		return(NULL);
	}
	for(unsigned int i=0;i<4;i++)
	{
		atg_colour fg=prio_colours[i];
		atg_element *btn=atg_create_element_button(prio_labels[i], fg, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE});
		if(!btn)
		{
			atg_free_element(rv);
			return(NULL);
		}
		if(atg_pack_element(b, btn))
		{
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
	if(!(e->type==ATG_CUSTOM)) return(NULL);
	atg_box *b=e->elem.box;
	if(!b) return(NULL);
	if(!b->elems) return(NULL);
	for(unsigned int i=0;i<b->nelems;i++)
	{
		if(e->userdata)
		{
			if(*(unsigned int *)e->userdata==i)
				b->elems[i]->elem.button->content->bgcolour=(atg_colour){159, 159, 159, ATG_ALPHA_OPAQUE};
			else
				b->elems[i]->elem.button->content->bgcolour=(atg_colour){31, 31, 31, ATG_ALPHA_OPAQUE};
		}
		else
			b->elems[i]->elem.button->content->bgcolour=(atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE};
	}
	return(atg_render_box(e));
}

void priority_selector_match_click_callback(__attribute__((unused)) struct atg_event_list *list, atg_element *element, SDL_MouseButtonEvent button, unsigned int xoff, unsigned int yoff)
{
	atg_box *b=element->elem.box;
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
