/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	intel_fighters: fighters' intel screen
*/

#include "intel_fighters.h"
#include <stdbool.h>
#include "ui.h"
#include "globals.h"
#include "bits.h"
#include "date.h"

atg_element *intel_fighters_box;
atg_element *IF_full, *IF_bombers_tab, *IF_fighters_tab, *IF_targets_tab, *IF_cont;
atg_element **IF_types, **IF_namebox, *IF_side_image, *IF_text_box, *IF_stat_box;
unsigned int IF_i;
SDL_Surface *IF_blank;

void update_intel_fighters(const game *state);

enum f_stat_i
{
	STAT_TYPE,
	/* Counts */
	STAT_OPER,
	STAT_RADAR,
	/* Numbers with units */
	STAT_COST,
	STAT_SPEED,
	/* Woolly gameplay numbers */
	STAT_ARM,
	STAT_MNV,

	NUM_STATS
};

const char *type_fn(unsigned int ti);
int oper_fn(unsigned int ti, const game *state);
int radar_fn(unsigned int ti, const game *state);
int cost_fn(unsigned int ti, const game *state);
int speed_fn(unsigned int ti, const game *state);
int arm_fn(unsigned int ti, const game *state);
int mnv_fn(unsigned int ti, const game *state);

struct f_stat_row
{
	const char *name;
	char value_buf[6];
	const char *unit;
	bool unit_first;
	unsigned int bar_min, bar_max;
	bool bar_rev; // reverse colours
	int (*v_fn)(unsigned int ti, const game *state);
	const char *(*t_fn)(unsigned int ti);
}
f_stat_rows[NUM_STATS]=
{
	[STAT_TYPE] ={.name="Type",               .unit=" ",   .unit_first=false, .bar_min=0,   .bar_max=0,     .t_fn=type_fn,  .bar_rev=false},
	[STAT_OPER] ={.name="Number Operational", .unit=" ",   .unit_first=false, .bar_min=0,   .bar_max=0,     .v_fn=oper_fn,  .bar_rev=false},
	[STAT_RADAR]={.name="Number with Radar",  .unit=" ",   .unit_first=false, .bar_min=0,   .bar_max=0,     .v_fn=radar_fn, .bar_rev=false},
	[STAT_COST] ={.name="Cost",               .unit="RM",  .unit_first=false, .bar_min=0,   .bar_max=20000, .v_fn=cost_fn,  .bar_rev=true },
	[STAT_SPEED]={.name="Speed",              .unit="mph", .unit_first=false, .bar_min=160, .bar_max=400,   .v_fn=speed_fn, .bar_rev=false},
	[STAT_ARM]  ={.name="Armament",           .unit=" ",   .unit_first=false, .bar_min=0,   .bar_max=100,   .v_fn=arm_fn,   .bar_rev=false},
	[STAT_MNV]  ={.name="Maneuvrability",     .unit=" ",   .unit_first=false, .bar_min=0,   .bar_max=100,   .v_fn=mnv_fn,   .bar_rev=false},
};

