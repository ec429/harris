/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	hashtable: associative-array hashtable
*/

#include <stdint.h>

#define LIST_POISON 0x00100100

struct hlist
{
	uint64_t hash;
	struct hlist *next;
	struct hlist *prev;
};

/* Allocated size of buckets depends on hbytes; it equals 24 * (1 << (hbytes << 3)).
 * For hbytes=1 this is 6144 bytes (6kB); for hbytes=2 it is 1.5MB
 */
struct hashtable
{
	unsigned int hbytes; // use this many bytes from LSB end of hash as bucket ID
	struct hlist **buckets;
};

int hashtable_create(struct hashtable *tbl, unsigned int hbytes);
void hashtable_insert(struct hashtable *tbl, struct hlist *item);
void hashtable_remove(struct hashtable *tbl, struct hlist *item);
struct hlist *hashtable_lookup(struct hashtable *tbl, uint64_t hash);
void hashtable_wipe(struct hashtable *tbl);

#define FOR_ALL_HASH_BUCKETS(_tbl, _bucket, _bi) \
	for (_bi = 0, (_bucket) = (_tbl)->buckets[0]; !(_bi >> ((_tbl)->hbytes << 3)); (_bucket) = (_tbl)->buckets[++_bi])
#define FOR_ALL_BUCKET_ITEMS(_bucket, _item) \
	for ((_item) = (_bucket); (_item); (_item) = (_item)->next)
