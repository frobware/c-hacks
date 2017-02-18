#ifndef HASHTBL_FUNCS_H
#define HASHTBL_FUNCS_H

/* Copyright (c) 2010, 2017 <Andrew McDermott>
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

#include <string.h>		/* strcmp */
#if !defined(_MSC_VER)
#include <stdint.h>		/* intptr_t */
#endif

#if defined(_MSC_VER)
#define INLINE __inline
#else
#define INLINE inline
#endif

static INLINE unsigned int hashtbl_string_hash(const void *k)
{
	/* djb implementation as posted to comp.lang.c */
	const unsigned char *str = (const unsigned char *)k;
	unsigned int hash = 5381;

	if (str) {
		while (*str++ != '\0') {
			hash = 33 * hash ^ *str;
		}
	}
	return hash;
}

static INLINE int hashtbl_string_equals(const void *a, const void *b)
{
	return strcmp(((const char *)a), (const char *)b) == 0;
}

static INLINE unsigned int hashtbl_int_hash(const void *k)
{
	return *(unsigned int *)k;
}

static INLINE int hashtbl_int_equals(const void *a, const void *b)
{
	return *((const int *)a) == *((const int *)b);
}

static INLINE unsigned int hashtbl_int64_hash(const void *k)
{
	return (unsigned int)*(long long *)k;
}

static INLINE int hashtbl_int64_equals(const void *a, const void *b)
{
	return *((const long long int *)a) == *((const long long int *)b);
}

static INLINE unsigned int hashtbl_direct_hash(const void *k)
{
	/* Magic numbers from Java 1.4. */
	unsigned int h = (unsigned int)(uintptr_t) k;
	h ^= (h >> 20) ^ (h >> 12);
	return h ^ (h >> 7) ^ (h >> 4);
}

static INLINE int hashtbl_direct_equals(const void *a, const void *b)
{
	return a == b;
}

#endif
