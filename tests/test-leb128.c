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

#include "CUnitTest.h"
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <limits.h>
#include <assert.h>

#include <c-hacks/leb128.h>

/* Returns the actual LEB128 encoding count for X. */
static size_t encoding_count_ull(unsigned long long value)
{
        size_t count;
        for (count = 1; value >= 0x80; value >>= 7, count++) {};
        return count;
}

/* Returns the actual signed LEB128 encoding count for X. */
static size_t encoding_count_ll(long long value)
{
        int more = 1;
        size_t count = 0;

        while (more) {
                unsigned char byte = value & 0x7f;
                value >>= 7;
                if ((value == 0 && ((byte & 0x40) != 0x40)) ||
                    (value == -1 && ((byte & 0x40) == 0x40))) {
                        more = 0;
                }
                count++;
        }

        return count;
}

/*
 * Populates BUF with a repeating pattern to try and force
 * non-expected results.
 */
static void reset_buffer(unsigned char *buf, size_t sz)
{
        size_t i;

        for (i = 0; i < sz; i++) {
                buf[i] = (i % 2) == 0 ? 0x80 : 0x7f;
        }
}

/* Validates that actual encoding counts matches well known values. */

static int test0_known_unsigned_sizes(void)
{
        CUT_ASSERT_EQUAL(1, encoding_count_ull(0));
        CUT_ASSERT_EQUAL(2, encoding_count_ull(UINT8_MAX));
        CUT_ASSERT_EQUAL(3, encoding_count_ull(UINT16_MAX));
        CUT_ASSERT_EQUAL(5, encoding_count_ull(UINT32_MAX));
        CUT_ASSERT_EQUAL(10, encoding_count_ull(UINT64_MAX));
        return 0;
}

/* Validates that actual encoding counts matches well known values. */

static int test0_known_signed_sizes(void)
{
        CUT_ASSERT_EQUAL(1, encoding_count_ll(0));
        CUT_ASSERT_EQUAL(2, encoding_count_ll(INT8_MAX));
        CUT_ASSERT_EQUAL(2, encoding_count_ll(INT8_MIN));
        CUT_ASSERT_EQUAL(3, encoding_count_ll(INT16_MAX));
        CUT_ASSERT_EQUAL(3, encoding_count_ll(INT16_MIN));
        CUT_ASSERT_EQUAL(5, encoding_count_ll(INT32_MAX));
        CUT_ASSERT_EQUAL(5, encoding_count_ll(INT32_MIN));
        CUT_ASSERT_EQUAL(10, encoding_count_ll(INT64_MAX));
        CUT_ASSERT_EQUAL(10, encoding_count_ll(INT64_MIN));
        return 0;
}

/* Validates power-of-2 numbers starting from 1. */

static int test1_leb128_encode_ull(void)
{
        unsigned long long i = 1;
        unsigned char buf[10];
        size_t expected_count, actual_count, j;

        do {
                reset_buffer(buf, sizeof buf);
                expected_count = encoding_count_ull(i);
                CUT_ASSERT_TRUE(expected_count <= sizeof buf);
                actual_count = x_leb128_encode_ull(buf, i);
                CUT_ASSERT_EQUAL(expected_count, actual_count);
                CUT_ASSERT_EQUAL(0, (buf[actual_count -1] & 0x80));
                for (j = 0; j < actual_count -1; j++) {
                        CUT_ASSERT_EQUAL(0x80, (buf[j] & 0x80));
                }
                i <<= 1;
        } while (i);

        return 0;
}

static int test2_leb128_encode_ul(void)
{
        unsigned long i = 1;
        unsigned char buf[10];
        size_t expected_count, actual_count, j;

        do {
                reset_buffer(buf, sizeof buf);
                expected_count = encoding_count_ull(i);
                CUT_ASSERT_TRUE(expected_count <= sizeof buf);
                actual_count = x_leb128_encode_ul(buf, i);
                CUT_ASSERT_EQUAL(expected_count, actual_count);
                CUT_ASSERT_EQUAL(0, (buf[actual_count -1] & 0x80));
                for (j = 0; j < actual_count -1; j++) {
                        CUT_ASSERT_EQUAL(0x80, (buf[j] & 0x80));
                }
                i <<= 1;
        } while (i);

        return 0;
}

static int test3_leb128_encode_ll(void)
{
        long long i = 1, iterations = 0;
        unsigned char buf[10];
        size_t expected_count, actual_count, j;

        do {
                reset_buffer(buf, sizeof buf);
                expected_count = encoding_count_ll(i);
                CUT_ASSERT_TRUE(expected_count <= sizeof buf);
                actual_count = x_leb128_encode_ll(buf, i);
                CUT_ASSERT_EQUAL(expected_count, actual_count);
                CUT_ASSERT_EQUAL(0, (buf[actual_count -1] & 0x80));
                for (j = 0; j < actual_count -1; j++) {
                        CUT_ASSERT_EQUAL(0x80, (buf[j] & 0x80));
                }
                i <<= 1;
        } while (++iterations != sizeof(i) * 8);

        CUT_ASSERT_EQUAL(64, iterations);

        /* Handle 0 as special case. */
        expected_count = encoding_count_ll(0);
        CUT_ASSERT_TRUE(expected_count <= sizeof buf);
        actual_count = x_leb128_encode_ll(buf, 0);
        CUT_ASSERT_EQUAL(1, actual_count);
        CUT_ASSERT_EQUAL(0, (buf[actual_count -1] & 0x80));

        return 0;
}

