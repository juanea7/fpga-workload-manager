///// Per-benchmark files

#ifndef _STENCIL3D_SUPPORT_H_
#define _STENCIL3D_SUPPORT_H_

#include "stencil3d.h"

const int STENCIL3D_INPUT_SIZE = sizeof(struct stencil3d_bench_args_t);
void stencil3d_input_to_data(int fd, void *vdata);
void stencil3d_data_to_input(int fd, void *vdata);
void stencil3d_output_to_data(int fd, void *vdata);
void stencil3d_data_to_output(int fd, void *vdata);
int stencil3d_check_data(void *vdata, void *vref);

#endif /*_STENCIL3D_SUPPORT_H_*/