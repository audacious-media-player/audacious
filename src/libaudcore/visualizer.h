/*
 * visualizer.h
 * Copyright 2014 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#ifndef LIBAUDCORE_VISUALIZER_H
#define LIBAUDCORE_VISUALIZER_H

#include <libaudcore/export.h>

class LIBAUDCORE_PUBLIC Visualizer
{
public:
    enum {
        MonoPCM = (1 << 0),
        MultiPCM = (1 << 1),
        Freq = (1 << 2)
    };

    const int type_mask;
    constexpr Visualizer (int type_mask) :
        type_mask (type_mask) {}

    /* reset internal state and clear display */
    virtual void clear () = 0;

    /* 512 frames of a single-channel PCM signal */
    virtual void render_mono_pcm (const float * pcm) {}

    /* 512 frames of an interleaved multi-channel PCM signal */
    virtual void render_multi_pcm (const float * pcm, int channels) {}

    /* intensity of frequencies 1/512, 2/512, ..., 256/512 of sample rate */
    virtual void render_freq (const float * freq) {}
};

#endif /* LIBAUDCORE_VISUALIZER_H */
