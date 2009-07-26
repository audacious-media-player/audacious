/** 
 * @file SFMT.h 
 *
 * @brief SIMD oriented Fast Mersenne Twister(SFMT) pseudorandom
 * number generator
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (Hiroshima University)
 *
 * Copyright (C) 2006, 2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima
 * University. All rights reserved.
 *
 * The new BSD License is applied to this software, see LICENSE.txt.
 */

#ifndef SFMT_H
#define SFMT_H

#include <glib.h>

#if defined(__GNUC__)
#  define ALWAYSINLINE __attribute__((always_inline))
#else
#  define ALWAYSINLINE /* ... */
#endif

guint32 gen_rand32(void);
guint64 gen_rand64(void);
void fill_array32(guint32 *array, gint size);
void fill_array64(guint64 *array, gint size);
void init_gen_rand(guint32 seed);
void init_by_array(guint32 *init_key, gint key_length);
const char *get_idstring(void);
gint get_min_array_size32(void);
gint get_min_array_size64(void);

/* These real versions are due to Isaku Wada */
/** generates a random number on [0,1]-real-interval */
inline static gdouble to_real1(guint32 v)
{
    return v * (1.0/4294967295.0); 
    /* divided by 2^32-1 */ 
}

/** generates a random number on [0,1]-real-interval */
inline static gdouble genrand_real1(void)
{
    return to_real1(gen_rand32());
}

/** generates a random number on [0,1)-real-interval */
inline static gdouble to_real2(guint32 v)
{
    return v * (1.0/4294967296.0); 
    /* divided by 2^32 */
}

/** generates a random number on [0,1)-real-interval */
inline static gdouble genrand_real2(void)
{
    return to_real2(gen_rand32());
}

/** generates a random number on (0,1)-real-interval */
inline static gdouble to_real3(guint32 v)
{
    return (((double)v) + 0.5)*(1.0/4294967296.0); 
    /* divided by 2^32 */
}

/** generates a random number on (0,1)-real-interval */
inline static gdouble genrand_real3(void)
{
    return to_real3(gen_rand32());
}
/** These real versions are due to Isaku Wada */

/** generates a random number on [0,1) with 53-bit resolution*/
inline static gdouble to_res53(guint64 v) 
{ 
    return v * (1.0/18446744073709551616.0L);
}

/** generates a random number on [0,1) with 53-bit resolution*/
inline static gdouble genrand_res53(void) 
{ 
    return to_res53(gen_rand64());
} 
#endif
