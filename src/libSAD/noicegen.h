#ifndef NOICEGEN_H
#define NOICEGEN_H

#include <inttypes.h>

int triangular_dither_noise(int nbits);
float triangular_dither_noise_f(void);
void noicegen_init_rand(uint32_t seed);

#endif /*NOICEGEN_H*/
