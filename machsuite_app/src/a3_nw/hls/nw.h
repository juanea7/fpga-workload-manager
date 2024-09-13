#include <stdio.h>
#include <stdlib.h>
//~ #include "support.h"

#define ALEN 63
#define BLEN 63

//~ void needwun(char SEQA[ALEN], char SEQB[BLEN],
             //~ char alignedA[ALEN+BLEN], char alignedB[ALEN+BLEN],
             //~ int M[(ALEN+1)*(BLEN+1)], char ptr[(ALEN+1)*(BLEN+1)]);
////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct bench_args_t {
  char seqA[ALEN];
  char seqB[BLEN];
  char alignedA[ALEN+BLEN];
  char alignedB[ALEN+BLEN];
  int M[(ALEN+1)*(BLEN+1)];
  char ptr[(ALEN+1)*(BLEN+1)];
};
