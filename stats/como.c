/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	stats/como: Extract confidence & morale data from game event log
*/

#include <stdio.h>
#include "bits.h"
#include "hist_record.h"
#include "saving.h"

int main(void)
{
	struct hist_record rec;
	char *line;
	char outbuf[80];
	int rc;

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
		if (rec.type == HT_MISC)
		{
			if (rec.misc.type == ME_CONFID)
			{
				writedate(rec.date, outbuf);
				printf("%s CO "FLOAT"\n", outbuf, CAST_FLOAT rec.misc.confid.confid);
			}
			else if (rec.misc.type == ME_MORALE)
			{
				writedate(rec.date, outbuf);
				printf("%s MO "FLOAT"\n", outbuf, CAST_FLOAT rec.misc.morale.morale);
			}
		}
	}
	return 0;
}
