#ifndef _NW_H_
#define _NW_H_

#include <stdio.h>
#include <stdlib.h>

#define NW_ALEN 63
#define NW_BLEN 63
////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct nw_bench_args_t {
  char seqA[NW_ALEN];
  char seqB[NW_BLEN];
  char alignedA[NW_ALEN+NW_BLEN];
  char alignedB[NW_ALEN+NW_BLEN];
  int M[(NW_ALEN+1)*(NW_BLEN+1)];
  char ptr[(NW_ALEN+1)*(NW_BLEN+1)];
};

#endif /*_NW_H_*/