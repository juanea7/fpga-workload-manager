/*
Implementation based on:
Hong, Oguntebi, Olukotun. "Efficient Parallel Graph Exploration on Multi-Core CPU and GPU." PACT, 2011.
*/

#ifndef _QUEUE_H_
#define _QUEUE_H_

//Ponwer el endif y seguir con los demás kernels, y compilar y verque tal

#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

// Terminology (but not values) from graph500 spec
//   graph density = 2^-(2*QUEUE_SCALE - QUEUE_EDGE_FACTOR)
#define QUEUE_SCALE 8
#define QUEUE_EDGE_FACTOR 16

#define QUEUE_N_NODES (1<<QUEUE_SCALE)
#define QUEUE_N_EDGES (QUEUE_N_NODES*QUEUE_EDGE_FACTOR)

// upper limit
#define QUEUE_N_LEVELS 10

// Larger than necessary for small graphs, but appropriate for large ones
typedef uint32_t queue_edge_index_t;
typedef uint32_t queue_node_index_t;

typedef struct queue_edge_t_struct {
  // These fields are common in practice, but we elect not to use them.
  //weight_t weight;
  //node_index_t src;
  queue_node_index_t dst;
} queue_edge_t;

typedef struct queue_node_t_struct {
  queue_edge_index_t edge_begin;
  queue_edge_index_t edge_end;
} queue_node_t;

typedef uint32_t queue_level_t;
#define QUEUE_MAX_LEVEL INT8_MAX

////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct queue_bench_args_t {
  queue_node_t nodes[QUEUE_N_NODES];
  queue_edge_t edges[QUEUE_N_EDGES];
  queue_node_index_t starting_node;
  queue_level_t level[QUEUE_N_NODES];
  queue_edge_index_t level_counts[QUEUE_N_LEVELS];
};

#endif /*_QUEUE_H_*/