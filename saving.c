/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	saving: functions to save and load games from file
*/

#include "saving.h"
#include <stdio.h>
#include <string.h>
#include "bits.h"
#include "control.h"
#include "date.h"
#include "globals.h"
#include "history.h"
#include "mods.h"
#include "rand.h"
#include "version.h"

bool version_newer(const unsigned int v1[3], const unsigned int v2[3]) // true iff v1 newer than v2
{
	for(unsigned int i=0;i<3;i++)
	{
		if(v1[i]>v2[i]) return(true);
		if(v1[i]<v2[i]) return(false);
	}
	return(false);
}

int loadgame(const char *fn, game *state)
{
	FILE *fs = fopen(fn, "r");
	if(!fs)
	{
		fprintf(stderr, "Failed to open %s!\n", fn);
		perror("fopen");
		return(1);
	}
	unsigned int s_version[3]={0,0,0};
	unsigned int version[3]={VER_MAJ,VER_MIN,VER_REV};
	bool warned_pff=false, warned_acid=false, warned_mark=false;
	state->weather.seed=0;
	for(unsigned int j=0;j<ntypes;j++)
		types[j].newmark=0;
	for(unsigned int i=0;i<DIFFICULTY_CLASSES;i++) // default everything to medium
		state->difficulty[i]=1;
	for(unsigned int i=0;i<nflaks;i++) // all flaksites are unmapped if not saved
		flaks[i].mapped=false;
	for(unsigned int i=0;i<NEVENTS;i++)
		state->evented[i]=false;
	state->tfav[0]=state->tfav[1]=T_CLASSES;
	state->tfd[0]=state->tfd[1]=0;
	state->ifav[0]=state->ifav[1]=I_CLASSES;
	state->ifd[0]=state->ifd[1]=0;
	// supply default tpipe settings for old games.  Assume they're early, so want OTU-only settings.
	state->tpipe[TPIPE_OTU] = (struct tpipe_settings){.dwell = 80, .cont = 0};
	state->tpipe[TPIPE_HCU] = (struct tpipe_settings){.dwell = -1, .cont = 0};
	state->tpipe[TPIPE_LFS] = (struct tpipe_settings){.dwell = -1, .cont = 0};
	while(!feof(fs))
	{
		char *line=fgetl(fs);
		if(!line) break;
		if((!*line)||(*line=='#'))
		{
			free(line);
			continue;
		}
		char tag[64];
		char *dat=strchr(line, ':');
		if(dat) *dat++=0;
		strncpy(tag, line, 64);
		int e=0,f; // poor-man's try...
		if(strcmp(tag, "HARR")==0)
		{
			f=sscanf(dat, "%u.%u.%u\n", s_version, s_version+1, s_version+2);
			if(f!=3)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			if(version_newer(s_version, version))
			{
				fprintf(stderr, "Warning - file is newer version than program;\n may cause strange behaviour\n");
			}
		}
		else if(!(s_version[0]||s_version[1]||s_version[2]))
		{
			fprintf(stderr, "8 File does not start with valid HARR tag\n");
			e|=8;
		}
		else if(strcmp(tag, "DATE")==0)
		{
			state->now=readdate(dat, (date){3, 9, 1939});
		}
		else if(strcmp(tag, "DClasses")==0)
		{
			unsigned int dcl;
			f=sscanf(dat, "%u\n", &dcl);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			if(dcl!=DIFFICULTY_CLASSES)
			{
				fprintf(stderr, "2 Value mismatch: different DClasses value (%u!=%u)\n", dcl, DIFFICULTY_CLASSES);
				e|=2;
			}
		}
		else if(strcmp(tag, "Difficulty")==0)
		{
			unsigned int class, level;
			f=sscanf(dat, "%u,%u\n", &class, &level);
			if(f!=2)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(class>=DIFFICULTY_CLASSES)
			{
				fprintf(stderr, "32 Invalid value %u for class in tag \"%s\"\n", class, tag);
				e|=32;
			}
			else if(level>2)
			{
				fprintf(stderr, "32 Invalid value %u for level in tag \"%s\"\n", level, tag);
				e|=32;
			}
			else if(!class)
			{
				for(unsigned int i=0;i<DIFFICULTY_CLASSES;i++)
					state->difficulty[i]=level;
			}
			else
			{
				state->difficulty[class]=level;
			}
		}
		else if(strcmp(tag, "Confid")==0)
		{
			f=sscanf(dat, FLOAT"\n", CAST_FLOAT_PTR &state->confid);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
		}
		else if(strcmp(tag, "Morale")==0)
		{
			f=sscanf(dat, FLOAT"\n", CAST_FLOAT_PTR &state->morale);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
		}
		else if(strcmp(tag, "Budget")==0)
		{
			f=sscanf(dat, "%u+%u\n", &state->cash, &state->cshr);
			if(f!=2)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
		}
		else if(strcmp(tag, "TPrio")==0)
		{
			f=sscanf(dat, "%u,%u,%u,%u\n", &state->tfav[0], &state->tfd[0], &state->tfav[1], &state->tfd[1]);
			if(f!=4)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
		}
		else if(strcmp(tag, "IPrio")==0)
		{
			f=sscanf(dat, "%u,%u,%u,%u\n", &state->ifav[0], &state->ifd[0], &state->ifav[1], &state->ifd[1]);
			if(f!=4)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
		}
		else if(strcmp(tag, "Types")==0)
		{
			unsigned int sntypes;
			f=sscanf(dat, "%u\n", &sntypes);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(sntypes!=ntypes)
			{
				fprintf(stderr, "2 Value mismatch: different ntypes value (%u!=%u)\n", sntypes, ntypes);
				e|=2;
			}
			else
			{
				for(unsigned int i=0;i<ntypes;i++)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					unsigned int j, prio, pribuf, pc, pcbuf;
					f=sscanf(line, "NoType %u:\n", &j);
					if(f==1)
					{
						state->btypes[j]=false;
					}
					else
					{
						f=sscanf(line, "Prio %u:%u,%u,%u,%u\n", &j, &prio, &pribuf, &pc, &pcbuf);
						if(f!=5)
						{
							fprintf(stderr, "1 Too few arguments to part %u of tag \"%s\"\n", i, tag);
							e|=1;
							break;
						}
						if(j!=i)
						{
							fprintf(stderr, "4 Index mismatch in part %u (%u?) of tag \"%s\"\n", i, j, tag);
							e|=4;
							break;
						}
						rawtypes[j].prio=prio;
						rawtypes[j].pribuf=pribuf;
						rawtypes[j].pc=pc;
						rawtypes[j].pcbuf=pcbuf;
						state->btypes[j]=true;
					}
				}
			}
		}
		else if(strcmp(tag, "Navaids")==0)
		{
			unsigned int snnav;
			f=sscanf(dat, "%u\n", &snnav);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(snnav!=NNAVAIDS)
			{
				fprintf(stderr, "2 Value mismatch: different nnav value (%u!=%u)\n", snnav, NNAVAIDS);
				e|=2;
			}
			else
			{
				for(unsigned int i=0;i<NNAVAIDS;i++)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					unsigned int j, pbuf;
					int prio;
					f=sscanf(line, "NPrio %u:%d,%u\n", &j, &prio, &pbuf);
					if(f!=3)
					{
						fprintf(stderr, "1 Too few arguments to part %u of tag \"%s\"\n", i, tag);
						e|=1;
						break;
					}
					if(j!=i)
					{
						fprintf(stderr, "4 Index mismatch in part %u (%u?) of tag \"%s\"\n", i, j, tag);
						e|=4;
						break;
					}
					state->nap[j]=prio;
					state->napb[j]=pbuf;
				}
			}
		}
		else if(strcmp(tag, "Bombers")==0)
		{
			f=sscanf(dat, "%u\n", &state->nbombers);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else
			{
				free(state->bombers);
				state->bombers=malloc(state->nbombers*sizeof(ac_bomber));
				for(unsigned int i=0;i<state->nbombers;i++)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					unsigned int j;
					unsigned int failed,nav,pff,mark;
					char p_id[9];
					f=sscanf(line, "Type %u:%u,%u,%u,%8s,%u\n", &j, &failed, &nav, &pff, p_id, &mark);
					// TODO: this is for oldsave compat and should probably be removed later
					if(f==3)
					{
						f=4;
						if(!warned_pff) fprintf(stderr, "Warning: added missing 'PFF' field in tag \"%s\"\n", tag);
						warned_pff=true;
						pff=0;
					}
					if(f==4)
					{
						f=5;
						if(!warned_acid) fprintf(stderr, "Warning: added missing 'A/c ID' field in tag \"%s\"\n", tag);
						warned_acid=true;
						pacid(rand_acid(), p_id);
					}
					if(f==5)
					{
						f=6;
						if(!warned_mark) fprintf(stderr, "Warning: added missing 'mark' field in tag \"%s\"\n", tag);
						warned_mark=true;
						mark=0;
					}
					if(f!=6)
					{
						fprintf(stderr, "1 Too few arguments to part %u of tag \"%s\"\n", i, tag);
						e|=1;
						break;
					}
					if(j>ntypes)
					{
						fprintf(stderr, "4 Index mismatch in part %u of tag \"%s\"\n", j, tag);
						e|=4;
						break;
					}
					state->bombers[i]=(ac_bomber){.type=j, .failed=failed, .pff=pff, .mark=mark};
					for(unsigned int n=0;n<NNAVAIDS;n++)
						state->bombers[i].nav[n]=(nav>>n)&1;
					for(unsigned int k=0;k<MAX_CREW;k++)
						state->bombers[i].crew[k]=-1;
					if(gacid(p_id, &state->bombers[i].id))
					{
						fprintf(stderr, "32 Invalid value \"%s\" for a/c ID in tag \"%s\"\n", p_id, tag);
						e|=32;
						break;
					}
				}
			}
		}
		else if(strcmp(tag, "TPipe")==0)
		{
			unsigned int sntp;
			f=sscanf(dat, "%u\n", &sntp);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(sntp!=TPIPE__MAX)
			{
				fprintf(stderr, "2 Value mismatch: different ntpipe value (%u!=%u)\n", sntp, TPIPE__MAX);
				e|=2;
			}
			else
			{
				for(unsigned int i=0;i<TPIPE__MAX;i++)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					unsigned int j, cont;
					int dwell;
					f=sscanf(line, "TStage %u:%d,%u\n", &j, &dwell, &cont);
					if(f!=3)
					{
						fprintf(stderr, "1 Too few arguments to part %u of tag \"%s\"\n", i, tag);
						e|=1;
						break;
					}
					if(j!=i)
					{
						fprintf(stderr, "4 Index mismatch in part %u (%u?) of tag \"%s\"\n", i, j, tag);
						e|=4;
						break;
					}
					state->tpipe[j].dwell=dwell;
					state->tpipe[j].cont=cont;
				}
			}
		}
		else if(strcmp(tag, "Crews")==0)
		{
			f=sscanf(dat, "%u\n", &state->ncrews);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else
			{
				free(state->crews);
				state->crews=malloc(state->ncrews*sizeof(crewman));
				for(unsigned int i=0;i<state->ncrews;i++)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					char status[11];
					char class;
					double skill;
					unsigned int lrate, tops, ft;
					int assi;
					char p_id[17];
					f=sscanf(line, "%10s %c:%la,%u,%u,%u,%d,%16s\n", status, &class, &skill, &lrate, &tops, &ft, &assi, p_id);
					if(f!=8)
					{
						fprintf(stderr, "1 Too few arguments to part %u of tag \"%s\"\n", i, tag);
						e|=1;
						break;
					}
					state->crews[i]=(crewman){.skill=skill, .lrate=lrate, .tour_ops=tops, .full_tours=ft, .assignment=assi};
					if((state->crews[i].class=lookup_crew_letter(class))==CCLASS_NONE)
					{
						fprintf(stderr, "32 Invalid value '%c' for crew class in tag \"%s\"\n", class, tag);
						e|=32;
						break;
					}
					if((state->crews[i].status=lookup_crew_status(status))==CSTATUS_NONE)
					{
						fprintf(stderr, "32 Invalid value \"%s\" for crew status in tag \"%s\"\n", status, tag);
						e|=32;
						break;
					}
					if(state->crews[i].status!=CSTATUS_ESCAPEE)
						state->crews[i].assignment=-1;
					if(gcmid(p_id, &state->crews[i].id))
					{
						fprintf(stderr, "32 Invalid value \"%s\" for c/m ID in tag \"%s\"\n", p_id, tag);
						e|=32;
						break;
					}
				}
			}
		}
		else if(strcmp(tag, "GProd")==0)
		{
			unsigned int snclass;
			f=sscanf(dat, "%u\n", &snclass);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(snclass!=ICLASS_MIXED)
			{
				fprintf(stderr, "2 Value mismatch: different iclasses value (%u!=%u)\n", snclass, ICLASS_MIXED);
				e|=2;
			}
			else
			{
				for(unsigned int i=0;i<snclass;i++)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					if(*line=='#')
					{
						i--;
						continue;
					}
					unsigned int j;
					f=sscanf(line, "IClass %u:"FLOAT"\n", &j, CAST_FLOAT_PTR &state->gprod[i]);
					if(f!=2)
					{
						fprintf(stderr, "1 Too few arguments to part %u of tag \"%s\"\n", i, tag);
						e|=1;
						break;
					}
					if(j!=i)
					{
						fprintf(stderr, "4 Index mismatch in part %u (%u?) of tag \"%s\"\n", i, j, tag);
						e|=4;
						break;
					}
				}
			}
		}
		else if(strcmp(tag, "FTypes")==0)
		{
			unsigned int snftypes;
			f=sscanf(dat, "%u\n", &snftypes);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(snftypes!=nftypes)
			{
				fprintf(stderr, "2 Value mismatch: different nftypes value (%u!=%u)\n", snftypes, nftypes);
				e|=2;
			}
		}
		else if(strcmp(tag, "FBases")==0)
		{
			unsigned int snfbases;
			f=sscanf(dat, "%u\n", &snfbases);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(snfbases!=nfbases)
			{
				fprintf(stderr, "2 Value mismatch: different nfbases value (%u!=%u)\n", snfbases, nfbases);
				e|=2;
			}
		}
		else if(strcmp(tag, "Fighters")==0)
		{
			f=sscanf(dat, "%u\n", &state->nfighters);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else
			{
				free(state->fighters);
				state->fighters=malloc(state->nfighters*sizeof(ac_fighter));
				for(unsigned int i=0;i<state->nfighters;i++)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					unsigned int j;
					unsigned int base;
					unsigned int flags;
					char p_id[9];
					f=sscanf(line, "Type %u:%u,%u,%8s\n", &j, &base, &flags, p_id);
					if(f!=4)
					{
						fprintf(stderr, "1 Too few arguments to part %u of tag \"%s\"\n", i, tag);
						e|=1;
						break;
					}
					if(j>nftypes)
					{
						fprintf(stderr, "4 Index mismatch in part %u of tag \"%s\"\n", j, tag);
						e|=4;
						break;
					}
					state->fighters[i]=(ac_fighter){.type=j, .base=base, .radar=flags&1};
					if(gacid(p_id, &state->fighters[i].id))
					{
						fprintf(stderr, "32 Invalid value \"%s\" for a/c ID in tag \"%s\"\n", p_id, tag);
						e|=32;
						break;
					}
				}
			}
		}
		else if(strcmp(tag, "Flaks")==0)
		{
			unsigned int snflaks;
			f=sscanf(dat, "%u\n", &snflaks);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(snflaks!=nflaks)
			{
				fprintf(stderr, "2 Value mismatch: different nflaks value (%u!=%u)\n", snflaks, nflaks);
				e|=2;
			}
			else
			{
				for(unsigned int i=0;i<nflaks;i++)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					unsigned int j;
					unsigned int flags;
					f=sscanf(line, "Site %u:%u\n", &j, &flags);
					if(f!=2)
					{
						fprintf(stderr, "1 Too few arguments to part %u of tag \"%s\"\n", i, tag);
						e|=1;
						break;
					}
					if(j!=i)
					{
						fprintf(stderr, "4 Index mismatch in part %u (%u?) of tag \"%s\"\n", j, i, tag);
						e|=4;
						break;
					}
					flaks[i].mapped=flags&1;
				}
			}
		}
		else if(strcmp(tag, "Targets init")==0)
		{
			double dmg,flk,heat,flam;
			f=sscanf(dat, FLOAT","FLOAT","FLOAT","FLOAT"\n", CAST_FLOAT_PTR &dmg, CAST_FLOAT_PTR &flk, CAST_FLOAT_PTR &heat, CAST_FLOAT_PTR &flam);
			if(f!=4)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
				break;
			}
			for(unsigned int i=0;i<ntargs;i++)
			{
				state->dmg[i]=dmg;
				state->flk[i]=flk*targs[i].flak/100.0;
				state->heat[i]=heat;
				state->flam[i]=flam;
			}
		}
		else if(strcmp(tag, "Targets")==0)
		{
			unsigned int sntargs;
			f=sscanf(dat, "%u\n", &sntargs);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(sntargs!=ntargs)
			{
				fprintf(stderr, "2 Value mismatch: different ntargs value (%u!=%u)\n", sntargs, ntargs);
				e|=2;
			}
			else
			{
				for(unsigned int i=0;i<ntargs;i++)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					if(*line=='#')
					{
						i--;
						continue;
					}
					unsigned int j;
					double dmg, flk, heat, flam;
					f=sscanf(line, "Targ %u:"FLOAT","FLOAT","FLOAT","FLOAT"\n", &j, CAST_FLOAT_PTR &dmg, CAST_FLOAT_PTR &flk, CAST_FLOAT_PTR &heat, CAST_FLOAT_PTR &flam);
					if(f!=5)
					{
						fprintf(stderr, "1 Too few arguments to part %u of tag \"%s\"\n", j, tag);
						e|=1;
						break;
					}
					while((i<j)&&(i<ntargs))
					{
						state->dmg[i]=100;
						state->flk[i]=targs[i].flak;
						state->heat[i]=0;
						state->flam[i]=40;
						i++;
					}
					if(j!=i)
					{
						fprintf(stderr, "4 Index mismatch in part %u (%u?) of tag \"%s\"\n", i, j, tag);
						e|=4;
						break;
					}
					state->dmg[i]=dmg;
					state->flk[i]=flk*targs[i].flak/100.0;
					state->heat[i]=heat;
					state->flam[i]=flam;
				}
			}
		}
		else if(strcmp(tag, "Weather state")==0)
		{
			state->weather.seed=0;
			f=sscanf(dat, FLOAT","FLOAT"\n", CAST_FLOAT_PTR &state->weather.push, CAST_FLOAT_PTR &state->weather.slant);
			if(f!=2)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else
			{
				free(line);
				line=fgetl(fs);
				if(!line)
				{
					fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
					e|=64;
				}
				for(unsigned int x=0;x<256;x++)
				{
					size_t p=0;
					for(unsigned int y=0;y<128;y++)
					{
						int bytes;
						if(sscanf(line+p, FLOAT","FLOAT",%n", CAST_FLOAT_PTR &state->weather.p[x][y], CAST_FLOAT_PTR &state->weather.t[x][y], &bytes)!=2)
						{
							fprintf(stderr, "1 Too few arguments to part (%u,%u) of tag \"%s\"\n", x, y, tag);
							e|=1;
							break;
						}
						p+=bytes;
					}
					if(e) break;
					if(x==255) break;
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
				}
			}
		}
		else if(strcmp(tag, "Weather rand")==0)
		{
			unsigned int seed;
			f=sscanf(dat, "%u\n", &seed);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(!seed)
			{
				fprintf(stderr, "32 Invalid seed %u to tag \"%s\"\n", seed, tag);
				e|=32;
			}
			else
				state->weather.seed=seed;
		}
		else if(strcmp(tag, "Evented")==0)
		{
			unsigned int event;
			if(!strncmp(dat, "EVENT_", 6))
			{
				char *nl=strchr(dat, '\n');
				if(nl) *nl=0;
				int ev=find_event(dat+6);
				if(ev<0)
				{
					fprintf(stderr, "32 Invalid value for event \"%s\" in tag \"%s\"\n", dat, tag);
					e|=32;
				}
				else
					state->evented[ev]=true;
			}
			else
			{
				f=sscanf(dat, "%u\n", &event);
				if(f!=1)
				{
					fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
					e|=1;
				}
				else if(event>=NEVENTS)
				{
					fprintf(stderr, "32 Invalid value for event %u in tag \"%s\"\n", event, tag);
					e|=32;
				} else
					state->evented[event]=true;
			}
		}
		else if(strcmp(tag, "Messages")==0)
		{
			unsigned int snmsgs;
			f=sscanf(dat, "%u\n", &snmsgs);
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if(snmsgs > MAXMSGS)
			{
				fprintf(stderr, "2 Value mismatch: too many messages (%u>%u)\n", snmsgs, MAXMSGS);
				e|=2;
			}
			else
			{
				for(unsigned int i=0;i<MAXMSGS;i++)
				{
					free(state->msg[i]);
					state->msg[i]=NULL;
				}
				unsigned int msg=0;
				size_t len=0;
				while(msg<snmsgs)
				{
					free(line);
					line=fgetl(fs);
					if(!line)
					{
						fprintf(stderr, "64 Unexpected EOF in tag \"%s\"\n", tag);
						e|=64;
						break;
					}
					if(strcmp(line, ".")==0)
					{
						msg++;
						len=0;
					}
					else
					{
						size_t nl=len+strlen(line);
						char *nmsg=realloc(state->msg[msg], nl+2);
						if(!nmsg)
						{
							perror("16 realloc");
							e|=16;
						}
						else
						{
							state->msg[msg]=nmsg;
							strcpy(state->msg[msg]+len, line);
							strcpy(state->msg[msg]+nl, "\n");
							len=nl+1;
						}
					}
				}
			}
		}
		else if(strcmp(tag, "History")==0)
		{
			#ifdef WINDOWS
			unsigned long nents;
			f=sscanf(dat, "%lu\n", &nents);
			#else
			size_t nents;
			f=sscanf(dat, "%zu\n", &nents);
			#endif
			if(f!=1)
			{
				fprintf(stderr, "1 Too few arguments to tag \"%s\"\n", tag);
				e|=1;
			}
			else if((f=hist_load(fs, nents, &state->hist)))
			{
				fprintf(stderr, "32 Invalid value: error %d somewhere in hist_load, in tag \"%s\"\n", f, tag);
				e|=32;
			}
		}
		else
		{
			fprintf(stderr, "128 Bad tag \"%s\"\n", tag);
			e|=128;
		}
		free(line);
		if(e!=0) // ...catch
		{
			fprintf(stderr, "Error (%d) reading from %s\n", e, fn);
			fclose(fs);
			return(e);
		}
	}
	filter_pff=0;
	filter_elite=0;
	for(unsigned int n=0;n<NNAVAIDS;n++)
		filter_nav[n]=0;
	for(unsigned int i=0;i<ntypes;i++)
		types[i]=rawtypes[i];
	for(unsigned int m=0;m<nmods;m++)
		if(!datebefore(state->now, mods[m].d))
			apply_mod(m);
	/* compat for pre-tpipe games */
	if(!datebefore(state->now, event[EVENT_HCU]) && state->tpipe[TPIPE_HCU].dwell == -1)
	{
		state->tpipe[TPIPE_OTU].cont=50;
		state->tpipe[TPIPE_HCU].dwell=30;
	}
	if(!datebefore(state->now, event[EVENT_LFS]) && state->tpipe[TPIPE_LFS].dwell == -1)
	{
		state->tpipe[TPIPE_HCU].cont=50;
		state->tpipe[TPIPE_LFS].dwell=10;
	}
	return(0);
}

