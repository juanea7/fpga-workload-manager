///// Per-benchmark files

#ifndef _MERGE_SUPPORT_H_
#define _MERGE_SUPPORT_H_

#include "merge.h"

const int MERGE_INPUT_SIZE = sizeof(struct merge_bench_args_t);
void merge_input_to_data(int fd, void *vdata);
void merge_data_to_input(int fd, void *vdata);
void merge_output_to_data(int fd, void *vdata);
void merge_data_to_output(int fd, void *vdata);
int merge_check_data(void *vdata, void *vref);

#endif /*_MERGE_SUPPORT_H_*/