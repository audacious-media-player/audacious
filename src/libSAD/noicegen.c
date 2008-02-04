#include <stdio.h>
#include <assert.h>
#include "../../config.h"

#ifdef HAVE_SSE2
#  define SSE2 1
#endif

#ifdef HAVE_ALTIVEC
#  define ALTIVEC 1
#endif

#define MEXP 19937

#include "SFMT.h"
#include "SFMT.c"

#include "noicegen.h"

int triangular_dither_noise(int nbits)
{
    // parameter nbits : the peak-to-peak amplitude desired (in bits)
    //  use with nbits set to    2 + nber of bits to be trimmed.
    // (because triangular is made from two uniformly distributed processes,
    // it starts at 2 bits peak-to-peak amplitude)
    // see The Theory of Dithered Quantization by Robert Alexander Wannamaker
    // for complete proof of why that's optimal

    int v = (gen_rand32() / 2 - gen_rand32() / 2);   // in ]-2^31, 2^31[
    //int signe = (v>0) ? 1 : -1;
    int P = 1 << (32 - nbits); // the power of 2
    v /= P;
    // now v in ]-2^(nbits-1), 2^(nbits-1) [ 

    return v;
}

float triangular_dither_noise_f() {
  // Сonditionally assume we have 16 bits in fractional part
  // Please, check it thoroughly: is this assumption correct in floatin-point arithmetic?
  return (float) triangular_dither_noise(17) / 65536.0;
}

void noicegen_init_rand(uint32_t seed) {
  init_gen_rand(seed);
}
