#ifndef LIBSAD_NOICEGEN_H
#define LIBSAD_NOICEGEN_H

#include <glib.h>

gint triangular_dither_noise(gint nbits);
gdouble triangular_dither_noise_f(void);
void noicegen_init_rand(guint32 seed);

#endif /* LIBSAD_NOICEGEN_H */
