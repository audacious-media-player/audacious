#ifdef _AUDACIOUS_CORE
#  ifdef HAVE_CONFIG_H
#    include "config.h"
#  endif
#endif

#include "noicegen.h"

#define MEXP 19937

#include "SFMT.h"
#include "SFMT.c"

gint triangular_dither_noise(gint nbits)
{
    /* Parameter nbits : the peak-to-peak amplitude desired (in bits)
     * use with nbits set to 2 + number of bits to be trimmed.
     * (because triangular is made from two uniformly distributed
     * processes, it starts at 2 bits peak-to-peak amplitude).
     * Refer to "The Theory of Dithered Quantization" by Robert Alexander
     * Wannamaker for complete proof of why that's optimal.
     */
    gint v = (gen_rand32() / 2 - gen_rand32() / 2);   // in ]-2^31, 2^31[
    //gint signe = (v>0) ? 1 : -1;
    gint P = 1 << (32 - nbits); // the power of 2
    v /= P;
    // now v in ]-2^(nbits-1), 2^(nbits-1) [ 

    return v;
}

gdouble triangular_dither_noise_f(void)
{
    /* Here we unconditionally assume we have 16 bits in fractional part.
     * Please, check it thoroughly: is this assumption correct in
     * floating-point arithmetic?
     */
    return (gdouble) triangular_dither_noise(17) / 65536.0;
}

void noicegen_init_rand(guint32 seed)
{
    init_gen_rand(seed);
}
