/*
Implementation based on algorithm described in:
"Stencil computation optimization and auto-tuning on state-of-the-art multicore architectures"
K. Datta, M. Murphy, V. Volkov, S. Williams, J. Carter, L. Oliker, D. Patterson, J. Shalf, K. Yelick
SC 2008
*/

#ifndef _STENCIL3D_H_
#define _STENCIL3D_H_

#include <stdio.h>
#include <stdlib.h>

//Define input sizes
#define stencil3d_height_size 16 //32
#define stencil3d_col_size 16 //32
#define stencil3d_row_size 16
//Data Bounds
#define STENCIL3D_TYPE int32_t
#define STENCIL3D_MAX 1000
#define STENCIL3D_MIN 1
//Convenience Macros
#define STENCIL3D_SIZE (stencil3d_row_size * stencil3d_col_size * stencil3d_height_size)
#define INDX(_stencil3d_row_size,_stencil3d_col_size,_i,_j,_k) ((_i)+_stencil3d_row_size*((_j)+_stencil3d_col_size*(_k)))

////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct stencil3d_bench_args_t {
  STENCIL3D_TYPE C[2];
  STENCIL3D_TYPE orig[STENCIL3D_SIZE];
  STENCIL3D_TYPE sol[STENCIL3D_SIZE];
};

#endif /*_STENCIL3D_H_*/