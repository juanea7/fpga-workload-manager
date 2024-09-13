/*
Based on algorithm described here:
http://www.cs.berkeley.edu/~mhoemmen/matrix-seminar/slides/UCB_sparse_tutorial_1.pdf
*/

#ifndef _CRS_H_
#define _CRS_H_

#include <stdlib.h>
#include <stdio.h>

// These constants valid for the IEEE 494 bus interconnect matrix
#define CRS_NNZ 1666
#define CRS_N 494

#define CRS_TYPE float //double
////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct crs_bench_args_t {
  CRS_TYPE val[CRS_NNZ];
  int32_t cols[CRS_NNZ];
  int32_t rowDelimiters[CRS_N+1];
  CRS_TYPE vec[CRS_N];
  CRS_TYPE out[CRS_N];
};

#endif /*_CRS_H_*/