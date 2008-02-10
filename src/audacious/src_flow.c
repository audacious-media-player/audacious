/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2008  Audacious development team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#define AUD_DEBUG

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef USE_SRC

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <samplerate.h>

#include "main.h"
#include "plugin.h"
#include "input.h"
#include "playback.h"
#include "util.h"

#include "src_flow.h"

static SRC_STATE *src_state = NULL;
static SRC_DATA src_data;

static int overSamplingFs = 48000; /* hmmm... does everybody have 96kHz-enabled hardware? i'm in doubt --asphyx */
static int inputFs = 0;
static int input_nch = 0;
static int converter_type = SRC_SINC_BEST_QUALITY;
static int srcError = 0;

static float *srcOut = NULL;
static unsigned int lengthOfSrcOut = 0;

void
src_flow_free()
{
    AUDDBG("\n");
    if(src_state != NULL) src_state = src_delete(src_state);
    src_state = NULL;
    if(srcOut != NULL) free(srcOut);
    srcOut = NULL;
    lengthOfSrcOut = 0;
    inputFs = 0;
    overSamplingFs = 0;
    input_nch = 0;
}

gint
src_flow_init(gint infreq, gint nch)
{
    AUDDBG("input_rate=%d, nch=%d\n", infreq, nch);
    src_flow_free();

    /* don't resample if sampling rates are the same --nenolod */
    if (infreq == cfg.src_rate || !cfg.enable_src) return infreq;
    
    overSamplingFs = cfg.src_rate;
    inputFs = infreq;
    input_nch = nch;
    converter_type = cfg.src_type;
    
    src_state = src_new(converter_type, nch, &srcError);
    if (src_state != NULL) {
        src_data.src_ratio = (float)overSamplingFs / (float)infreq;
        return overSamplingFs;
    } else {
        AUDDBG("src_new(): %s\n\n", src_strerror(srcError));
        return infreq;
    }
}

void
src_flow(FlowContext *context) {
   
    if(!cfg.enable_src) return;
    
    if(context->fmt != FMT_FLOAT) {
        context->error = TRUE;
        return;
    }

    if(src_state == NULL || context->srate != inputFs || context->channels != input_nch) {
        AUDDBG("reinitializing src\n");
        src_flow_init(context->srate, context->channels);
    }

    int lrLength = context->len;
    int overLrLength = (int)floor(lrLength * (src_data.src_ratio + 1));
    
    if(lengthOfSrcOut < overLrLength || srcOut == NULL) {
        AUDDBG("reallocating srcOut\n");
        lengthOfSrcOut = overLrLength;
        srcOut = smart_realloc(srcOut, &lengthOfSrcOut);
    }
    
    src_data.data_in = (float*)context->data;
    src_data.data_out = srcOut;
    src_data.end_of_input = 0;
    src_data.input_frames = lrLength / context->channels / sizeof(float);
    src_data.output_frames = overLrLength / context->channels / sizeof(float);
    if ((srcError = src_process(src_state, &src_data)) > 0) {
        AUDDBG("src_process(): %s\n", src_strerror(srcError));
        context->error = TRUE;
        return;
    } else {
        context->data = (gpointer) srcOut;
        context->len = src_data.output_frames_gen * context->channels * sizeof(float);
        return;
    }
}

#endif /* USE_SRC */

