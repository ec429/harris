/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	raid_results: the raid results screen
*/

#include "raid_results.h"

#include <math.h>
#include "ui.h"
#include "globals.h"
#include "bits.h"
#include "date.h"
#include "history.h"
#include "run_raid.h"
#include "weather.h"

atg_element *raid_results_box;
atg_element *RS_resize, *RS_full, *RS_cont;

int raid_results_create(void)
{
	raid_results_box=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
	if(!raid_results_box)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return(1);
	}
	return(0);
}

static void create_trow(void)
{
	atg_element *RS_trow=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
	if(!RS_trow)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return;
	}
	if(atg_ebox_pack(raid_results_box, RS_trow))
	{
		perror("atg_ebox_pack");
		atg_free_element(RS_trow);
		return;
	}
	atg_element *RS_title=atg_create_element_label("Raid Result Statistics ", 15, (atg_colour){223, 255, 0, ATG_ALPHA_OPAQUE});
	if(!RS_title)
	{
		fprintf(stderr, "atg_create_element_label failed\n");
		return;
	}
	if(atg_ebox_pack(RS_trow, RS_title))
	{
		perror("atg_ebox_pack");
		atg_free_element(RS_title);
		return;
	}
	RS_resize=atg_create_element_image(resizebtn);
	if(!RS_resize)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return;
	}
	RS_resize->w=16;
	RS_resize->clickable=true;
	if(atg_ebox_pack(RS_trow, RS_resize))
	{
		perror("atg_ebox_pack");
		atg_free_element(RS_resize);
		RS_resize=NULL;
		return;
	}
	RS_full=atg_create_element_image(fullbtn);
	if(!RS_full)
	{
		fprintf(stderr, "atg_create_element_image failed\n");
		return;
	}
	RS_full->w=24;
	RS_full->clickable=true;
	if(atg_ebox_pack(RS_trow, RS_full))
	{
		perror("atg_ebox_pack");
		atg_free_element(RS_full);
		RS_full=NULL;
		return;
	}
	RS_cont=atg_create_element_button("Continue", (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE}, (atg_colour){47, 31, 31, ATG_ALPHA_OPAQUE});
	if(!RS_cont)
	{
		fprintf(stderr, "atg_create_element_button failed\n");
		return;
	}
	if(atg_ebox_pack(RS_trow, RS_cont))
	{
		perror("atg_ebox_pack");
		atg_free_element(RS_cont);
		RS_cont=NULL;
		return;
	}
}

