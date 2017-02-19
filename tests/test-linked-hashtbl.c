/* Copyright (c) 2009, 2017 <Andrew McDermott>
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

/* l_hashtbl_test.c - unit tests for l_hashtbl */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "CUnitTest.h"

#include <c-hacks/hashtbl-funcs.h>
#include <c-hacks/linked-hashtbl.h>

#ifndef LINKED_HASHTBL_MAX_LOAD_FACTOR
#define LINKED_HASHTBL_MAX_LOAD_FACTOR	0.75f
#endif

#ifndef LINKED_HASHTBL_MAX_TABLE_SIZE
#define LINKED_HASHTBL_MAX_TABLE_SIZE	(1 << 14)
#endif

#define UNUSED_PARAMETER(X)	(void)(X)
#define NELEMENTS(X)		(sizeof((X)) / sizeof((X)[0]))
#define STREQ(A,B)		strcmp((A), (B)) == 0

static int ht_size = 1;

struct test_val {
	int x[13];
	float f;
	char c;
	int v;
	long long int v64;
};

struct test_key {
	char *s[10];
	float fval;
	char c;
	int k;
};

static unsigned int key_hash(const void *k)
{
	return (unsigned int)((struct test_key *)k)->k;
}

static int key_equals(const void *a, const void *b)
{
	return ((struct test_key *)a)->k == ((struct test_key *)b)->k;
}

static int test6_apply_fn1(const void *k, const void *v, const void *u)
{
	UNUSED_PARAMETER(k);
	*(unsigned int *)u += (unsigned int)((struct test_val *)v)->v;
	return 1;
}

static int test6_apply_fn2(const void *k, const void *v, const void *u)
{
	UNUSED_PARAMETER(k);
	UNUSED_PARAMETER(v);
	*(int *)u *= 2;
	return 0;
}

static int test9_apply_fn1(const void *k, const void *v, const void *u)
{
	struct test_key *k1 = (struct test_key *)k;
	struct test_val *v1 = (struct test_val *)v;
	int test9_max = *(int *)u;
	CUT_ASSERT_EQUAL(v1->v - test9_max, k1->k);
	return 1;
}

/* Test basic hash table creation/clear/size. */

static int test1(void)
{
	struct l_hashtbl *h;
	struct l_hashtbl_iter iter;

	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     key_hash, key_equals, NULL, NULL, NULL, NULL,
			     NULL);
	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	l_hashtbl_clear(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	l_hashtbl_delete(h);
	return 0;
}

/* Test lookup of non-existent key. */

static int test2(void)
{
	struct test_key k;
	struct l_hashtbl *h;
	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     key_hash, key_equals, NULL, NULL, NULL, NULL,
			     NULL);
	CUT_ASSERT_NOT_NULL(h);
	memset(&k, 0, sizeof(k));
	k.k = 2;
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	CUT_ASSERT_NULL(l_hashtbl_lookup(h, &k));
	l_hashtbl_clear(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	l_hashtbl_delete(h);
	return 0;
}

/* Test lookup of key insert. */

static int test3(void)
{
	struct test_key k;
	struct test_val v;
	struct l_hashtbl *h;
	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     key_hash, key_equals, NULL, NULL, NULL, NULL,
			     NULL);
	CUT_ASSERT_NOT_NULL(h);
	memset(&k, 0, sizeof(k));
	memset(&v, 0, sizeof(v));
	k.k = 3;
	v.v = 300;
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &k, &v));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &k, &v));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &k, &v));
	CUT_ASSERT_EQUAL(1, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(&v, l_hashtbl_lookup(h, &k));
	CUT_ASSERT_EQUAL(300, ((struct test_val *)l_hashtbl_lookup(h, &k))->v);
	CUT_ASSERT_TRUE(l_hashtbl_load_factor(h) > 0);
	l_hashtbl_clear(h);
	CUT_ASSERT_TRUE(l_hashtbl_load_factor(h) == 0.0);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	l_hashtbl_delete(h);
	return 0;
}

/* Test lookup of multiple key insert. */

