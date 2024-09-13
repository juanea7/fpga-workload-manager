#include "bulk.h"
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "machsuite_support.h"


/* Input format:
%% Section 1
uint32_t[1]: starting node
%% Section 2
uint32_t[BULK_N_NODES*2]: node structures (start and end indices of edge lists)
%% Section 3
uint32_t[BULK_N_EDGES]: edges structures (just destination node id)
*/

void bulk_input_to_data(int fd, void *vdata) {
  struct bulk_bench_args_t *data = (struct bulk_bench_args_t *)vdata;
  char *p, *s;
  uint32_t *nodes;
  int64_t i;

  // Zero-out everything.
  memset(vdata,0,sizeof(struct bulk_bench_args_t));
  // Max-ify levels
  for(i=0; i<BULK_N_NODES; i++) {
    data->level[i]=BULK_MAX_LEVEL;
  }
  // Load input string
  p = readfile(fd);
  // Section 1: starting node
  s = find_section_start(p,1);
  parse_uint32_t_array(s, &data->starting_node, 1);

  // Section 2: node structures
  s = find_section_start(p,2);
  nodes = (uint32_t *)malloc(BULK_N_NODES*2*sizeof(uint32_t));
  parse_uint32_t_array(s, nodes, BULK_N_NODES*2);
  for(i=0; i<BULK_N_NODES; i++) {
    data->nodes[i].edge_begin = nodes[2*i];
    data->nodes[i].edge_end = nodes[2*i+1];
  }
  free(nodes);
  // Section 3: edge structures
  s = find_section_start(p,3);
  parse_uint32_t_array(s, (uint32_t *)(data->edges), BULK_N_EDGES);
  free(p);
}

void bulk_data_to_input(int fd, void *vdata) {
  uint32_t *nodes;
  int64_t i;

  struct bulk_bench_args_t *data = (struct bulk_bench_args_t *)vdata;
  // Section 1: starting node
  write_section_header(fd);
  write_uint32_t_array(fd, &data->starting_node, 1);
  // Section 2: node structures
  write_section_header(fd);
  nodes = (uint32_t *)malloc(BULK_N_NODES*2*sizeof(uint32_t));
  for(i=0; i<BULK_N_NODES; i++) {
    nodes[2*i]  = data->nodes[i].edge_begin;
    nodes[2*i+1]= data->nodes[i].edge_end;
  }
  write_uint32_t_array(fd, nodes, BULK_N_NODES*2);
  free(nodes);
  // Section 3: edge structures
  write_section_header(fd);
  write_uint32_t_array(fd, (uint32_t *)(&data->edges), BULK_N_EDGES);
}

/* Output format:
%% Section 1
uint32_t[BULK_N_LEVELS]: horizon counts
*/

void bulk_output_to_data(int fd, void *vdata) {
  struct bulk_bench_args_t *data = (struct bulk_bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bulk_bench_args_t));
  // Load input string
  p = readfile(fd);
  // Section 1: horizon counts
  s = find_section_start(p,1);
  parse_uint32_t_array(s, data->level_counts, BULK_N_LEVELS);
  free(p);
}

void bulk_data_to_output(int fd, void *vdata) {
  struct bulk_bench_args_t *data = (struct bulk_bench_args_t *)vdata;
  // Section 1
  write_section_header(fd);
  write_uint32_t_array(fd, data->level_counts, BULK_N_LEVELS);
}

int bulk_check_data( void *vdata, void *vref ) {
  struct bulk_bench_args_t *data = (struct bulk_bench_args_t *)vdata;
  struct bulk_bench_args_t *ref = (struct bulk_bench_args_t *)vref;
  int has_errors = 0;
  int i;

  // Check that the horizons have the same number of nodes
  for(i=0; i<BULK_N_LEVELS; i++) {
    has_errors |= (data->level_counts[i]!=ref->level_counts[i]);
  }

  // Return true if it's correct.
  return !has_errors;
}
