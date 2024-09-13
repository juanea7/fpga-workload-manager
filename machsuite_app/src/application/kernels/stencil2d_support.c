#include "stencil2d.h"
#include <string.h>
#include <sys/time.h> // struct timeval, gettimeofday()
#include "machsuite_support.h"


#define EPSILON (1.0e-6)

/* Input format:
%% Section 1
TYPE[stencil2d_row_size*stencil2d_col_size]: input matrix
%% Section 2
TYPE[f_size]: filter coefficients
*/

void stencil2d_input_to_data(int fd, void *vdata) {
  struct stencil2d_bench_args_t *data = (struct stencil2d_bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct stencil2d_bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,STENCIL2D_TYPE,_array)(s, data->orig, stencil2d_row_size*stencil2d_col_size);

  s = find_section_start(p,2);
  STAC(parse_,STENCIL2D_TYPE,_array)(s, data->filter, stencil2d_f_size);
  free(p);
}

void stencil2d_data_to_input(int fd, void *vdata) {
  struct stencil2d_bench_args_t *data = (struct stencil2d_bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,STENCIL2D_TYPE,_array)(fd, data->orig, stencil2d_row_size*stencil2d_col_size);

  write_section_header(fd);
  STAC(write_,STENCIL2D_TYPE,_array)(fd, data->filter, stencil2d_f_size);
}

/* Output format:
%% Section 1
TYPE[stencil2d_row_size*stencil2d_col_size]: solution matrix
*/

void stencil2d_output_to_data(int fd, void *vdata) {
  struct stencil2d_bench_args_t *data = (struct stencil2d_bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct stencil2d_bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,STENCIL2D_TYPE,_array)(s, data->sol, stencil2d_row_size*stencil2d_col_size);
  free(p);
}

void stencil2d_data_to_output(int fd, void *vdata) {
  struct stencil2d_bench_args_t *data = (struct stencil2d_bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,STENCIL2D_TYPE,_array)(fd, data->sol, stencil2d_row_size*stencil2d_col_size);
}

int stencil2d_check_data( void *vdata, void *vref ) {
  struct stencil2d_bench_args_t *data = (struct stencil2d_bench_args_t *)vdata;
  struct stencil2d_bench_args_t *ref = (struct stencil2d_bench_args_t *)vref;
  int has_errors = 0;
  int row, col;
  STENCIL2D_TYPE diff;

  for(row=0; row<stencil2d_row_size; row++) {
    for(col=0; col<stencil2d_col_size; col++) {
      diff = data->sol[row*stencil2d_col_size + col] - ref->sol[row*stencil2d_col_size + col];
      has_errors |= (diff<-EPSILON) || (EPSILON<diff);
    }
  }

  // Return true if it's correct.
  return !has_errors;
}