static int test4(void)
{
	struct test_key k1, k2;
	struct test_val v1, v2;
	struct l_hashtbl *h;
	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     key_hash, key_equals, NULL, NULL, NULL, NULL,
			     NULL);
	CUT_ASSERT_NOT_NULL(h);
	memset(&k1, 0, sizeof(k1));
	memset(&k2, 0, sizeof(k2));
	memset(&v1, 0, sizeof(v1));
	memset(&v2, 0, sizeof(v2));
	k1.k = 3;
	v1.v = 300;
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &k1, &v1));
	CUT_ASSERT_EQUAL(1, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(&v1, l_hashtbl_lookup(h, &k1));
	CUT_ASSERT_EQUAL(300, ((struct test_val *)l_hashtbl_lookup(h, &k1))->v);

	l_hashtbl_clear(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));

	k2.k = 4;
	v2.v = 400;
	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &k2, &v2));
	CUT_ASSERT_EQUAL(1, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(&v2, l_hashtbl_lookup(h, &k2));
	CUT_ASSERT_EQUAL(400, ((struct test_val *)l_hashtbl_lookup(h, &k2))->v);

	l_hashtbl_clear(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));

	l_hashtbl_delete(h);
	return 0;
}

/* Test multiple key lookups. */

static int test5(void)
{
	struct test_key k1, k2;
	struct test_val v1, v2;
	struct l_hashtbl *h;
	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     key_hash, key_equals, NULL, NULL, NULL, NULL,
			     NULL);
	CUT_ASSERT_NOT_NULL(h);

	memset(&k1, 0, sizeof(k1));
	memset(&k2, 0, sizeof(k2));
	memset(&k1, 0, sizeof(k1));
	memset(&k2, 0, sizeof(k2));

	k1.k = 3;
	v1.v = 300;
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &k1, &v1));
	CUT_ASSERT_EQUAL(1, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(&v1, l_hashtbl_lookup(h, &k1));
	CUT_ASSERT_EQUAL(300, ((struct test_val *)l_hashtbl_lookup(h, &k1))->v);
	CUT_ASSERT_EQUAL(1, l_hashtbl_count(h));

	k2.k = 4;
	v2.v = 400;
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &k2, &v2));
	CUT_ASSERT_EQUAL(2, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(&v2, l_hashtbl_lookup(h, &k2));
	CUT_ASSERT_EQUAL(400, ((struct test_val *)l_hashtbl_lookup(h, &k2))->v);

	CUT_ASSERT_EQUAL(&v1, l_hashtbl_lookup(h, &k1));
	CUT_ASSERT_EQUAL(300, ((struct test_val *)l_hashtbl_lookup(h, &k1))->v);

	l_hashtbl_clear(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));

	l_hashtbl_delete(h);
	return 0;
}

/* Test hashtable apply function. */

static int test6(void)
{
	int accumulator = 0;
	struct test_key k1, k2;
	struct test_val v1, v2;
	struct l_hashtbl *h;

	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     key_hash, key_equals, NULL, NULL, NULL, NULL,
			     NULL);

	CUT_ASSERT_NOT_NULL(h);

	memset(&k1, 0, sizeof(k1));
	memset(&k2, 0, sizeof(k2));
	memset(&v1, 0, sizeof(v1));
	memset(&v2, 0, sizeof(v2));

	k1.k = 3;
	v1.v = 300;
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &k1, &v1));
	CUT_ASSERT_EQUAL(1, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(&v1, l_hashtbl_lookup(h, &k1));
	CUT_ASSERT_EQUAL(300, ((struct test_val *)l_hashtbl_lookup(h, &k1))->v);
	CUT_ASSERT_EQUAL(1, l_hashtbl_count(h));

	k2.k = 4;
	v2.v = 400;
	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &k2, &v2));
	CUT_ASSERT_EQUAL(2, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(&v2, l_hashtbl_lookup(h, &k2));
	CUT_ASSERT_EQUAL(400, ((struct test_val *)l_hashtbl_lookup(h, &k2))->v);

	CUT_ASSERT_EQUAL(2, l_hashtbl_apply(h, test6_apply_fn1, &accumulator));
	CUT_ASSERT_EQUAL(700, accumulator);

	CUT_ASSERT_EQUAL(1, l_hashtbl_apply(h, test6_apply_fn2, &accumulator));
	CUT_ASSERT_EQUAL(1400, accumulator);

	l_hashtbl_clear(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));

	l_hashtbl_delete(h);
	return 0;
}

