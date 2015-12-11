/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	hashtable: associative-array hashtable
*/

#include "hashtable.h"
#include <stdio.h>
#include <stdlib.h>

int hashtable_create(struct hashtable *tbl, unsigned int hbytes)
{
	tbl->hbytes = hbytes;
	tbl->buckets = calloc(1 << (hbytes << 3), sizeof(struct hlist *));
	if (!tbl->buckets)
	{
		perror("calloc");
		return 1;
	}
	return 0;
}

void hashtable_insert(struct hashtable *tbl, struct hlist *item)
{
	uint64_t bucket_mask = (1 << (tbl->hbytes << 3)) - 1;
	unsigned int bucket_id = item->hash & bucket_mask;
	struct hlist *bucket = tbl->buckets[bucket_id];

	item->next = bucket;
	if (bucket)
		bucket->prev = item;
	item->prev = NULL;
	tbl->buckets[bucket_id] = item;
}

void hashtable_remove(struct hashtable *tbl, struct hlist *item)
{
	uint64_t bucket_mask = (1 << (tbl->hbytes << 3)) - 1;
	unsigned int bucket_id = item->hash & bucket_mask;

	if (item->prev)
		item->prev->next = item->next;
	else
		tbl->buckets[bucket_id] = item->next;
	if (item->next)
		item->next->prev = item->prev;
	item->next = item->prev = (struct hlist *)LIST_POISON;
}

struct hlist *hashtable_lookup(struct hashtable *tbl, uint64_t hash)
{
	uint64_t bucket_mask = (1 << (tbl->hbytes << 3)) - 1;
	unsigned int bucket_id = hash & bucket_mask;
	struct hlist *bucket = tbl->buckets[bucket_id], *item;

	/* Search until we hit NULL, or find the one we want.
	 * Either way, item will contain the right thing to return.
	 */
	for (item = bucket; item && item->hash != hash; item=item->next)
		;
	return item;
}
