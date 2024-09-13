#include "aes.h"
#include <string.h>
#include <sys/time.h>
#include "machsuite_support.h"


/* Input format:
%%: Section 1
uint8_t[32]: key
%%: Section 2
uint8_t[16]: input-text
*/

void aes_input_to_data(int fd, void *vdata) {
  struct aes_bench_args_t *data = (struct aes_bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct aes_bench_args_t));
  // Load input string
  p = readfile(fd);
  // Section 1: key
  s = find_section_start(p,1);
  parse_uint8_t_array(s, data->k, 32);
  // Section 2: input-text
  s = find_section_start(p,2);
  parse_uint8_t_array(s, data->buf, 16);
  free(p);
}

void aes_data_to_input(int fd, void *vdata) {
  struct aes_bench_args_t *data = (struct aes_bench_args_t *)vdata;
  // Section 1
  write_section_header(fd);
  write_uint8_t_array(fd, data->k, 32);
  // Section 2
  write_section_header(fd);
  write_uint8_t_array(fd, data->buf, 16);
}

/* Output format:
%% Section 1
uint8_t[16]: output-text
*/

void aes_output_to_data(int fd, void *vdata) {
  struct aes_bench_args_t *data = (struct aes_bench_args_t *)vdata;

  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct aes_bench_args_t));
  // Load input string
  p = readfile(fd);
  // Section 1: output-text
  s = find_section_start(p,1);
  parse_uint8_t_array(s, data->buf, 16);
  free(p);
}

void aes_data_to_output(int fd, void *vdata) {
  struct aes_bench_args_t *data = (struct aes_bench_args_t *)vdata;
  // Section 1
  write_section_header(fd);
  write_uint8_t_array(fd, data->buf, 16);
}

int aes_check_data( void *vdata, void *vref ) {
  struct aes_bench_args_t *data = (struct aes_bench_args_t *)vdata;
  struct aes_bench_args_t *ref = (struct aes_bench_args_t *)vref;
  int has_errors = 0;

  // Exact compare encrypted output buffers
  has_errors |= memcmp(&data->buf, &ref->buf, 16*sizeof(uint8_t));

  // Return true if it's correct.
  return !has_errors;
}