/* Test key/value remove(). */

static int test7(void)
{
	struct test_key k;
	struct test_val v;
	struct l_hashtbl *h;

	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     key_hash, key_equals, NULL, NULL, NULL, NULL,
			     NULL);

	CUT_ASSERT_NOT_NULL(h);

	memset(&k, 0, sizeof(k));
	memset(&v, 0, sizeof(v));

	k.k = 3;
	v.v = 300;
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &k, &v));
	CUT_ASSERT_EQUAL(1, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(&v, l_hashtbl_lookup(h, &k));
	CUT_ASSERT_EQUAL(300, ((struct test_val *)l_hashtbl_lookup(h, &k))->v);
	CUT_ASSERT_EQUAL(0, l_hashtbl_remove(h, &k));
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(1, l_hashtbl_remove(h, &k));
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	l_hashtbl_clear(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	l_hashtbl_delete(h);

	return 0;
}

/* Test key insert and remove with malloc'ed keys and values. */

static int test8(void)
{
	struct l_hashtbl *h;
	struct test_key *k = malloc(sizeof(struct test_key));
	struct test_val *v1 = malloc(sizeof(struct test_val));
	struct test_val *v2 = malloc(sizeof(struct test_val));

	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     key_hash, key_equals, free, free, NULL, NULL,
			     NULL);

	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_NOT_NULL(k);
	CUT_ASSERT_NOT_NULL(v1);
	CUT_ASSERT_NOT_NULL(v2);

	memset(k, 0, sizeof(*k));
	memset(v1, 0, sizeof(*v1));
	memset(v2, 0, sizeof(*v2));

	k->k = 3;
	v1->v = 300;
	v2->v = 600;

	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, k, v1));
	CUT_ASSERT_EQUAL(1, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(v1, l_hashtbl_lookup(h, k));
	CUT_ASSERT_EQUAL(300, ((struct test_val *)l_hashtbl_lookup(h, k))->v);

	/* Replace value for same key. */
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, k, v2));
	CUT_ASSERT_EQUAL(1, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(v2, l_hashtbl_lookup(h, k));
	CUT_ASSERT_EQUAL(600, ((struct test_val *)l_hashtbl_lookup(h, k))->v);

	CUT_ASSERT_EQUAL(0, l_hashtbl_remove(h, k));
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	l_hashtbl_clear(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	l_hashtbl_delete(h);
	return 0;
}

static int test9(void)
{
	int test9_max = 100;
	struct l_hashtbl *h;
	int i;
	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     key_hash, key_equals, free, free, NULL, NULL,
			     NULL);
	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));

	for (i = 0; i < test9_max; i++) {
		struct test_key *k = malloc(sizeof(struct test_key));
		struct test_val *v = malloc(sizeof(struct test_val));
		CUT_ASSERT_NOT_NULL(k);
		CUT_ASSERT_NOT_NULL(v);
		memset(k, 0, sizeof(*k));
		memset(v, 0, sizeof(*v));
		k->k = i;
		v->v = i + test9_max;
		CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, k, v));
		CUT_ASSERT_EQUAL(i + 1, (int)l_hashtbl_count(h));
		CUT_ASSERT_EQUAL(i + test9_max, ((struct test_val *)
						 l_hashtbl_lookup(h, k))->v);
	}

	l_hashtbl_apply(h, test9_apply_fn1, &test9_max);

	for (i = 0; i < test9_max; i++) {
		struct test_key k;
		struct test_val *v;
		memset(&k, 0, sizeof(k));
		k.k = i;
		v = l_hashtbl_lookup(h, &k);
		CUT_ASSERT_NOT_NULL(v);
		CUT_ASSERT_EQUAL(i + test9_max, v->v);
	}

	for (i = 99; i >= 0; i--) {
		struct test_key k;
		struct test_val *v;
		memset(&k, 0, sizeof(k));
		k.k = i;
		v = l_hashtbl_lookup(h, &k);
		CUT_ASSERT_NOT_NULL(v);
		CUT_ASSERT_EQUAL(v->v - test9_max, i);
	}

	l_hashtbl_clear(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	l_hashtbl_delete(h);

	return 0;
}

