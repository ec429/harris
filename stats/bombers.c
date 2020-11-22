/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	stats/bombers: Extract bomber production/loss data from game event log
*/

#include <stdio.h>
#include <unistd.h>
#include "bits.h"
#include "globals.h"
#include "hist_record.h"
#include "hashtable.h"
#include "load_data.h"
#include "saving.h"

void write_counts(date d, unsigned int (*bcounts)[3], int ca[2])
{
	char outbuf[160];
	unsigned int i, k;
	size_t wp;

	writedate(d, outbuf);
	printf("@ %s\n", outbuf);
	for (k = 0; k < 3; k++)
	{
		outbuf[0] = "=+-"[k];
		outbuf[wp = 1] = 0;
		for (i = 0; i < ntypes; i++)
		{
			wp += snprintf(outbuf + wp, 80 - wp, " %u", bcounts[i][k]);
			if (k)
				bcounts[i][k] = 0;
		}
		printf("%s\n", outbuf);
	}
	printf("$ %d %d\n", ca[0], ca[1]);
}

int main(int argc, char **argv)
{
	struct hist_record rec;
	char *line;
	game initgame;
	bool localdat = false;
	char cwd_buf[1024];
	unsigned int (*bcounts)[3];
	int ca[2] = {0, 0};
	date current = {0, 0, 0};
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
	bcounts = calloc(ntypes, sizeof(*bcounts));
	if (!bcounts)
	{
		perror("calloc");
		return 5;
	}
	rc = set_init_state(&initgame);
	if (rc)
		return rc;

	fprintf(stderr, "Data files loaded\n");

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
			for (unsigned int i = 0; i < initgame.nbombers; i++)
				bcounts[initgame.bombers[i].type][0]++;
			ca[0] = initgame.cshr;
			ca[1] = initgame.cash;
		}
		else if (rec.type == HT_AC)
		{
			if (rec.ac.type == AE_CT)
			{
				if (!rec.ac.fighter)
				{
					bcounts[rec.ac.ac_type][0]++;
					bcounts[rec.ac.ac_type][1]++;
				}
			}
			else if (rec.ac.type == AE_CROB)
			{
				if (!rec.ac.fighter)
				{
					bcounts[rec.ac.ac_type][0]--;
					bcounts[rec.ac.ac_type][2]++;
				}
			}
		}
		else if (rec.type == HT_MISC)
		{
			if (rec.misc.type == ME_CASH)
			{
				ca[0] = rec.misc.cash.delta;
				ca[1] = rec.misc.cash.current;
			}
		}
		if (diffdate(rec.date, current))
			write_counts(current = rec.date, bcounts, ca);
	}
	return 0;
}
