/*
Implementation based on http://www-igm.univ-mlv.fr/~lecroq/string/node8.html
*/

#ifndef _KMP_H_
#define _KMP_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define KMP_PATTERN_SIZE 4
#define KMP_STRING_SIZE (8192)

////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct kmp_bench_args_t {
  char pattern[KMP_PATTERN_SIZE];
  char input[KMP_STRING_SIZE];
  int32_t kmpNext[KMP_PATTERN_SIZE];
  int32_t n_matches[1];
};

#endif /*_KMP_H_*/