/* Test direct hash/equals functions. */

static int test10(void)
{
	int i;
	int keys[] = { 100, 200, 300 };
	int values[] = { 1000, 2000, 3000 };
	struct l_hashtbl *h;
	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_direct_hash, hashtbl_direct_equals, NULL,
			     NULL, NULL, NULL, NULL);
	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	for (i = 0; i < (int)NELEMENTS(keys); i++) {
		CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[i], &values[i]));
		CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &keys[i]));
		CUT_ASSERT_EQUAL(values[i],
				 *(int *)l_hashtbl_lookup(h, &keys[i]));
	}
	CUT_ASSERT_EQUAL((int)NELEMENTS(keys), l_hashtbl_count(h));
	for (i = 0; i < (int)NELEMENTS(keys); i++) {
		CUT_ASSERT_EQUAL(0, l_hashtbl_remove(h, &keys[i]));
	}
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	l_hashtbl_clear(h);
	l_hashtbl_delete(h);
	return 0;
}

/* Test int hash/equals functions. */

static int test11(void)
{
	unsigned int i;
	int keys[] = { 100, 200, 300 };
	int values[] = { 1000, 2000, 3000 };
	struct l_hashtbl *h;
	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_int_hash, hashtbl_int_equals, NULL, NULL,
			     NULL, NULL, NULL);
	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	for (i = 0; i < NELEMENTS(keys); i++) {
		int x = keys[i];
		CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[i], &values[i]));
		CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &x));
		CUT_ASSERT_EQUAL(values[i], *(int *)l_hashtbl_lookup(h, &x));
	}
	CUT_ASSERT_EQUAL(NELEMENTS(keys), l_hashtbl_count(h));
	for (i = 0; i < NELEMENTS(keys); i++) {
		int x = keys[i];
		CUT_ASSERT_EQUAL(0, l_hashtbl_remove(h, &x));
	}
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	l_hashtbl_clear(h);
	l_hashtbl_delete(h);
	return 0;
}

/* Test string hash/equals functions. */

static int test12(void)
{
	unsigned int i;
	char *keys[] = { "100", "200", "300" };
	char *values[] = { "100", "200", "300" };
	struct l_hashtbl *h;

	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_string_hash, hashtbl_string_equals, NULL,
			     NULL, NULL, NULL, NULL);

	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));

	for (i = 0; i < NELEMENTS(keys); i++) {
		CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, keys[i], values[i]));
		CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, keys[i]));
		CUT_ASSERT_EQUAL(values[i], l_hashtbl_lookup(h, keys[i]));
		CUT_ASSERT_TRUE(STREQ
				(keys[i],
				 (char *)l_hashtbl_lookup(h, keys[i])));
		CUT_ASSERT_TRUE(STREQ
				(values[i],
				 (char *)l_hashtbl_lookup(h, keys[i])));
	}

	l_hashtbl_delete(h);
	return 0;
}

/* Test initial_capacity boundary values. */

