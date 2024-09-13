#include <string.h>
#include <sys/time.h> // struct timeval, gettimeofday()
#include "stencil3d.h"
#include "machsuite_support.h"


#define EPSILON (1.0e-6)

/* Input format:
%% Section 1
TYPE[2]: stencil coefficients (inner/outer)
%% Section 2
TYPE[SIZE]: input matrix
*/

void stencil3d_input_to_data(int fd, void *vdata) {
  struct stencil3d_bench_args_t *data = (struct stencil3d_bench_args_t *)vdata;
  char *p, *s;
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,STENCIL3D_TYPE,_array)(s, data->C, 2);

  s = find_section_start(p,2);
  STAC(parse_,STENCIL3D_TYPE,_array)(s, data->orig, STENCIL3D_SIZE);
  free(p);
}

void stencil3d_data_to_input(int fd, void *vdata) {
  struct stencil3d_bench_args_t *data = (struct stencil3d_bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,STENCIL3D_TYPE,_array)(fd, data->C, 2);

  write_section_header(fd);
  STAC(write_,STENCIL3D_TYPE,_array)(fd, data->orig, STENCIL3D_SIZE);
}

/* Output format:
%% Section 1
TYPE[SIZE]: solution matrix
*/

void stencil3d_output_to_data(int fd, void *vdata) {
  struct stencil3d_bench_args_t *data = (struct stencil3d_bench_args_t *)vdata;
  char *p, *s;
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,STENCIL3D_TYPE,_array)(s, data->sol, STENCIL3D_SIZE);
  free(p);
}

void stencil3d_data_to_output(int fd, void *vdata) {
  struct stencil3d_bench_args_t *data = (struct stencil3d_bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,STENCIL3D_TYPE,_array)(fd, data->sol, STENCIL3D_SIZE);
}

int stencil3d_check_data( void *vdata, void *vref ) {
  struct stencil3d_bench_args_t *data = (struct stencil3d_bench_args_t *)vdata;
  struct stencil3d_bench_args_t *ref = (struct stencil3d_bench_args_t *)vref;
  int has_errors = 0;
  int i;
  STENCIL3D_TYPE diff;

  for(i=0; i<STENCIL3D_SIZE; i++) {
    diff = data->sol[i] - ref->sol[i];
    has_errors |= (diff<-EPSILON) || (EPSILON<diff);
  }

  // Return true if it's correct.
  return !has_errors;
}
