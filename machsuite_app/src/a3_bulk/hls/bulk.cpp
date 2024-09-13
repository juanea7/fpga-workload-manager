/*
Implementations based on:
Harish and Narayanan. "Accelerating large graph algorithms on the GPU using CUDA." HiPC, 2007.
Hong, Oguntebi, Olukotun. "Efficient Parallel Graph Exploration on Multi-Core CPU and GPU." PACT, 2011.
*/

#include "bfs.h"
#include "artico3.h"

//~ void bfs(node_t nodes[N_NODES], edge_t edges[N_EDGES],
            //~ node_index_t starting_node, level_t level[N_NODES],
            //~ edge_index_t level_counts[N_LEVELS])
A3_KERNEL(a3in_t edges, a3inout_t pack) {

  /* ARTICo³ unpacking */
  a3data_t *nodes_begin = &pack[0];
  a3data_t *nodes_end = &pack[N_NODES];
  a3data_t *starting_node = &pack[2*N_NODES];
  a3data_t *level = &pack[2*N_NODES+1];
  a3data_t *level_counts = &pack[3*N_NODES+1];
  /* END ARTICo³ unpacking */

  node_index_t n;
  edge_index_t e;
  level_t horizon;
  edge_index_t cnt;

  level[starting_node[0]] = 0; //level[starting_node] = 0;
  level_counts[0] = 1;

  loop_horizons: for( horizon=0; horizon<N_LEVELS; horizon++ ) {
    cnt = 0;
    // Add unmarked neighbors of the current horizon to the next horizon
    loop_nodes: for( n=0; n<N_NODES; n++ ) {
      if( level[n]==horizon ) {
        edge_index_t tmp_begin = nodes_begin[n]; //nodes[n].edge_begin;
        edge_index_t tmp_end = nodes_end[n]; //nodes[n].edge_end;
        loop_neighbors: for( e=tmp_begin; e<tmp_end; e++ ) {
          node_index_t tmp_dst = edges[e]; //edges[e].dst;
          level_t tmp_level = level[tmp_dst];

          if( tmp_level ==MAX_LEVEL ) { // Unmarked
            level[tmp_dst] = horizon+1;
            ++cnt;
          }
        }
      }
    }
    if( (level_counts[horizon+1]=cnt)==0 )
      break;
  }
}