static int test13(void)
{
	struct l_hashtbl *h;

	h = l_hashtbl_create(-1, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_direct_hash, hashtbl_direct_equals, NULL,
			     NULL, NULL, NULL, NULL);
	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(1, l_hashtbl_capacity(h));
	l_hashtbl_delete(h);

	h = l_hashtbl_create(0, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_direct_hash, hashtbl_direct_equals, NULL,
			     NULL, NULL, NULL, NULL);
	CUT_ASSERT_EQUAL(1, l_hashtbl_capacity(h));
	CUT_ASSERT_NOT_NULL(h);
	l_hashtbl_delete(h);

	h = l_hashtbl_create(LINKED_HASHTBL_MAX_TABLE_SIZE + 1,
			     LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_direct_hash, hashtbl_direct_equals, NULL,
			     NULL, NULL, NULL, NULL);
	CUT_ASSERT_EQUAL(LINKED_HASHTBL_MAX_TABLE_SIZE, l_hashtbl_capacity(h));
	CUT_ASSERT_NOT_NULL(h);
	l_hashtbl_delete(h);

	h = l_hashtbl_create(127, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_direct_hash, hashtbl_direct_equals, NULL,
			     NULL, NULL, NULL, NULL);
	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(128, l_hashtbl_capacity(h));
	l_hashtbl_delete(h);

	h = l_hashtbl_create(128, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_direct_hash, hashtbl_direct_equals, NULL,
			     NULL, NULL, NULL, NULL);
	CUT_ASSERT_NOT_NULL(h);
	l_hashtbl_resize(h, 128);
	CUT_ASSERT_EQUAL(128, l_hashtbl_capacity(h));
	l_hashtbl_resize(h, 0);
	CUT_ASSERT_EQUAL(128, l_hashtbl_capacity(h));
	l_hashtbl_resize(h, 99);
	CUT_ASSERT_EQUAL(128, l_hashtbl_capacity(h));
	l_hashtbl_resize(h, 128);
	CUT_ASSERT_EQUAL(128, l_hashtbl_capacity(h));
	l_hashtbl_resize(h, LINKED_HASHTBL_MAX_TABLE_SIZE);
	CUT_ASSERT_EQUAL(LINKED_HASHTBL_MAX_TABLE_SIZE, l_hashtbl_capacity(h));
	l_hashtbl_resize(h, 1 + LINKED_HASHTBL_MAX_TABLE_SIZE);
	CUT_ASSERT_EQUAL(LINKED_HASHTBL_MAX_TABLE_SIZE, l_hashtbl_capacity(h));
	l_hashtbl_delete(h);

	h = l_hashtbl_create(ht_size, -1.0f, 1, 0, hashtbl_direct_hash,
			     hashtbl_direct_equals, NULL, NULL, NULL, NULL,
			     NULL);
	CUT_ASSERT_NOT_NULL(h);
	l_hashtbl_delete(h);

	h = l_hashtbl_create(ht_size, 1.1f, 1, 0, NULL, NULL, NULL, NULL, NULL,
			     NULL, NULL);
	CUT_ASSERT_NOT_NULL(h);
	l_hashtbl_delete(h);

	return 0;
}

/* Test l_hashtbl_iter */

static int test14(void)
{
	unsigned int i;
	char *keys[] = { "100", "200", "300" };
	char *vals[] = { "1000", "2000", "3000" };
	struct l_hashtbl *h;
	struct l_hashtbl_iter iter;

	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_string_hash, hashtbl_string_equals, NULL,
			     NULL, NULL, NULL, NULL);

	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));

	for (i = 0; i < NELEMENTS(keys); i++) {
		CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, keys[i], vals[i]));
		CUT_ASSERT_EQUAL(vals[i], l_hashtbl_lookup(h, keys[i]));
		CUT_ASSERT_TRUE(STREQ
				(vals[i],
				 (char *)l_hashtbl_lookup(h, keys[i])));
	}

	/* Iteration order should reflect insertion order. */

	i = 3;
	l_hashtbl_iter_init(h, &iter, 1);
	while (l_hashtbl_iter_next(&iter)) {
		CUT_ASSERT_EQUAL(keys[i - 1], iter.key);
		CUT_ASSERT_EQUAL(vals[i - 1], iter.val);
		i--;
	}
	CUT_ASSERT_EQUAL(0, i);

	/* Reverse iteration. */

	i = 0;
	l_hashtbl_iter_init(h, &iter, -1);
	while (l_hashtbl_iter_next(&iter)) {
		CUT_ASSERT_EQUAL(keys[i], iter.key);
		CUT_ASSERT_EQUAL(vals[i], iter.val);
		i++;
	}
	CUT_ASSERT_EQUAL(3, i);

	l_hashtbl_clear(h);
	l_hashtbl_delete(h);
	return 0;

}

/* Test lots of insertions and removals. */

/* Pull this out of test18() to avoid blowing the stack. */
#define TEST15_N 12

static int test15_bigtable[1 << TEST15_N];

