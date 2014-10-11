/*
	harris - a strategy game
	Copyright (C) 2012-2014 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	saving: functions to save and load games from file
*/

#include "saving.h"
#include <stdio.h>
#include <string.h>
#include "bits.h"
#include "date.h"
#include "globals.h"
#include "history.h"
#include "rand.h"
#include "version.h"

#ifdef WINDOWS /* Because of this little bugger, savegames from Windows won't work on Linux/Unix and vice-versa */
#define FLOAT	"%llx"
#define CAST_FLOAT_PTR	(unsigned long long *)
#define CAST_FLOAT	(unsigned long long)
#else
#define FLOAT	"%la"
#define CAST_FLOAT_PTR
#define CAST_FLOAT
#endif

bool version_newer(const unsigned char v1[3], const unsigned char v2[3]) // true iff v1 newer than v2
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
	unsigned char s_version[3]={0,0,0};
	unsigned char version[3]={VER_MAJ,VER_MIN,VER_REV};
	bool warned_pff=false, warned_acid=false;
	state->weather.seed=0;
	for(unsigned int i=0;i<DIFFICULTY_CLASSES;i++) // default everything to medium
		state->difficulty[i]=1;
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
			f=sscanf(dat, "%hhu.%hhu.%hhu\n", s_version, s_version+1, s_version+2);
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
				fprintf(stderr, "2 Value mismatch: different DClasses value\n");
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
				fprintf(stderr, "2 Value mismatch: different ntypes value\n");
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
						types[j].prio=prio;
						types[j].pribuf=pribuf;
						types[j].pc=pc;
						types[j].pcbuf=pcbuf;
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
				fprintf(stderr, "2 Value mismatch: different nnav value\n");
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
					unsigned int failed,nav,pff;
					char p_id[9];
					f=sscanf(line, "Type %u:%u,%u,%u,%8s\n", &j, &failed, &nav, &pff, p_id);
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
					if(f!=5)
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
					state->bombers[i]=(ac_bomber){.type=j, .failed=failed, .pff=pff};
					for(unsigned int n=0;n<NNAVAIDS;n++)
						state->bombers[i].nav[n]=(nav>>n)&1;
					if(gacid(p_id, &state->bombers[i].id))
					{
						fprintf(stderr, "32 Invalid value \"%s\" for a/c ID in tag \"%s\"\n", p_id, tag);
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
				fprintf(stderr, "2 Value mismatch: different iclasses value\n");
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
				fprintf(stderr, "2 Value mismatch: different nftypes value\n");
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
				fprintf(stderr, "2 Value mismatch: different nfbases value\n");
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
				fprintf(stderr, "2 Value mismatch: different ntargs value\n");
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
				fprintf(stderr, "2 Value mismatch: too many messages\n");
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
	char p_id[9];
	fprintf(fs, "HARR:%hhu.%hhu.%hhu\n", VER_MAJ, VER_MIN, VER_REV);
	fprintf(fs, "DATE:%02d-%02d-%04d\n", state.now.day, state.now.month, state.now.year);
	fprintf(fs, "DClasses:%u\n", DIFFICULTY_CLASSES);
	for(unsigned int i=0;i<DIFFICULTY_CLASSES;i++)
		fprintf(fs, "Difficulty:%u,%u\n", i, state.difficulty[i]);
	fprintf(fs, "Confid:"FLOAT"\n", CAST_FLOAT state.confid);
	fprintf(fs, "Morale:"FLOAT"\n", CAST_FLOAT state.morale);
	fprintf(fs, "Budget:%u+%u\n", state.cash, state.cshr);
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
		fprintf(fs, "Type %u:%u,%u,%u,%s\n", state.bombers[i].type, state.bombers[i].failed?1:0, nav, state.bombers[i].pff?1:0, p_id);
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
	fprintf(fs, "Targets:%hhu\n", ntargs);
	for(unsigned int i=0;i<ntargs;i++)
		fprintf(fs, "Targ %hhu:"FLOAT","FLOAT","FLOAT","FLOAT"\n", i, CAST_FLOAT state.dmg[i], CAST_FLOAT (targs[i].flak?state.flk[i]*100.0/(double)targs[i].flak:0), CAST_FLOAT state.heat[i], CAST_FLOAT state.flam[i]);
	fprintf(fs, "Weather state:"FLOAT","FLOAT"\n", CAST_FLOAT state.weather.push, CAST_FLOAT state.weather.slant);
	for(unsigned int x=0;x<256;x++)
	{
		for(unsigned int y=0;y<128;y++)
			fprintf(fs, FLOAT","FLOAT",", CAST_FLOAT state.weather.p[x][y], CAST_FLOAT state.weather.t[x][y]);
		fprintf(fs, "\n");
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
