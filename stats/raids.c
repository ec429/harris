/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	stats/raids: Extract raids & losses data from game event log
*/

#include <stdio.h>
#include "bits.h"
#include "hist_record.h"
#include "saving.h"
#include "hashtable.h"

struct raid // records what target a bomber was raiding
{
	struct hlist list; // bomber acid is list.hash
	unsigned int type;
	unsigned int targ;
};

struct result
{
	unsigned int type, targ;
	unsigned int raids, losses;
	struct result *next;
};

struct result *find_or_add(struct result *head, unsigned int type, unsigned int targ);
void write_result_line(date d, struct result *results);
void free_results(struct result *head);
void dump_debug_data(struct result *head, struct hashtable raids);

int main(void)
{
	struct hist_record rec;
	char *line;
	struct result *results = NULL;
	struct hashtable raids;
	date current = {0, 0, 0};
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
		free(line);
	}

	rc = hashtable_create(&raids, 2);
	if (rc)
	{
		fprintf(stderr, "Error %d creating hashtable 'raids'\n", rc);
		return 5;
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

		if (diffdate(rec.date, current))
		{
			if (current.year)
				write_result_line(current, results);
			free_results(results);
			results = NULL;
			hashtable_wipe(&raids);
			current = rec.date;
		}

		if (rec.type == HT_AC)
		{
			if (rec.ac.type == AE_RAID)
			{
				struct result *res = find_or_add(results, rec.ac.ac_type, rec.ac.raid.targ);
				struct raid *r;
				if (!res) // malloc failed, we already perror()ed
					return 5;
				if (!results)
					results = res;
				res->raids++;
				if (!(r = malloc(sizeof(*r))))
				{
					perror("malloc");
					return 5;
				}
				*r = (struct raid){.list.hash = rec.ac.id, .type = rec.ac.ac_type, .targ = rec.ac.raid.targ};
				hashtable_insert(&raids, &r->list);
			}
			else if (rec.ac.type == AE_CROB && rec.ac.crob.ty == CROB_CR && !rec.ac.fighter)
			{
				struct hlist *lr;
				struct raid *r;
				struct result *res;
				if (!(lr = hashtable_lookup(&raids, rec.ac.id)))
				{
					printf("Unknown raid %08x crashed\n", rec.ac.id);
					dump_debug_data(results, raids);
					return 7;
				}
				r = container_of(lr, struct raid, list);
				if (!(res = find_or_add(results, rec.ac.ac_type, r->targ))) // Can't happen, there should have been a RA earlier
					return 5;
				if (!results) // ditto
					results = res;
				res->losses++;
			}
		}
	}
	write_result_line(current, results);
	return 0;
}

struct result *new_result(unsigned int type, unsigned int targ)
{
	struct result *new;

	new = malloc(sizeof(*new));
	if (!new)
	{
		perror("malloc");
		return NULL;
	}
	*new = (struct result){.type=type, .targ=targ, .raids=0, .losses=0, .next=NULL};
	return new;
}

struct result *find_or_add(struct result *head, unsigned int type, unsigned int targ)
{
	struct result *prev;

	if (!head)
	{
		return new_result(type, targ);
	}
	for (prev = head; head; prev = head, head = head->next)
		if (head->type == type && head->targ == targ)
			break;
	if (!head)
		prev->next = head = new_result(type, targ);
	return head;
}

void write_result_line(date d, struct result *results)
{
	char outbuf[80];

	writedate(d, outbuf);
	printf("@ %s\n", outbuf);
	for (; results; results = results->next)
	{
		printf("%u,%u %u,%u\n", results->type, results->targ, results->raids, results->losses);
	}
}

void free_results(struct result *head)
{
	struct result *next;

	for(; head; head = next)
	{
		if (head)
		{
			next = head->next;
			head->next = (struct result *)LIST_POISON;
		}
		else
		{
			next = NULL;
		}
		free(head);
	}
}

void dump_debug_data(struct result *head, struct hashtable raids)
{
	struct hlist *bucket, *lr;
	struct raid *r;
	unsigned int bi;

	write_result_line((date){9999, 99, 99}, head);
	puts("");
	FOR_ALL_HASH_BUCKETS(&raids, bucket, bi)
		FOR_ALL_BUCKET_ITEMS(bucket, lr)
		{
			r = container_of(lr, struct raid, list);
			printf("%08x: %u,%u\n", (unsigned int)lr->hash, r->type, r->targ);
		}
}
