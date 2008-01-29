#ifndef DITHER_H
#define DITHER_H

#include "common.h"

#define SAD_ERROR_INCORRECT_INPUT_SAMPLEFORMAT -2
#define SAD_ERROR_INCORRECT_OUTPUT_SAMPLEFORMAT -3
#define SAD_ERROR_CORRUPTED_PRIVATE_DATA -4

typedef sad_sint32 (*SAD_get_sample_proc) (void *buf, int nch, int ch, int i);
typedef void (*SAD_put_sample_proc) (void *buf, sad_sint32 sample, int nch, int ch, int i);

typedef struct {
  SAD_get_sample_proc get_sample;
  SAD_put_sample_proc put_sample;
} SAD_buffer_ops;

/* private data */
typedef struct {} SAD_dither_t;

SAD_dither_t* SAD_dither_init(SAD_buffer_format *inbuf_format, SAD_buffer_format *outbuf_format, int *error);
int SAD_dither_free(SAD_dither_t* state);
int SAD_dither_process_buffer (SAD_dither_t *state, void *inbuf, void *outbuf, int frames);
int SAD_dither_apply_replaygain (SAD_dither_t *state, SAD_replaygain_info *rg_info, SAD_replaygain_mode *mode);
int SAD_dither_set_scale (SAD_dither_t *state, float scale);
int SAD_dither_set_dither (SAD_dither_t *state, int dither);

#endif