static int test15(void)
{
	int i;
	struct l_hashtbl *h;

	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_direct_hash, hashtbl_direct_equals, NULL,
			     NULL, NULL, NULL, NULL);
	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));

	for (i = 0; i < (1 << TEST15_N); i++) {
		int *k = &test15_bigtable[i];
		test15_bigtable[i] = i;
		CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, k, k));
		CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, k));
	}

	/* Iteration order should reflect insertion order. */

	for (i = 0; i < (1 << TEST15_N); i++) {
		int *k = &test15_bigtable[i];
		CUT_ASSERT_EQUAL(0, l_hashtbl_remove(h, k));
	}

	l_hashtbl_delete(h);
	return 0;
}

/* Test LRU behaviour. */

static int test16(void)
{
	int i;
	struct l_hashtbl *h;
	static int keys[] = { 100, 200, 300 };
	struct l_hashtbl_iter iter;

	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_direct_hash, hashtbl_direct_equals, NULL,
			     NULL, NULL, NULL, NULL);

	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));

	for (i = 0; i < (int)NELEMENTS(keys); i++) {
		CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[i], NULL));
	}

	CUT_ASSERT_EQUAL(3, l_hashtbl_count(h));

	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[2], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[1], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[0], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	CUT_ASSERT_EQUAL(0, l_hashtbl_remove(h, &keys[0]));
	CUT_ASSERT_EQUAL(2, l_hashtbl_count(h));
	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[2], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[1], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	CUT_ASSERT_EQUAL(0, l_hashtbl_remove(h, &keys[2]));
	CUT_ASSERT_EQUAL(1, l_hashtbl_count(h));
	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[1], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	CUT_ASSERT_EQUAL(0, l_hashtbl_remove(h, &keys[1]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	for (i = 0; i < (int)NELEMENTS(keys); i++) {
		CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[i], NULL));
	}

	CUT_ASSERT_EQUAL(0, l_hashtbl_remove(h, &keys[0]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[0], NULL));

	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[0], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[2], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[1], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	CUT_ASSERT_EQUAL(0, l_hashtbl_remove(h, &keys[1]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[1], NULL));

	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[1], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[0], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[2], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	CUT_ASSERT_EQUAL(3, l_hashtbl_count(h));

	l_hashtbl_delete(h);
	return 0;
}

/* Test MRU behaviour. */

static int test17(void)
{
	struct l_hashtbl *h;
	static int keys[] = { 100, 200, 300 };
	struct l_hashtbl_iter iter;

	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 1,
			     hashtbl_direct_hash, hashtbl_direct_equals, NULL,
			     NULL, NULL, NULL, NULL);

	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));

	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[0], &keys[0]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[1], &keys[1]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[2], &keys[2]));
	CUT_ASSERT_EQUAL(3, l_hashtbl_count(h));

	CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &keys[2]));
	CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &keys[1]));
	CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &keys[0]));

	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[0], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[1], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[2], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &keys[1]));
	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[1], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[0], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[2], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &keys[1]));
	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[1], *(int *)iter.key);

	CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &keys[0]));
	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[0], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[1], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[2], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[2], NULL));
	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[0], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[1], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[2], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	CUT_ASSERT_EQUAL(0, l_hashtbl_remove(h, &keys[2]));
	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[0], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[1], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &keys[1]));
	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[1], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[0], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	l_hashtbl_delete(h);
	return 0;
}

/* Test LRU eviction behaviour. */

static int test18_remove_eldest_1(const struct l_hashtbl *h,
				  unsigned long count)
{
	UNUSED_PARAMETER(h);
	CUT_ASSERT_EQUAL(1, count);
	return 1;
}

static int test18(void)
{
	struct l_hashtbl *h;
	static int keys[] = { 100, 200, 300 };

	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_direct_hash, hashtbl_direct_equals, NULL,
			     NULL, NULL, NULL, test18_remove_eldest_1);

	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[0], &keys[0]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[1], &keys[1]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[2], &keys[2]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));

	l_hashtbl_delete(h);
	return 0;
}

static int test19_remove_eldest_1(const struct l_hashtbl *h,
				  unsigned long count)
{
	UNUSED_PARAMETER(h);
	UNUSED_PARAMETER(count);
	return (count > 3) ? 1 : 0;
}

/* Test iteration. */

