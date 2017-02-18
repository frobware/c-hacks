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
 * list and unlinking the element in the list.
 */

#include <stddef.h>		/* size_t, offsetof, NULL */
#include <stdlib.h>		/* malloc, free */
#include <string.h>		/* strcmp */
#if !defined(_MSC_VER)
#include <stdint.h>		/* intptr_t */
#endif
#include <c-hacks/hashtbl.h>
#include <c-hacks/hashtbl-funcs.h>

#define UNUSED_PARAMETER(X) (void)(X)

#ifndef HASHTBL_MAX_TABLE_SIZE
#define HASHTBL_MAX_TABLE_SIZE (1 << 30)
#endif

#if defined(_MSC_VER)
#define INLINE __inline
#else
#define INLINE inline
#endif

struct hashtbl {
	double max_load_factor;
	HASHTBL_HASH_FN hash_fn;
	HASHTBL_EQUALS_FN equals_fn;
	unsigned long nentries;
	int table_size;
	int resize_threshold;
	int auto_resize;
	HASHTBL_KEY_FREE_FN key_free_fn;
	HASHTBL_VAL_FREE_FN val_free_fn;
	HASHTBL_MALLOC_FN malloc_fn;
	HASHTBL_FREE_FN free_fn;
	struct hashtbl_entry **table;
};

struct hashtbl_entry {
	struct hashtbl_entry *next;
	void *key;
	void *val;
	unsigned int hash;	/* hash of key */
};

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

static INLINE struct hashtbl_entry **tbl_entry_ref(struct hashtbl *h,
						   unsigned int hashval)
{
	return &h->table[(int)hashval & (h->table_size - 1)];
}

static INLINE struct hashtbl_entry *tbl_entry(struct hashtbl *h,
					      unsigned int hashval)
{
	return h->table[(int)hashval & (h->table_size - 1)];
}

static INLINE int resize_threshold(int capacity, double max_load_factor)
{
	return (int)(((double)capacity * max_load_factor) + 0.5);
}

static INLINE void unlink_entry(struct hashtbl *h, struct hashtbl_entry **head,
				struct hashtbl_entry *entry)
{
	*head = entry->next;
	h->nentries--;
}

static INLINE void link_entry(struct hashtbl *h, struct hashtbl_entry *entry)
{
	struct hashtbl_entry **head = tbl_entry_ref(h, entry->hash);

	entry->next = *head;
	*head = entry;
	h->nentries++;
}

