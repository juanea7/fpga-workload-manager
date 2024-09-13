///// Per-benchmark files

#ifndef _BULK_SUPPORT_H_
#define _BULK_SUPPORT_H_

#include "bulk.h"

const int BULK_INPUT_SIZE = sizeof(struct bulk_bench_args_t);
void bulk_input_to_data(int fd, void *vdata);
void bulk_data_to_input(int fd, void *vdata);
void bulk_output_to_data(int fd, void *vdata);
void bulk_data_to_output(int fd, void *vdata);
int bulk_check_data(void *vdata, void *vref);

#endif /*_BULK_SUPPORT_H_*/