///// Per-benchmark files

#ifndef _STRIDED_SUPPORT_H_
#define _STRIDED_SUPPORT_H_

#include "strided.h"

const int STRIDED_INPUT_SIZE = sizeof(struct strided_bench_args_t);
void strided_input_to_data(int fd, void *vdata);
void strided_data_to_input(int fd, void *vdata);
void strided_output_to_data(int fd, void *vdata);
void strided_data_to_output(int fd, void *vdata);
int strided_check_data(void *vdata, void *vref);

#endif /*_STRIDED_SUPPORT_H_*/