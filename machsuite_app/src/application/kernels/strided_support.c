#include "strided.h"
#include <string.h>
#include <sys/time.h>
#include "machsuite_support.h"


#define EPSILON ((double)1.0e-3) //((double)1.0e-6)

/* Input format:
%% Section 1
double: signal (real part)
%% Section 2
double: signal (complex part)
%% Section 3
double: twiddle factor (real part)
%% Section 4
double: twiddle factor (complex part)
*/

void strided_input_to_data(int fd, void *vdata) {
  struct strided_bench_args_t *data = (struct strided_bench_args_t *)vdata;
  char *p, *s;
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  parse_float_array(s, data->real, STRIDED_FFT_SIZE);

  s = find_section_start(p,2);
  parse_float_array(s, data->img, STRIDED_FFT_SIZE);

  s = find_section_start(p,3);
  parse_float_array(s, data->real_twid, STRIDED_FFT_SIZE/2);

  s = find_section_start(p,4);
  parse_float_array(s, data->img_twid, STRIDED_FFT_SIZE/2);
  free(p);
}

void strided_data_to_input(int fd, void *vdata) {
  struct strided_bench_args_t *data = (struct strided_bench_args_t *)vdata;

  write_section_header(fd);
  write_float_array(fd, data->real, STRIDED_FFT_SIZE);

  write_section_header(fd);
  write_float_array(fd, data->img, STRIDED_FFT_SIZE);

  write_section_header(fd);
  write_float_array(fd, data->real_twid, STRIDED_FFT_SIZE/2);

  write_section_header(fd);
  write_float_array(fd, data->img_twid, STRIDED_FFT_SIZE/2);
}

/* Output format:
%% Section 1
double: freq (real part)
%% Section 2
double: freq (complex part)
*/

void strided_output_to_data(int fd, void *vdata) {
  struct strided_bench_args_t *data = (struct strided_bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct strided_bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  parse_float_array(s, data->real, STRIDED_FFT_SIZE);

  s = find_section_start(p,2);
  parse_float_array(s, data->img, STRIDED_FFT_SIZE);
  free(p);
}

void strided_data_to_output(int fd, void *vdata) {
  struct strided_bench_args_t *data = (struct strided_bench_args_t *)vdata;

  write_section_header(fd);
  write_float_array(fd, data->real, STRIDED_FFT_SIZE);

  write_section_header(fd);
  write_float_array(fd, data->img, STRIDED_FFT_SIZE);
}

int strided_check_data( void *vdata, void *vref ) {
  struct strided_bench_args_t *data = (struct strided_bench_args_t *)vdata;
  struct strided_bench_args_t *ref = (struct strided_bench_args_t *)vref;
  int has_errors = 0;
  int i;
  double real_diff, img_diff;

  for(i=0; i<STRIDED_FFT_SIZE; i++) {
    real_diff = data->real[i] - ref->real[i];
    img_diff = data->img[i] - ref->img[i];
    has_errors |= (real_diff<-EPSILON) || (EPSILON<real_diff);
    //if( has_errors )
      //printf("%d (real): %f (%f)\n", i, real_diff, EPSILON);
    has_errors |= (img_diff<-EPSILON) || (EPSILON<img_diff);
    //if( has_errors )
      //printf("%d (img): %f (%f)\n", i, img_diff, EPSILON);
  }

  // Return true if it's correct.
  return !has_errors;
}