static int test4_leb128_encode_l(void)
{
        long i = 1, iterations = 0;
        unsigned char buf[10];
        size_t expected_count, actual_count, j;

        do {
                reset_buffer(buf, sizeof buf);
                expected_count = encoding_count_ll(i-1);
                CUT_ASSERT_TRUE(expected_count <= sizeof buf);
                actual_count = x_leb128_encode_l(buf, i-1);
                CUT_ASSERT_EQUAL(expected_count, actual_count);
                CUT_ASSERT_EQUAL(0, (buf[actual_count -1] & 0x80));
                for (j = 0; j < actual_count -1; j++) {
                        CUT_ASSERT_EQUAL(0x80, (buf[j] & 0x80));
                }
                i <<= 1;
        } while (++iterations != sizeof(i) * 8);

        CUT_ASSERT_TRUE(iterations >= 32 && iterations <= 64);

        return 0;
}

static int test5_leb128_decode_ull(void)
{
        unsigned long long i = 1, iterations = 0;
        unsigned char buf[10];
        size_t expected_count, actual_count;

        do {
                unsigned long long result;
                const unsigned char *p;
                reset_buffer(buf, sizeof buf);
                expected_count = encoding_count_ull(i);
                actual_count = x_leb128_encode_ull(buf, i);
                p = x_leb128_decode_ull(buf, &result);
                CUT_ASSERT_EQUAL(actual_count, p - buf);
                CUT_ASSERT_EQUAL(expected_count, p - buf);
                CUT_ASSERT_EQUAL(i, result);
                i <<= 1;
        } while (++iterations != sizeof(i) * 8);

        CUT_ASSERT_EQUAL(64, iterations);

        return 0;
}

static int test6_leb128_decode_ul(void)
{
        unsigned long i = 1, iterations = 0;
        unsigned char buf[10];
        size_t expected_count, actual_count;

        do {
                unsigned long result;
                const unsigned char *p;
                reset_buffer(buf, sizeof buf);
                expected_count = encoding_count_ull(i);
                actual_count = x_leb128_encode_ul(buf, i);
                p = x_leb128_decode_ul(buf, &result);
                CUT_ASSERT_EQUAL(actual_count, p - buf);
                CUT_ASSERT_EQUAL(expected_count, p - buf);
                CUT_ASSERT_EQUAL(i, result);
                i <<= 1;
        } while (++iterations != sizeof(i) * 8);

        CUT_ASSERT_TRUE(iterations >= 32 && iterations <= 64);

        return 0;
}

static int test7_leb128_decode_ll(void)
{
	long long i = 1, iterations = 0;
        unsigned char buf[10];

        do {
                const unsigned char *p;
                long long result;
		size_t expected_count, actual_count;
                reset_buffer(buf, sizeof buf);
                expected_count = encoding_count_ll(-i);
                CUT_ASSERT_TRUE(expected_count <= sizeof buf);
                actual_count = x_leb128_encode_ll(buf, -i);
                p = x_leb128_decode_ll(buf, &result);
                CUT_ASSERT_EQUAL(actual_count, p - buf);
                CUT_ASSERT_EQUAL(expected_count, p - buf);
                CUT_ASSERT_EQUAL(-i, result);
                i <<= 1;
        } while (++iterations != sizeof(i) * 8);

        CUT_ASSERT_TRUE(iterations >= 32 && iterations <= 64);

        return 0;
}

static int test8_leb128_decode_l(void)
{
        long val = -1, last_val;
        unsigned char buf[10];
        size_t expected_count, actual_count;
	int i, iterations = sizeof(val) * 8;

	for (i = 0; i < iterations; i++) {
                const unsigned char *p;
                long result;
                reset_buffer(buf, sizeof buf);
                expected_count = encoding_count_ll(val);
                actual_count = x_leb128_encode_l(buf, val);
                p = x_leb128_decode_l(buf, &result);
                CUT_ASSERT_EQUAL(actual_count, p - buf);
                CUT_ASSERT_EQUAL(expected_count, p - buf);
                CUT_ASSERT_EQUAL(val, result);

#if 0
		reset_buffer(buf, sizeof buf);
                expected_count = encoding_count_ll(val+1);
                actual_count = x_leb128_encode_l(buf, val+1);
                p = x_leb128_decode_l(buf, &result);
                CUT_ASSERT_EQUAL(actual_count, p - buf);
                CUT_ASSERT_EQUAL(expected_count, p - buf);
                CUT_ASSERT_EQUAL(val+1, result);
#endif
#if 0
		reset_buffer(buf, sizeof buf);
                expected_count = encoding_count_ll(val-1);
                actual_count = x_leb128_encode_l(buf, val-1);
                p = x_leb128_decode_l(buf, &result);
                CUT_ASSERT_EQUAL(actual_count, p - buf);
                CUT_ASSERT_EQUAL(expected_count, p - buf);
                CUT_ASSERT_EQUAL(val-1, result);
#endif
		last_val = val;
                val <<= 1;
	}

        CUT_ASSERT_TRUE(iterations >= 32 && iterations <= 64);
	CUT_ASSERT_EQUAL((-LONG_MAX-1), last_val);

        return 0;
}

CUT_BEGIN_TEST_HARNESS
CUT_RUN_TEST(test0_known_unsigned_sizes);
CUT_RUN_TEST(test0_known_signed_sizes);
CUT_RUN_TEST(test1_leb128_encode_ull);
CUT_RUN_TEST(test2_leb128_encode_ul);
CUT_RUN_TEST(test3_leb128_encode_ll);
CUT_RUN_TEST(test4_leb128_encode_l);
CUT_RUN_TEST(test5_leb128_decode_ull);
CUT_RUN_TEST(test6_leb128_decode_ul);
CUT_RUN_TEST(test7_leb128_decode_ll);
CUT_RUN_TEST(test8_leb128_decode_l);

CUT_END_TEST_HARNESS
