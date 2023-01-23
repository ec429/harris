/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	mods: bombertype modifications
*/

#include "mods.h"

#include "globals.h"

int apply_mod(unsigned int m)
{
	unsigned int bt=mods[m].bt;
	unsigned int mark=mods[m].mark;
	if(mods[m].s==NUM_BSTATS)
		return(0);
	if(mark)
		types[bt].newmark=mark;
	for(;mark<MAX_MARKS;mark++)
	{
		struct bomberstats *bm=types[bt].mark+mark;
		switch(mods[m].s)
		{
			case BSTAT_COST:
				bm->cost=mods[m].v.i;
				break;
			case BSTAT_SPEED:
				bm->speed=mods[m].v.i;
				break;
			case BSTAT_ALT:
				bm->alt=mods[m].v.i;
				break;
			case BSTAT_CAPWT:
				bm->capwt=mods[m].v.i;
				break;
			case BSTAT_CAPBULK:
				bm->capbulk=mods[m].v.i;
				break;
			case BSTAT_SVP:
				bm->svp=mods[m].v.i;
				break;
			case BSTAT_DEFN:
				bm->defn=mods[m].v.i;
				break;
			case BSTAT_FAIL:
				bm->fail=mods[m].v.i;
				break;
			case BSTAT_ACCU:
				bm->accu=mods[m].v.i;
				break;
			case BSTAT_RANGE:
				bm->range=mods[m].v.i;
				return(0);
			case BSTAT_CREW:
				for(unsigned int c=0;c<MAX_CREW;c++)
					bm->crew[c]=mods[m].v.crew[c];
				return(0);
			case BSTAT_LOADS:
				if(mods[m].mark)
					fprintf(stderr, "Warning, mod %s tries to change loads on mark %u, but loads are not marked\n", mods[m].desc, mods[m].mark);
				if(mods[m].v.i<NBOMBLOADS)
				{
					types[bt].load[mods[m].v.i]=true;
					return(0);
				}
				fprintf(stderr, "ignoring excessive bombload ID %d\n", mods[m].v.i);
				return(1);
			case BSTAT_NAVS:
				if(mods[m].v.i<NNAVAIDS)
				{
					bm->nav[mods[m].v.i]=true;
					return(0);
				}
				fprintf(stderr, "ignoring excessive navaid ID %d\n", mods[m].v.i);
				return(1);
			case BSTAT_FLAGS:
				switch(mods[m].v.f)
				{
					case BFLAG_OVLTANK:
						bm->ovltank=true;
						return(0);
					case BFLAG_CREWBG:
						bm->crewbg=true;
						return(0);
					case BFLAG_NCREWBG:
						bm->crewbg=false;
						return(0);
					case BFLAG_CREWWG:
						bm->crewwg=true;
						return(0);
					case BFLAG_NCREWWG:
						bm->crewwg=false;
						return(0);
					default:
						fprintf(stderr, "ignoring unsupported flag %d\n", mods[m].v.f);
						return(1);
				}
			default:
				fprintf(stderr, "ignoring unsupported stat %d\n", mods[m].s);
				return(1);
		}
	}
	return(0);
}
