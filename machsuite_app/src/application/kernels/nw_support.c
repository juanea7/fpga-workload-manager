#include "nw.h"
#include <string.h>
#include <sys/time.h>
#include "machsuite_support.h"


/* Input format:
%% Section 1
char[]: sequence A
%% Section 2
char[]: sequence B
*/

void nw_input_to_data(int fd, void *vdata) {
  struct nw_bench_args_t *data = (struct nw_bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct nw_bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  parse_string(s, data->seqA, NW_ALEN);

  s = find_section_start(p,2);
  parse_string(s, data->seqB, NW_BLEN);
  free(p);

}

void nw_data_to_input(int fd, void *vdata) {
  struct nw_bench_args_t *data = (struct nw_bench_args_t *)vdata;

  write_section_header(fd);
  write_string(fd, data->seqA, NW_ALEN);

  write_section_header(fd);
  write_string(fd, data->seqB, NW_BLEN);

  write_section_header(fd);
}

/* Output format:
%% Section 1
char[sum_size]: aligned sequence A
%% Section 2
char[sum_size]: aligned sequence B
*/

void nw_output_to_data(int fd, void *vdata) {
  struct nw_bench_args_t *data = (struct nw_bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct nw_bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  parse_string(s, data->alignedA, NW_ALEN+NW_BLEN);

  s = find_section_start(p,2);
  parse_string(s, data->alignedB, NW_ALEN+NW_BLEN);
  free(p);
}

void nw_data_to_output(int fd, void *vdata) {
  struct nw_bench_args_t *data = (struct nw_bench_args_t *)vdata;

  write_section_header(fd);
  write_string(fd, data->alignedA, NW_ALEN+NW_BLEN);

  write_section_header(fd);
  write_string(fd, data->alignedB, NW_ALEN+NW_BLEN);

  write_section_header(fd);
}

int nw_check_data( void *vdata, void *vref ) {
  struct nw_bench_args_t *data = (struct nw_bench_args_t *)vdata;
  struct nw_bench_args_t *ref = (struct nw_bench_args_t *)vref;
  int has_errors = 0;

  has_errors |= memcmp(data->alignedA, ref->alignedA, NW_ALEN+NW_BLEN);
  has_errors |= memcmp(data->alignedB, ref->alignedB, NW_ALEN+NW_BLEN);

  // Return true if it's correct.
  return !has_errors;
}
