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
		switch(mods[m].s)
		{
			case BSTAT_COST:
				types[bt].mark[mark].cost=mods[m].v.i;
				break;
			case BSTAT_SPEED:
				types[bt].mark[mark].speed=mods[m].v.i;
				break;
			case BSTAT_ALT:
				types[bt].mark[mark].alt=mods[m].v.i;
				break;
			case BSTAT_CAPWT:
				types[bt].mark[mark].capwt=mods[m].v.i;
				break;
			case BSTAT_CAPBULK:
				types[bt].mark[mark].capbulk=mods[m].v.i;
				break;
			case BSTAT_SVP:
				types[bt].mark[mark].svp=mods[m].v.i;
				break;
			case BSTAT_DEFN:
				types[bt].mark[mark].defn=mods[m].v.i;
				break;
			case BSTAT_FAIL:
				types[bt].mark[mark].fail=mods[m].v.i;
				break;
			case BSTAT_ACCU:
				types[bt].mark[mark].accu=mods[m].v.i;
				break;
			case BSTAT_RANGE:
				types[bt].mark[mark].range=mods[m].v.i;
				return(0);
			case BSTAT_CREW:
				if(mods[m].mark)
					fprintf(stderr, "Warning, mod %s tries to change crew on mark %u, but crew is not marked\n", mods[m].desc, mods[m].mark);
				for(unsigned int c=0;c<MAX_CREW;c++)
					types[bt].crew[c]=mods[m].v.crew[c];
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
			case BSTAT_FLAGS:
				if(mods[m].mark)
					fprintf(stderr, "Warning, mod %s tries to change flags on mark %u, but flags are not marked\n", mods[m].desc, mods[m].mark);
				switch(mods[m].v.f)
				{
					case BFLAG_OVLTANK:
						types[bt].ovltank=true;
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
