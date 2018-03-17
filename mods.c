/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	mods: bombertype modifications
*/

#include "mods.h"

#include "globals.h"

int apply_mod(const game *state, unsigned int b)
{
	unsigned int bt=btechs[b].bt;
	switch(btechs[b].s)
	{
		case BSTAT_COST:
			types[bt].cost=btechs[bt].delta.i;
			return(0);
		case BSTAT_SPEED:
			types[bt].speed=btechs[bt].delta.i;
			return(0);
		case BSTAT_ALT:
			types[bt].alt=btechs[bt].delta.i;
			return(0);
		case BSTAT_CAPWT:
			types[bt].capwt=btechs[bt].delta.i;
			return(0);
		case BSTAT_CAPBULK:
			types[bt].capbulk=btechs[bt].delta.i;
			return(0);
		case BSTAT_SVP:
			types[bt].svp=btechs[bt].delta.i;
			return(0);
		case BSTAT_DEFN:
			types[bt].defn=btechs[bt].delta.i;
			return(0);
		case BSTAT_FAIL:
			types[bt].fail=btechs[bt].delta.i;
			return(0);
		case BSTAT_ACCU:
			types[bt].accu=btechs[bt].delta.i;
			return(0);
		case BSTAT_RANGE:
			types[bt].range=btechs[bt].delta.i;
			return(0);
		case BSTAT_CREW:
			for(unsigned int c=0;c<MAX_CREW;c++)
				types[bt].crew[c]=btechs[bt].delta.crew[c];
			return(0);
		case BSTAT_LOADS:
			if(btechs[bt].delta.i<NBOMBLOADS)
			{
				types[bt].load[btechs[bt].delta.i]=true;
				return(0);
			}
			fprintf(stderr, "ignoring excessive bombload ID %d\n", btechs[bt].delta.i);
			return(1);
		case BSTAT_FLAGS:
			switch(btechs[bt].delta.f)
			{
				case BFLAG_OVLTANK:
					types[bt].ovltank=true;
					return(0);
				default:
					fprintf(stderr, "ignoring unsupported flag %d\n", btechs[bt].delta.f);
					return(1);
			}
		case BSTAT_EXIST:
			types[bt].exist=true;
			types[bt].novelty=state->now;
			if((types[bt].novelty.month+=4)>12)
			{
				types[bt].novelty.month-=12;
				types[bt].novelty.year++;
			}
			return(0);
		default:
			fprintf(stderr, "ignoring unsupported stat %d\n", btechs[bt].s);
			return(1);
	}
}