static void create_typerow(unsigned int *dj)
{
	atg_element *RS_typerow=atg_create_element_box(ATG_BOX_PACK_HORIZONTAL, (atg_colour){47, 33, 33, ATG_ALPHA_OPAQUE});
	if(!RS_typerow)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return;
	}
	RS_typerow->h=72;
	if(atg_ebox_pack(raid_results_box, RS_typerow))
	{
		perror("atg_ebox_pack");
		atg_free_element(RS_typerow);
		return;
	}
	atg_element *padding=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 31, 31, ATG_ALPHA_TRANSPARENT});
	if(!padding)
	{
		fprintf(stderr, "atg_create_element_box failed\n");
		return;
	}
	padding->h=72;
	padding->w=RS_firstcol_w;
	if(atg_ebox_pack(RS_typerow, padding))
	{
		perror("atg_ebox_pack");
		atg_free_element(padding);
		return;
	}
	unsigned int ntcols=0;
	for(unsigned int i=0;i<ntypes;i++)
	{
		if(!dj[i]) continue;
		atg_element *RS_typecol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 35, 35, ATG_ALPHA_OPAQUE});
		if(!RS_typecol)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return;
		}
		RS_typecol->h=72;
		RS_typecol->w=RS_cell_w;
		if(atg_ebox_pack(RS_typerow, RS_typecol))
		{
			perror("atg_ebox_pack");
			atg_free_element(RS_typecol);
			return;
		}
		atg_element *picture=atg_create_element_image(types[i].picture);
		if(!picture)
		{
			fprintf(stderr, "atg_create_element_image failed\n");
			return;
		}
		picture->w=38;
		if(atg_ebox_pack(RS_typecol, picture))
		{
			perror("atg_ebox_pack");
			atg_free_element(picture);
			return;
		}
		atg_element *manu=atg_create_element_label(types[i].manu, 10, (atg_colour){239, 239, 0, ATG_ALPHA_OPAQUE});
		if(!manu)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return;
		}
		if(atg_ebox_pack(RS_typecol, manu))
		{
			perror("atg_ebox_pack");
			atg_free_element(manu);
			return;
		}
		atg_element *name=atg_create_element_label(types[i].name, 12, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
		if(!name)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return;
		}
		if(atg_ebox_pack(RS_typecol, name))
		{
			perror("atg_ebox_pack");
			atg_free_element(name);
			return;
		}
		ntcols++;
	}
	if(ntcols!=1)
	{
		atg_element *RS_tocol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){49, 37, 37, ATG_ALPHA_OPAQUE});
		if(!RS_tocol)
		{
			fprintf(stderr, "atg_create_element_box failed\n");
			return;
		}
		RS_tocol->h=72;
		RS_tocol->w=RS_cell_w;
		if(atg_ebox_pack(RS_typerow, RS_tocol))
		{
			perror("atg_ebox_pack");
			atg_free_element(RS_tocol);
			return;
		}
		atg_element *total=atg_create_element_label("Total", 18, (atg_colour){255, 255, 255, ATG_ALPHA_TRANSPARENT});
		if(!total)
		{
			fprintf(stderr, "atg_create_element_label failed\n");
			return;
		}
		if(atg_ebox_pack(RS_tocol, total))
		{
			perror("atg_ebox_pack");
			atg_free_element(total);
			return;
		}
	}
}

