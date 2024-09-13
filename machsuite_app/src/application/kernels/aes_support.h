///// Per-benchmark files

#ifndef _AES_SUPPORT_H_
#define _AES_SUPPORT_H_

#include "aes.h"

const int AES_INPUT_SIZE = sizeof(struct aes_bench_args_t);
void aes_input_to_data(int fd, void *vdata);
void aes_data_to_input(int fd, void *vdata);
void aes_output_to_data(int fd, void *vdata);
void aes_data_to_output(int fd, void *vdata);
int aes_check_data(void *vdata, void *vref);

#endif /*_AES_SUPPORT_H_*/