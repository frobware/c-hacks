#ifndef _leb128_h_
#define _leb128_h_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t x_leb128_encode_ull(void *buf, unsigned long long value);
size_t x_leb128_encode_ul(void *buf, unsigned long value);
size_t x_leb128_encode_l(void *buf, long value);
size_t x_leb128_encode_ll(void *buf, long long value);

const unsigned char *x_leb128_decode_ul(const void *buf, unsigned long *result);
const unsigned char *x_leb128_decode_ull(const void *buf, unsigned long long *result);
const unsigned char *x_leb128_decode_l(const void *buf, long *result);
const unsigned char *x_leb128_decode_ll(const void *buf, long long *result);

#ifdef _cplusplus
}
#endif  /* __cplusplus */

#endif  /* _leb128_h_ */
