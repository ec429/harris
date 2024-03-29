/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	post_raid: post-raid processing (pretends to be a screen, but isn't really)
*/

#include "post_raid.h"

#include <math.h>
#include "ui.h"
#include "globals.h"
#include "bits.h"
#include "date.h"
#include "history.h"
#include "mods.h"
#include "rand.h"
#include "control.h"
#include "post_raid.h"
#include "handle_squadrons.h"

void force_tprio(game *state, enum t_class cls, unsigned int days);
void force_iprio(game *state, enum i_class cls, unsigned int days);
void produce(int targ, game *state, double amount);
void train_students(game *state);

atg_element *post_raid_box;

int post_raid_create(void)
{
	// nothing to do
	return(0);
}

#ifdef PARANOID_NB_CHECKS
static bool valid=true;

static void validate_nb(const char *prefix, const game *state)
{
	if(!valid) return;
	unsigned int nb[state->nsquads][3];
	memset(nb, 0, sizeof(nb));
	for(unsigned int i=0;i<state->nbombers;i++)
	{
		int s=state->bombers[i].squadron, f=state->bombers[i].flight;
		if(s<0) continue;
		if(f<0)
		{
			fprintf(stderr, "%s: Warning, bomber %u has flt %d but no sqn\n", prefix, i, f);
			continue;
		}
		nb[s][f]++;
	}
	for(unsigned int s=0;s<state->nsquads;s++)
	{
		for(unsigned int f=0;f<3;f++)
		{
			if(state->squads[s].nb[f]!=nb[s][f] || nb[s][f]>10)
			{
				fprintf(stderr, "%s: Warning, nb mismatch (%u.%u) %u!=%u\n", prefix, s, f, state->squads[s].nb[f], nb[s][f]);
				valid=false;
				for(unsigned int i=0;i<state->nbombers;i++)
				{
					if(state->bombers[i].squadron==(int)s && state->bombers[i].flight==(int)f)
						fprintf(stderr, "%s: Found bomber %u (trn %d wear %g)\n", prefix, i, state->bombers[i].train, state->bombers[i].wear);
				}
			}
		}
		if(!state->squads[s].third_flight && nb[s][2])
			fprintf(stderr, "%s: Warning, ghost third flight with %u a/c in sqn %u\n", prefix, nb[s][2], s);

	}
}
#else
static inline void validate_nb(const char *prefix __attribute__((unused)), const game *state __attribute__((unused))) {}
#endif

screen_id post_raid_screen(__attribute__((unused)) atg_canvas *canvas, game *state)
{
	validate_nb("init post_raid", state);
	co_append(&state->hist, state->now, (harris_time){11, 05}, state->confid);
	mo_append(&state->hist, state->now, (harris_time){11, 05}, state->morale);
	date tomorrow=nextday(state->now);
	for(unsigned int i=0;i<state->nbombers;i++)
	{
		unsigned int type=state->bombers[i].type;
		if(!datewithin(tomorrow, types[type].entry, types[type].exit))
		{
			clear_sqn(state, i);
			clear_crew(state, i);
			ob_append(&state->hist, state->now, (harris_time){11, 10}, state->bombers[i].id, false, type);
			fixup_crew_assignments(state, i, false, 0);
			state->nbombers--;
			for(unsigned int j=i;j<state->nbombers;j++)
				state->bombers[j]=state->bombers[j+1];
			i--;
			continue;
		}
		if(!datebefore(tomorrow, types[type].train)&&!state->bombers[i].train)
		{
			clear_sqn(state, i);
			clear_crew(state, i);
			state->bombers[i].train=true;
		}
	}
	update_sqn_list(state);
	for(unsigned int b=0;b<nbases;b++)
	{
		double eff;
		bases[b].eff=1.0;
		if(mixed_base(state, b, &eff))
			bases[b].eff*=eff;
		if(mixed_group(state, base_grp(bases[b]), &eff))
			bases[b].eff*=eff;
	}
	validate_nb("after obs", state);
	for(unsigned int i=0;i<state->nbombers;i++)
	{
		unsigned int type=state->bombers[i].type;
		if(state->bombers[i].failed)
		{
			unsigned int s=max(state->bombers[i].squadron, 0), b=state->squads[s].base;
			if(brandp((bstats(state->bombers[i]).svp*bases[b].eff/100.0)/2.0))
			{
				fa_append(&state->hist, state->now, (harris_time){11, 10}, state->bombers[i].id, false, type, 0);
				state->bombers[i].failed=false;
			}
		}
		else
		{
			if(brandp((1-bstats(state->bombers[i]).svp/100.0)/2.4))
			{
				fa_append(&state->hist, state->now, (harris_time){11, 10}, state->bombers[i].id, false, type, 1);
				state->bombers[i].failed=true;
			}
		}
	}
	// TODO: if confid or morale too low, SACKED
	state->cshr+=state->confid*2.0;
	state->cshr+=state->morale;
	state->cshr*=.96;
	state->cash+=state->cshr;
	state->confid*=.98;
	ca_append(&state->hist, state->now, (harris_time){11, 20}, state->cshr, state->cash);
	// Choose new [dis]favoured targets
	for(unsigned int i=0;i<2;i++)
	{
		if(!state->tfd[i]--)
		{
			bool have=false;
			state->tfd[i]=7+irandu(14);
			state->tfav[i]=irandu(T_CLASSES);
			if(state->tfav[i]!=state->tfav[1-i])
				for(unsigned int j=0;j<ntargs;j++)
				{
					if(targs[j].class!=state->tfav[i]) continue;
					if(!datewithin(state->now, targs[j].entry, targs[j].exit)) continue;
					if(!state->dmg[j]) continue;
					have=true;
					break;
				}
			if(!have)
				state->tfav[i]=T_CLASSES;
			tp_append(&state->hist, state->now, (harris_time){11, 22}, state->tfav[i], i);
		}
		if(!state->ifd[i]--)
		{
			bool have=false;
			state->ifd[i]=14+irandu(28);
			state->ifav[i]=irandu(I_CLASSES);
			if(state->ifav[i]!=state->ifav[1-i])
				for(unsigned int j=0;j<ntargs;j++)
				{
					if(targs[j].iclass!=state->ifav[i]) continue;
					if(!datewithin(state->now, targs[j].entry, targs[j].exit)) continue;
					if(!state->dmg[j]) continue;
					have=true;
					break;
				}
			if(!have)
				state->ifav[i]=I_CLASSES;
			ip_append(&state->hist, state->now, (harris_time){11, 22}, state->ifav[i], i);
		}
	}
	// Apply any mods
	for(unsigned int m=0;m<nmods;m++)
		if(!diffdate(tomorrow, mods[m].d))
		{
			unsigned int bt=mods[m].bt;
			if(apply_mod(m))
				fprintf(stderr, "Failed to apply mod `%s' to %s %s\n", mods[m].desc, types[bt].manu, types[bt].name);
			else
				fprintf(stderr, "Applied mod `%s' to %s %s\n", mods[m].desc, types[bt].manu, types[bt].name);
			if(mods[m].s==BSTAT_CREW)
			{
				// remove mismatched students
				for(unsigned int i=0;i<state->nbombers;i++)
				{
					if(!state->bombers[i].train)
						continue;
					if(state->bombers[i].type!=bt)
						continue;
					for(unsigned int j=0;j<MAX_CREW;j++)
					{
						int k=state->bombers[i].crew[j];
						int s=state->crews[k].squadron;
						if(k<0)
							continue;
						if(state->crews[k].class!=bstats(state->bombers[i]).crew[j])
						{
							state->bombers[i].crew[j]=-1;
							state->crews[k].assignment=-1;
							if(s>=0)
								state->squads[s].nc[state->crews[k].class]++;
						}
					}
				}
			}
		}
	// Update bomber prodn caps
	for(unsigned int i=0;i<ntypes;i++)
	{
		if(!state->btypes[i]) continue;
		if(!diffdate(tomorrow, types[i].entry)) // when a type is new, get 15 of them free
		{
			for(unsigned int a=0;a<15;a++)
			{
				unsigned int n=state->nbombers++;
				ac_bomber *nb=realloc(state->bombers, state->nbombers*sizeof(ac_bomber));
				if(!nb)
				{
					perror("realloc"); // TODO more visible warning
					state->nbombers=n;
					break;
				}
				(state->bombers=nb)[n]=(ac_bomber){.type=i, .failed=false, .id=rand_acid(), .mark=types[i].newmark, .squadron=-1, .flight=-1};
				for(unsigned int j=0;j<NNAVAIDS;j++)
					nb[n].nav[j]=false;
				for(unsigned int j=0;j<MAX_CREW;j++)
					nb[n].crew[j]=-1;
				if((!datebefore(state->now, event[EVENT_ALLGEE]))&&bstats(nb[n]).nav[NAV_GEE])
					nb[n].nav[NAV_GEE]=true;
				ct_append(&state->hist, state->now, (harris_time){11, 25}, state->bombers[n].id, false, state->bombers[n].type, state->bombers[n].mark);
			}
		}
		if(!datewithin(state->now, types[i].entry, types[i].otub?types[i].exit:types[i].train)) continue;
		types[i].pcbuf=min(types[i].pcbuf, 180000)+types[i].pc;
	}
	validate_nb("before prodn", state);
	// purchase additional planes based on priorities and subject to production capacity constraints
	while(true)
	{
		unsigned int m=0;
		for(unsigned int i=1;i<ntypes;i++)
		{
			if(!state->btypes[i]) continue;
			if(!datewithin(state->now, types[i].entry, types[i].otub?types[i].exit:types[i].train)) continue;
			if(types[i].pribuf>types[m].pribuf) m=i;
		}
		if(types[m].pribuf<8)
		{
			bool any=false;
			for(unsigned int i=0;i<ntypes;i++)
			{
				if(!state->btypes[i]) continue;
				if(!datewithin(state->now, types[i].entry, types[i].otub?types[i].exit:types[i].train)) continue;
				unsigned int prio=(unsigned int [4]){0, 1, 3, 6}[types[i].prio];
				if(datebefore(state->now, types[i].novelty)) prio=max(prio, 1);
				types[i].pribuf+=prio;
				if(prio) any=true;
			}
			if(!any) break;
			continue;
		}
		if(!state->btypes[m]) // is possible, if m==0
			break;
		unsigned int cost=newstats(types[m]).cost;
		if(cost>state->cash) break;
		if(cost>types[m].pcbuf) break;
		unsigned int n=state->nbombers++;
		ac_bomber *nb=realloc(state->bombers, state->nbombers*sizeof(ac_bomber));
		if(!nb)
		{
			perror("realloc"); // TODO more visible warning
			state->nbombers=n;
			break;
		}
		(state->bombers=nb)[n]=(ac_bomber){.type=m, .failed=false, .id=rand_acid(), .mark=types[m].newmark, .squadron=-1, .flight=-1, .wear=0};
		for(unsigned int j=0;j<NNAVAIDS;j++)
			nb[n].nav[j]=false;
		if((!datebefore(state->now, event[EVENT_ALLGEE]))&&bstats(nb[n]).nav[NAV_GEE])
			nb[n].nav[NAV_GEE]=true;
		for(unsigned int j=0;j<MAX_CREW;j++)
			nb[n].crew[j]=-1;
		state->cash-=cost;
		types[m].pcbuf-=cost;
		types[m].pc+=cost/(types[m].slowgrow?200:150);
		types[m].pribuf-=8;
		ct_append(&state->hist, state->now, (harris_time){11, 30}, state->bombers[n].id, false, state->bombers[n].type, state->bombers[n].mark);
	}
	// install navaids
	for(unsigned int n=0;n<NNAVAIDS;n++)
	{
		if(datebefore(state->now, event[navevent[n]])) continue;
		date x=event[navevent[n]];
		x.month+=2;
		if(x.month>12) {x.month-=12;x.year++;}
		bool notnew=datebefore(x, state->now);
		state->napb[n]+=notnew?25:10;
		int i=state->nap[n];
		if(i>=0&&!datewithin(state->now, types[i].entry, types[i].train)) continue;
		unsigned int nac=0;
		for(int j=state->nbombers-1;(state->napb[n]>=navprod[n])&&(j>=0);j--)
		{
			if(i<0)
			{
				if(!bstats(state->bombers[j]).nav[n]) continue;
			}
			else
			{
				if((int)state->bombers[j].type!=i)	continue;
			}
			if(state->bombers[j].failed) continue;
			if(state->bombers[j].train) continue;
			if(state->bombers[j].nav[n]) continue;
			state->bombers[j].nav[n]=true;
			na_append(&state->hist, state->now, (harris_time){11, 35}, state->bombers[j].id, false, state->bombers[j].type, n);
			state->napb[n]-=navprod[n];
			if(++nac>=(notnew?10:4)) break;
		}
	}
	// crews in non-operational squadrons train a little
	for(unsigned int i=0;i<state->ncrews;i++)
	{
		if(state->crews[i].status!=CSTATUS_CREWMAN)
			continue;
		if(state->crews[i].squadron<0)
			continue;
		unsigned int s=state->crews[i].squadron;
		if(!state->squads[s].rtime)
			continue;
		if(state->crews[i].assignment<0)
			continue;
		unsigned int k=state->crews[i].assignment;
		if(state->bombers[k].failed)
			continue;
		// apply training-wear only once
		if(state->bombers[k].crew[0]==(int)i)
			apply_wear(&state->bombers[k], 0.2);
		if((state->crews[i].skill<1)||brandp(5.0/pow(state->crews[i].skill, 1.5)))
		{
			double new=min(state->crews[i].skill+irandu(state->crews[i].lrate)/100.0, 100.0);
			if(floor(new)>floor(state->crews[i].skill))
				sk_append(&state->hist, state->now, (harris_time){11, 40}, state->crews[i].id, state->crews[i].class, new);
			state->crews[i].skill=new;
		}
		unsigned int type=state->squads[s].btype;
		if(state->bombers[k].type!=type)
		{
			fprintf(stderr, "Warning: assignment type mismatch: %u in %u type %u!=%u sqn %u\n", i, k, state->bombers[k].type, type, s);
			continue;
		}
		if(types[type].heavy)
			state->crews[i].heavy=min(state->crews[i].heavy+50.0/max_dwell[TPIPE_HCU], 100.0);
		if(types[type].lfs)
			state->crews[i].lanc=min(state->crews[i].lanc+50.0/max_dwell[TPIPE_HCU], 100.0);
	}
	validate_nb("after training", state);
	// update squadrons
	for(unsigned int s=0;s<state->nsquads;s++)
	{
		if(state->squads[s].rtime)
			state->squads[s].rtime--;
		/* On PFF day, everyone at a PFF base gets transferred to No. 8 Group */
		if(!diffdate(tomorrow, event[EVENT_PFF]) && bases[state->squads[s].base].group==8)
			for(unsigned int i=0;i<state->ncrews;i++)
				if(state->crews[i].squadron==(int)s)
					state->crews[i].group=8;
	}
	// update bases
	if(state->paving>=0)
	{
		unsigned int b=state->paving;
		if(++state->pprog>=PAVE_TIME)
		{
			bases[b].paved=true;
			state->paving=-1;
			state->pprog=0;
			char pave_msg[256];
			snprintf(pave_msg, sizeof(pave_msg),
			"The airfield at %s has been upgraded to Class A standard.\n"
			"Concrete runways, peri-track and dispersal hardstandings have all been laid.\n"
			"\n"
			"Visit the Stations & Squadrons screen to select another station to upgrade.\n",
			bases[b].name);
			if(msgadd(canvas, state, tomorrow, bases[b].name, pave_msg))
				fprintf(stderr, "failed to msgadd paved: %s\n", bases[b].name);
		}
	}
	// train crews, and recruit more
	train_students(state);
	refill_students(state, true);
	validate_nb("before scrap", state);
	// scrap worn-out aircraft, move partly-worn aircraft to training unit
	for(unsigned int i=0;i<state->nbombers;i++)
	{
		unsigned int type=state->bombers[i].type;
		if(state->bombers[i].wear >= 99.99)
		{
#ifdef PARANOID_NB_CHECKS
			fprintf(stderr, "Scrapping bomber %u (sqn %d flt %d)\n", i, state->bombers[i].squadron, state->bombers[i].flight);
			validate_nb("scrappage", state);
#endif
			clear_sqn(state, i);
			clear_crew(state, i);
scrap:
			sc_append(&state->hist, state->now, (harris_time){11, 42}, state->bombers[i].id, false, type);
			fixup_crew_assignments(state, i, false, 0);
			state->nbombers--;
			for(unsigned int j=i;j<state->nbombers;j++)
				state->bombers[j]=state->bombers[j+1];
			i--;
			validate_nb("scrapped", state);
			continue;
		}
		if(state->bombers[i].wear>=types[type].twear[state->bombers[i].mark])
		{
#ifdef PARANOID_NB_CHECKS
			if(!state->bombers[i].train)
			{
				fprintf(stderr, "Pensioning bomber %u type %u (sqn %d flt %d)\n", i, type, state->bombers[i].squadron, state->bombers[i].flight);
				validate_nb("pension", state);
			}
#endif
			clear_sqn(state, i);
			clear_crew(state, i);
			if(types[type].noarm)
				goto scrap;
			else
				state->bombers[i].train=true;
			validate_nb("sent to train", state);
		}
	}
	// crews go to instructors and vice-versa; escapees return home
	for(unsigned int i=0;i<state->ncrews;i++)
	{
		switch(state->crews[i].status)
		{
			case CSTATUS_ESCAPEE:
				if(--state->crews[i].assignment>0)
					break;
				state->crews[i].status=CSTATUS_CREWMAN;
				st_append(&state->hist, state->now, (harris_time){11, 44}, state->crews[i].id, state->crews[i].class, state->crews[i].status);
				state->crews[i].assignment=-1;
				if(state->crews[i].squadron>=0)
				{
					unsigned int s=state->crews[i].squadron;
					state->squads[s].nc[state->crews[i].class]++;
				}
				// fall through
				// to CREWMAN handling for end-of-tour check (in case we were shot down after bombing)
			case CSTATUS_CREWMAN:
				if(state->crews[i].tour_ops>=30)
				{
					state->crews[i].status=CSTATUS_INSTRUC;
					st_append(&state->hist, state->now, (harris_time){11, 44}, state->crews[i].id, state->crews[i].class, state->crews[i].status);
					state->crews[i].tour_ops=0;
					state->crews[i].full_tours++;
					int j=state->crews[i].assignment;
					if(j>=0)
					{
						unsigned int k;
						for(k=0;k<MAX_CREW;k++)
							if(state->bombers[j].crew[k]==(int)i)
								break;
						if(k<MAX_CREW)
							state->bombers[j].crew[k]=-1;
						else // can't happen
							fprintf(stderr, "Warning: crew linkage error b%u c%u\n", k, i);
					}
					else if(state->crews[i].squadron>=0)
					{
						unsigned int s=state->crews[i].squadron;
						if(!state->squads[s].nc[state->crews[i].class]--)
							fprintf(stderr, "Warning: sqn nc went negative for %d.%d\n", s, state->crews[i].class);
					}
					state->crews[i].assignment=-1;
					state->crews[i].squadron=-1;
					state->crews[i].group=0;
				}
			break;
			case CSTATUS_INSTRUC:
				if(++state->crews[i].tour_ops>180)
				{
					state->crews[i].status=CSTATUS_CREWMAN;
					st_append(&state->hist, state->now, (harris_time){11, 44}, state->crews[i].id, state->crews[i].class, state->crews[i].status);
					state->crews[i].tour_ops=0;
					state->crews[i].assignment=-1;
					state->crews[i].squadron=-1;
					state->crews[i].group=0;
				}
			break;
			default:
			break;
		}
	}
	validate_nb("after tops", state);
	// German production
	unsigned int rcity=GET_DC(state,RCITY),
	             rother=GET_DC(state,ROTHER);
	for(unsigned int i=0;i<ICLASS_MIXED;i++)
		state->dprod[i]=0;
	double reprate=state->gprod[ICLASS_RAIL]/9000;
	state->gprod[ICLASS_RAIL]=0;
	for(unsigned int i=0;i<ntargs;i++)
	{
		if(targs[i].city<0)
		{
			if(!datebefore(state->now, targs[i].exit)) continue;
		}
		else
		{
			if(!datebefore(state->now, targs[targs[i].city].exit)) continue;
		}
		switch(targs[i].class)
		{
			case TCLASS_CITY:
			{
				state->flam[i]=(state->flam[i]*0.8)+8.0;
				double dflk=(targs[i].flak*state->dmg[i]*.0005)-(state->flk[i]*.05);
				state->flk[i]+=dflk;
				if(dflk)
					tfk_append(&state->hist, state->now, (harris_time){11, 45}, i, dflk, state->flk[i]);
				if(!targs[i].hit)
				{
					double ddmg=min(state->dmg[i]*.1*reprate/(double)rcity, 100-state->dmg[i]);
					state->dmg[i]+=ddmg;
					if(ddmg)
						tdm_append(&state->hist, state->now, (harris_time){11, 45}, i, ddmg, state->dmg[i]);
				}
				produce(i, state, state->dmg[i]*targs[i].prod*0.8);
			}
			break;
			case TCLASS_LEAFLET:
			{
				if(!targs[i].hit)
				{
					double ddmg=min(state->dmg[i]*.2/(double)rother, 100-state->dmg[i]);
					state->dmg[i]+=ddmg;
					if(ddmg)
						tdm_append(&state->hist, state->now, (harris_time){11, 45}, i, ddmg, state->dmg[i]);
				}
				produce(i, state, state->dmg[i]*targs[i].prod/20.0);
			}
			break;
			case TCLASS_SHIPPING:
			break;
			case TCLASS_MINING:
				state->gprod[ICLASS_RAIL]+=state->dmg[i]*targs[i].prod*0.4;
				state->dprod[ICLASS_RAIL]+=state->dmg[i]*targs[i].prod*0.4;
				state->dmg[i]=min(state->dmg[i]+0.2, 100.0);
			break;
			case TCLASS_AIRFIELD:
			case TCLASS_BRIDGE:
			case TCLASS_ROAD:
				if(!state->dmg[i])
					goto unflak;
			break;
			case TCLASS_INDUSTRY:
				if(state->dmg[i])
				{
					double ddmg=min(state->dmg[i]*reprate/(double)rother, 100-state->dmg[i]), cscale=targs[i].city<0?1.0:state->dmg[targs[i].city]/100.0;
					if(cscale==0)
						goto unflak;
					if(!targs[i].hit)
					{
						state->dmg[i]+=ddmg;
						if(ddmg)
							tdm_append(&state->hist, state->now, (harris_time){11, 45}, i, ddmg, state->dmg[i]);
					}
					produce(i, state, state->dmg[i]*targs[i].prod*cscale/2.0);
				}
				else
				{
					unflak:
					if(state->flk[i])
					{
						tfk_append(&state->hist, state->now, (harris_time){11, 45}, i, -state->flk[i], 0);
						state->flk[i]=0;
					}
				}
			break;
			default: // shouldn't ever get here
				fprintf(stderr, "Bad targs[%d].class = %d\n", i, targs[i].class);
			break;
		}
		targs[i].hit=false;
	}
	state->gprod[ICLASS_ARM]*=datebefore(state->now, event[EVENT_BARBAROSSA])?0.96:0.94;
	state->gprod[ICLASS_BB]*=0.99;
	state->gprod[ICLASS_RAIL]*=0.99;
	state->gprod[ICLASS_OIL]*=0.984;
	state->gprod[ICLASS_UBOOT]*=0.95; // not actually used for anything
	// German fighters
	unsigned int fcount[nftypes];
	memset(fcount, 0, sizeof(fcount));
	unsigned int maxradpri=0;
	for(unsigned int i=0;i<state->nfighters;i++)
	{
		unsigned int type=state->fighters[i].type;
		if(state->fighters[i].crashed||!datewithin(state->now, ftypes[type].entry, ftypes[type].exit))
		{
			if(!state->fighters[i].crashed&&!datewithin(state->now, ftypes[type].entry, ftypes[type].exit))
				ob_append(&state->hist, state->now, (harris_time){11, 48}, state->fighters[i].id, true, type);
			state->nfighters--;
			for(unsigned int j=i;j<state->nfighters;j++)
				state->fighters[j]=state->fighters[j+1];
			i--;
			continue;
		}
		if((!state->fighters[i].radar)&&(ftypes[type].radpri>maxradpri))
			maxradpri=ftypes[type].radpri;
		fcount[type]++;
		if(brandp(0.1))
		{
			unsigned int base;
			do
				base=state->fighters[i].base=irandu(nfbases);
			while(!datewithin(state->now, fbases[base].entry, fbases[base].exit));
		}
	}
	if(!datebefore(state->now, event[EVENT_L_BC]) && maxradpri)
	{
		unsigned int rcount=4;
		for(int i=state->nfighters-1;i>=0;i--)
		{
			if(state->gprod[ICLASS_RADAR]<5000 || !rcount) break;
			unsigned int type=state->fighters[i].type;
			if((!state->fighters[i].radar)&&(ftypes[type].radpri==maxradpri))
			{
				state->fighters[i].radar=true;
				na_append(&state->hist, state->now, (harris_time){11, 49}, state->fighters[i].id, true, type, 0);
				state->gprod[ICLASS_RADAR]-=5000;
				rcount--;
			}
		}
	}
	unsigned int mfcost=0;
	for(unsigned int i=0;i<nftypes;i++)
	{
		if(!datewithin(state->now, ftypes[i].entry, ftypes[i].exit)) continue;
		mfcost=max(mfcost, ftypes[i].cost);
	}
	while(state->gprod[ICLASS_AC]>=mfcost)
	{
		double p[nftypes], cumu_p=0;
		for(unsigned int j=0;j<nftypes;j++)
		{
			if(!datewithin(state->now, ftypes[j].entry, ftypes[j].exit))
			{
				p[j]=0;
				continue;
			}
			date start=ftypes[j].entry, end=ftypes[j].exit;
			if(start.year<1939) start=(date){1938, 6, 1};
			if(end.year>1945) end=(date){1946, 8, 1};
			int dur=diffdate(end, start), age=diffdate(state->now, start);
			double fr_age=age/(double)dur;
			p[j]=(0.8-fr_age)/sqrt(1+fcount[j]);
			p[j]=max(p[j], 0);
			cumu_p+=p[j];
		}
		double d=drandu(cumu_p);
		unsigned int i=0;
		while(d>=p[i]) d-=p[i++];
		if(!datewithin(state->now, ftypes[i].entry, ftypes[i].exit)) continue; // should be impossible as p[i] == 0
		if(ftypes[i].cost>state->gprod[ICLASS_AC]) break; // should also be impossible as cost <= mfcost <= state->gprod
		unsigned int n=state->nfighters++;
		ac_fighter *newf=realloc(state->fighters, state->nfighters*sizeof(ac_fighter));
		if(!newf)
		{
			perror("realloc"); // TODO more visible warning
			state->nfighters=n;
			break;
		}
		unsigned int base;
		do
			base=irandu(nfbases);
		while(!datewithin(state->now, fbases[base].entry, fbases[base].exit));
		(state->fighters=newf)[n]=(ac_fighter){.type=i, .base=base, .crashed=false, .landed=true, .k=-1, .targ=-1, .damage=0, .id=rand_acid()};
		state->gprod[ICLASS_AC]-=ftypes[i].cost;
		ct_append(&state->hist, state->now, (harris_time){11, 50}, state->fighters[n].id, true, i, 0 /* No fighter marks (yet?) */);
	}
	for(unsigned int i=0;i<ICLASS_MIXED;i++)
		gp_append(&state->hist, state->now, (harris_time){11, 55}, i, state->gprod[i], state->dprod[i]);
	for(unsigned int i=0;i<ntypes;i++)
	{
		if(!state->btypes[i]) continue;
		if(!diffdate(tomorrow, types[i].entry))
		{
			if(msgadd(canvas, state, tomorrow, types[i].name, types[i].newtext))
				fprintf(stderr, "failed to msgadd newtype: %s\n", types[i].name);
		}
		if(!diffdate(tomorrow, types[i].train) && types[i].traintext)
		{
			if(msgadd(canvas, state, tomorrow, types[i].name, types[i].traintext))
				fprintf(stderr, "failed to msgadd traintype: %s\n", types[i].name);
		}
		if(!diffdate(tomorrow, types[i].exit) && types[i].exittext)
		{
			if(msgadd(canvas, state, tomorrow, types[i].name, types[i].exittext))
				fprintf(stderr, "failed to msgadd exittype: %s\n", types[i].name);
		}
	}
	for(unsigned int i=0;i<nftypes;i++)
		if(!diffdate(tomorrow, ftypes[i].entry))
		{
			if(msgadd(canvas, state, tomorrow, ftypes[i].name, ftypes[i].newtext))
				fprintf(stderr, "failed to msgadd newftype: %s\n", ftypes[i].name);
		}
	for(unsigned int ev=0;ev<NEVENTS;ev++)
	{
		if(!diffdate(tomorrow, event[ev]))
		{
			if(msgadd(canvas, state, tomorrow, event_names[ev], evtext[ev]))
				fprintf(stderr, "failed to msgadd event: %s\n", event_names[ev]);
			if(ev==EVENT_UBOAT) /* Prioritise U-Boat targets */
				force_iprio(state, ICLASS_UBOOT, 24);
			else if(ev==EVENT_MANNHEIM) /* Prioritise cities */
				force_tprio(state, TCLASS_CITY, 7);
			else if(ev==EVENT_NORWAY||ev==EVENT_BARGE) /* don't Ignore shipping */
				force_tprio(state, TCLASS_SHIPPING, 0);
			else if(ev==EVENT_HCU) /* enable the HCU */
				state->tpipe[TPIPE_HCU].dwell=30;
			else if(ev==EVENT_LFS) /* enable the LFS */
				state->tpipe[TPIPE_LFS].dwell=10;
		}
	}
	state->now=tomorrow;
	validate_nb("fini post_raid", state);
	return(SCRN_CONTROL);
}

/* For these two functions, days==0 just means "ensure it's not Ignore" */
void force_tprio(game *state, enum t_class cls, unsigned int days)
{
	if(state->tfav[0]==cls)
	{
		state->tfd[0]=max(state->tfd[0], days);
	}
	else
	{
		if(days)
		{
			state->tfav[0]=cls;
			state->tfd[0]=days;
			tp_append(&state->hist, state->now, (harris_time){11, 57}, state->tfav[0], 0);
		}
		if(state->tfav[1]==cls)
		{
			state->tfav[1]=T_CLASSES;
			state->tfd[1]=0;
			tp_append(&state->hist, state->now, (harris_time){11, 57}, state->tfav[1], 1);
		}
	}
}

void force_iprio(game *state, enum i_class cls, unsigned int days)
{
	if(state->ifav[0]==cls)
	{
		state->ifd[0]=max(state->ifd[0], days);
	}
	else
	{
		if(days)
		{
			state->ifav[0]=cls;
			state->ifd[0]=days;
			ip_append(&state->hist, state->now, (harris_time){11, 57}, state->ifav[0], 0);
		}
		if(state->ifav[1]==cls)
		{
			state->ifav[1]=I_CLASSES;
			state->ifd[1]=0;
			ip_append(&state->hist, state->now, (harris_time){11, 57}, state->ifav[1], 1);
		}
	}
}

void post_raid_free(void)
{
	// nothing to do
}

void produce(int targ, game *state, double amount)
{
	switch(targs[targ].iclass)
	{
		case ICLASS_BB:
		case ICLASS_RAIL:
		case ICLASS_STEEL:
		case ICLASS_OIL:
		case ICLASS_UBOOT:
		break;
		case ICLASS_ARM:
			amount=min(amount, state->gprod[ICLASS_STEEL]*1.2);
			state->gprod[ICLASS_STEEL]-=amount;
		break;
		case ICLASS_AC:
			amount=min(amount, state->gprod[ICLASS_BB]*2.0);
			state->gprod[ICLASS_BB]-=amount/2.0;
		break;
		case ICLASS_RADAR:
			if(datebefore(state->now, event[EVENT_LICHTENSTEIN]))
				return;
		break;
		case ICLASS_MIXED:
#define ADD(class, qty)	do { state->gprod[class]+=qty; state->dprod[class]+=qty; } while(0)
			ADD(ICLASS_OIL, amount/7.0);
			ADD(ICLASS_RAIL, amount/21.0);
			ADD(ICLASS_ARM, amount/7.0);
			ADD(ICLASS_AC, amount/21.0);
			ADD(ICLASS_BB, amount/14.0);
			ADD(ICLASS_STEEL, amount/35.0);
			if(!datebefore(state->now, event[EVENT_LICHTENSTEIN]))
				ADD(ICLASS_RADAR, amount/200.0);
#undef ADD
			return;
		default:
			fprintf(stderr, "Bad targs[%d].iclass = %d\n", targ, targs[targ].iclass);
		break;
	}
	state->gprod[targs[targ].iclass]+=amount;
	state->dprod[targs[targ].iclass]+=amount;
}

void refill_students(game *state, bool refill)
{
	unsigned int pool[CREW_CLASSES];
	bool eats=!datebefore(state->now, event[EVENT_EATS]);
	for(unsigned int i=0;i<CREW_CLASSES;i++)
	{
		unsigned int scount=0, acount=0;
		pool[i]=cclasses[i].initpool;
		for(unsigned int j=0;j<state->ncrews;j++)
		{
			if(state->crews[j].status==CSTATUS_STUDENT)
			{
				if(state->crews[j].class!=i) continue;
				scount++;
				if(state->crews[j].assignment>=0)
					acount++;
			}
			else if(state->crews[j].status==CSTATUS_INSTRUC)
			{
				if(state->crews[j].class==i)
				{
					pool[i]+=cclasses[i].pupils;
					if(eats&&cclasses[i].eats)
						pool[i]++;
				}
				else if(cclasses[state->crews[j].class].extra_pupil==i)
					pool[i]++;
			}
		}
		pool[i]/=GET_DC(state, TPOOL);
		if(i==CCLASS_E&&datebefore(state->now, event[EVENT_FLT_ENG]))
			pool[i]=0;
		if(refill && scount<pool[i])
		{
			unsigned int add=min(pool[i]-scount, max((pool[i]-scount)/16, 1));
			unsigned int nc=state->ncrews+add;
			crewman *new=realloc(state->crews, nc*sizeof(crewman));
			if(!new)
			{
				perror("realloc");
				// nothing we can do about it except give up refilling
				return;
			}
			state->crews=new;
			for(unsigned int j=state->ncrews;j<nc;j++)
			{
				state->crews[j]=(crewman){
					.id=rand_cmid(),
					.class=i,
					.status=CSTATUS_STUDENT,
					.skill=0,
					.lrate=60+irandu(60),
					.tour_ops=0,
					.training=TPIPE_OTU,
					.assignment=-1,
					.group=0,
					.squadron=-1,
				};
				ge_append(&state->hist, state->now, (harris_time){11, 43}, state->crews[j].id, i, state->crews[j].lrate);
			}
			state->ncrews=nc;
		}
		if(pool[i]<acount) {
			fprintf(stderr, "Warning: student pool error (class %c), %u<%u\n", cclasses[i].letter, pool[i], acount);
			pool[i]=acount;
		}
		pool[i]-=acount;
	}
	for(enum tpipe t=TPIPE__MAX;t-->0;) {
		if (state->tpipe[t].dwell<0)
			continue;
		for(unsigned int k=0;k<state->nbombers;k++) {
			if(!state->bombers[k].train)
				continue;
			unsigned int type=state->bombers[k].type;
			switch(t) {
			case TPIPE_LFS: /* Lancs (LFS flag) only */
				if(!(types[type].lfs))
					continue;
				break;
			case TPIPE_HCU: /* Any heavy */
				if(!types[type].heavy)
					continue;
				break;
			case TPIPE_OTU: /* Neither heavy nor noarm (i.e. no Mosquito) */
				if(types[type].heavy||types[type].noarm)
					continue;
				break;
			default:
				break;
			}
			for(unsigned int c=0;c<MAX_CREW;c++)
			{
				enum cclass i=bstats(state->bombers[k]).crew[c];
				if(i==CCLASS_NONE) {
					if(types[type].otub&&!datebefore(state->now, event[EVENT_OTUB]))
						i=CCLASS_B;
					else
						continue;
				}
				if(state->bombers[k].crew[c]>=0)
					continue;
				if(!pool[i])
					continue;
				for(unsigned int j=0;j<state->ncrews;j++)
					if(state->crews[j].status==CSTATUS_STUDENT)
						if(state->crews[j].class==i)
							if(state->crews[j].training==t)
								if(state->crews[j].assignment<0)
								{
									state->crews[j].assignment=k;
									state->bombers[k].crew[c]=j;
									pool[i]--;
									break;
								}
			}
		}
	}
}

void train_students(game *state)
{
	for(unsigned int i=0;i<state->ncrews;i++)
	{
		unsigned int type=(unsigned int)-1;
		int k=state->crews[i].assignment;
		enum tpipe stage;

		if(state->crews[i].status!=CSTATUS_STUDENT)
			continue;
		if(k>=0)
			type=state->bombers[k].type;
		stage=state->crews[i].training;
		if(k<0&&!(state->crews[i].class==CCLASS_E&&stage==TPIPE_OTU)) // engineers don't need an a/c for OTU-equivalent training
			continue;
		if(k>=0&&state->bombers[k].crew[0]==(int)i)
			apply_wear(&state->bombers[k], 0.5);
		if((int)state->crews[i].tour_ops++>=state->tpipe[stage].dwell)
		{
			int c;
			state->crews[i].assignment=-1;
			if(k>=0) { /* Might not be, if CCLASS_E */
				for(c=0;c<MAX_CREW;c++)
					if(state->bombers[k].crew[c]==(int)i)
					{
						state->bombers[k].crew[c]=-1;
						break;
					}
				if(c==MAX_CREW)
					fprintf(stderr, "Warning: student assignment error (%u, %u)\n", k, i);
			}
			state->crews[i].tour_ops=0;
			if(stage+1<TPIPE__MAX && state->tpipe[stage+1].dwell > 0 && ((stage+1==TPIPE_HCU&&(state->crews[i].class==CCLASS_E||(state->crews[i].class==CCLASS_B&&!datebefore(state->now, event[EVENT_OTUB]))))||brandp(state->tpipe[stage].cont/100.0)))
			{
				state->crews[i].training++;
			}
			else
			{
				state->crews[i].full_tours=0;
				state->crews[i].status=CSTATUS_CREWMAN;
				state->crews[i].squadron=-1;
				state->crews[i].group=0;
				st_append(&state->hist, state->now, (harris_time){11, 44}, state->crews[i].id, state->crews[i].class, state->crews[i].status);
			}
			continue;
		}
		switch(stage)
		{
			case TPIPE_OTU:
				if((state->crews[i].skill<1)||brandp(1.0/pow(state->crews[i].skill, 0.8)))
				{
					double new=min(state->crews[i].skill+irandu(state->crews[i].lrate)/50.0, 100.0);
					if(floor(new)>floor(state->crews[i].skill))
						sk_append(&state->hist, state->now, (harris_time){11, 43}, state->crews[i].id, state->crews[i].class, new);
					state->crews[i].skill=new;
				}
			break;
			case TPIPE_HCU:
				state->crews[i].heavy=min(state->crews[i].heavy+100.0/max_dwell[TPIPE_HCU], 100.0);
				if(types[type].lfs)
					state->crews[i].lanc=min(state->crews[i].lanc+100.0/max_dwell[TPIPE_HCU], 100.0);
			break;
			case TPIPE_LFS:
				state->crews[i].lanc=min(state->crews[i].lanc+100.0/max_dwell[TPIPE_LFS], 100.0);
			break;
			default:
				fprintf(stderr, "Warning: student pipe error %d\n", stage);
			break;
		}
	}
}
