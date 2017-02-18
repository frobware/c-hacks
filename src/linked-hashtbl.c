/* Copyright (c) 2009, 2010, 2017 <Andrew McDermott>
 *
 * Source can be cloned from:
 *
 *     https://github.com/frobware/c-hacks.git
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * A hash table implementation based on external chaining.
 *
 * Each slot in the bucket array is a pointer to a linked list that
 * contains the key-value pairs that are hashed to the same location.
 * Lookup requires scanning the hashed slot's list for a match with
 * the given key.  Insertion requires adding a new entry to the head
 * of the list in the hashed slot.  Removal requires searching the
 * list and unlinking the element in the list.	The hash table also
 * maintains a doubly-linked list running through all of the entries;
 * this list is used for iteration, which is normally the order in
 * which keys are inserted into the table.  Alternatively, iteration
 * can be based on access.
 */

#include <stddef.h>		/* size_t, offsetof, NULL */
#include <stdlib.h>		/* malloc, free */
#include <string.h>		/* strcmp */
#if !defined(_MSC_VER)
#include <stdint.h>		/* intptr_t */
#endif
#include <c-hacks/linked-hashtbl.h>

#define UNUSED_PARAMETER(X) (void)(X)

#ifndef LINKED_HASHTBL_MAX_TABLE_SIZE
#define LINKED_HASHTBL_MAX_TABLE_SIZE (1 << 30)
#endif

#if defined(_MSC_VER)
#define INLINE __inline
#else
#define INLINE inline
#endif

#define LIST_ENTRY(PTR, TYPE, FIELD)			\
	(TYPE *)(((TYPE *)PTR) - offsetof(TYPE, FIELD))

struct l_hashtbl_list_head {
	struct l_hashtbl_list_head *next, *prev;
};

struct l_hashtbl {
	struct l_hashtbl_list_head all_entries;
	double max_load_factor;
	LINKED_HASHTBL_HASH_FN hash_fn;
	LINKED_HASHTBL_EQUALS_FN equals_fn;
	unsigned long nentries;
	int table_size;
	int resize_threshold;
	int auto_resize;
	int access_order;
	LINKED_HASHTBL_KEY_FREE_FN key_free_fn;
	LINKED_HASHTBL_VAL_FREE_FN val_free_fn;
	LINKED_HASHTBL_MALLOC_FN malloc_fn;
	LINKED_HASHTBL_FREE_FN free_fn;
	LINKED_HASHTBL_EVICTOR_FN evictor_fn;
	struct l_hashtbl_entry **table;
};

struct l_hashtbl_entry {
	struct l_hashtbl_list_head list;	/* all_entries list */
	struct l_hashtbl_entry *next;	/* per slot list */
	void *key;
	void *val;
	unsigned int hash;	/* hash of key */
};

static INLINE void list_init(struct l_hashtbl_list_head *head)
{
	head->next = head;
	head->prev = head;
}

/* Add node between head and head->next. */

static INLINE void list_add_before(struct l_hashtbl_list_head *node,
				   struct l_hashtbl_list_head *head)
{
	struct l_hashtbl_list_head *prev = head;
	struct l_hashtbl_list_head *next = head->next;
	node->next = next;
	node->prev = prev;
	prev->next = node;
	next->prev = node;
}

static INLINE void list_remove(struct l_hashtbl_list_head *node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
}

static INLINE int resize_threshold(int capacity, double max_load_factor)
{
	return (int)(((double)capacity * max_load_factor) + 0.5);
}

static INLINE unsigned int direct_hash(const void *k)
{
	/* Magic numbers from Java 1.4. */
	unsigned int h = (unsigned int)(uintptr_t) k;
	h ^= (h >> 20) ^ (h >> 12);
	return h ^ (h >> 7) ^ (h >> 4);
}

static INLINE int direct_equals(const void *a, const void *b)
{
	return a == b;
}

static int roundup_to_next_power_of_2(int x)
{
	int n = 1;
	while (n < x)
		n <<= 1;
	return n;
}

static int is_power_of_2(int x)
{
	return ((x & (x - 1)) == 0);
}

static INLINE void record_access(struct l_hashtbl *h,
				 struct l_hashtbl_entry *entry)
{
	if (h->access_order) {
		/* move to head of all_entries */
		list_remove(&entry->list);
		list_add_before(&entry->list, &h->all_entries);
	}
}

static INLINE struct l_hashtbl_entry **tbl_entry_ref(struct l_hashtbl *h,
						     unsigned int hashval)
{
	return &h->table[(int)hashval & (h->table_size - 1)];
}

static INLINE struct l_hashtbl_entry *tbl_entry(struct l_hashtbl *h,
						unsigned int hashval)
{
	return h->table[(int)hashval & (h->table_size - 1)];
}

static INLINE int remove_eldest(const struct l_hashtbl *h,
				unsigned long nentries)
{
	UNUSED_PARAMETER(h);
	UNUSED_PARAMETER(nentries);
	return 0;
}

