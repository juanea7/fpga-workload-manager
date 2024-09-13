#ifndef _MERGE_H_
#define _MERGE_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MERGE_SIZE 2048
#define MERGE_TYPE int32_t
#define MERGE_TYPE_MAX INT32_MAX

////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct merge_bench_args_t {
  MERGE_TYPE a[MERGE_SIZE];
};

#endif /*_MERGE_H_*/