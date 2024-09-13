/*
Based on algorithm described here:
http://www.cs.berkeley.edu/~mhoemmen/matrix-seminar/slides/UCB_sparse_tutorial_1.pdf
*/

#include "spmv.h"
#include "artico3.h"

//~ void spmv(TYPE val[NNZ], int32_t cols[NNZ], int32_t rowDelimiters[N+1], TYPE vec[N], TYPE out[N]){
A3_KERNEL(a3in_t val, a3in_t cols, a3in_t rowDelimiters, a3in_t vec, a3out_t out) {
    int i, j;
    TYPE sum, Si;

    spmv_1 : for(i = 0; i < N; i++){
        sum = 0; Si = 0;
        int tmp_begin = rowDelimiters[i];
        int tmp_end = rowDelimiters[i+1];
        spmv_2 : for (j = tmp_begin; j < tmp_end; j++){
            Si = a3tof(val[j]) * a3tof(vec[cols[j]]);
            sum = sum + Si;
        }
        out[i] = ftoa3(sum);
    }
}