static INLINE struct l_hashtbl_entry *find_entry(struct l_hashtbl *h,
						 unsigned int hv, const void *k)
{
	struct l_hashtbl_entry *entry = tbl_entry(h, hv);

	while (entry != NULL) {
		if (entry->hash == hv && h->equals_fn(entry->key, k))
			break;
		entry = entry->next;
	}

	return entry;
}

/*
 * Remove an entry from the hash table without deleting the underlying
 * instance.  Returns the entry, or NULL if not found.
 */
static struct l_hashtbl_entry *remove_key(struct l_hashtbl *h, const void *k)
{
	unsigned int hv = h->hash_fn(k);
	struct l_hashtbl_entry **slot_ref = tbl_entry_ref(h, hv);
	struct l_hashtbl_entry *entry = *slot_ref;

	while (entry != NULL) {
		if (entry->hash == hv && h->equals_fn(entry->key, k)) {
			/* advance previous node to next entry. */
			*slot_ref = entry->next;
			h->nentries--;
			list_remove(&entry->list);
			break;
		}
		slot_ref = &entry->next;
		entry = entry->next;
	}

	return entry;
}

int l_hashtbl_insert(struct l_hashtbl *h, void *k, void *v)
{
	struct l_hashtbl_entry *entry, **slot_ref;
	unsigned int hv = h->hash_fn(k);

	if ((entry = find_entry(h, hv, k)) != NULL) {
		/* Replace the current value. This should not affect
		 * the iteration order as the key already exists. */
		if (h->val_free_fn != NULL)
			h->val_free_fn(entry->val);
		entry->val = v;
		return 0;
	}

	if ((entry = h->malloc_fn(sizeof(*entry))) == NULL)
		return 1;

	entry->key = k;
	entry->val = v;
	entry->hash = hv;

	/* Link new entry at the head of the chain for this slot. */
	slot_ref = tbl_entry_ref(h, hv);
	entry->next = *slot_ref;

	/* Move new entry to the head of all entries. */
	*slot_ref = entry;
	list_add_before(&entry->list, &h->all_entries);

	h->nentries++;

	if (h->evictor_fn(h, h->nentries)) {
		/* Evict oldest entry. */
		struct l_hashtbl_list_head *node = h->all_entries.prev;
		entry = LIST_ENTRY(node, struct l_hashtbl_entry, list);
		l_hashtbl_remove(h, entry->key);
	}

	if (h->auto_resize) {
		if (h->nentries >= (unsigned int)h->resize_threshold) {
			/* auto resize failures are benign. */
			(void)l_hashtbl_resize(h, 2 * h->table_size);
		}
	}

	return 0;
}

void *l_hashtbl_lookup(struct l_hashtbl *h, const void *k)
{
	unsigned int hv = h->hash_fn(k);
	struct l_hashtbl_entry *entry = find_entry(h, hv, k);

	if (entry != NULL) {
		record_access(h, entry);
		return entry->val;
	}

	return NULL;
}

int l_hashtbl_remove(struct l_hashtbl *h, const void *k)
{
	struct l_hashtbl_entry *entry = remove_key(h, k);

	if (entry != NULL) {
		if (h->key_free_fn != NULL)
			h->key_free_fn(entry->key);
		if (h->val_free_fn != NULL && entry->val != NULL)
			h->val_free_fn(entry->val);
		h->free_fn(entry);
		return 0;
	}

	return 1;
}

void l_hashtbl_clear(struct l_hashtbl *h)
{
	struct l_hashtbl_list_head *node, *tmp, *head = &h->all_entries;
	struct l_hashtbl_entry *entry;
	size_t nbytes = (size_t) h->table_size * sizeof(*h->table);

	for (node = head->next, tmp = node->next; node != head;
	     node = tmp, tmp = node->next) {
		entry = LIST_ENTRY(node, struct l_hashtbl_entry, list);
		if (h->key_free_fn != NULL)
			h->key_free_fn(entry->key);
		if (h->val_free_fn != NULL)
			h->val_free_fn(entry->val);
		list_remove(&entry->list);
		h->free_fn(entry);
		h->nentries--;
	}

	memset(h->table, 0, nbytes);
	list_init(&h->all_entries);
}

void l_hashtbl_delete(struct l_hashtbl *h)
{
	l_hashtbl_clear(h);
	h->free_fn(h->table);
	h->free_fn(h);
}

unsigned long l_hashtbl_count(const struct l_hashtbl *h)
{
	return h->nentries;
}

int l_hashtbl_capacity(const struct l_hashtbl *h)
{
	return h->table_size;
}

