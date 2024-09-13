/*
Implementation based on http://www-igm.univ-mlv.fr/~lecroq/string/node8.html
*/

#include "kmp.h"
#include "artico3.h"

//~ void CPF(char pattern[PATTERN_SIZE], int32_t kmpNext[PATTERN_SIZE]) {
void CPF(a3data_t pattern[PATTERN_SIZE], a3data_t kmpNext[PATTERN_SIZE]) {
    int32_t k, q;
    k = 0;
    kmpNext[0] = 0;

    c1 : for(q = 1; q < PATTERN_SIZE; q++){
        c2 : while(k > 0 && pattern[k] != pattern[q]){
            k = kmpNext[q];
        }
        if(pattern[k] == pattern[q]){
            k++;
        }
        kmpNext[q] = k;
    }
}


//~ int kmp(char pattern[PATTERN_SIZE], char input[STRING_SIZE], int32_t kmpNext[PATTERN_SIZE], int32_t n_matches[1]) {
A3_KERNEL(a3in_t input, a3inout_t pack) {

    /* ARTICo³ unpacking */
    a3data_t *pattern = &pack[0];
    a3data_t *kmpNext = &pack[PATTERN_SIZE];
    a3data_t *n_matches = &pack[2*PATTERN_SIZE];
    /* END ARTICo³ unpacking */

    int32_t i, q;
    n_matches[0] = 0;

    CPF(pattern, kmpNext);

    q = 0;
    k1 : for(i = 0; i < STRING_SIZE; i++){
        k2 : while (q > 0 && pattern[q] != input[i]){
            q = kmpNext[q];
        }
        if (pattern[q] == input[i]){
            q++;
        }
        if (q >= PATTERN_SIZE){
            n_matches[0]++;
            q = kmpNext[q - 1];
        }
    }
    //~ return 0;
}
