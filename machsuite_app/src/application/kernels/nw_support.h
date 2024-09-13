///// Per-benchmark files

#ifndef _NW_SUPPORT_H_
#define _NW_SUPPORT_H_

#include "nw.h"

const int NW_INPUT_SIZE = sizeof(struct nw_bench_args_t);
void nw_input_to_data(int fd, void *vdata);
void nw_data_to_input(int fd, void *vdata);
void nw_output_to_data(int fd, void *vdata);
void nw_data_to_output(int fd, void *vdata);
int nw_check_data(void *vdata, void *vref);

#endif /*_NW_SUPPORT_H_*/