struct l_hashtbl *l_hashtbl_create(int capacity, double max_load_factor,
				   int auto_resize, int access_order,
				   LINKED_HASHTBL_HASH_FN hash_fn,
				   LINKED_HASHTBL_EQUALS_FN equals_fn,
				   LINKED_HASHTBL_KEY_FREE_FN key_free_fn,
				   LINKED_HASHTBL_VAL_FREE_FN val_free_fn,
				   LINKED_HASHTBL_MALLOC_FN malloc_fn,
				   LINKED_HASHTBL_FREE_FN free_fn,
				   LINKED_HASHTBL_EVICTOR_FN evictor_fn)
{
	struct l_hashtbl *h;

	malloc_fn = (malloc_fn != NULL) ? malloc_fn : malloc;
	free_fn = (free_fn != NULL) ? free_fn : free;
	hash_fn = (hash_fn != NULL) ? hash_fn : direct_hash;
	equals_fn = (equals_fn != NULL) ? equals_fn : direct_equals;
	evictor_fn = (evictor_fn != NULL) ? evictor_fn : remove_eldest;

	if ((h = malloc_fn(sizeof(*h))) == NULL)
		return NULL;

	if (max_load_factor < 0.0) {
		max_load_factor = 0.75f;
	} else if (max_load_factor > 1.0) {
		max_load_factor = 1.0f;
	}

	h->max_load_factor = max_load_factor;
	h->hash_fn = hash_fn;
	h->equals_fn = equals_fn;
	h->nentries = 0;
	h->table_size = 0;	/* must be 0 for resize() to work */
	h->resize_threshold = 0;
	h->auto_resize = auto_resize;
	h->access_order = access_order;
	h->key_free_fn = key_free_fn;
	h->val_free_fn = val_free_fn;
	h->malloc_fn = malloc_fn;
	h->free_fn = free_fn;
	h->evictor_fn = evictor_fn;
	h->table = NULL;
	list_init(&h->all_entries);

	if (l_hashtbl_resize(h, capacity) != 0) {
		free_fn(h);
		h = NULL;
	}

	return h;
}

int l_hashtbl_resize(struct l_hashtbl *h, int capacity)
{
	struct l_hashtbl_list_head *node, *head = &h->all_entries;
	struct l_hashtbl_entry *entry, **new_table;
	size_t nbytes;
	struct l_hashtbl tmp_h;

	if (capacity < 1) {
		capacity = 1;
	} else if (capacity >= LINKED_HASHTBL_MAX_TABLE_SIZE) {
		capacity = LINKED_HASHTBL_MAX_TABLE_SIZE;
	} else if (!is_power_of_2(capacity)) {
		capacity = roundup_to_next_power_of_2(capacity);
	}

	/* Don't grow if there is no change to the current size. */

	if (capacity < h->table_size || capacity == h->table_size)
		return 0;

	nbytes = (size_t) capacity *sizeof(*new_table);

	if ((tmp_h.table = h->malloc_fn(nbytes)) == NULL)
		return 1;

	memset(tmp_h.table, 0, nbytes);
	tmp_h.table_size = capacity;

	/* Transfer all entries from old table to new table. */

	for (node = head->next; node != head; node = node->next) {
		struct l_hashtbl_entry **slot_ref;
		entry = LIST_ENTRY(node, struct l_hashtbl_entry, list);
		slot_ref = tbl_entry_ref(&tmp_h, entry->hash);
		entry->next = *slot_ref;
		*slot_ref = entry;
	}

	if (h->table != NULL)
		h->free_fn(h->table);
	h->table = tmp_h.table;
	h->table_size = capacity;
	h->resize_threshold = resize_threshold(capacity, h->max_load_factor);

	return 0;
}

unsigned long l_hashtbl_apply(const struct l_hashtbl *h,
			      LINKED_HASHTBL_APPLY_FN apply, void *client_data)
{
	unsigned long nentries = 0;
	struct l_hashtbl_list_head *node;
	const struct l_hashtbl_list_head *head = &h->all_entries;

	for (node = head->next; node != head; node = node->next) {
		struct l_hashtbl_entry *entry;
		entry = LIST_ENTRY(node, struct l_hashtbl_entry, list);
		nentries++;
		if (apply(entry->key, entry->val, client_data) != 1)
			return nentries;
	}

	return nentries;
}

void l_hashtbl_iter_init(struct l_hashtbl *h, struct l_hashtbl_iter *iter,
			 int direction)
{
	struct l_hashtbl_list_head **pos;
	struct l_hashtbl_list_head **end;

	/* We have to do some funky casting in order to initialize the
	 * private fields as they are declared const -- we don't want
	 * clients changing them but we need to. */

	pos = (struct l_hashtbl_list_head **)&iter->pos;
	end = (struct l_hashtbl_list_head **)&iter->end;

	*(int *)&iter->direction = direction;
	*pos = (direction >= 1) ? h->all_entries.next : h->all_entries.prev;
	*end = &h->all_entries;
	iter->key = iter->val = NULL;
}

int l_hashtbl_iter_next(struct l_hashtbl_iter *iter)
{
	struct l_hashtbl_entry *entry;
	struct l_hashtbl_list_head *node, **node_ref;

	node = (struct l_hashtbl_list_head *)iter->pos;
	node_ref = (struct l_hashtbl_list_head **)&iter->pos;

	if (node == iter->end)
		return 0;

	if (iter->direction >= 1)
		*node_ref = node->next;
	else
		*node_ref = node->prev;

	entry = LIST_ENTRY(node, struct l_hashtbl_entry, list);
	iter->key = entry->key;
	iter->val = entry->val;

	return 1;
}

double l_hashtbl_load_factor(const struct l_hashtbl *h)
{
	return (double)h->nentries / (double)h->table_size;
}
