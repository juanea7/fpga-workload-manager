#ifndef _STRIDED_H_
#define _STRIDED_H_

#include <stdio.h>
#include <stdlib.h>

#define STRIDED_FFT_SIZE 1024
#define STRIDED_twoPI 6.28318530717959

////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct strided_bench_args_t {
        float real[STRIDED_FFT_SIZE];
        float img[STRIDED_FFT_SIZE];
        float real_twid[STRIDED_FFT_SIZE/2];
        float img_twid[STRIDED_FFT_SIZE/2];
};

#endif /*_STRIDED_H_*/