int savegame(const char *fn, game state)
{
	FILE *fs = fopen(fn, "w");
	if(!fs)
	{
		fprintf(stderr, "Failed to open %s!\n", fn);
		perror("fopen");
		return(1);
	}
	char p_id[17];
	fprintf(fs, "HARR:%u.%u.%u\n", VER_MAJ, VER_MIN, VER_REV);
	fprintf(fs, "DATE:%02d-%02d-%04d\n", state.now.day, state.now.month, state.now.year);
	fprintf(fs, "DClasses:%u\n", DIFFICULTY_CLASSES);
	for(unsigned int i=0;i<DIFFICULTY_CLASSES;i++)
		fprintf(fs, "Difficulty:%u,%u\n", i, state.difficulty[i]);
	fprintf(fs, "Confid:"FLOAT"\n", CAST_FLOAT state.confid);
	fprintf(fs, "Morale:"FLOAT"\n", CAST_FLOAT state.morale);
	fprintf(fs, "Budget:%u+%u\n", state.cash, state.cshr);
	fprintf(fs, "TPrio:%u,%u,%u,%u\n", state.tfav[0], state.tfd[0], state.tfav[1], state.tfd[1]);
	fprintf(fs, "IPrio:%u,%u,%u,%u\n", state.ifav[0], state.ifd[0], state.ifav[1], state.ifd[1]);

	fprintf(fs, "Types:%u\n", ntypes);
	for(unsigned int i=0;i<ntypes;i++)
		if(state.btypes[i])
			fprintf(fs, "Prio %u:%u,%u,%u,%u\n", i, types[i].prio, types[i].pribuf, types[i].pc, types[i].pcbuf);
		else
			fprintf(fs, "NoType %u:\n", i);
	fprintf(fs, "Navaids:%u\n", NNAVAIDS);
	for(unsigned int n=0;n<NNAVAIDS;n++)
		fprintf(fs, "NPrio %u:%d,%u\n", n, state.nap[n], state.napb[n]);
	fprintf(fs, "Bombers:%u\n", state.nbombers);
	for(unsigned int i=0;i<state.nbombers;i++)
	{
		unsigned int nav=0;
		for(unsigned int n=0;n<NNAVAIDS;n++)
			nav|=(state.bombers[i].nav[n]?(1<<n):0);
		pacid(state.bombers[i].id, p_id);
		fprintf(fs, "Type %u:%u,%u,%u,%s,%u\n", state.bombers[i].type, state.bombers[i].failed?1:0, nav, state.bombers[i].pff?1:0, p_id, state.bombers[i].mark);
	}
	fprintf(fs, "TPipe:%u\n", TPIPE__MAX);
	for(enum tpipe i=0;i<TPIPE__MAX;i++)
		fprintf(fs, "TStage %u:%d,%u\n", i, state.tpipe[i].dwell, state.tpipe[i].cont);
	fprintf(fs, "Crews:%u\n", state.ncrews);
	for(unsigned int i=0;i<state.ncrews;i++)
	{
		pcmid(state.crews[i].id, p_id);
		fprintf(fs, "%s %c:%la,%u,%u,%u,%d,%s\n", cstatuses[state.crews[i].status], cclasses[state.crews[i].class].letter, state.crews[i].skill, state.crews[i].lrate, state.crews[i].tour_ops, state.crews[i].full_tours, state.crews[i].assignment, p_id);
	}
	fprintf(fs, "GProd:%u\n", ICLASS_MIXED);
	for(unsigned int i=0;i<ICLASS_MIXED;i++)
		fprintf(fs, "IClass %u:"FLOAT"\n", i, CAST_FLOAT state.gprod[i]);
	fprintf(fs, "FTypes:%u\n", nftypes);
	fprintf(fs, "FBases:%u\n", nfbases);
	fprintf(fs, "Fighters:%u\n", state.nfighters);
	for(unsigned int i=0;i<state.nfighters;i++)
	{
		pacid(state.fighters[i].id, p_id);
		unsigned int flags = 0;
		if(state.fighters[i].radar)
			flags |= 1;
		fprintf(fs, "Type %u:%u,%u,%s\n", state.fighters[i].type, state.fighters[i].base, flags, p_id);
	}
	fprintf(fs, "Flaks:%u\n", nflaks);
	for(unsigned int i=0;i<nflaks;i++)
	{
		unsigned int flags=(flaks[i].mapped?1:0);
		fprintf(fs, "Site %u:%u\n", i, flags);
	}
	fprintf(fs, "Targets:%u\n", ntargs);
	for(unsigned int i=0;i<ntargs;i++)
		fprintf(fs, "Targ %u:"FLOAT","FLOAT","FLOAT","FLOAT"\n", i, CAST_FLOAT state.dmg[i], CAST_FLOAT (targs[i].flak?state.flk[i]*100.0/(double)targs[i].flak:0), CAST_FLOAT state.heat[i], CAST_FLOAT state.flam[i]);
	fprintf(fs, "Weather state:"FLOAT","FLOAT"\n", CAST_FLOAT state.weather.push, CAST_FLOAT state.weather.slant);
	for(unsigned int x=0;x<256;x++)
	{
		for(unsigned int y=0;y<128;y++)
			fprintf(fs, FLOAT","FLOAT",", CAST_FLOAT state.weather.p[x][y], CAST_FLOAT state.weather.t[x][y]);
		fprintf(fs, "\n");
	}
	for(unsigned int i=0;i<NEVENTS;i++)
	{
		if (state.evented[i])
			fprintf(fs, "Evented:EVENT_%s\n", event_names[i]);
	}
	unsigned int msgs=0;
	for(unsigned int i=0;i<MAXMSGS;i++)
		if(state.msg[i]) msgs++;
	fprintf(fs, "Messages:%u\n", msgs);
	for(unsigned int i=0;i<MAXMSGS;i++)
		if(state.msg[i])
		{
			fputs(state.msg[i], fs);
			fputs(".\n", fs);
		}
	hist_save(state.hist, fs);
	fclose(fs);
	return(0);
}
