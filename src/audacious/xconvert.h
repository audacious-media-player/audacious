/*
 *  Copyright (C) 2003  Haavard Kvaalen <havardk@xmms.org>
 *
 *  Licensed under GNU GPL version 3.
 */

#ifndef _AUDACIOUS_XCONVERT_H
#define _AUDACIOUS_XCONVERT_H

#include <audacious/plugin.h>

struct xmms_convert_buffers;

struct xmms_convert_buffers *xmms_convert_buffers_new(void);
/*
 * Free the data assosiated with the buffers, without destroying the
 * context.  The context can be reused.
 */
void xmms_convert_buffers_free(struct xmms_convert_buffers *buf);
void xmms_convert_buffers_destroy(struct xmms_convert_buffers *buf);


typedef int (*convert_func_t) (struct xmms_convert_buffers * buf,
                               void **data, int length);
typedef int (*convert_channel_func_t) (struct xmms_convert_buffers * buf,
                                       void **data, int length);
typedef int (*convert_freq_func_t) (struct xmms_convert_buffers * buf,
                                    void **data, int length, int ifreq,
                                    int ofreq);


convert_func_t xmms_convert_get_func(AFormat output, AFormat input);
convert_channel_func_t xmms_convert_get_channel_func(AFormat fmt,
                                                     int output,
                                                     int input);
convert_freq_func_t xmms_convert_get_frequency_func(AFormat fmt,
                                                    int channels);

#endif
