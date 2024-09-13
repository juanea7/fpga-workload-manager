#include "fft.h"
#include "artico3.h"

//~ void fft(double real[FFT_SIZE], double img[FFT_SIZE], double real_twid[FFT_SIZE/2], double img_twid[FFT_SIZE/2]){
A3_KERNEL(a3inout_t real, a3inout_t img, a3in_t real_twid, a3in_t img_twid) {
    int even, odd, span, log, rootindex;
    float temp;
    log = 0;

    outer:for(span=FFT_SIZE>>1; span; span>>=1, log++){
        inner:for(odd=span; odd<FFT_SIZE; odd++){
            odd |= span;
            even = odd ^ span;

            temp = a3tof(real[even]) + a3tof(real[odd]);
            real[odd] = ftoa3(a3tof(real[even]) - a3tof(real[odd]));
            real[even] = ftoa3(temp);

            temp = a3tof(img[even]) + a3tof(img[odd]);
            img[odd] = ftoa3(a3tof(img[even]) - a3tof(img[odd]));
            img[even] = ftoa3(temp);

            rootindex = (even<<log) & (FFT_SIZE - 1);
            if(rootindex){
                temp = a3tof(real_twid[rootindex]) * a3tof(real[odd]) -
                    a3tof(img_twid[rootindex])  * a3tof(img[odd]);
                img[odd] = ftoa3(a3tof(real_twid[rootindex]) * a3tof(img[odd]) +
                    a3tof(img_twid[rootindex]) * a3tof(real[odd]));
                real[odd] = ftoa3(temp);
            }
        }
    }
}
