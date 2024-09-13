#ifndef _STENCIL2D_H_
#define _STENCIL2D_H_

#include <stdio.h>
#include <stdlib.h>

//Define input sizes
#define stencil2d_col_size 64
#define stencil2d_row_size 64
#define stencil2d_f_size 9

//Data Bounds
#define STENCIL2D_TYPE int32_t
#define STENCIL2D_MAX 1000
#define STENCIL2D_MIN 1

//Set number of iterations to execute
#define STENCIL2D_MAX_ITERATION 1

////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct stencil2d_bench_args_t {
    STENCIL2D_TYPE orig[stencil2d_row_size*stencil2d_col_size];
    STENCIL2D_TYPE sol[stencil2d_row_size*stencil2d_col_size];
    STENCIL2D_TYPE filter[stencil2d_f_size];
};

#endif /*_STENCIL2D_H_*/