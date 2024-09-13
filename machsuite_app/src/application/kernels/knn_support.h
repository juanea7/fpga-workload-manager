///// Per-benchmark files

#ifndef _KNN_SUPPORT_H_
#define _KNN_SUPPORT_H_

#include "knn.h"

const int KNN_INPUT_SIZE = sizeof(struct knn_bench_args_t);
void knn_input_to_data(int fd, void *vdata);
void knn_data_to_input(int fd, void *vdata);
void knn_output_to_data(int fd, void *vdata);
void knn_data_to_output(int fd, void *vdata);
int knn_check_data(void *vdata, void *vref);

#endif /*_KNN_SUPPORT_H_*/