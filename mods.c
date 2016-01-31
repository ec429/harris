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
	switch(mods[m].s)
	{
		case BSTAT_COST:
			types[bt].cost=mods[m].v.i;
			return(0);
		case BSTAT_SPEED:
			types[bt].speed=mods[m].v.i;
			return(0);
		case BSTAT_ALT:
			types[bt].alt=mods[m].v.i;
			return(0);
		case BSTAT_CAPWT:
			types[bt].capwt=mods[m].v.i;
			return(0);
		case BSTAT_CAPBULK:
			types[bt].capbulk=mods[m].v.i;
			return(0);
		case BSTAT_SVP:
			types[bt].svp=mods[m].v.i;
			return(0);
		case BSTAT_DEFN:
			types[bt].defn=mods[m].v.i;
			return(0);
		case BSTAT_FAIL:
			types[bt].fail=mods[m].v.i;
			return(0);
		case BSTAT_ACCU:
			types[bt].accu=mods[m].v.i;
			return(0);
		case BSTAT_RANGE:
			types[bt].range=mods[m].v.i;
			return(0);
		case BSTAT_CREW:
			for(unsigned int c=0;c<MAX_CREW;c++)
				types[bt].crew[c]=mods[m].v.crew[c];
			return(0);
		case BSTAT_LOADS:
			if(mods[m].v.i<NBOMBLOADS)
			{
				types[bt].load[mods[m].v.i]=true;
				return(0);
			}
			fprintf(stderr, "ignoring excessive bombload ID %d\n", mods[m].v.i);
			return(1);
		case BSTAT_FLAGS:
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
