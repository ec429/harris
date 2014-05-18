/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	intel_targets: targets' intel screen
*/

#include "intel_targets.h"
#include <stdbool.h>
#include "ui.h"
#include "globals.h"
#include "bits.h"
#include "date.h"
#include "render.h"

atg_element *intel_targets_box;
atg_element *IT_full, *IT_bombers_tab, *IT_fighters_tab, *IT_targets_tab, *IT_cont;
atg_element **IT_targs, *IT_map, *IT_text_box, *IT_stat_box;
unsigned int IT_i;

void update_intel_targets(const game *state);

enum t_stat_i
{
	STAT_TCLASS,
	STAT_DMG,
	STAT_FLAK,
	STAT_PROD,
	STAT_ESIZ,

	NUM_STATS
};

const char *tclass_fn(unsigned int ti);
int dmg_fn(unsigned int ti, const game *state);
int flak_fn(unsigned int ti, const game *state);
int prod_fn(unsigned int ti, const game *state);
int esiz_fn(unsigned int ti, const game *state);

struct t_stat_row
{
	const char *name;
	int (*v_fn)(unsigned int ti, const game *state);
	const char *(*t_fn)(unsigned int ti);
	unsigned int bar_min, bar_max;
	bool bar_rev; // reverse colours
}
t_stat_rows[NUM_STATS]=
{
	[STAT_TCLASS]={.name="Target class",  .t_fn=tclass_fn},
	[STAT_DMG   ]={.name="How intact",    .v_fn=dmg_fn,  .bar_min=0, .bar_max=100, .bar_rev=false},
	[STAT_FLAK  ]={.name="Flak strength", .v_fn=flak_fn, .bar_min=0, .bar_max=40,  .bar_rev=true },
	[STAT_PROD  ]={.name="Production",    .v_fn=prod_fn, .bar_min=0, .bar_max=100, .bar_rev=false},
	[STAT_ESIZ  ]={.name="Ease to hit",   .v_fn=esiz_fn, .bar_min=0, .bar_max=50,  .bar_rev=false},
};

