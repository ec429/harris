/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	raid_results: the raid results screen
*/

#include "raid_results.h"

#include <math.h>
#include "ui.h"
#include "globals.h"
#include "bits.h"
#include "history.h"
#include "run_raid.h"
#include "weather.h"

atg_box *raid_results_box;
atg_element **RS_typecol, *RS_tocol;
atg_element *RS_resize, *RS_full, *RS_cont;

int raid_results_create(void)
{
	raid_results_box=atg_create_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
	if(!raid_results_box)
	{
		fprintf(stderr, "atg_create_box failed\n");
		return(1);
	}
	else
	{
		atg_element *RS_trow=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!RS_trow)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		if(atg_pack_element(raid_results_box, RS_trow))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_box *b=RS_trow->elem.box;
		if(!b)
		{
			fprintf(stderr, "RS_trow->elem.box==NULL\n");
			return(1);
		}
		atg_element *RS_title=atg_create_element_label("Raid Result Statistics ", 15, (atg_colour){223, 255, 0, ATG_ALPHA_OPAQUE});
		if(!RS_title)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_pack_element(b, RS_title))
		{
			perror("atg_pack_element");
			return(1);
		}
		RS_resize=atg_create_element_image(resizebtn);
		if(!RS_resize)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		RS_resize->w=16;
		RS_resize->clickable=true;
		if(atg_pack_element(b, RS_resize))
		{
			perror("atg_pack_element");
			return(1);
		}
		RS_full=atg_create_element_image(fullbtn);
		if(!RS_full)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return(1);
		}
		RS_full->w=24;
		RS_full->clickable=true;
		if(atg_pack_element(b, RS_full))
		{
			perror("atg_pack_element");
			return(1);
		}
		RS_cont=atg_create_element_button("Continue", (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE}, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
		if(!RS_cont)
		{
			fprintf(stderr, "atg_create_element_button failed\n");
			return(1);
		}
		if(atg_pack_element(b, RS_cont))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	atg_element *RS_typerow=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 33, 33, ATG_ALPHA_OPAQUE});
	if(!RS_typerow)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	RS_typerow->h=72;
	if(atg_pack_element(raid_results_box, RS_typerow))
	{
		perror("atg_pack_element");
		return(1);
	}
	else
	{
		atg_box *b=RS_typerow->elem.box;
		if(!b)
		{
			fprintf(stderr, "RS_typerow->elem.box==NULL\n");
			return(1);
		}
		atg_element *padding=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 31, 31, ATG_ALPHA_TRANSPARENT});
		if(!padding)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		padding->h=72;
		padding->w=RS_firstcol_w;
		if(atg_pack_element(b, padding))
		{
			perror("atg_pack_element");
			return(1);
		}
		if(!(RS_typecol=calloc(ntypes, sizeof(atg_element *))))
		{
			perror("calloc");
			return(1);
		}
		for(unsigned int i=0;i<ntypes;i++)
		{
			RS_typecol[i]=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 35, 35, ATG_ALPHA_OPAQUE});
			if(!RS_typecol[i])
			{
				fprintf(stderr, "atg_create_element_box failed\n");
				return(1);
			}
			RS_typecol[i]->h=72;
			RS_typecol[i]->w=RS_cell_w;
			if(atg_pack_element(b, RS_typecol[i]))
			{
				perror("atg_pack_element");
				return(1);
			}
			atg_box *b2=RS_typecol[i]->elem.box;
			if(!b2)
			{
				fprintf(stderr, "RS_typecol[%u]->elem.box==NULL\n", i);
				return(1);
			}
			SDL_Surface *pic=SDL_CreateRGBSurface(SDL_HWSURFACE, 36, 40, types[i].picture->format->BitsPerPixel, types[i].picture->format->Rmask, types[i].picture->format->Gmask, types[i].picture->format->Bmask, types[i].picture->format->Amask);
			if(!pic)
			{
				fprintf(stderr, "pic=SDL_CreateRGBSurface: %s\n", SDL_GetError());
				return(1);
			}
			SDL_FillRect(pic, &(SDL_Rect){0, 0, pic->w, pic->h}, SDL_MapRGB(pic->format, 0, 0, 0));
			SDL_BlitSurface(types[i].picture, NULL, pic, &(SDL_Rect){(36-types[i].picture->w)>>1, (40-types[i].picture->h)>>1, 0, 0});
			atg_element *picture=atg_create_element_image(pic);
			SDL_FreeSurface(pic);
			if(!picture)
			{
				fprintf(stderr, "atg_create_element_image failed\n");
				return(1);
			}
			picture->w=38;
			if(atg_pack_element(b2, picture))
			{
				perror("atg_pack_element");
				return(1);
			}
			atg_element *manu=atg_create_element_label(types[i].manu, 10, (atg_colour){239, 239, 0, ATG_ALPHA_OPAQUE});
			if(!manu)
			{
				fprintf(stderr, "atg_create_element_label failed\n");
				return(1);
			}
			if(atg_pack_element(b2, manu))
			{
				perror("atg_pack_element");
				return(1);
			}
			atg_element *name=atg_create_element_label(types[i].name, 12, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
			if(!name)
			{
				fprintf(stderr, "atg_create_element_label failed\n");
				return(1);
			}
			if(atg_pack_element(b2, name))
			{
				perror("atg_pack_element");
				return(1);
			}
		}
		RS_tocol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){49, 37, 37, ATG_ALPHA_OPAQUE});
		if(!RS_tocol)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return(1);
		}
		RS_tocol->h=72;
		RS_tocol->w=RS_cell_w;
		if(atg_pack_element(b, RS_tocol))
		{
			perror("atg_pack_element");
			return(1);
		}
		atg_box *tb=RS_tocol->elem.box;
		if(!tb)
		{
			fprintf(stderr, "RS_tocol->elem.box==NULL\n");
			return(1);
		}
		atg_element *total=atg_create_element_label("Total", 18, (atg_colour){255, 255, 255, ATG_ALPHA_TRANSPARENT});
		if(!total)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return(1);
		}
		if(atg_pack_element(tb, total))
		{
			perror("atg_pack_element");
			return(1);
		}
	}
	return(0);
}

