/* SPDX-License-Identifier: GPL-2.0 */

/* list.h lovingly copied from Linux kernel. */
#include <stddef.h>

#ifndef _LIST_H
#define _LIST_H

#define LIST_POISON1	(void *)0x100
#define LIST_POISON2	(void *)0x122

struct list_head {
	struct list_head *prev, *next;
};

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline void list_add_tail(struct list_head *head, struct list_head *new)
{
	struct list_head *tail = head->prev;

	head->prev = new;
	tail->next = new;
	new->next = head;
	new->prev = tail;
}

static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}

static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

/* We skip the type checking from Linux' container_of(), because the type
 * is always list_head here.  And we never pass 'member', we just assume
 * it's always 'list'.  This may bite us if we ever need to put a struct
 * onto two different lists...
 */
#define list_entry(ptr, type) ({					\
	void *__mptr = (void *)(ptr);					\
	((type *)(__mptr - offsetof(type, list))); })
#define list_first_entry(ptr, type)				        \
	list_entry((ptr)->next, type)
#define list_next_entry(pos)						\
	list_entry((pos)->list.next, typeof(*(pos)))
#define list_entry_is_head(pos, head)					\
	(&pos->list == (head))
#define list_for_each_entry(pos, head)					\
	for (pos = list_first_entry(head, typeof(*pos));		\
	     !list_entry_is_head(pos, head);				\
	     pos = list_next_entry(pos))

#endif // _LIST_H