static int test19(void)
{
	struct l_hashtbl *h;
	static int keys[] = { 100, 200, 300, 400, 500, 600 };
	struct l_hashtbl_iter iter;

	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_direct_hash, hashtbl_direct_equals, NULL,
			     NULL, NULL, NULL, test19_remove_eldest_1);

	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[0], &keys[0]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[1], &keys[1]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[2], &keys[2]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[3], &keys[3]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[4], &keys[4]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[5], &keys[5]));
	CUT_ASSERT_EQUAL(3, l_hashtbl_count(h));

	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[5], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[4], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[3], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &keys[4]));
	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[5], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[4], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[3], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &keys[3]));
	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[5], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[4], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[3], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	l_hashtbl_delete(h);
	return 0;
}

static int test20_remove_eldest_1(const struct l_hashtbl *h,
				  unsigned long count)
{
	UNUSED_PARAMETER(h);
	UNUSED_PARAMETER(count);
	return (count > 3) ? 1 : 0;
}

/* Test MRU eviction behaviour. */

static int test20(void)
{
	struct l_hashtbl *h;
	static int keys[] = { 100, 200, 300, 400, 500, 600 };
	struct l_hashtbl_iter iter;

	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 1,
			     hashtbl_direct_hash, hashtbl_direct_equals, NULL,
			     NULL, NULL, NULL, test20_remove_eldest_1);

	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[0], &keys[0]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[1], &keys[1]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[2], &keys[2]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[3], &keys[3]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[4], &keys[4]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[5], &keys[5]));
	CUT_ASSERT_EQUAL(3, l_hashtbl_count(h));

	CUT_ASSERT_NULL(l_hashtbl_lookup(h, &keys[0]));
	CUT_ASSERT_NULL(l_hashtbl_lookup(h, &keys[1]));
	CUT_ASSERT_NULL(l_hashtbl_lookup(h, &keys[2]));
	CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &keys[3]));
	CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &keys[4]));
	CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &keys[5]));

	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[5], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[4], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[3], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &keys[4]));
	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[4], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[5], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[3], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &keys[3]));
	l_hashtbl_iter_init(h, &iter, 1);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[3], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[4], *(int *)iter.key);
	CUT_ASSERT_TRUE(l_hashtbl_iter_next(&iter));
	CUT_ASSERT_EQUAL(keys[5], *(int *)iter.key);
	CUT_ASSERT_FALSE(l_hashtbl_iter_next(&iter));

	l_hashtbl_delete(h);
	return 0;
}

static void *test21_malloc(size_t n)
{
	UNUSED_PARAMETER(n);
	return 0;
}

/* Test that creation fails. */

static int test21(void)
{
	struct l_hashtbl *h;

	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_direct_hash, hashtbl_direct_equals, NULL,
			     NULL, test21_malloc, NULL, NULL);

	CUT_ASSERT_NULL(h);

	return 0;
}

static void *test22_malloc(size_t n)
{
	static int invoke_count = 0;
	switch (++invoke_count) {
	case 1:
		return malloc(n);
	default:
		return 0;
	}
}

static void test22_free(void *p)
{
	free(p);
}

/* Test that table allocation in l_hashtbl_create fails. */

static int test22(void)
{
	struct l_hashtbl *h;

	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_direct_hash, hashtbl_direct_equals, NULL,
			     NULL, test22_malloc, test22_free, NULL);

	CUT_ASSERT_NULL(h);

	return 0;
}

static void *test23_malloc(size_t n)
{
	static int invoke_count = 0;
	switch (++invoke_count) {
	case 1:
		return malloc(n);
	case 2:
		return malloc(n);
	default:
		return 0;
	}
}

static void test23_free(void *p)
{
	free(p);
}

/* Test that insertion allocation fails. */

