/*
*   Byte-oriented AES-256 implementation.
*   All lookup tables replaced with 'on the fly' calculations.
*/

#ifndef _AES_H_
#define _AES_H_

#include <inttypes.h>

typedef struct {
  uint8_t key[32];
  uint8_t enckey[32];
  uint8_t deckey[32];
} aes256_context;

////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct aes_bench_args_t {
  aes256_context ctx;
  uint8_t k[32];
  uint8_t buf[16];
};

#endif /*_AES_H*/