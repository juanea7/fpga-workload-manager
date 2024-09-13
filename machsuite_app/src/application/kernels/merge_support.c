#include "merge.h"
#include <string.h>
#include <sys/time.h>
#include "machsuite_support.h"


/* Input format:
%% Section 1
MERGE_TYPE[MERGE_SIZE]: the array
*/

void merge_input_to_data(int fd, void *vdata) {
  struct merge_bench_args_t *data = (struct merge_bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct merge_bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,MERGE_TYPE,_array)(s, data->a, MERGE_SIZE);
  free(p);
}

void merge_data_to_input(int fd, void *vdata) {
  struct merge_bench_args_t *data = (struct merge_bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,MERGE_TYPE,_array)(fd, data->a, MERGE_SIZE);
}

/* Output format:
%% Section 1
MERGE_TYPE[MERGE_SIZE]: the array
*/

void merge_output_to_data(int fd, void *vdata) {
  struct merge_bench_args_t *data = (struct merge_bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct merge_bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,MERGE_TYPE,_array)(s, data->a, MERGE_SIZE);
  free(p);
}

void merge_data_to_output(int fd, void *vdata) {
  struct merge_bench_args_t *data = (struct merge_bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,MERGE_TYPE,_array)(fd, data->a, MERGE_SIZE);
}

int merge_check_data( void *vdata, void *vref ) {
  struct merge_bench_args_t *data = (struct merge_bench_args_t *)vdata;
  struct merge_bench_args_t *ref = (struct merge_bench_args_t *)vref;
  int has_errors = 0;
  int i;
  MERGE_TYPE data_sum, ref_sum;

  // Check sortedness and sum
  data_sum = data->a[0];
  ref_sum = ref->a[0];
  for( i=1; i<MERGE_SIZE; i++ ) {
    has_errors |= data->a[i-1] > data->a[i];
    data_sum += data->a[i];
    ref_sum += ref->a[i];
  }
  has_errors |= (data_sum!=ref_sum);

  // Return true if it's correct.
  return !has_errors;
}
