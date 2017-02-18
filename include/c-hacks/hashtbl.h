#ifndef HASHTBL_H
#define HASHTBL_H

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

/**
 * A hash table: efficiently map keys to values.
 *
 * SYNOPSIS
 *
 * 1. A hash table is created with hashtbl_create().
 * 2. To insert an entry use hashtbl_insert().
 * 3. To lookup a key use hashtbl_lookup().
 * 4. To remove a key use hashtbl_remove().
 * 5. To apply a function to all entries use hashtbl_apply().
 * 5. To clear all keys use hashtbl_clear().
 * 6. To delete a hash table instance use hashtbl_delete().
 * 7. To iterate over all entries use hashtbl_iter_init(), hashtbl_iter_next().
 *
 * Note: neither the keys or the values are copied so their lifetime
 * must match that of the hash table.  NULL keys are not permitted.
 * Inserting, removing or lookup up NULL keys is therefore undefined.
 */

#include <stddef.h>		/* size_t */

/* Opaque types. */
struct hashtbl;
struct hashtbl_entry;

/* Hash function. */
typedef unsigned int (*HASHTBL_HASH_FN) (const void *k);

/* Key equality function. */
typedef int (*HASHTBL_EQUALS_FN) (const void *a, const void *b);

/* Apply function. */
typedef int (*HASHTBL_APPLY_FN) (const void *key, const void *val,
				 const void *client_data);

/* Functions for deleting keys and values. */
typedef void (*HASHTBL_KEY_FREE_FN) (void *k);
typedef void (*HASHTBL_VAL_FREE_FN) (void *v);

/* Functions for allocating and freeing memory. */
typedef void *(*HASHTBL_MALLOC_FN) (size_t n);
typedef void (*HASHTBL_FREE_FN) (void *ptr);

/* Function for evicting oldest entries. */
typedef int (*HASHTBL_EVICTOR_FN) (const struct hashtbl * h,
				   unsigned long count);

struct hashtbl_iter {
	void *key;
	void *val;
	/* The remaining fields are private: don't modify them. */
	const int pos;
	const struct hashtbl_entry *const entry;
};

/*
 * Creates a new hash table.
 *
 * @param initial_capacity - initial size of the table
 * @param max_load_factor  - before resizing (0.0 uses a default value)
 * @param auto_resize	   - if true, table grows (pow2) as new keys are added
 * @param hash_func	   - function that computes a hash value from a key
 * @param equals_func	   - function that checks keys for equality
 * @param key_free_func	   - function to delete keys
 * @param val_free_func	   - function to delete values
 * @param malloc_func	   - function to allocate memory (e.g., malloc)
 * @param free_func	   - function to free memory (e.g., free)
 *
 * Returns non-null if the table was created successfully.
 */
struct hashtbl *hashtbl_create(int initial_capacity, double max_load_factor,
			       int auto_resize, HASHTBL_HASH_FN hash_fun,
			       HASHTBL_EQUALS_FN equals_fun,
			       HASHTBL_KEY_FREE_FN key_free_func,
			       HASHTBL_VAL_FREE_FN val_free_func,
			       HASHTBL_MALLOC_FN malloc_func,
			       HASHTBL_FREE_FN free_func);

/*
 * Deletes the hash table instance.
 *
 * All the entries are removed via hashtbl_clear().
 *
 * @param h - hash table
 */
void hashtbl_delete(struct hashtbl *h);

/*
 * Removes a key and value from the table.
 *
 * @param h - hash table instance
 * @param k - key to remove
 *
 * Returns 0 if key was found, otherwise 1.
 */
int hashtbl_remove(struct hashtbl *h, const void *k);

/*
 * Clears all entries and reclaims memory used by each entry.
 */
void hashtbl_clear(struct hashtbl *h);

/*
 * Inserts a new key with associated value.
 *
 * @param h - hash table instance
 * @param k - key to insert
 * @param v - value associated with key
 *
 * Returns 0 on success, or 1 if a new entry cannot be created.
 */
int hashtbl_insert(struct hashtbl *h, void *k, void *v);

/*
 * Lookup an existing key.
 *
 * @param h - hash table instance
 * @param k - the search key
 *
 * Returns the value associated with key, or NULL if key is not present.
 */
void *hashtbl_lookup(struct hashtbl *h, const void *k);

/*
 * Returns the number of entries in the table.
 *
 * @param h - hash table instance
 */
unsigned long hashtbl_count(const struct hashtbl *h);

/*
 * Returns the table's capacity.
 *
 * @param h - hash table instance
 */
int hashtbl_capacity(const struct hashtbl *h);

/*
 * Apply a function to all entries in the table.
 *
 * The apply function should return 0 to terminate the enumeration
 * early.
 *
 * @param h  - hash table instance
 * @param fn - function to apply to each table entry
 * @param p  - arbitrary user data
 *
 * Returns the number of entries the function was applied to.
 */
unsigned long hashtbl_apply(const struct hashtbl *h, HASHTBL_APPLY_FN fn,
			    void *p);

/*
 * Returns the load factor of the hash table.
 *
 * @param h - hash table instance
 *
 * The load factor is a ratio and is calculated as:
 *
 *   hashtbl_count() / hashtbl_capacity()
 */
double hashtbl_load_factor(const struct hashtbl *h);

/*
 * Resize the hash table.
 *
 * Returns 0 on success, or 1 if no memory could be allocated.
 */
int hashtbl_resize(struct hashtbl *h, int new_capacity);

/*
 * Initialize an iterator.
 *
 * @param h - hash table instance
 * @param iter - iterator to initialize
 */
void hashtbl_iter_init(struct hashtbl *h, struct hashtbl_iter *iter);

/*
 * Advances the iterator.
 *
 * Returns 1 while there more entries, otherwise 0.  The key and value
 * for each entry can be accessed through the iterator structure.
 */
int hashtbl_iter_next(struct hashtbl *h, struct hashtbl_iter *iter);

#endif				/* HASHTBL_H */
