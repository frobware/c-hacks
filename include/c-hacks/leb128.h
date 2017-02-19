/* Copyright (c) 2017 <Andrew McDermott>
 *
 * Source can be cloned from:
 *
 *     https://github.com/frobware/c-hacks
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

#ifndef _leb128_h_
#define _leb128_h_

#include <stdlib.h>

size_t leb128_encode_ull(void *buf, unsigned long long value);
size_t leb128_encode_ul(void *buf, unsigned long value);
size_t leb128_encode_l(void *buf, long value);
size_t leb128_encode_ll(void *buf, long long value);

const unsigned char *leb128_decode_ul(const void *buf, unsigned long *result);
const unsigned char *leb128_decode_ull(const void *buf,
					 unsigned long long *result);
const unsigned char *leb128_decode_l(const void *buf, long *result);
const unsigned char *leb128_decode_ll(const void *buf, long long *result);

#endif