screen_id raid_results_screen(atg_canvas *canvas, game *state)
{
	if(totalraids)
	{
		atg_event e;
		// Raid Results table
		unsigned int dj[ntypes], nj[ntypes], tbj[ntypes], tlj[ntypes], tsj[ntypes], tmj[ntypes], lj[ntypes];
		unsigned int D=0, N=0, Tb=0, Tl=0, Ts=0, Tm=0, L=0;
		double confD=0, confN=0;
		unsigned int scoreTb=0, scoreTl=0, scoreTm=0;
		double scoreTs=0;
		unsigned int ntcols=0;
		bool berlin=!datebefore(state->now, event[EVENT_LONDON]);
		for(unsigned int j=0;j<ntypes;j++)
		{
			dj[j]=nj[j]=tbj[j]=tlj[j]=tsj[j]=tmj[j]=lj[j]=0;
			unsigned int scoretbj=0, scoretlj=0, scoretmj=0;
			for(unsigned int i=0;i<ntargs;i++)
			{
				dj[j]+=dij[i][j];
				nj[j]+=nij[i][j];
				double sf=1;
				if(targs[i].class==state->tfav[0])
					sf*=1.2;
				if(targs[i].class==state->tfav[1])
					sf*=0.8;
				if(targs[i].iclass==state->ifav[0])
					sf*=1.2;
				if(targs[i].iclass==state->ifav[1])
					sf*=0.8;
				if(targs[i].berlin&&(berlin||targs[i].class==TCLASS_LEAFLET)) sf*=2;
				if(targs[i].iclass==ICLASS_UBOOT) sf*=1.2;
				confD+=dij[i][j]*sf;
				confN+=nij[i][j]*sf;
				switch(targs[i].class)
				{
					case TCLASS_AIRFIELD:
					case TCLASS_ROAD:
					case TCLASS_BRIDGE:
						sf*=1.5;
						// fall through
					case TCLASS_INDUSTRY:
						sf*=2;
						// fall through
					case TCLASS_CITY:
						tbj[j]+=tij[i][j];
						if(canscore[i]) scoretbj+=tij[i][j]*sf;
					break;
					case TCLASS_LEAFLET:
						tlj[j]+=tij[i][j];
						if(canscore[i]) scoretlj+=tij[i][j]*sf;
					break;
					case TCLASS_SHIPPING:
						tsj[j]+=tij[i][j];
						scoreTs+=tij[i][j]*sf;
					break;
					case TCLASS_MINING:
						tmj[j]+=tij[i][j];
						if(canscore[i]) scoretmj+=tij[i][j]*sf;
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
			if(dj[j]) ntcols++;
		}
		atg_ebox_empty(raid_results_box);
		create_trow();
		create_typerow(dj);
		double bsf=1;
		if(state->tfav[0]==TCLASS_BRIDGE)
			bsf*=1.2;
		if(state->tfav[1]==TCLASS_BRIDGE)
			bsf*=0.8;
		state->cshr+=scoreTb*  80e-4;
		state->cshr+=scoreTl*   6e-4;
		state->cshr+=scoreTm*  12e-4;
		state->cshr+=scoreTs* 600;
		state->cshr+=bridge* 2000e-2*bsf;
		double par=0.2+((state->now.year-1939)*0.12);
		state->confid+=(confN/(double)D-par)*(1.0+log2(confD)/2.0)*0.8;
		state->confid+=Ts*0.2;
		state->confid+=cidam*0.1;
		state->confid=min(max(state->confid, 0), 100);
		state->morale+=(1.75-L*100.0/(double)D)/5.0;
		if((L==0)&&(D>15)) state->morale+=0.3;
		if(D>=100) state->morale+=0.2;
		if(D>=1000) state->morale+=1.0;
		state->morale=min(max(state->morale, 0), 100);
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
					atg_ebox_pack(raid_results_box, row);
					atg_element *tncol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){47, 35, 35, ATG_ALPHA_OPAQUE});
					if(tncol)
					{
						tncol->h=RS_cell_h;
						tncol->w=RS_firstcol_w;
						atg_ebox_pack(row, tncol);
						if(targs[i].name)
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
									if(tname) atg_ebox_pack(tncol, tname);
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
								if(tname) atg_ebox_pack(tncol, tname);
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
							atg_ebox_pack(row, tbcol);
							char dt[20],nt[20],tt[20],lt[20];
							if(dij[i][j]||nij[i][j])
							{
								snprintf(dt, 20, "Dispatched:%u", dij[i][j]);
								atg_element *dl=atg_create_element_label(dt, 10, (atg_colour){191, 191, 0, ATG_ALPHA_OPAQUE});
								if(dl) atg_ebox_pack(tbcol, dl);
								snprintf(nt, 20, "Hit Target:%u", nij[i][j]);
								atg_element *nl=atg_create_element_label(nt, 10, (atg_colour){191, 191, 0, ATG_ALPHA_OPAQUE});
								if(nl) atg_ebox_pack(tbcol, nl);
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
									if(tl) atg_ebox_pack(tbcol, tl);
								}
								if(lij[i][j])
								{
									snprintf(lt, 20, "A/c Lost  :%u", lij[i][j]);
									atg_element *ll=atg_create_element_label(lt, 10, (atg_colour){191, 0, 0, ATG_ALPHA_OPAQUE});
									if(ll) atg_ebox_pack(tbcol, ll);
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
							atg_ebox_pack(row, totcol);
							char dt[20],nt[20],tt[20],lt[20];
							if(di||ni)
							{
								snprintf(dt, 20, "Dispatched:%u", di);
								atg_element *dl=atg_create_element_label(dt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
								if(dl) atg_ebox_pack(totcol, dl);
								snprintf(nt, 20, "Hit Target:%u", ni);
								atg_element *nl=atg_create_element_label(nt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
								if(nl) atg_ebox_pack(totcol, nl);
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
									if(tl) atg_ebox_pack(totcol, tl);
								}
								if(li)
								{
									snprintf(lt, 20, "A/c Lost  :%u", li);
									atg_element *ll=atg_create_element_label(lt, 10, (atg_colour){255, 0, 0, ATG_ALPHA_OPAQUE});
									if(ll) atg_ebox_pack(totcol, ll);
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
				atg_ebox_pack(raid_results_box, row);
				atg_element *tncol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){51, 39, 39, ATG_ALPHA_OPAQUE});
				if(tncol)
				{
					tncol->h=RS_lastrow_h;
					tncol->w=RS_firstcol_w;
					atg_ebox_pack(row, tncol);
					atg_element *tname=atg_create_element_label("Total", 18, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
					if(tname) atg_ebox_pack(tncol, tname);
				}
				for(unsigned int j=0;j<ntypes;j++)
				{
					if(!dj[j]) continue;
					atg_element *tbcol=atg_create_element_box(ATG_BOX_PACK_VERTICAL, (atg_colour){49, 37, 37, ATG_ALPHA_OPAQUE});
					if(tbcol)
					{
						tbcol->h=RS_lastrow_h;
						tbcol->w=RS_cell_w;
						atg_ebox_pack(row, tbcol);
						char dt[20],nt[20],tt[20],lt[20];
						if(dj[j]||nj[j])
						{
							snprintf(dt, 20, "Dispatched:%u", dj[j]);
							atg_element *dl=atg_create_element_label(dt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
							if(dl) atg_ebox_pack(tbcol, dl);
							snprintf(nt, 20, "Hit Target:%u", nj[j]);
							atg_element *nl=atg_create_element_label(nt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
							if(nl) atg_ebox_pack(tbcol, nl);
							if(nj[j])
							{
								if(tbj[j])
								{
									snprintf(tt, 20, "Bombs (lb):%u", tbj[j]);
									atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
									if(tl) atg_ebox_pack(tbcol, tl);
								}
								if(tmj[j])
								{
									snprintf(tt, 20, "Mines (lb):%u", tmj[j]);
									atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
									if(tl) atg_ebox_pack(tbcol, tl);
								}
								if(tlj[j])
								{
									snprintf(tt, 20, "Leaflets  :%u", tlj[j]);
									atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
									if(tl) atg_ebox_pack(tbcol, tl);
								}
								if(tsj[j])
								{
									snprintf(tt, 20, "Ships sunk:%u", tsj[j]);
									atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 0, ATG_ALPHA_OPAQUE});
									if(tl) atg_ebox_pack(tbcol, tl);
								}
							}
							if(lj[j])
							{
								snprintf(lt, 20, "A/c Lost  :%u", lj[j]);
								atg_element *ll=atg_create_element_label(lt, 10, (atg_colour){255, 0, 0, ATG_ALPHA_OPAQUE});
								if(ll) atg_ebox_pack(tbcol, ll);
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
						atg_ebox_pack(row, totcol);
						char dt[20],nt[20],tt[20],lt[20];
						if(D||N)
						{
							snprintf(dt, 20, "Dispatched:%u", D);
							atg_element *dl=atg_create_element_label(dt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
							if(dl) atg_ebox_pack(totcol, dl);
							snprintf(nt, 20, "Hit Target:%u", N);
							atg_element *nl=atg_create_element_label(nt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
							if(nl) atg_ebox_pack(totcol, nl);
							if(N)
							{
								if(Tb)
								{
									snprintf(tt, 20, "Bombs (lb):%u", Tb);
									atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
									if(tl) atg_ebox_pack(totcol, tl);
								}
								if(Tm)
								{
									snprintf(tt, 20, "Mines (lb):%u", Tm);
									atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
									if(tl) atg_ebox_pack(totcol, tl);
								}
								if(Tl)
								{
									snprintf(tt, 20, "Leaflets  :%u", Tl);
									atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
									if(tl) atg_ebox_pack(totcol, tl);
								}
								if(Ts)
								{
									snprintf(tt, 20, "Ships sunk:%u", Ts);
									atg_element *tl=atg_create_element_label(tt, 10, (atg_colour){255, 255, 255, ATG_ALPHA_OPAQUE});
									if(tl) atg_ebox_pack(totcol, tl);
								}
							}
							if(L)
							{
								snprintf(lt, 20, "A/c Lost  :%u", L);
								atg_element *ll=atg_create_element_label(lt, 10, (atg_colour){255, 0, 0, ATG_ALPHA_OPAQUE});
								if(ll) atg_ebox_pack(totcol, ll);
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
	atg_free_element(raid_results_box);
}