static INLINE struct hashtbl_entry *find_entry(struct hashtbl *h,
					       unsigned int hv, const void *k)
{
	struct hashtbl_entry *entry = tbl_entry(h, hv);

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
static struct hashtbl_entry *remove_key(struct hashtbl *h, const void *k)
{
	unsigned int hv = h->hash_fn(k);
	struct hashtbl_entry **head = tbl_entry_ref(h, hv);
	struct hashtbl_entry *entry = *head;

	while (entry != NULL) {
		if (entry->hash == hv && h->equals_fn(entry->key, k)) {
			unlink_entry(h, head, entry);
			break;
		}
		head = &entry->next;
		entry = entry->next;
	}

	return entry;
}

static struct hashtbl_entry *hashtbl_entry_new(struct hashtbl *h,
					       unsigned int hv, void *k,
					       void *v)
{
	struct hashtbl_entry *entry;

	if ((entry = h->malloc_fn(sizeof(*entry))) == NULL)
		return NULL;

	entry->key = k;
	entry->val = v;
	entry->hash = hv;
	entry->next = NULL;

	return entry;
}

int hashtbl_insert(struct hashtbl *h, void *k, void *v)
{
	struct hashtbl_entry *entry;
	unsigned int hv = h->hash_fn(k);

	if ((entry = find_entry(h, hv, k)) != NULL) {
		if (h->val_free_fn != NULL)
			h->val_free_fn(entry->val);
		entry->val = v;
		return 0;
	}

	if (h->auto_resize) {
		if (h->nentries >= (unsigned int)h->resize_threshold) {
			/* auto resize failures are benign. */
			(void)hashtbl_resize(h, 2 * h->table_size);
		}
	}

	if ((entry = hashtbl_entry_new(h, hv, k, v)) == NULL)
		return 1;

	link_entry(h, entry);

	return 0;
}

void *hashtbl_lookup(struct hashtbl *h, const void *k)
{
	struct hashtbl_entry *entry = find_entry(h, h->hash_fn(k), k);

	return (entry != NULL) ? entry->val : NULL;
}

int hashtbl_remove(struct hashtbl *h, const void *k)
{
	struct hashtbl_entry *entry = remove_key(h, k);

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

void hashtbl_clear(struct hashtbl *h)
{
	int i;
	struct hashtbl_entry *entry, *next;

	for (i = 0; i < h->table_size; i++) {
		next = h->table[i];
		while ((entry = next) != NULL) {
			if (h->key_free_fn != NULL)
				h->key_free_fn(entry->key);
			if (h->val_free_fn != NULL)
				h->val_free_fn(entry->val);
			next = entry->next;
			entry->next = NULL;
			h->free_fn(entry);
			h->nentries--;
		}
		h->table[i] = next;
	}
}

void hashtbl_delete(struct hashtbl *h)
{
	hashtbl_clear(h);
	h->free_fn(h->table);
	h->free_fn(h);
}

unsigned long hashtbl_count(const struct hashtbl *h)
{
	return h->nentries;
}

int hashtbl_capacity(const struct hashtbl *h)
{
	return h->table_size;
}

struct hashtbl *hashtbl_create(int capacity, double max_load_factor,
			       int auto_resize, HASHTBL_HASH_FN hash_fn,
			       HASHTBL_EQUALS_FN equals_fn,
			       HASHTBL_KEY_FREE_FN key_free_fn,
			       HASHTBL_VAL_FREE_FN val_free_fn,
			       HASHTBL_MALLOC_FN malloc_fn,
			       HASHTBL_FREE_FN free_fn)
{
	struct hashtbl *h;

	malloc_fn = (malloc_fn != NULL) ? malloc_fn : malloc;
	free_fn = (free_fn != NULL) ? free_fn : free;
	hash_fn = (hash_fn != NULL) ? hash_fn : hashtbl_direct_hash;
	equals_fn = (equals_fn != NULL) ? equals_fn : hashtbl_direct_equals;

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
	h->key_free_fn = key_free_fn;
	h->val_free_fn = val_free_fn;
	h->malloc_fn = malloc_fn;
	h->free_fn = free_fn;
	h->table = NULL;

	if (hashtbl_resize(h, capacity) != 0) {
		free_fn(h);
		h = NULL;
	}

	return h;
}

int hashtbl_resize(struct hashtbl *h, int capacity)
{
	int i;
	struct hashtbl_entry **new_table;
	size_t nbytes;
	struct hashtbl tmp_h;

	if (capacity < 1) {
		capacity = 1;
	} else if (capacity >= HASHTBL_MAX_TABLE_SIZE) {
		capacity = HASHTBL_MAX_TABLE_SIZE;
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
	tmp_h.nentries = 0;
	tmp_h.table_size = capacity;

	/* Transfer all entries from old table to new table. */

	for (i = 0; i < h->table_size; i++) {
		struct hashtbl_entry **head = &h->table[i];
		struct hashtbl_entry *entry;

		while ((entry = *head) != NULL) {
			unlink_entry(h, head, entry);
			link_entry(&tmp_h, entry);
			/* Look for other chained keys in this slot */
			head = &h->table[i];
		}
	}

	if (h->table != NULL)
		h->free_fn(h->table);
	h->table = tmp_h.table;
	h->table_size = tmp_h.table_size;
	h->nentries = tmp_h.nentries;
	h->resize_threshold = resize_threshold(capacity, h->max_load_factor);

	return 0;
}

unsigned long hashtbl_apply(const struct hashtbl *h, HASHTBL_APPLY_FN apply,
			    void *client_data)
{
	unsigned long nentries = 0;
	int i;

	for (i = 0; i < h->table_size; i++) {
		struct hashtbl_entry *entry = h->table[i];

		while (entry != NULL) {
			nentries++;
			if (!apply(entry->key, entry->val, client_data))
				return nentries;
			entry = entry->next;
		}
	}

	return nentries;
}

void hashtbl_iter_init(struct hashtbl *h, struct hashtbl_iter *iter)
{
	iter->key = iter->val = NULL;

	/* We have to do some funky casting in order to initialize the
	 * private fields as they are declared const -- we don't want
	 * clients changing them but we need to. */

	*(int *)&iter->pos = h->table_size;
	*(int *)&iter->pos = 0;
	*(struct hashtbl_entry **)&iter->entry = NULL;
}

int hashtbl_iter_next(struct hashtbl *h, struct hashtbl_iter *iter)
{
	int i;

	/*
	 * If we're already walking a chain then continue down that
	 * chain.
	 */
	if (iter->entry != NULL) {
		*(struct hashtbl_entry **)&iter->entry = iter->entry->next;
		if (iter->entry != NULL) {
			iter->key = iter->entry->key;
			iter->val = iter->entry->val;
			return 1;
		} else {
			*(int *)&iter->pos = iter->pos + 1;
		}
	}

	for (i = iter->pos; i < h->table_size; i++) {
		*(struct hashtbl_entry **)&iter->entry = h->table[i];
		*(int *)&iter->pos = i;
		if (iter->entry != NULL) {
			iter->key = iter->entry->key;
			iter->val = iter->entry->val;
			return 1;
		}
	}

	return 0;
}

double hashtbl_load_factor(const struct hashtbl *h)
{
	return (double)h->nentries / (double)h->table_size;
}
