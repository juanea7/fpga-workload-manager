/*
Implementations based on:
Harish and Narayanan. "Accelerating large graph algorithms on the GPU using CUDA." HiPC, 2007.
Hong, Oguntebi, Olukotun. "Efficient Parallel Graph Exploration on Multi-Core CPU and GPU." PACT, 2011.
*/

#ifndef _BULK_H_
#define _BULK_H_

#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

// Terminology (but not values) from graph500 spec
//   graph density = 2^-(2*SCALE - EDGE_FACTOR)
#define BULK_SCALE 8
#define BULK_EDGE_FACTOR 16

#define BULK_N_NODES (1LL<<BULK_SCALE)
#define BULK_N_EDGES (BULK_N_NODES*BULK_EDGE_FACTOR)

// upper limit
#define BULK_N_LEVELS 10

// Larger than necessary for small graphs, but appropriate for large ones
typedef uint32_t bulk_edge_index_t;
typedef uint32_t bulk_node_index_t;

typedef struct bulk_edge_t_struct {
  // These fields are common in practice, but we elect not to use them.
  //weight_t weight;
  //node_index_t src;
  bulk_node_index_t dst;
} bulk_edge_t;

typedef struct bulk_node_t_struct {
  bulk_edge_index_t edge_begin;
  bulk_edge_index_t edge_end;
} bulk_node_t;

typedef uint32_t bulk_level_t;
#define BULK_MAX_LEVEL INT8_MAX

////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct bulk_bench_args_t {
  bulk_node_t nodes[BULK_N_NODES];
  bulk_edge_t edges[BULK_N_EDGES];
  bulk_node_index_t starting_node;
  bulk_level_t level[BULK_N_NODES];
  bulk_edge_index_t level_counts[BULK_N_LEVELS];
};

#endif /*_BULK_H_*/