int intel_targets_create(void)
{
	if(!(intel_targets_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR)))
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	atg_element *title=atg_create_element_label("HARRIS: Intel & Information", 12, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!title)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(intel_targets_box, title))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *tab_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!tab_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(intel_targets_box, tab_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(IT_bombers_tab=atg_create_element_button("Bombers", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(tab_box, IT_bombers_tab))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(IT_fighters_tab=atg_create_element_button("Fighters", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(tab_box, IT_fighters_tab))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(IT_targets_tab=atg_create_element_button("Targets", (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE}, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(tab_box, IT_targets_tab))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(IT_cont=atg_create_element_button("Continue", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(tab_box, IT_cont))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	IT_full=atg_create_element_image(fullbtn);
	if(!IT_full)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	IT_full->clickable=true;
	if(atg_ebox_pack(tab_box, IT_full))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *main_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!main_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	main_box->w=default_w;
	main_box->h=default_h-18-14;
	if(atg_ebox_pack(intel_targets_box, main_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *left_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!left_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(main_box, left_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *tllbl=atg_create_element_label("Target List", 12, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE});
	if(!tllbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(left_box, tllbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(IT_targs=calloc(ntargs, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	for(unsigned int i=0;i<ntargs;i++)
	{
		if(!(IT_targs[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE})))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		IT_targs[i]->w=160;
		IT_targs[i]->h=11;
		IT_targs[i]->clickable=true;
		if(atg_ebox_pack(left_box, IT_targs[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *shim=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE});
		if(!shim)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		shim->w=4;
		if(atg_ebox_pack(IT_targs[i], shim))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		atg_element *label=atg_create_element_label(targs[i].name, 10, (atg_colour){255, 255, 239, ATG_ALPHA_OPAQUE});
		if(!label)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		label->cache=true;
		if(atg_ebox_pack(IT_targs[i], label))
		{
			perror("atg_ebox_pack");
			return(1);
		}
	}
	atg_element *right_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR);
	if(!right_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(main_box, right_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *dm_box=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
	if(!dm_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	if(atg_ebox_pack(right_box, dm_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(IT_stat_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	IT_stat_box->w=384;
	IT_stat_box->h=256;
	if(atg_ebox_pack(dm_box, IT_stat_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	IT_map=atg_create_element_image(terrain);
	if(!IT_map)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	IT_map->h=terrain->h+2;
	if(atg_ebox_pack(dm_box, IT_map))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	atg_element *text_guard=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(!text_guard)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	text_guard->w=640;
	if(atg_ebox_pack(right_box, text_guard))
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
	if(!(IT_text_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	IT_text_box->w=636;
	if(atg_ebox_pack(text_guard, IT_text_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	return(0);
}

screen_id intel_targets_screen(atg_canvas *canvas, game *state)
{
	atg_event e;
	while(1)
	{
		update_intel_targets(state);
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
							return(intel_caller);
					}
				break;
				case ATG_EV_CLICK:;
					atg_ev_click c=e.event.click;
					if(c.e==IT_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
					}
					else if(c.e)
					{
						unsigned int i;
						for(i=0;i<ntargs;i++)
						{
							if(c.e==IT_targs[i])
								break;
						}
						if(i<ntargs)
						{
							IT_i=i;
						}
						else
						{
							fprintf(stderr, "Clicked on unknown clickable!\n");
						}
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==IT_bombers_tab)
					{
						return(SCRN_INTELBMB);
					}
					else if(trigger.e==IT_fighters_tab)
					{
						return(SCRN_INTELFTR);
					}
					else if(trigger.e==IT_targets_tab)
					{
						// do nothing
					}
					else if(trigger.e==IT_cont)
					{
						return(intel_caller);
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

void intel_targets_free(void)
{
	atg_free_element(intel_targets_box);
}

void update_intel_targets(const game *state)
{
	for(unsigned int i=0;i<ntargs;i++)
	{
		IT_targs[i]->hidden=!datewithin(state->now, targs[i].entry, targs[i].exit);
		if((IT_i<i)&&IT_targs[IT_i]->hidden&&!IT_targs[i]->hidden)
			IT_i=i;
		atg_box *tb=IT_targs[i]->elemdata;
		if(tb)
		{
			if(i==IT_i)
				tb->bgcolour=(atg_colour){127, 103, 107, ATG_ALPHA_OPAQUE};
			else
				tb->bgcolour=(atg_colour){95, 95, 103, ATG_ALPHA_OPAQUE};
		}
	}
	atg_image *mi=IT_map->elemdata;
	if(mi)
	{
		SDL_FreeSurface(mi->data);
		mi->data=SDL_ConvertSurface(terrain, terrain->format, terrain->flags);
		SDL_FreeSurface(flak_overlay);
		flak_overlay=render_flak(state->now);
		SDL_BlitSurface(flak_overlay, NULL, mi->data, NULL);
		SDL_FreeSurface(target_overlay);
		target_overlay=render_targets(state->now);
		SDL_BlitSurface(target_overlay, NULL, mi->data, NULL);
		SDL_FreeSurface(seltarg_overlay);
		seltarg_overlay=render_seltarg(IT_i);
		SDL_BlitSurface(seltarg_overlay, NULL, mi->data, NULL);
	}
	atg_box *ntb=atg_create_box(ATG_BOX_PACK_VERTICAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(ntb)
	{
		atg_free_box_box(IT_text_box->elemdata);
		IT_text_box->elemdata=ntb;
		intel *i=targs[IT_i].p_intel;
		if(i)
		{
			const char *bodytext=i->text;
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
				if(atg_pack_element(ntb, r))
				{
					perror("atg_ebox_pack");
					atg_free_element(r);
					break;
				}
				x+=l;
				if(bodytext[x]=='\n') x++;
			}
		}
	}
	atg_box *nsb=atg_create_box(ATG_BOX_PACK_VERTICAL, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE});
	if(nsb)
	{
		atg_free_box_box(IT_stat_box->elemdata);
		IT_stat_box->elemdata=nsb;
		atg_element *shim=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR);
		if(!shim)
			fprintf(stderr, "atg_create_element_box failed\n");
		shim->w=384;
		shim->h=256-14*NUM_STATS;
		if(atg_pack_element(nsb, shim))
		{
			perror("atg_ebox_pack");
			atg_free_element(shim);
		}
		for(unsigned int i=0;i<NUM_STATS;i++)
		{
			atg_element *row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE});
			if(!row)
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				break;
			}
			if(atg_pack_element(nsb, row))
			{
				perror("atg_ebox_pack");
				atg_free_element(row);
				break;
			}
			atg_element *s_name=atg_create_element_label(t_stat_rows[i].name, 12, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
			if(s_name)
			{
				s_name->w=100;
				if(atg_ebox_pack(row, s_name))
				{
					perror("atg_ebox_pack");
					atg_free_element(s_name);
					break;
				}
			}
			if(t_stat_rows[i].v_fn)
			{
				int val=t_stat_rows[i].v_fn(IT_i, state);
				if(val<0)
				{
					atg_element *na=atg_create_element_label("n/a", 12, (atg_colour){95, 95, 95, ATG_ALPHA_OPAQUE});
					if(na)
					{
						na->w=100;
						if(atg_ebox_pack(row, na))
						{
							perror("atg_ebox_pack");
							atg_free_element(na);
							break;
						}
					}
				}
				else
				{
					SDL_Surface *bar=SDL_CreateRGBSurface(SDL_HWSURFACE, 102, 14, 24, 0xff0000, 0xff00, 0xff, 0);
					if(!bar)
					{
						fprintf(stderr, "bar: SDL_CreateRGBSurface: %s\n", SDL_GetError());
						break;
					}
					SDL_FillRect(bar, &(SDL_Rect){0, 0, bar->w, bar->h}, SDL_MapRGB(bar->format, 191, 191, 191));
					SDL_FillRect(bar, &(SDL_Rect){1, 1, bar->w-2, bar->h-2}, SDL_MapRGB(bar->format, 0, 0, 23));
					int bar_x=(val-t_stat_rows[i].bar_min)*100.0/(t_stat_rows[i].bar_max-t_stat_rows[i].bar_min);
					clamp(bar_x, 0, 100);
					int dx=bar_x;
					if(t_stat_rows[i].bar_rev)
						dx=100-dx;
					unsigned int r=255-(dx*2.55), g=dx*2.55;
					SDL_FillRect(bar, &(SDL_Rect){1, 1, bar_x, bar->h-2}, SDL_MapRGB(bar->format, r, g, 0));
					atg_element *s_bar=atg_create_element_image(bar);
					SDL_FreeSurface(bar);
					if(!s_bar)
					{
						break;
					}
					else
					{
						if(atg_ebox_pack(row, s_bar))
						{
							perror("atg_ebox_pack");
							atg_free_element(s_bar);
							break;
						}
					}
				}
			}
			else if(t_stat_rows[i].t_fn)
			{
				atg_element *text=atg_create_element_label(t_stat_rows[i].t_fn(IT_i), 12, (atg_colour){0, 0, 31, ATG_ALPHA_OPAQUE});
				if(text)
				{
					text->w=284;
					if(atg_ebox_pack(row, text))
					{
						perror("atg_ebox_pack");
						atg_free_element(text);
						break;
					}
				}
			}
		}
	}
}

const char *tclass_fn(unsigned int ti)
{
	switch(targs[ti].class)
	{
		case TCLASS_CITY:
			return("City");
		case TCLASS_SHIPPING:
			return("Shipping");
		case TCLASS_MINING:
			return("Minelaying");
		case TCLASS_LEAFLET:
			return("Leafleting");
		case TCLASS_AIRFIELD:
			return("Airfield");
		case TCLASS_BRIDGE:
			return("Bridge");
		case TCLASS_ROAD:
			return("Road junction");
		case TCLASS_INDUSTRY:
			switch(targs[ti].iclass)
			{
				case ICLASS_BB:
					return("Industry - Ball Bearings");
				case ICLASS_OIL:
					return("Industry - Oil/Petrochemical");
				case ICLASS_RAIL:
					return("Industry - Rail/Transportation");
				case ICLASS_UBOOT:
					return("Industry - U-Boats/Shipyards");
				case ICLASS_ARM:
					return("Industry - Armament");
				case ICLASS_STEEL:
					return("Industry - Steel/Smelting");
				case ICLASS_AC:
					return("Industry - Aircraft");
				case ICLASS_RADAR:
					return("Industry - Radar/Electronics");
				case ICLASS_MIXED:
					return("Industry - Mixed");
				default:
					return("Industry - unknown");
			}
		default:
			return("unknown");
	}
}

int dmg_fn(unsigned int ti, const game *state)
{
	if(targs[ti].class==TCLASS_SHIPPING)
		return(-1);
	return(state->dmg[ti]);
}

int flak_fn(unsigned int ti, const game *state)
{
	return(state->flk[ti]);
}

int prod_fn(unsigned int ti, __attribute__((unused)) const game *state)
{
	return(targs[ti].prod);
}

int esiz_fn(unsigned int ti, __attribute__((unused)) const game *state)
{
	if(targs[ti].class==TCLASS_CITY)
		return(targs[ti].psiz);
	return(targs[ti].esiz);
}