int intel_fighters_create(void)
{
	if(!(intel_fighters_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, GAME_BG_COLOUR)))
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
	if(atg_ebox_pack(intel_fighters_box, title))
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
	if(atg_ebox_pack(intel_fighters_box, tab_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(IF_bombers_tab=atg_create_element_button("Bombers", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(tab_box, IF_bombers_tab))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(IF_fighters_tab=atg_create_element_button("Fighters", (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE}, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(tab_box, IF_fighters_tab))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(IF_targets_tab=atg_create_element_button("Targets", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){47, 47, 47, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(tab_box, IF_targets_tab))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(IF_cont=atg_create_element_button("Continue", (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE}, (atg_colour){63, 63, 63, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return(1);
	}
	if(atg_ebox_pack(tab_box, IF_cont))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	IF_full=atg_create_element_image(fullbtn);
	if(!IF_full)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	IF_full->clickable=true;
	if(atg_ebox_pack(tab_box, IF_full))
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
	if(atg_ebox_pack(intel_fighters_box, main_box))
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
	atg_element *ftlbl=atg_create_element_label("Fighter Types", 12, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE});
	if(!ftlbl)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return(1);
	}
	if(atg_ebox_pack(left_box, ftlbl))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(IF_types=calloc(nftypes, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	if(!(IF_namebox=calloc(nftypes, sizeof(atg_element *))))
	{
		perror("calloc");
		return(1);
	}
	for(unsigned int i=0;i<nftypes;i++)
	{
		if(!(IF_types[i]=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, GAME_BG_COLOUR)))
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		IF_types[i]->w=160;
		IF_types[i]->clickable=true;
		if(atg_ebox_pack(left_box, IF_types[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		SDL_Surface *pic=SDL_CreateRGBSurface(SDL_HWSURFACE, 36, 40, ftypes[i].picture->format->BitsPerPixel, ftypes[i].picture->format->Rmask, ftypes[i].picture->format->Gmask, ftypes[i].picture->format->Bmask, ftypes[i].picture->format->Amask);
		if(!pic)
		{
			fprintf(stderr, "pic=SDL_CreateRGBSurface: %s\n", SDL_GetError());
			return(1);
		}
		SDL_FillRect(pic, &(SDL_Rect){0, 0, pic->w, pic->h}, SDL_MapRGB(pic->format, 0, 0, 0));
		SDL_BlitSurface(ftypes[i].picture, NULL, pic, &(SDL_Rect){(36-ftypes[i].picture->w)>>1, (40-ftypes[i].picture->h)>>1, 0, 0});
		atg_element *fpic=atg_create_element_image(pic);
		SDL_FreeSurface(pic);
		if(!fpic)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		fpic->w=38;
		fpic->h=42;
		fpic->cache=true;
		if(atg_ebox_pack(IF_types[i], fpic))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		IF_namebox[i]=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){31, 31, 31, ATG_ALPHA_OPAQUE});
		if(!IF_namebox[i])
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		IF_namebox[i]->w=122;
		IF_namebox[i]->h=42;
		if(atg_ebox_pack(IF_types[i], IF_namebox[i]))
		{
			perror("atg_ebox_pack");
			return(1);
		}
		if(ftypes[i].manu&&ftypes[i].name)
		{
			atg_element *manu=atg_create_element_label(ftypes[i].manu, 10, (atg_colour){175, 199, 255, ATG_ALPHA_OPAQUE});
			if(!manu)
			{
				fprintf(stderr, "atg_create_element_label failed\n");
				return(1);
			}
			if(atg_ebox_pack(IF_namebox[i], manu))
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
			if(atg_ebox_pack(IF_namebox[i], name))
			{
				perror("atg_ebox_pack");
				return(1);
			}
		}
		else
		{
			fprintf(stderr, "Missing manu or name in ftype %u\n", i);
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
	if(!(IF_blank=SDL_CreateRGBSurface(SDL_HWSURFACE, 414, 150, 24, 0xff0000, 0xff00, 0xff, 0)))
	{
		fprintf(stderr, "IF_blank: SDL_CreateRGBSurface: %s\n", SDL_GetError());
		return(1);
	}
	SDL_FillRect(IF_blank, &(SDL_Rect){0, 0, IF_blank->w, IF_blank->h}, SDL_MapRGB(IF_blank->format, 0, 0, 23));
	IF_side_image=atg_create_element_image(IF_blank);
	if(!IF_side_image)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return(1);
	}
	IF_side_image->w=414;
	IF_side_image->h=150;
	if(atg_ebox_pack(right_box, IF_side_image))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	if(!(IF_stat_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	IF_stat_box->w=414;
	if(atg_ebox_pack(right_box, IF_stat_box))
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
	text_guard->w=414;
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
	if(!(IF_text_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE})))
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	IF_text_box->w=410;
	if(atg_ebox_pack(text_guard, IF_text_box))
	{
		perror("atg_ebox_pack");
		return(1);
	}
	return(0);
}

screen_id intel_fighters_screen(atg_canvas *canvas, game *state)
{
	atg_event e;
	while(1)
	{
		update_intel_fighters(state);
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
					if(c.e==IF_full)
					{
						fullscreen=!fullscreen;
						atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
					}
					else if(c.e)
					{
						unsigned int i;
						for(i=0;i<nftypes;i++)
						{
							if(c.e==IF_types[i])
								break;
						}
						if(i<ntypes)
						{
							IF_i=i;
						}
						else
						{
							fprintf(stderr, "Clicked on unknown clickable!\n");
						}
					}
				break;
				case ATG_EV_TRIGGER:;
					atg_ev_trigger trigger=e.event.trigger;
					if(trigger.e==IF_bombers_tab)
					{
						return(SCRN_INTELBMB);
					}
					else if(trigger.e==IF_fighters_tab)
					{
						// do nothing
					}
					else if(trigger.e==IF_targets_tab)
					{
						return(SCRN_INTELTRG);
					}
					else if(trigger.e==IF_cont)
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

void intel_fighters_free(void)
{
	atg_free_element(intel_fighters_box);
}

void update_intel_fighters(const game *state)
{
	for(unsigned int i=0;i<nftypes;i++)
	{
		IF_types[i]->hidden=datebefore(state->now, ftypes[i].entry);
		atg_box *nb=IF_namebox[i]->elemdata;
		if(nb)
		{
			if(i==IF_i)
				nb->bgcolour=(atg_colour){47, 47, 63, ATG_ALPHA_OPAQUE};
			else if(!datebefore(state->now, ftypes[i].exit))
				nb->bgcolour=(atg_colour){15, 15, 63, ATG_ALPHA_OPAQUE};
			else
				nb->bgcolour=(atg_colour){31, 31, 31, ATG_ALPHA_OPAQUE};
		}
	}
	atg_image *si=IF_side_image->elemdata;
	if(si)
	{
		if(ftypes[IF_i].side_image)
			si->data=ftypes[IF_i].side_image;
		else
			si->data=IF_blank;
	}
	atg_box *ntb=atg_create_box(ATG_BOX_PACK_VERTICAL, (atg_colour){239, 239, 239, ATG_ALPHA_OPAQUE});
	if(ntb)
	{
		atg_free_box_box(IF_text_box->elemdata);
		IF_text_box->elemdata=ntb;
		const char *bodytext=ftypes[IF_i].text;
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
	atg_box *nsb=atg_create_box(ATG_BOX_PACK_VERTICAL, (atg_colour){223, 223, 223, ATG_ALPHA_OPAQUE});
	if(nsb)
	{
		atg_free_box_box(IF_stat_box->elemdata);
		IF_stat_box->elemdata=nsb;
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
			atg_element *s_name=atg_create_element_label(f_stat_rows[i].name, 12, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
			if(s_name)
			{
				s_name->w=120+(f_stat_rows[i].unit_first?0:8);
				if(atg_ebox_pack(row, s_name))
				{
					perror("atg_ebox_pack");
					atg_free_element(s_name);
					break;
				}
			}
			if(f_stat_rows[i].t_fn)
			{
				atg_element *text=atg_create_element_label(f_stat_rows[i].t_fn(IF_i), 12, (atg_colour){0, 0, 31, ATG_ALPHA_OPAQUE});
				if(text)
				{
					if(atg_ebox_pack(row, text))
					{
						perror("atg_ebox_pack");
						atg_free_element(text);
						break;
					}
				}
			}
			else if(f_stat_rows[i].v_fn)
			{
				int val=f_stat_rows[i].v_fn(IF_i, state);
				if(val<0)
				{
					snprintf(f_stat_rows[i].value_buf, 6, "  n/a");
					atg_element *s_value=atg_create_element_label(f_stat_rows[i].value_buf, 12, (atg_colour){95, 95, 95, ATG_ALPHA_OPAQUE});
					if(s_value)
					{
						s_value->w=72;
						if(atg_ebox_pack(row, s_value))
						{
							perror("atg_ebox_pack");
							atg_free_element(s_value);
							break;
						}
					}
				}
				else
				{
					if(f_stat_rows[i].unit_first)
					{
						atg_element *s_unit=atg_create_element_label(f_stat_rows[i].unit, 12, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
						if(s_unit)
						{
							s_unit->w=8;
							if(atg_ebox_pack(row, s_unit))
							{
								perror("atg_ebox_pack");
								atg_free_element(s_unit);
								break;
							}
						}
					}
					snprintf(f_stat_rows[i].value_buf, 6, "%5d", val);
					atg_element *s_value=atg_create_element_label(f_stat_rows[i].value_buf, 12, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
					if(s_value)
					{
						s_value->w=40+(f_stat_rows[i].unit_first?32:0);
						if(atg_ebox_pack(row, s_value))
						{
							perror("atg_ebox_pack");
							atg_free_element(s_value);
							break;
						}
					}
					if(!f_stat_rows[i].unit_first)
					{
						atg_element *s_unit=atg_create_element_label(f_stat_rows[i].unit, 12, (atg_colour){0, 0, 0, ATG_ALPHA_OPAQUE});
						if(s_unit)
						{
							s_unit->w=32;
							if(atg_ebox_pack(row, s_unit))
							{
								perror("atg_ebox_pack");
								atg_free_element(s_unit);
								break;
							}
						}
					}
				}
				SDL_Surface *bar=SDL_CreateRGBSurface(SDL_HWSURFACE, 102, 14, 24, 0xff0000, 0xff00, 0xff, 0);
				if(!bar)
				{
					fprintf(stderr, "bar: SDL_CreateRGBSurface: %s\n", SDL_GetError());
					break;
				}
				SDL_FillRect(bar, &(SDL_Rect){0, 0, bar->w, bar->h}, SDL_MapRGB(bar->format, 191, 191, 191));
				if(f_stat_rows[i].bar_min!=f_stat_rows[i].bar_max)
				{
					SDL_FillRect(bar, &(SDL_Rect){1, 1, bar->w-2, bar->h-2}, SDL_MapRGB(bar->format, 0, 0, 23));
					int bar_x=(val-f_stat_rows[i].bar_min)*100.0/(f_stat_rows[i].bar_max-f_stat_rows[i].bar_min);
					clamp(bar_x, 0, 100);
					int dx=bar_x;
					if(f_stat_rows[i].bar_rev)
						dx=100-dx;
					unsigned int r=255-(dx*2.55), g=dx*2.55;
					SDL_FillRect(bar, &(SDL_Rect){1, 1, bar_x, bar->h-2}, SDL_MapRGB(bar->format, r, g, 0));
				}
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
	}
}


const char *type_fn(unsigned int ti)
{
	return(ftypes[ti].night?"Night":"  Day");
}

int oper_fn(unsigned int ti, const game *state)
{
	int rv=0;
	for(unsigned int i=0;i<state->nfighters;i++)
		if(state->fighters[i].type==ti)
			rv++;
	return(rv);
}

int radar_fn(unsigned int ti, const game *state)
{
	if(!ftypes[ti].radpri)
		return(-1);
	int rv=0;
	for(unsigned int i=0;i<state->nfighters;i++)
		if(state->fighters[i].radar&&state->fighters[i].type==ti)
			rv++;
	return(rv);
}

int cost_fn(unsigned int ti, __attribute__((unused)) const game *state)
{
	return(ftypes[ti].cost);
}

int speed_fn(unsigned int ti, __attribute__((unused)) const game *state)
{
	return(ftypes[ti].speed);
}

int arm_fn(unsigned int ti, __attribute__((unused)) const game *state)
{
	return(ftypes[ti].arm);
}

int mnv_fn(unsigned int ti, __attribute__((unused)) const game *state)
{
	return(ftypes[ti].mnv);
}
