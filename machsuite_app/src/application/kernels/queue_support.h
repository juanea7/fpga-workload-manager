///// Per-benchmark files

#ifndef _QUEUE_SUPPORT_H_
#define _QUEUE_SUPPORT_H_

#include "queue.h"

const int QUEUE_INPUT_SIZE = sizeof(struct queue_bench_args_t);
void queue_input_to_data(int fd, void *vdata);
void queue_data_to_input(int fd, void *vdata);
void queue_output_to_data(int fd, void *vdata);
void queue_data_to_output(int fd, void *vdata);
int queue_check_data(void *vdata, void *vref);

#endif /*_QUEUE_SUPPORT_H_*/