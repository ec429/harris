/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	stats/fighters: Extract fighter force data from game event log
*/

#include <stdio.h>
#include <unistd.h>
#include "bits.h"
#include "globals.h"
#include "hist_record.h"
#include "hashtable.h"
#include "load_data.h"
#include "saving.h"

struct fighter
{
	struct hlist list; // acid is list.hash
	unsigned int ftype;
	bool radar;
};

void write_counts(date d, unsigned int (*fcounts)[2])
{
	char outbuf[80];
	unsigned int i;

	writedate(d, outbuf);
	for (i = 0; i < nftypes; i++)
		printf("%s %u %u %u\n", outbuf, i, fcounts[i][0], fcounts[i][1]);
}

int main(int argc, char **argv)
{
	struct hist_record rec;
	char *line;
	game initgame;
	struct hashtable fighters;
	bool localdat = false;
	char cwd_buf[1024];
	unsigned int (*fcounts)[2];
	int arg;
	int rc;
	for (arg = 1; arg < argc; arg++)
	{
		if (!strcmp(argv[arg], "--localdat"))
		{
			localdat=true;
		}
		else
		{
			fprintf(stderr, "Unrecognised argument '%s'\n", argv[arg]);
			return 5;
		}
	}

	if(!(cwd=getcwd(cwd_buf, 1024)))
	{
		perror("getcwd");
		return 5;
	}

	fprintf(stderr, "Loading data files...\n");
	
#ifndef WINDOWS
	if(chdir(localdat?cwd:DATIDIR))
	{
		perror("Failed to enter data dir: chdir");
		if(!localdat)
			fprintf(stderr, "(Maybe try with --localdat, or make install)\n");
		return 5;
	}
#endif

	rc = load_data();
	if (rc)
		return rc;
	fcounts = calloc(nftypes, sizeof(*fcounts));
	if (!fcounts)
	{
		perror("calloc");
		return 5;
	}
	rc = set_init_state(&initgame);
	if (rc)
		return rc;
	
	fprintf(stderr, "Data files loaded\n");

	rc = hashtable_create(&fighters, 2);
	if (rc)
	{
		fprintf(stderr, "Error %d creating hashtable 'fighters'\n", rc);
		return 5;
	}

	while(!feof(stdin))
	{
		line = fgetl(stdin);
		if (!line)
		{
			fprintf(stderr, "Unexpected EOF\n");
			return 3;
		}
		if (!strncmp(line, "History:", 8))
		{
			free(line);
			break;
		}
	}

	while(!feof(stdin))
	{
		line = fgetl(stdin);
		if (!line) break;
		rc = parse_event(&rec, line);
		if (rc)
		{
			fprintf(stderr, "Error %d in line: %s\n", rc, line);
			free(line);
			return rc;
		}
		free(line);
		if (rec.type == HT_INIT)
		{
			char savbuf[80];

			snprintf(savbuf, 80, "save/%s", rec.init);
			rc = loadgame(savbuf, &initgame);
			if (rc)
			{
				fprintf(stderr, "Error %d loading INITgame '%s'\n", rc, savbuf);
				return 6;
			}
			for (unsigned int i = 0; i < initgame.nfighters; i++)
			{
				struct fighter *f = malloc(sizeof(*f));
				if (!f)
				{
					perror("malloc");
					return 5;
				}
				f->list.hash = initgame.fighters[i].id;
				f->ftype = initgame.fighters[i].type;
				f->radar = initgame.fighters[i].radar;
				fcounts[f->ftype][0]++;
				fcounts[f->ftype][1] += f->radar;
				hashtable_insert(&fighters, &f->list);
			}
			write_counts(rec.date, fcounts);
		}
		else if (rec.type == HT_AC)
		{
			if (rec.ac.type == AE_CT)
			{
				if (rec.ac.fighter)
				{
					struct fighter *f = malloc(sizeof(*f));
					if (!f)
					{
						perror("malloc");
						return 5;
					}
					f->list.hash = rec.ac.id;
					f->ftype = rec.ac.ac_type;
					f->radar = false;
					fcounts[f->ftype][0]++;
					hashtable_insert(&fighters, &f->list);
				}
			}
			else if (rec.ac.type == AE_NAV)
			{
				struct hlist *fl;
				struct fighter *f;

				if (rec.ac.fighter)
				{
					fl = hashtable_lookup(&fighters, rec.ac.id);
					if (!fl)
					{
						fprintf(stderr, "Radar installed in unknown fighter %08x\n", rec.ac.id);
						return 7;
					}
					else
					{
						f = container_of(fl, struct fighter, list);
					}
					f->radar = true;
					fcounts[f->ftype][1]++;
				}
			}
			else if (rec.ac.type == AE_CROB)
			{
				if (rec.ac.fighter)
				{
					struct hlist *fl = hashtable_lookup(&fighters, rec.ac.id);
					struct fighter *f;

					if (!fl)
					{
						fprintf(stderr, "Unknown fighter %08x crashed or scrapped\n", rec.ac.id);
						return 7;
					}
					f = container_of(fl, struct fighter, list);
					fcounts[f->ftype][0]--;
					fcounts[f->ftype][1] -= f->radar;
				}
			}
		}
		else if (rec.type == HT_MISC && rec.misc.type == ME_GPROD && rec.misc.gprod.iclass == 0)
		{
			write_counts(rec.date, fcounts);
		}
	}
	return 0;
}
