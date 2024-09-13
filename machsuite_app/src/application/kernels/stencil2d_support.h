///// Per-benchmark files

#ifndef _STENCIL2D_SUPPORT_H_
#define _STENCIL2D_SUPPORT_H_

#include "stencil2d.h"

const int STENCIL2D_INPUT_SIZE = sizeof(struct stencil2d_bench_args_t);
void stencil2d_input_to_data(int fd, void *vdata);
void stencil2d_data_to_input(int fd, void *vdata);
void stencil2d_output_to_data(int fd, void *vdata);
void stencil2d_data_to_output(int fd, void *vdata);
int stencil2d_check_data(void *vdata, void *vref);

#endif /*_STENCIL2D_SUPPORT_H_*/