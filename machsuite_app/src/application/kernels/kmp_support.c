#include "kmp.h"
#include <string.h>
#include <sys/time.h>
#include "machsuite_support.h"


/* Input format:
%% Section 1
char[KMP_PATTERN_SIZE]: pattern
%% Section 2
char[KMP_STRING_SIZE]: text
*/

void kmp_input_to_data(int fd, void *vdata) {
  struct kmp_bench_args_t *data = (struct kmp_bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct kmp_bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  parse_string(s, data->pattern, KMP_PATTERN_SIZE);

  s = find_section_start(p,2);
  parse_string(s, data->input, KMP_STRING_SIZE);
  free(p);
}

void kmp_data_to_input(int fd, void *vdata) {
  struct kmp_bench_args_t *data = (struct kmp_bench_args_t *)vdata;

  write_section_header(fd);
  write_string(fd, data->pattern, KMP_PATTERN_SIZE);

  write_section_header(fd);
  write_string(fd, data->input, KMP_STRING_SIZE);
}

/* Output format:
%% Section 1
int[1]: number of matches
*/

void kmp_output_to_data(int fd, void *vdata) {
  struct kmp_bench_args_t *data = (struct kmp_bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct kmp_bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  parse_int32_t_array(s, data->n_matches, 1);
  free(p);
}

void kmp_data_to_output(int fd, void *vdata) {
  struct kmp_bench_args_t *data = (struct kmp_bench_args_t *)vdata;

  write_section_header(fd); // No section header
  write_int32_t_array(fd, data->n_matches, 1);
}

int kmp_check_data( void *vdata, void *vref ) {
  struct kmp_bench_args_t *data = (struct kmp_bench_args_t *)vdata;
  struct kmp_bench_args_t *ref = (struct kmp_bench_args_t *)vref;
  int has_errors = 0;

  has_errors |= (data->n_matches[0] != ref->n_matches[0]);

  // Return true if it's correct.
  return !has_errors;
}
