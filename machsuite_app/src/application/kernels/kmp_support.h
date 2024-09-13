///// Per-benchmark files

#ifndef _KMP_SUPPORT_H_
#define _KMP_SUPPORT_H_

#include "kmp.h"

const int KMP_INPUT_SIZE = sizeof(struct kmp_bench_args_t);
void kmp_input_to_data(int fd, void *vdata);
void kmp_data_to_input(int fd, void *vdata);
void kmp_output_to_data(int fd, void *vdata);
void kmp_data_to_output(int fd, void *vdata);
int kmp_check_data(void *vdata, void *vref);

#endif /*_KMP_SUPPORT_H_*/