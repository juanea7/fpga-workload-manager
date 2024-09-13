/*
Implemenataion based on:
A. Danalis, G. Marin, C. McCurdy, J. S. Meredith, P. C. Roth, K. Spafford, V. Tipparaju, and J. S. Vetter.
The scalable heterogeneous computing (shoc) benchmark suite.
In Proceedings of the 3rd Workshop on General-Purpose Computation on Graphics Processing Units, 2010.
*/

#include "md.h"
#include "artico3.h"

//~ void md_kernel(TYPE force_x[nAtoms],
               //~ TYPE force_y[nAtoms],
               //~ TYPE force_z[nAtoms],
               //~ TYPE position_x[nAtoms],
               //~ TYPE position_y[nAtoms],
               //~ TYPE position_z[nAtoms],
               //~ int32_t NL[nAtoms*maxNeighbors])
A3_KERNEL(a3in_t NL, a3inout_t pack) {

    /* ARTICo³ unpacking */
    a3data_t *force_x = &pack[0];
    a3data_t *force_y = &pack[nAtoms];
    a3data_t *force_z = &pack[2*nAtoms];
    a3data_t *position_x = &pack[3*nAtoms];
    a3data_t *position_y = &pack[4*nAtoms];
    a3data_t *position_z = &pack[5*nAtoms];
    /* END ARTICo³ unpacking */

    TYPE delx, dely, delz, r2inv;
    TYPE r6inv, potential, force, j_x, j_y, j_z;
    TYPE i_x, i_y, i_z, fx, fy, fz;

    int32_t i, j, jidx;

loop_i : for (i = 0; i < nAtoms; i++){
             i_x = a3tof(position_x[i]);
             i_y = a3tof(position_y[i]);
             i_z = a3tof(position_z[i]);
             fx = 0;
             fy = 0;
             fz = 0;
loop_j : for( j = 0; j < maxNeighbors; j++){
             // Get neighbor
             jidx = NL[i*maxNeighbors + j];
             // Look up x,y,z positions
             j_x = a3tof(position_x[jidx]);
             j_y = a3tof(position_y[jidx]);
             j_z = a3tof(position_z[jidx]);
             // Calc distance
             delx = i_x - j_x;
             dely = i_y - j_y;
             delz = i_z - j_z;
             r2inv = 1.0/( delx*delx + dely*dely + delz*delz );
             // Assume no cutoff and aways account for all nodes in area
             r6inv = r2inv * r2inv * r2inv;
             potential = r6inv*(lj1*r6inv - lj2);
             // Sum changes in force
             force = r2inv*potential;
             fx += delx * force;
             fy += dely * force;
             fz += delz * force;
         }
         //Update forces after all neighbors accounted for.
         force_x[i] = ftoa3(fx);
         force_y[i] = ftoa3(fy);
         force_z[i] = ftoa3(fz);
         //printf("dF=%lf,%lf,%lf\n", fx, fy, fz);
         }
}
