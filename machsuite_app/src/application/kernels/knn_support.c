#include "knn.h"
#include <string.h>
#include <sys/time.h>
#include "machsuite_support.h"


#define EPSILON ((KNN_TYPE)1.0e-3) //((KNN_TYPE)1.0e-6)

/* Input format:
%% Section 1
KNN_TYPE[knn_nAtoms]: x positions
%% Section 2
KNN_TYPE[knn_nAtoms]: y positions
%% Section 3
KNN_TYPE[knn_nAtoms]: z positions
%% Section 4
int32_t[knn_nAtoms*knn_maxNeighbors]: neighbor list
*/

void knn_input_to_data(int fd, void *vdata) {
  struct knn_bench_args_t *data = (struct knn_bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct knn_bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,KNN_TYPE,_array)(s, data->position_x, knn_nAtoms);

  s = find_section_start(p,2);
  STAC(parse_,KNN_TYPE,_array)(s, data->position_y, knn_nAtoms);

  s = find_section_start(p,3);
  STAC(parse_,KNN_TYPE,_array)(s, data->position_z, knn_nAtoms);

  s = find_section_start(p,4);
  parse_int32_t_array(s, data->NL, knn_nAtoms*knn_maxNeighbors);
  free(p);
}

void knn_data_to_input(int fd, void *vdata) {
  struct knn_bench_args_t *data = (struct knn_bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,KNN_TYPE,_array)(fd, data->position_x, knn_nAtoms);

  write_section_header(fd);
  STAC(write_,KNN_TYPE,_array)(fd, data->position_y, knn_nAtoms);

  write_section_header(fd);
  STAC(write_,KNN_TYPE,_array)(fd, data->position_z, knn_nAtoms);

  write_section_header(fd);
  write_int32_t_array(fd, data->NL, knn_nAtoms*knn_maxNeighbors);

}

/* Output format:
%% Section 1
KNN_TYPE[knn_nAtoms]: new x force
%% Section 2
KNN_TYPE[knn_nAtoms]: new y force
%% Section 3
KNN_TYPE[knn_nAtoms]: new z force
*/

void knn_output_to_data(int fd, void *vdata) {
  struct knn_bench_args_t *data = (struct knn_bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct knn_bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,KNN_TYPE,_array)(s, data->force_x, knn_nAtoms);

  s = find_section_start(p,2);
  STAC(parse_,KNN_TYPE,_array)(s, data->force_y, knn_nAtoms);

  s = find_section_start(p,3);
  STAC(parse_,KNN_TYPE,_array)(s, data->force_z, knn_nAtoms);
  free(p);
}

void knn_data_to_output(int fd, void *vdata) {
  struct knn_bench_args_t *data = (struct knn_bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,KNN_TYPE,_array)(fd, data->force_x, knn_nAtoms);

  write_section_header(fd);
  STAC(write_,KNN_TYPE,_array)(fd, data->force_y, knn_nAtoms);

  write_section_header(fd);
  STAC(write_,KNN_TYPE,_array)(fd, data->force_z, knn_nAtoms);
}

int knn_check_data( void *vdata, void *vref ) {
  struct knn_bench_args_t *data = (struct knn_bench_args_t *)vdata;
  struct knn_bench_args_t *ref = (struct knn_bench_args_t *)vref;
  int has_errors = 0;
  int i;
  KNN_TYPE diff_x, diff_y, diff_z;

  for( i=0; i<knn_nAtoms; i++ ) {
    diff_x = data->force_x[i] - ref->force_x[i];
    diff_y = data->force_y[i] - ref->force_y[i];
    diff_z = data->force_z[i] - ref->force_z[i];
    has_errors |= (diff_x<-EPSILON) || (EPSILON<diff_x);
    has_errors |= (diff_y<-EPSILON) || (EPSILON<diff_y);
    has_errors |= (diff_z<-EPSILON) || (EPSILON<diff_z);
  }

  // Return true if it's correct.
  return !has_errors;
}
