#include "crs.h"
#include <string.h>
#include <sys/time.h>
#include "machsuite_support.h"


#define EPSILON 1.0e-3 //1.0e-6

/* Input format:
%% Section 1
CRS_TYPE[NNZ]: the nonzeros of the matrix
%% Section 2
int32_t[NNZ]: the column index of the nonzeros
%% Section 3
int32_t[N+1]: the start of each row of nonzeros
%% Section 4
CRS_TYPE[N]: the dense vector
*/

void crs_input_to_data(int fd, void *vdata) {
  struct crs_bench_args_t *data = (struct crs_bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct crs_bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,CRS_TYPE,_array)(s, data->val, CRS_NNZ);

  s = find_section_start(p,2);
  parse_int32_t_array(s, data->cols, CRS_NNZ);

  s = find_section_start(p,3);
  parse_int32_t_array(s, data->rowDelimiters, CRS_N+1);

  s = find_section_start(p,4);
  STAC(parse_,CRS_TYPE,_array)(s, data->vec, CRS_N);
  free(p);
}

void crs_data_to_input(int fd, void *vdata) {
  struct crs_bench_args_t *data = (struct crs_bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,CRS_TYPE,_array)(fd, data->val, CRS_NNZ);

  write_section_header(fd);
  write_int32_t_array(fd, data->cols, CRS_NNZ);

  write_section_header(fd);
  write_int32_t_array(fd, data->rowDelimiters, CRS_N+1);

  write_section_header(fd);
  STAC(write_,CRS_TYPE,_array)(fd, data->vec, CRS_N);
}

/* Output format:
%% Section 1
CRS_TYPE[N]: The output vector
*/

void crs_output_to_data(int fd, void *vdata) {
  struct crs_bench_args_t *data = (struct crs_bench_args_t *)vdata;
  char *p, *s;
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,CRS_TYPE,_array)(s, data->out, CRS_N);
  free(p);
}

void crs_data_to_output(int fd, void *vdata) {
  struct crs_bench_args_t *data = (struct crs_bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,CRS_TYPE,_array)(fd, data->out, CRS_N);
}

int crs_check_data( void *vdata, void *vref ) {
  struct crs_bench_args_t *data = (struct crs_bench_args_t *)vdata;
  struct crs_bench_args_t *ref = (struct crs_bench_args_t *)vref;
  int has_errors = 0;
  int i;
  CRS_TYPE diff;

  for(i=0; i<CRS_N; i++) {
    diff = data->out[i] - ref->out[i];
    has_errors |= (diff<-EPSILON) || (EPSILON<diff);
  }

  // Return true if it's correct.
  return !has_errors;
}
