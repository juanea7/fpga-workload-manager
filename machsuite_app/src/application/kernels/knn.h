/*
Implemenataion based on:
A. Danalis, G. Marin, C. McCurdy, J. S. Meredith, P. C. Roth, K. Spafford, V. Tipparaju, and J. S. Vetter.
The scalable heterogeneous computing (shoc) benchmark suite.
In Proceedings of the 3rd Workshop on General-Purpose Computation on Graphics Processing Units, 2010.
*/

#ifndef _KNN_H_
#define _KNN_H_

#include <stdlib.h>
#include <stdio.h>

#define KNN_TYPE float //double

// Problem Constants
#define knn_nAtoms        256
#define knn_maxNeighbors  16
// LJ coefficients
#define knn_lj1           1.5
#define knn_lj2           2.0
////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct knn_bench_args_t {
  KNN_TYPE force_x[knn_nAtoms];
  KNN_TYPE force_y[knn_nAtoms];
  KNN_TYPE force_z[knn_nAtoms];
  KNN_TYPE position_x[knn_nAtoms];
  KNN_TYPE position_y[knn_nAtoms];
  KNN_TYPE position_z[knn_nAtoms];
  int32_t NL[knn_nAtoms*knn_maxNeighbors];
};

#endif /*_KNN_H_*/