screen_id raid_results_screen(atg_canvas *canvas, game *state)
{
	if(totalraids)
	{
		atg_event e;
		// Raid Results table
		for(unsigned int i=2;i<raid_results_box->nelems;i++)
			atg_free_element(raid_results_box->elems[i]);
		raid_results_box->nelems=2;
		unsigned int dj[ntypes], nj[ntypes], tbj[ntypes], tlj[ntypes], tsj[ntypes], tmj[ntypes], lj[ntypes];
		unsigned int D=0, N=0, Tb=0, Tl=0, Ts=0, Tm=0, L=0;
		unsigned int scoreTb=0, scoreTl=0, scoreTm=0;
		unsigned int ntcols=0;
		for(unsigned int j=0;j<ntypes;j++)
		{
			dj[j]=nj[j]=tbj[j]=tlj[j]=tsj[j]=tmj[j]=lj[j]=0;
			unsigned int scoretbj=0, scoretlj=0, scoretmj=0;
			for(unsigned int i=0;i<ntargs;i++)
			{
				dj[j]+=dij[i][j];
				nj[j]+=nij[i][j];
				switch(targs[i].class)
				{
					case TCLASS_CITY:
					case TCLASS_AIRFIELD:
					case TCLASS_ROAD:
					case TCLASS_BRIDGE:
					case TCLASS_INDUSTRY:
						tbj[j]+=tij[i][j];
						unsigned int sf=1;
						if(targs[i].berlin) sf*=2;
						if(targs[i].class==TCLASS_INDUSTRY) sf*=2;
						if(targs[i].iclass==ICLASS_UBOOT) sf*=1.2;
						if(canscore[i]) scoretbj+=tij[i][j]*sf;
					break;
					case TCLASS_LEAFLET:
						tlj[j]+=tij[i][j];
						if(canscore[i]) scoretlj+=tij[i][j]*(targs[i].berlin?2:1);
					break;
					case TCLASS_SHIPPING:
						tsj[j]+=tij[i][j];
					break;
					case TCLASS_MINING:
						tmj[j]+=tij[i][j];
						if(canscore[i]) scoretmj+=tij[i][j];
					break;
					default: // shouldn't ever get here
						fprintf(stderr, "Bad targs[%d].class = %d\n", i, targs[i].class);
					break;
				}
				lj[j]+=lij[i][j];
			}
			D+=dj[j];
			N+=nj[j];
			Tb+=tbj[j];
			scoreTb+=scoretbj;
			Tl+=tlj[j];
			scoreTl+=scoretlj;
			Ts+=tsj[j];
			Tm+=tmj[j];
			scoreTm+=scoretmj;
			L+=lj[j];
			RS_typecol[j]->hidden=!dj[j];
			if(dj[j]) ntcols++;
		}
		state->cshr+=scoreTb*  80e-4;
		state->cshr+=scoreTl*   6e-4;
		state->cshr+=scoreTm*  12e-4;
		state->cshr+=Ts*      600   ;
		state->cshr+=bridge* 2000e-2;
		double par=0.2+((state->now.year-1939)*0.125);
		state->confid+=(N/(double)D-par)*(1.0+log2(D)/2.0)*0.6;
		state->confid+=Ts*0.15;
		state->confid+=cidam*0.08;
		state->confid=min(max(state->confid, 0), 100);
		co_append(&state->hist, state->now, (time){11, 05}, state->confid);
		state->morale+=(1.75-L*100.0/(double)D)/5.0;
		if((L==0)&&(D>5)) state->morale+=0.3;
		if(D>=100) state->morale+=0.2;
		if(D>=1000) state->morale+=1.0;
		state->morale=min(max(state->morale, 0), 100);
		mo_append(&state->hist, state->now, (time){11, 05}, state->morale);
		if(RS_tocol)
			RS_tocol->hidden=(ntcols==1);
		unsigned int ntrows=0;
		for(unsigned int i=0;i<ntargs;i++)
		{
			state->heat[i]=state->heat[i]*.8+heat[i]/(double)D;
			unsigned int di=0, ni=0, ti=0, li=0;
			for(unsigned int j=0;j<ntypes;j++)
			{
				di+=dij[i][j];
				ni+=nij[i][j];
				ti+=tij[i][j];
				li+=lij[i][j];
			}
			if(di||ni)
			{
				ntrows++;
				atg_element *row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 33, 33, ATG_ALPHA_OPAQUE});
				if(row)
				{
					row->h=RS_cell_h;
					atg_pack_element(raid_results_box, row);
					atg_box *b=row->elem.box;
					if(b)
					{
						atg_element *tncol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 35, 35, ATG_ALPHA_OPAQUE});
						if(tncol)
						{
							tncol->h=RS_cell_h;
							tncol->w=RS_firstcol_w;
							atg_pack_element(b, tncol);
							atg_box *b2=tncol->elem.box;
							if(b2&&targs[i].name)
							{
								const char *np=targs[i].name;
								char line[17]="";
								size_t x=0, y;
								while(*np)
								{
									y=strcspn(np, " ");
									if(x&&(x+y>15))
									{
										atg_element *tname=atg_create_element_label(line, 12, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
										if(tname) atg_pack_element(b2, tname);
										line[x=0]=0;
									}
									if(x)
										line[x++]=' ';
									strncpy(line+x, np, y);
									x+=y;
									np+=y;
									while(*np==' ') np++;
									line[x]=0;
								}
								if(x)
								{
									atg_element *tname=atg_create_element_label(line, 12, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
									if(tname) atg_pack_element(b2, tname);
								}
							}
						}
						for(unsigned int j=0;j<ntypes;j++)
						{
							if(!dj[j]) continue;
							atg_element *tbcol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 35, 35, ATG_ALPHA_OPAQUE});
							if(tbcol)
							{
								tbcol->h=RS_cell_h;
								tbcol->w=RS_cell_w;
								atg_pack_element(b, tbcol);
								atg_box *b2=tbcol->elem.box;
								if(b2)
								{
									char dt[20],nt[20],tt[20],lt[20];
									if(dij[i][j]||nij[i][j])
									{
										snprintf(dt, 20, "Dispatched:%u", dij[i][j]);
										atg_element *dl=atg_create_element_label(dt, 10, (atg_colour){191, 191, 0, ATG_ALPHA_OPAQUE});
										if(dl) atg_pack_element(b2, dl);
										snprintf(nt, 20, "Hit Target:%u", nij[i][j]);
										atg_element *nl=atg_create_element_label(nt, 10, (atg_colour){191, 191, 0, ATG_ALPHA_OPAQUE});
										if(nl) atg_pack_element(b2, nl);
										if(nij[i][j])
										{
											switch(targs[i].class)
											{
												case TCLASS_CITY:
												case TCLASS_AIRFIELD:
												case TCLASS_ROAD:
												case TCLASS_BRIDGE:
												case TCLASS_INDUSTRY:
													snprintf(tt, 20, "Bombs (lb):%u", tij[i][j]);
												break;
												case TCLASS_MINING:
													snprintf(tt, 20, "Mines (lb):%u", tij[i][j]);
												break;
												case TCLASS_LEAFLET:
													snprintf(tt, 20, "Leaflets  :%u", tij[i][j]);
												break;
												case TCLASS_SHIPPING:
													snprintf(tt, 20, "Ships sunk:%u", tij[i][j]);
												break;
												default: // shouldn't ever get here
													fprintf(stderr, "Bad targs[%d].class = %d\n", i, targs[i].class);
												break;
											}
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){191, 191, 0, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
										if(lij[i][j])
										{
											snprintf(lt, 20, "A/c Lost  :%u", lij[i][j]);
											atg_element *ll=atg_create_element_label(lt, 10, (atg_colour){191, 0, 0, ATG_ALPHA_OPAQUE});
											if(ll) atg_pack_element(b2, ll);
										}
									}
								}
							}
						}
						if(ntcols!=1)
						{
							atg_element *totcol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){49, 37, 37, ATG_ALPHA_OPAQUE});
							if(totcol)
							{
								totcol->h=RS_cell_h;
								totcol->w=RS_cell_w;
								atg_pack_element(b, totcol);
								atg_box *b2=totcol->elem.box;
								if(b2)
								{
									char dt[20],nt[20],tt[20],lt[20];
									if(di||ni)
									{
										snprintf(dt, 20, "Dispatched:%u", di);
										atg_element *dl=atg_create_element_label(dt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
										if(dl) atg_pack_element(b2, dl);
										snprintf(nt, 20, "Hit Target:%u", ni);
										atg_element *nl=atg_create_element_label(nt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
										if(nl) atg_pack_element(b2, nl);
										if(ni)
										{
											switch(targs[i].class)
											{
												case TCLASS_CITY:
												case TCLASS_AIRFIELD:
												case TCLASS_ROAD:
												case TCLASS_BRIDGE:
												case TCLASS_INDUSTRY:
													snprintf(tt, 20, "Bombs (lb):%u", ti);
												break;
												case TCLASS_MINING:
													snprintf(tt, 20, "Mines (lb):%u", ti);
												break;
												case TCLASS_LEAFLET:
													snprintf(tt, 20, "Leaflets  :%u", ti);
												break;
												case TCLASS_SHIPPING:
													snprintf(tt, 20, "Ships sunk:%u", ti);
												break;
												default: // shouldn't ever get here
													fprintf(stderr, "Bad targs[%d].class = %d\n", i, targs[i].class);
												break;
											}
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
										if(li)
										{
											snprintf(lt, 20, "A/c Lost  :%u", li);
											atg_element *ll=atg_create_element_label(lt, 10, (atg_colour){255, 0, 0, ATG_ALPHA_OPAQUE});
											if(ll) atg_pack_element(b2, ll);
										}
									}
								}
							}
						}
					}
				}
			}
		}
		if(ntrows!=1)
		{
			atg_element *row=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){49, 37, 37, ATG_ALPHA_OPAQUE});
			if(row)
			{
				row->h=RS_lastrow_h;
				atg_pack_element(raid_results_box, row);
				atg_box *b=row->elem.box;
				if(b)
				{
					atg_element *tncol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){51, 39, 39, ATG_ALPHA_OPAQUE});
					if(tncol)
					{
						tncol->h=RS_lastrow_h;
						tncol->w=RS_firstcol_w;
						atg_pack_element(b, tncol);
						atg_box *b2=tncol->elem.box;
						if(b2)
						{
							atg_element *tname=atg_create_element_label("Total", 18, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
							if(tname) atg_pack_element(b2, tname);
						}
					}
					for(unsigned int j=0;j<ntypes;j++)
					{
						if(!dj[j]) continue;
						atg_element *tbcol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){49, 37, 37, ATG_ALPHA_OPAQUE});
						if(tbcol)
						{
							tbcol->h=RS_lastrow_h;
							tbcol->w=RS_cell_w;
							atg_pack_element(b, tbcol);
							atg_box *b2=tbcol->elem.box;
							if(b2)
							{
								char dt[20],nt[20],tt[20],lt[20];
								if(dj[j]||nj[j])
								{
									snprintf(dt, 20, "Dispatched:%u", dj[j]);
									atg_element *dl=atg_create_element_label(dt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
									if(dl) atg_pack_element(b2, dl);
									snprintf(nt, 20, "Hit Target:%u", nj[j]);
									atg_element *nl=atg_create_element_label(nt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
									if(nl) atg_pack_element(b2, nl);
									if(nj[j])
									{
										if(tbj[j])
										{
											snprintf(tt, 20, "Bombs (lb):%u", tbj[j]);
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
										if(tmj[j])
										{
											snprintf(tt, 20, "Mines (lb):%u", tmj[j]);
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
										if(tlj[j])
										{
											snprintf(tt, 20, "Leaflets  :%u", tlj[j]);
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
										if(tsj[j])
										{
											snprintf(tt, 20, "Ships sunk:%u", tsj[j]);
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
									}
									if(lj[j])
									{
										snprintf(lt, 20, "A/c Lost  :%u", lj[j]);
										atg_element *ll=atg_create_element_label(lt, 10, (atg_colour){255, 0, 0, ATG_ALPHA_OPAQUE});
										if(ll) atg_pack_element(b2, ll);
									}
								}
							}
						}
					}
					if(ntcols!=1)
					{
						atg_element *totcol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){51, 39, 39, ATG_ALPHA_OPAQUE});
						if(totcol)
						{
							totcol->h=RS_lastrow_h;
							totcol->w=RS_cell_w;
							atg_pack_element(b, totcol);
							atg_box *b2=totcol->elem.box;
							if(b2)
							{
								char dt[20],nt[20],tt[20],lt[20];
								if(D||N)
								{
									snprintf(dt, 20, "Dispatched:%u", D);
									atg_element *dl=atg_create_element_label(dt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
									if(dl) atg_pack_element(b2, dl);
									snprintf(nt, 20, "Hit Target:%u", N);
									atg_element *nl=atg_create_element_label(nt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
									if(nl) atg_pack_element(b2, nl);
									if(N)
									{
										if(Tb)
										{
											snprintf(tt, 20, "Bombs (lb):%u", Tb);
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
										if(Tm)
										{
											snprintf(tt, 20, "Mines (lb):%u", Tm);
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
										if(Tl)
										{
											snprintf(tt, 20, "Leaflets  :%u", Tl);
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
										if(Ts)
										{
											snprintf(tt, 20, "Ships sunk:%u", Ts);
											atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
											if(tl) atg_pack_element(b2, tl);
										}
									}
									if(L)
									{
										snprintf(lt, 20, "A/c Lost  :%u", L);
										atg_element *ll=atg_create_element_label(lt, 10, (atg_colour){255, 0, 0, ATG_ALPHA_OPAQUE});
										if(ll) atg_pack_element(b2, ll);
									}
								}
							}
						}
					}
				}
			}
		}
		atg_flip(canvas);
		int errupt=0;
		while(!errupt)
		{
			while(atg_poll_event(&e, canvas))
			{
				switch(e.type)
				{
					case ATG_EV_RAW:;
						SDL_Event s=e.event.raw;
						switch(s.type)
						{
							case SDL_QUIT:
								errupt++;
							break;
							case SDL_KEYDOWN:
								switch(s.key.keysym.sym)
								{
									case SDLK_SPACE:
										errupt++;
									break;
									default:
									break;
								}
							break;
						}
					break;
					case ATG_EV_CLICK:;
						atg_ev_click c=e.event.click;
						if(c.e==RS_resize)
						{
							mainsizex=default_w;
							mainsizey=default_h;
							atg_resize_canvas(canvas, mainsizex, mainsizey);
							atg_flip(canvas);
						}
						if(c.e==RS_full)
						{
							fullscreen=!fullscreen;
							atg_setopts_canvas(canvas, fullscreen?SDL_FULLSCREEN:SDL_RESIZABLE);
							atg_flip(canvas);
						}
					break;
					case ATG_EV_TRIGGER:;
						atg_ev_trigger t=e.event.trigger;
						if(t.e==RS_cont)
							errupt++;
						else
							fprintf(stderr, "Clicked unknown button %p\n", (void *)t.e);
					break;
					default:
					break;
				}
			}
			SDL_Delay(50);
		}
	}
	else
	{
		#if !NOWEATHER
		for(unsigned int it=0;it<512;it++)
			w_iter(&state->weather, lorw);
		#endif
	}
	return(SCRN_POSTRAID);
}

void raid_results_free(void)
{
	atg_free_box_box(raid_results_box);
}