static int test23(void)
{
	struct l_hashtbl *h;
	static int keys[] = { 100, 200, 300, 400, 500, 600 };

	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_direct_hash, hashtbl_direct_equals, NULL,
			     NULL, test23_malloc, test23_free, NULL);

	CUT_ASSERT_NOT_NULL(h);

	CUT_ASSERT_EQUAL(1, l_hashtbl_insert(h, &keys[0], &keys[0]));
	CUT_ASSERT_EQUAL(1, l_hashtbl_insert(h, &keys[1], &keys[1]));
	CUT_ASSERT_EQUAL(1, l_hashtbl_insert(h, &keys[2], &keys[2]));
	CUT_ASSERT_EQUAL(1, l_hashtbl_insert(h, &keys[3], &keys[3]));
	CUT_ASSERT_EQUAL(1, l_hashtbl_insert(h, &keys[4], &keys[4]));
	CUT_ASSERT_EQUAL(1, l_hashtbl_insert(h, &keys[5], &keys[5]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));

	l_hashtbl_delete(h);
	return 0;
}

static void *test24_malloc(size_t n)
{
	static int invoke_count = 0;

	if (++invoke_count >= 5) {
		return 0;
	} else {
		return malloc(n);
	}
}

static void test24_free(void *p)
{
	free(p);
}

/* Test that resize allocation fails. */

static int test24(void)
{
	struct l_hashtbl *h;
	static int keys[] = { 100, 200, 300, 400, 500, 600 };

	h = l_hashtbl_create(4, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_direct_hash, hashtbl_direct_equals, NULL,
			     NULL, test24_malloc, test24_free, NULL);

	CUT_ASSERT_NOT_NULL(h);

	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[0], &keys[0]));
	CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[1], &keys[1]));
	CUT_ASSERT_EQUAL(1, l_hashtbl_resize(h, 8));
	CUT_ASSERT_EQUAL(2, l_hashtbl_count(h));

	l_hashtbl_delete(h);
	return 0;
}

/* Test int64 hash/equals functions. */

static int test25(void)
{
	unsigned int i;
	long long int keys[] = { 1LL << 32, 1LL << 33, 1LL << 34 };
	long long int values[] = { 1LL << 35, 1LL << 36, 1LL << 37 };
	struct l_hashtbl *h;
	h = l_hashtbl_create(ht_size, LINKED_HASHTBL_MAX_LOAD_FACTOR, 1, 0,
			     hashtbl_int64_hash, hashtbl_int64_equals, NULL,
			     NULL, NULL, NULL, NULL);
	CUT_ASSERT_NOT_NULL(h);
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	for (i = 0; i < NELEMENTS(keys); i++) {
		long long int x = keys[i];
		CUT_ASSERT_EQUAL(0, l_hashtbl_insert(h, &keys[i], &values[i]));
		CUT_ASSERT_NOT_NULL(l_hashtbl_lookup(h, &x));
		CUT_ASSERT_EQUAL(values[i],
				 *(long long *)l_hashtbl_lookup(h, &x));
	}
	CUT_ASSERT_EQUAL(NELEMENTS(keys), l_hashtbl_count(h));
	for (i = 0; i < NELEMENTS(keys); i++) {
		long long x = keys[i];
		CUT_ASSERT_EQUAL(0, l_hashtbl_remove(h, &x));
	}
	CUT_ASSERT_EQUAL(0, l_hashtbl_count(h));
	l_hashtbl_clear(h);
	l_hashtbl_delete(h);
	return 0;
}

CUT_BEGIN_TEST_HARNESS CUT_RUN_TEST(test1);
CUT_RUN_TEST(test2);
CUT_RUN_TEST(test3);
CUT_RUN_TEST(test4);
CUT_RUN_TEST(test5);
CUT_RUN_TEST(test6);
CUT_RUN_TEST(test7);
CUT_RUN_TEST(test8);
CUT_RUN_TEST(test9);
CUT_RUN_TEST(test10);
CUT_RUN_TEST(test11);
CUT_RUN_TEST(test12);
CUT_RUN_TEST(test13);
CUT_RUN_TEST(test14);
CUT_RUN_TEST(test15);
CUT_RUN_TEST(test16);
CUT_RUN_TEST(test17);
CUT_RUN_TEST(test18);
CUT_RUN_TEST(test19);
CUT_RUN_TEST(test20);
CUT_RUN_TEST(test21);
CUT_RUN_TEST(test22);
CUT_RUN_TEST(test23);
CUT_RUN_TEST(test24);
CUT_RUN_TEST(test25);
CUT_END_TEST_HARNESS
