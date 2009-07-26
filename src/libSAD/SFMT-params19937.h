#ifndef LIBSAD_SFMT_PARAMS19937_H
#define LIBSAD_SFMT_PARAMS19937_H

#define POS1    122
#define SL1     18
#define SL2     1
#define SR1     11
#define SR2     1
#define MSK1    0xdfffffefU
#define MSK2    0xddfecb7fU
#define MSK3    0xbffaffffU
#define MSK4    0xbffffff6U
#define PARITY1 0x00000001U
#define PARITY2 0x00000000U
#define PARITY3 0x00000000U
#define PARITY4 0x13c9e684U


/* PARAMETERS FOR ALTIVEC */
#if defined(__APPLE__)          /* For OSX */
#  define ALTI_SL1          (vector guint)(SL1, SL1, SL1, SL1)
#  define ALTI_SR1          (vector guint)(SR1, SR1, SR1, SR1)
#  define ALTI_MSK          (vector guint)(MSK1, MSK2, MSK3, MSK4)
#  define ALTI_MSK64        (vector guint)(MSK2, MSK1, MSK4, MSK3)
#  define ALTI_SL2_PERM     (vector guchar)(1,2,3,23,5,6,7,0,9,10,11,4,13,14,15,8)
#  define ALTI_SL2_PERM64   (vector guchar)(1,2,3,4,5,6,7,31,9,10,11,12,13,14,15,0)
#  define ALTI_SR2_PERM     (vector guchar)(7,0,1,2,11,4,5,6,15,8,9,10,17,12,13,14)
#  define ALTI_SR2_PERM64   (vector guchar)(15,0,1,2,3,4,5,6,17,8,9,10,11,12,13,14)
#else /* For OTHER systems (Linux?) */
#  define ALTI_SL1          {SL1, SL1, SL1, SL1}
#  define ALTI_SR1          {SR1, SR1, SR1, SR1}
#  define ALTI_MSK          {MSK1, MSK2, MSK3, MSK4}
#  define ALTI_MSK64        {MSK2, MSK1, MSK4, MSK3}
#  define ALTI_SL2_PERM     {1,2,3,23,5,6,7,0,9,10,11,4,13,14,15,8}
#  define ALTI_SL2_PERM64   {1,2,3,4,5,6,7,31,9,10,11,12,13,14,15,0}
#  define ALTI_SR2_PERM     {7,0,1,2,11,4,5,6,15,8,9,10,17,12,13,14}
#  define ALTI_SR2_PERM64   {15,0,1,2,3,4,5,6,17,8,9,10,11,12,13,14}
#endif

#define IDSTR       "SFMT-19937:122-18-1-11-1:dfffffef-ddfecb7f-bffaffff-bffffff6"

#endif /* LIBSAD_SFMT_PARAMS19937_H */
