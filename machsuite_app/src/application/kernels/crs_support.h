///// Per-benchmark files

#ifndef _CRS_SUPPORT_H_
#define _CRS_SUPPORT_H_

#include "crs.h"

const int CRS_INPUT_SIZE = sizeof(struct crs_bench_args_t);
void crs_input_to_data(int fd, void *vdata);
void crs_data_to_input(int fd, void *vdata);
void crs_output_to_data(int fd, void *vdata);
void crs_data_to_output(int fd, void *vdata);
int crs_check_data(void *vdata, void *vref);

#endif /*_CRS_SUPPORT_H_*/