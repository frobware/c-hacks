/*
 * Utility functions to encode/decode LEB128 values.
 *
 * Algorithms based on the pseudo code from the Dwarf3 spec.  The
 * following documentation is taken from Wikipedia.
 *
 * Encoding Format
 * ---------------
 *
 * LEB128 allows small numbers to be stored in a single byte, while
 * also allowing encoding of arbitrarily long numbers. There are 2
 * versions of LEB128: unsigned LEB128 and signed LEB128. The decoder
 * must know whether the encoded value is unsigned or signed LEB128.
 *
 * Unsigned LEB128
 * ---------------
 * To encode an unsigned number using unsigned LEB128 first represent
 * the number in binary. Then zero extend the number up to a multiple
 * of 7 bits (such that the most significant 7 bits are not all
 * 0). Break the number up into groups of 7 bits. Output one encoded
 * byte for each 7 bit group, from least significant to most
 * significant group. Each byte will have the group in its 7 least
 * significant bits. Set the most significant bit on each byte except
 * the last byte. The number zero is encoded as a single byte 0x00.
 *
 * For example the unsigned number 624485 (0x98765) is encoded as 0xE5
 * 0x8E 0x26.
 *
 * Signed LEB128
 * -------------
 * A signed number is represented similarly, except that the two's
 * complement number is sign extended up to a multiple of 7 bits
 * (ensuring that the most significant bit is zero for a positive
 * number and one for a negative number). Then the number is broken
 * into groups as for the unsigned encoding.  For example the signed
 * number -624485 (0xFFF6789b) is encoded as 0x9b 0xf1 0x59.
 *
 *
 * Encoded Sequence	As sleb128	As uleb128
 * 00			0		0
 * 01			1		1
 * 7f			-1		127
 * 80 7f		-128		16256
 */

#include "leb128.h"
#include <assert.h>

/*
 * Encodes VALUE in unsigned LEB128 format to the buffer BUF.
 *
 * BUF is assumed to be big enough to accommodate the encoding.
 *
 * Returns the number of bytes the encoding required.
 */
size_t x_leb128_encode_ul(void *buf, unsigned long value)
{
	unsigned char *p = buf;

	do {
		*p = value & 0x7f;
		value >>= 7;
		if (value != 0) {
			/* still more bytes to come */
			*p |= 0x80;
		}
		p++;
	} while (value);

	return p - (unsigned char *)buf;
}

/*
 * Encodes VALUE in unsigned LEB128 format to the buffer BUF.
 *
 * BUF is assumed to be big enough to accommodate the encoding.
 *
 * Returns the number of bytes the encoding required.
 */
size_t x_leb128_encode_ull(void *buf, unsigned long long value)
{
	unsigned char *p = buf;

	do {
		*p = value & 0x7f;
		value >>= 7;
		if (value != 0) {
			/* still more bytes to come */
			*p |= 0x80;
		}
		p++;
	} while (value);

	return p - (unsigned char *)buf;
}

/*
 * Encodes VALUE in unsigned LEB128 format to the buffer BUF.
 *
 * BUF is assumed to be big enough to accommodate the encoding.
 *
 * Returns the number of bytes the encoding required.
 */
size_t x_leb128_encode_l(void *buf, long value)
{
	int more = 1;
	unsigned char *p = buf;

	while (more) {
		unsigned char byte = value & 0x7f;
		value >>= 7;
		/* Sign bit of byte is second high order bit (0x40),
		 * not 0x80. If (value == 0) && sign bit of byte is
		 * clear OR (value == -1) && sign bit of byte is set,
		 * then we're done. */
		if ((value ==  0 && ((byte & 0x40) != 0x40)) ||
		    (value == -1 && ((byte & 0x40) == 0x40))) {
			more = 0;
		} else {
			byte |= 0x80;
		}
		*p++ = byte;
	}

	return p - (unsigned char *)buf;
}

size_t x_leb128_encode_ll(void *buf, long long value)
{
	int more = 1;
	unsigned char *p = buf;

	while (more) {
		unsigned char byte = value & 0x7f;
		value >>= 7;
		/* Sign bit of byte is second high order bit (0x40),
		 * not 0x80. If (value == 0) && sign bit of byte is
		 * clear OR (value == -1) && sign bit of byte is set,
		 * then we're done. */
		if ((value ==  0 && ((byte & 0x40) != 0x40)) ||
		    (value == -1 && ((byte & 0x40) == 0x40))) {
			more = 0;
		} else {
			byte |= 0x80;
		}
		*p++ = byte;
	}

	return p - (unsigned char *)buf;
}

const unsigned char *x_leb128_decode_ul(const void *buf, unsigned long *result)
{
	int shift = 0;
	unsigned long value = 0;
	unsigned long byte;
	unsigned char *p = (unsigned char *)buf;

	while (1) {
		byte = *p++;
		value |= (byte & 0x7f) << shift;
		if ((byte & 0x80) == 0)
			break;
		shift += 7;
	}

	*result = value;
	return p;
}

const unsigned char *x_leb128_decode_ull(const void *buf, unsigned long long *result)
{
	int shift = 0;
	unsigned long long value = 0;
	unsigned long long byte;
	unsigned char *p = (unsigned char *)buf;

	while (1) {
		byte = *p++;
		value |= (byte & 0x7f) << shift;
		if ((byte & 0x80) == 0)
			break;
		shift += 7;
	}

	*result = value;
	return p;
}

const unsigned char *x_leb128_decode_l(const void *buf, long *result)
{
	int shift = 0;
	long value = 0;
	unsigned char *p = (unsigned char *)buf;
	long byte;
	
	do {
		byte = *p++;
		value |= (byte & 0x7f) << shift;
		shift += 7;
	} while ((byte & 0x80) != 0);

	if ((shift < (sizeof *result * 8)) && (byte & 0x40)) {
		value |= -(1L << shift); /* sign extend */
	}

	*result = value;
	return p;
}

const unsigned char *x_leb128_decode_ll(const void *buf, long long *result)
{
	int shift = 0;
	long long value = 0;
	long long byte;
	unsigned char *p = (unsigned char *)buf;

	do {
		byte = *p++;
		value |= (byte & 0x7f) << shift;
		shift += 7;
	} while ((byte & 0x80) != 0);

	if ((shift < (sizeof *result * 8)) && (byte & 0x40))
		value |= -(1LL << shift); /* sign extend */

	*result = value;

	return p;
}
