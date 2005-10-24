/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003 M. Bakker, Ahead Software AG, http://www.nero.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: decoder.c,v 1.82 2003/11/12 20:47:57 menno Exp $
**/

#include "common.h"
#include "structs.h"

#include <stdlib.h>
#include <string.h>

#include "decoder.h"
#include "mp4.h"
#include "syntax.h"
#include "error.h"
#include "output.h"
#ifdef SBR_DEC
#include "sbr_dec.h"
#endif
#ifdef SSR_DEC
#include "ssr.h"
#endif

#ifdef ANALYSIS
uint16_t dbg_count;
#endif

int8_t* FAADAPI faacDecGetErrorMessage(uint8_t errcode)
{
    if (errcode >= NUM_ERROR_MESSAGES)
        return NULL;
    return err_msg[errcode];
}

uint32_t FAADAPI faacDecGetCapabilities()
{
    uint32_t cap = 0;

    /* can't do without it */
    cap += LC_DEC_CAP;

#ifdef MAIN_DEC
    cap += MAIN_DEC_CAP;
#endif
#ifdef LTP_DEC
    cap += LTP_DEC_CAP;
#endif
#ifdef LD_DEC
    cap += LD_DEC_CAP;
#endif
#ifdef ERROR_RESILIENCE
    cap += ERROR_RESILIENCE_CAP;
#endif
#ifdef FIXED_POINT
    cap += FIXED_POINT_CAP;
#endif

    return cap;
}

faacDecHandle FAADAPI faacDecOpen()
{
    uint8_t i;
    faacDecHandle hDecoder = NULL;

    if ((hDecoder = (faacDecHandle)malloc(sizeof(faacDecStruct))) == NULL)
        return NULL;

    memset(hDecoder, 0, sizeof(faacDecStruct));

    hDecoder->config.outputFormat  = FAAD_FMT_16BIT;
    hDecoder->config.defObjectType = MAIN;
    hDecoder->config.defSampleRate = 44100; /* Default: 44.1kHz */
    hDecoder->config.downMatrix = 0;
    hDecoder->adts_header_present = 0;
    hDecoder->adif_header_present = 0;
#ifdef ERROR_RESILIENCE
    hDecoder->aacSectionDataResilienceFlag = 0;
    hDecoder->aacScalefactorDataResilienceFlag = 0;
    hDecoder->aacSpectralDataResilienceFlag = 0;
#endif
    hDecoder->frameLength = 1024;

    hDecoder->frame = 0;
    hDecoder->sample_buffer = NULL;

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        hDecoder->window_shape_prev[i] = 0;
        hDecoder->time_out[i] = NULL;
#ifdef SBR_DEC
        hDecoder->time_out2[i] = NULL;
#endif
#ifdef SSR_DEC
        hDecoder->ssr_overlap[i] = NULL;
        hDecoder->prev_fmd[i] = NULL;
#endif
#ifdef MAIN_DEC
        hDecoder->pred_stat[i] = NULL;
#endif
#ifdef LTP_DEC
        hDecoder->ltp_lag[i] = 0;
        hDecoder->lt_pred_stat[i] = NULL;
#endif
    }

#ifdef SBR_DEC
    for (i = 0; i < 32; i++)
    {
        hDecoder->sbr[i] = NULL;
    }
#endif

    hDecoder->drc = drc_init(REAL_CONST(1.0), REAL_CONST(1.0));

#if POW_TABLE_SIZE
    hDecoder->pow2_table = (real_t*)malloc(POW_TABLE_SIZE*sizeof(real_t));
    if (!hDecoder->pow2_table)
    {
        free(hDecoder);
        hDecoder = NULL;
        return hDecoder;
    }
    build_tables(hDecoder->pow2_table);
#endif

    return hDecoder;
}

faacDecConfigurationPtr FAADAPI faacDecGetCurrentConfiguration(faacDecHandle hDecoder)
{
    if (hDecoder)
    {
        faacDecConfigurationPtr config = &(hDecoder->config);

        return config;
    }

    return NULL;
}

uint8_t FAADAPI faacDecSetConfiguration(faacDecHandle hDecoder,
                                    faacDecConfigurationPtr config)
{
    if (hDecoder && config)
    {
        /* check if we can decode this object type */
        if (can_decode_ot(config->defObjectType) < 0)
            return 0;
        hDecoder->config.defObjectType = config->defObjectType;

        /* samplerate: anything but 0 should be possible */
        if (config->defSampleRate == 0)
            return 0;
        hDecoder->config.defSampleRate = config->defSampleRate;

        /* check output format */
        if ((config->outputFormat < 1) || (config->outputFormat > 9))
            return 0;
        hDecoder->config.outputFormat = config->outputFormat;

        if (config->downMatrix > 1)
            hDecoder->config.downMatrix = config->downMatrix;

        /* OK */
        return 1;
    }

    return 0;
}

int32_t FAADAPI faacDecInit(faacDecHandle hDecoder, uint8_t *buffer,
                            uint32_t buffer_size,
                            uint32_t *samplerate, uint8_t *channels)
{
    uint32_t bits = 0;
    bitfile ld;
    adif_header adif;
    adts_header adts;

    if ((hDecoder == NULL) || (samplerate == NULL) || (channels == NULL))
        return -1;

    hDecoder->sf_index = get_sr_index(hDecoder->config.defSampleRate);
    hDecoder->object_type = hDecoder->config.defObjectType;
    *samplerate = get_sample_rate(hDecoder->sf_index);
    *channels = 1;

    if (buffer != NULL)
    {
        faad_initbits(&ld, buffer, buffer_size);

        /* Check if an ADIF header is present */
        if ((buffer[0] == 'A') && (buffer[1] == 'D') &&
            (buffer[2] == 'I') && (buffer[3] == 'F'))
        {
            hDecoder->adif_header_present = 1;

            get_adif_header(&adif, &ld);
            faad_byte_align(&ld);

            hDecoder->sf_index = adif.pce[0].sf_index;
            hDecoder->object_type = adif.pce[0].object_type + 1;

            *samplerate = get_sample_rate(hDecoder->sf_index);
            *channels = adif.pce[0].channels;

            memcpy(&(hDecoder->pce), &(adif.pce[0]), sizeof(program_config));
            hDecoder->pce_set = 1;

            bits = bit2byte(faad_get_processed_bits(&ld));

        /* Check if an ADTS header is present */
        } else if (faad_showbits(&ld, 12) == 0xfff) {
            hDecoder->adts_header_present = 1;

            adts.old_format = hDecoder->config.useOldADTSFormat;
            adts_frame(&adts, &ld);

            hDecoder->sf_index = adts.sf_index;
            hDecoder->object_type = adts.profile + 1;

            *samplerate = get_sample_rate(hDecoder->sf_index);
            *channels = (adts.channel_configuration > 6) ?
                2 : adts.channel_configuration;
        }

        if (ld.error)
        {
            faad_endbits(&ld);
            return -1;
        }
        faad_endbits(&ld);
    }
    hDecoder->channelConfiguration = *channels;

#ifdef SBR_DEC
    /* implicit signalling */
    if (*samplerate <= 24000)
    {
        *samplerate *= 2;
        hDecoder->forceUpSampling = 1;
    }
#endif

    /* must be done before frameLength is divided by 2 for LD */
#ifdef SSR_DEC
    if (hDecoder->object_type == SSR)
        hDecoder->fb = ssr_filter_bank_init(hDecoder->frameLength/SSR_BANDS);
    else
#endif
        hDecoder->fb = filter_bank_init(hDecoder->frameLength);

#ifdef LD_DEC
    if (hDecoder->object_type == LD)
        hDecoder->frameLength >>= 1;
#endif

    if (can_decode_ot(hDecoder->object_type) < 0)
        return -1;

    return bits;
}

/* Init the library using a DecoderSpecificInfo */
int8_t FAADAPI faacDecInit2(faacDecHandle hDecoder, uint8_t *pBuffer,
                            uint32_t SizeOfDecoderSpecificInfo,
                            uint32_t *samplerate, uint8_t *channels)
{
    int8_t rc;
    mp4AudioSpecificConfig mp4ASC;

    if((hDecoder == NULL)
        || (pBuffer == NULL)
        || (SizeOfDecoderSpecificInfo < 2)
        || (samplerate == NULL)
        || (channels == NULL))
    {
        return -1;
    }

    hDecoder->adif_header_present = 0;
    hDecoder->adts_header_present = 0;

    /* decode the audio specific config */
    rc = AudioSpecificConfig2(pBuffer, SizeOfDecoderSpecificInfo, &mp4ASC,
        &(hDecoder->pce));

    /* copy the relevant info to the decoder handle */
    *samplerate = mp4ASC.samplingFrequency;
    if (mp4ASC.channelsConfiguration)
    {
        *channels = mp4ASC.channelsConfiguration;
    } else {
        *channels = hDecoder->pce.channels;
        hDecoder->pce_set = 1;
    }
    hDecoder->sf_index = mp4ASC.samplingFrequencyIndex;
    hDecoder->object_type = mp4ASC.objectTypeIndex;
#ifdef ERROR_RESILIENCE
    hDecoder->aacSectionDataResilienceFlag = mp4ASC.aacSectionDataResilienceFlag;
    hDecoder->aacScalefactorDataResilienceFlag = mp4ASC.aacScalefactorDataResilienceFlag;
    hDecoder->aacSpectralDataResilienceFlag = mp4ASC.aacSpectralDataResilienceFlag;
#endif
#ifdef SBR_DEC
    hDecoder->sbr_present_flag = mp4ASC.sbr_present_flag;
    hDecoder->forceUpSampling = mp4ASC.forceUpSampling;

    /* AAC core decoder samplerate is 2 times as low */
    if (hDecoder->sbr_present_flag == 1 || hDecoder->forceUpSampling == 1)
    {
        hDecoder->sf_index = get_sr_index(mp4ASC.samplingFrequency / 2);
    }
#endif

    if (rc != 0)
    {
        return rc;
    }
    hDecoder->channelConfiguration = mp4ASC.channelsConfiguration;
    if (mp4ASC.frameLengthFlag)
#ifdef ALLOW_SMALL_FRAMELENGTH
        hDecoder->frameLength = 960;
#else
        return -1;
#endif

    /* must be done before frameLength is divided by 2 for LD */
#ifdef SSR_DEC
    if (hDecoder->object_type == SSR)
        hDecoder->fb = ssr_filter_bank_init(hDecoder->frameLength/SSR_BANDS);
    else
#endif
        hDecoder->fb = filter_bank_init(hDecoder->frameLength);

#ifdef LD_DEC
    if (hDecoder->object_type == LD)
        hDecoder->frameLength >>= 1;
#endif

    return 0;
}

#ifdef DRM
int8_t FAADAPI faacDecInitDRM(faacDecHandle hDecoder, uint32_t samplerate,
                              uint8_t channels)
{
    uint8_t i;

    /* Special object type defined for DRM */
    hDecoder->config.defObjectType = DRM_ER_LC;

    hDecoder->config.defSampleRate = samplerate;
#ifdef ERROR_RESILIENCE // This shoudl always be defined for DRM
    hDecoder->aacSectionDataResilienceFlag = 1; /* VCB11 */
    hDecoder->aacScalefactorDataResilienceFlag = 0; /* no RVLC */
    hDecoder->aacSpectralDataResilienceFlag = 1; /* HCR */
#endif
    hDecoder->frameLength = 960;
    hDecoder->sf_index = get_sr_index(hDecoder->config.defSampleRate);
    hDecoder->object_type = hDecoder->config.defObjectType;

    if ((channels == DRMCH_STEREO) || (channels == DRMCH_SBR_STEREO))
        hDecoder->channelConfiguration = 2;
    else
        hDecoder->channelConfiguration = 1;

#ifdef SBR_DEC
    if (channels == DRMCH_SBR_LC_STEREO)
        hDecoder->lcstereo_flag = 1;
    else
        hDecoder->lcstereo_flag = 0;

    if ((channels == DRMCH_MONO) || (channels == DRMCH_STEREO))
        hDecoder->sbr_present_flag = 0;
    else
        hDecoder->sbr_present_flag = 1;

    /* Reset sbr for new initialization */
    sbrDecodeEnd(hDecoder->sbr[0]);
    hDecoder->sbr[0] = NULL;
#endif

    /* must be done before frameLength is divided by 2 for LD */
    hDecoder->fb = filter_bank_init(hDecoder->frameLength);

    /* Take care of buffers */
    if (hDecoder->sample_buffer) free(hDecoder->sample_buffer);
    hDecoder->sample_buffer = NULL;

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        hDecoder->window_shape_prev[i] = 0;

        if (hDecoder->time_out[i]) free(hDecoder->time_out[i]);
        hDecoder->time_out[i] = NULL;
#ifdef SBR_DEC
        if (hDecoder->time_out2[i]) free(hDecoder->time_out2[i]);
        hDecoder->time_out2[i] = NULL;
#endif
#ifdef SSR_DEC
        if (hDecoder->ssr_overlap[i]) free(hDecoder->ssr_overlap[i]);
        hDecoder->ssr_overlap[i] = NULL;
        if (hDecoder->prev_fmd[i]) free(hDecoder->prev_fmd[i]);
        hDecoder->prev_fmd[i] = NULL;
#endif
#ifdef MAIN_DEC
        if (hDecoder->pred_stat[i]) free(hDecoder->pred_stat[i]);
        hDecoder->pred_stat[i] = NULL;
#endif
#ifdef LTP_DEC
        hDecoder->ltp_lag[i] = 0;
        if (hDecoder->lt_pred_stat[i]) free(hDecoder->lt_pred_stat[i]);
        hDecoder->lt_pred_stat[i] = NULL;
#endif
    }

    return 0;
}
#endif

void FAADAPI faacDecClose(faacDecHandle hDecoder)
{
    uint8_t i;

    if (hDecoder == NULL)
        return;

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        if (hDecoder->time_out[i]) free(hDecoder->time_out[i]);
#ifdef SBR_DEC
        if (hDecoder->time_out2[i]) free(hDecoder->time_out2[i]);
#endif
#ifdef SSR_DEC
        if (hDecoder->ssr_overlap[i]) free(hDecoder->ssr_overlap[i]);
        if (hDecoder->prev_fmd[i]) free(hDecoder->prev_fmd[i]);
#endif
#ifdef MAIN_DEC
        if (hDecoder->pred_stat[i]) free(hDecoder->pred_stat[i]);
#endif
#ifdef LTP_DEC
        if (hDecoder->lt_pred_stat[i]) free(hDecoder->lt_pred_stat[i]);
#endif
    }

#ifdef SSR_DEC
    if (hDecoder->object_type == SSR)
        ssr_filter_bank_end(hDecoder->fb);
    else
#endif
        filter_bank_end(hDecoder->fb);

    drc_end(hDecoder->drc);

#ifndef FIXED_POINT
#if POW_TABLE_SIZE
    if (hDecoder->pow2_table) free(hDecoder->pow2_table);
#endif
#endif

    if (hDecoder->sample_buffer) free(hDecoder->sample_buffer);

#ifdef SBR_DEC
    for (i = 0; i < 32; i++)
    {
        if (hDecoder->sbr[i])
            sbrDecodeEnd(hDecoder->sbr[i]);
    }
#endif

    if (hDecoder) free(hDecoder);
}

void FAADAPI faacDecPostSeekReset(faacDecHandle hDecoder, int32_t frame)
{
    if (hDecoder)
    {
        hDecoder->postSeekResetFlag = 1;

        if (frame != -1)
            hDecoder->frame = frame;
    }
}

static void create_channel_config(faacDecHandle hDecoder, faacDecFrameInfo *hInfo)
{
    hInfo->num_front_channels = 0;
    hInfo->num_side_channels = 0;
    hInfo->num_back_channels = 0;
    hInfo->num_lfe_channels = 0;
    memset(hInfo->channel_position, 0, MAX_CHANNELS*sizeof(uint8_t));

    if (hDecoder->downMatrix)
    {
        hInfo->num_front_channels = 2;
        hInfo->channel_position[0] = FRONT_CHANNEL_LEFT;
        hInfo->channel_position[1] = FRONT_CHANNEL_RIGHT;
        return;
    }

    /* check if there is a PCE */
    if (hDecoder->pce_set)
    {
        uint8_t i, chpos = 0;
        uint8_t chdir, back_center = 0;

        hInfo->num_front_channels = hDecoder->pce.num_front_channels;
        hInfo->num_side_channels = hDecoder->pce.num_side_channels;
        hInfo->num_back_channels = hDecoder->pce.num_back_channels;
        hInfo->num_lfe_channels = hDecoder->pce.num_lfe_channels;

        chdir = hInfo->num_front_channels;
        if (chdir & 1)
        {
            hInfo->channel_position[chpos++] = FRONT_CHANNEL_CENTER;
            chdir--;
        }
        for (i = 0; i < chdir; i += 2)
        {
            hInfo->channel_position[chpos++] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[chpos++] = FRONT_CHANNEL_RIGHT;
        }

        for (i = 0; i < hInfo->num_side_channels; i += 2)
        {
            hInfo->channel_position[chpos++] = SIDE_CHANNEL_LEFT;
            hInfo->channel_position[chpos++] = SIDE_CHANNEL_RIGHT;
        }

        chdir = hInfo->num_back_channels;
        if (chdir & 1)
        {
            back_center = 1;
            chdir--;
        }
        for (i = 0; i < chdir; i += 2)
        {
            hInfo->channel_position[chpos++] = BACK_CHANNEL_LEFT;
            hInfo->channel_position[chpos++] = BACK_CHANNEL_RIGHT;
        }
        if (back_center)
        {
            hInfo->channel_position[chpos++] = BACK_CHANNEL_CENTER;
        }

        for (i = 0; i < hInfo->num_lfe_channels; i++)
        {
            hInfo->channel_position[chpos++] = LFE_CHANNEL;
        }

    } else {
        switch (hDecoder->channelConfiguration)
        {
        case 1:
            hInfo->num_front_channels = 1;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            break;
        case 2:
            hInfo->num_front_channels = 2;
            hInfo->channel_position[0] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[1] = FRONT_CHANNEL_RIGHT;
            break;
        case 3:
            hInfo->num_front_channels = 3;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            hInfo->channel_position[1] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[2] = FRONT_CHANNEL_RIGHT;
            break;
        case 4:
            hInfo->num_front_channels = 3;
            hInfo->num_back_channels = 1;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            hInfo->channel_position[1] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[2] = FRONT_CHANNEL_RIGHT;
            hInfo->channel_position[3] = BACK_CHANNEL_CENTER;
            break;
        case 5:
            hInfo->num_front_channels = 3;
            hInfo->num_back_channels = 2;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            hInfo->channel_position[1] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[2] = FRONT_CHANNEL_RIGHT;
            hInfo->channel_position[3] = BACK_CHANNEL_LEFT;
            hInfo->channel_position[4] = BACK_CHANNEL_RIGHT;
            break;
        case 6:
            hInfo->num_front_channels = 3;
            hInfo->num_back_channels = 2;
            hInfo->num_lfe_channels = 1;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            hInfo->channel_position[1] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[2] = FRONT_CHANNEL_RIGHT;
            hInfo->channel_position[3] = BACK_CHANNEL_LEFT;
            hInfo->channel_position[4] = BACK_CHANNEL_RIGHT;
            hInfo->channel_position[5] = LFE_CHANNEL;
            break;
        case 7:
            hInfo->num_front_channels = 3;
            hInfo->num_side_channels = 2;
            hInfo->num_back_channels = 2;
            hInfo->num_lfe_channels = 1;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            hInfo->channel_position[1] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[2] = FRONT_CHANNEL_RIGHT;
            hInfo->channel_position[3] = SIDE_CHANNEL_LEFT;
            hInfo->channel_position[4] = SIDE_CHANNEL_RIGHT;
            hInfo->channel_position[5] = BACK_CHANNEL_LEFT;
            hInfo->channel_position[6] = BACK_CHANNEL_RIGHT;
            hInfo->channel_position[7] = LFE_CHANNEL;
            break;
        default: /* channelConfiguration == 0 || channelConfiguration > 7 */
            {
                uint8_t i;
                uint8_t ch = hDecoder->fr_channels - hDecoder->has_lfe;
                if (ch & 1) /* there's either a center front or a center back channel */
                {
                    uint8_t ch1 = (ch-1)/2;
                    if (hDecoder->first_syn_ele == ID_SCE)
                    {
                        hInfo->num_front_channels = ch1 + 1;
                        hInfo->num_back_channels = ch1;
                        hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
                        for (i = 1; i <= ch1; i+=2)
                        {
                            hInfo->channel_position[i] = FRONT_CHANNEL_LEFT;
                            hInfo->channel_position[i+1] = FRONT_CHANNEL_RIGHT;
                        }
                        for (i = ch1+1; i < ch; i+=2)
                        {
                            hInfo->channel_position[i] = BACK_CHANNEL_LEFT;
                            hInfo->channel_position[i+1] = BACK_CHANNEL_RIGHT;
                        }
                    } else {
                        hInfo->num_front_channels = ch1;
                        hInfo->num_back_channels = ch1 + 1;
                        for (i = 0; i < ch1; i+=2)
                        {
                            hInfo->channel_position[i] = FRONT_CHANNEL_LEFT;
                            hInfo->channel_position[i+1] = FRONT_CHANNEL_RIGHT;
                        }
                        for (i = ch1; i < ch-1; i+=2)
                        {
                            hInfo->channel_position[i] = BACK_CHANNEL_LEFT;
                            hInfo->channel_position[i+1] = BACK_CHANNEL_RIGHT;
                        }
                        hInfo->channel_position[ch-1] = BACK_CHANNEL_CENTER;
                    }
                } else {
                    uint8_t ch1 = (ch)/2;
                    hInfo->num_front_channels = ch1;
                    hInfo->num_back_channels = ch1;
                    if (ch1 & 1)
                    {
                        hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
                        for (i = 1; i <= ch1; i+=2)
                        {
                            hInfo->channel_position[i] = FRONT_CHANNEL_LEFT;
                            hInfo->channel_position[i+1] = FRONT_CHANNEL_RIGHT;
                        }
                        for (i = ch1+1; i < ch-1; i+=2)
                        {
                            hInfo->channel_position[i] = BACK_CHANNEL_LEFT;
                            hInfo->channel_position[i+1] = BACK_CHANNEL_RIGHT;
                        }
                        hInfo->channel_position[ch-1] = BACK_CHANNEL_CENTER;
                    } else {
                        for (i = 0; i < ch1; i+=2)
                        {
                            hInfo->channel_position[i] = FRONT_CHANNEL_LEFT;
                            hInfo->channel_position[i+1] = FRONT_CHANNEL_RIGHT;
                        }
                        for (i = ch1; i < ch; i+=2)
                        {
                            hInfo->channel_position[i] = BACK_CHANNEL_LEFT;
                            hInfo->channel_position[i+1] = BACK_CHANNEL_RIGHT;
                        }
                    }
                }
                hInfo->num_lfe_channels = hDecoder->has_lfe;
                for (i = ch; i < hDecoder->fr_channels; i++)
                {
                    hInfo->channel_position[i] = LFE_CHANNEL;
                }
            }
            break;
        }
    }
}

void* FAADAPI faacDecDecode(faacDecHandle hDecoder,
                            faacDecFrameInfo *hInfo,
                            uint8_t *buffer, uint32_t buffer_size)
{
    adts_header adts;
    uint8_t channels = 0, ch_ele = 0;
    uint8_t output_channels = 0;
    bitfile ld;
    uint32_t bitsconsumed;
#ifdef DRM
    uint8_t *revbuffer;
    uint8_t *prevbufstart;
    uint8_t *pbufend;
#endif

    /* local copy of globals */
    uint8_t sf_index, object_type, channelConfiguration, outputFormat;
    uint8_t *window_shape_prev;
    uint16_t frame_len;
#ifdef MAIN_DEC
    pred_state **pred_stat;
#endif
    real_t **time_out;
#ifdef SBR_DEC
    real_t **time_out2;
#endif
#ifdef SSR_DEC
    real_t **ssr_overlap, **prev_fmd;
#endif
    fb_info *fb;
    drc_info *drc;
    program_config *pce;

    void *sample_buffer;

    /* safety checks */
    if ((hDecoder == NULL) || (hInfo == NULL) || (buffer == NULL))
    {
        return NULL;
    }

    sf_index = hDecoder->sf_index;
    object_type = hDecoder->object_type;
    channelConfiguration = hDecoder->channelConfiguration;
#ifdef MAIN_DEC
    pred_stat = hDecoder->pred_stat;
#endif
    window_shape_prev = hDecoder->window_shape_prev;
    time_out = hDecoder->time_out;
#ifdef SBR_DEC
    time_out2 = hDecoder->time_out2;
#endif
#ifdef SSR_DEC
    ssr_overlap = hDecoder->ssr_overlap;
    prev_fmd = hDecoder->prev_fmd;
#endif
    fb = hDecoder->fb;
    drc = hDecoder->drc;
    outputFormat = hDecoder->config.outputFormat;
    pce = &hDecoder->pce;
    frame_len = hDecoder->frameLength;


    memset(hInfo, 0, sizeof(faacDecFrameInfo));
    memset(hDecoder->internal_channel, 0, MAX_CHANNELS*sizeof(hDecoder->internal_channel[0]));

    /* initialize the bitstream */
    faad_initbits(&ld, buffer, buffer_size);

#ifdef DRM
    if (object_type == DRM_ER_LC)
    {
        faad_getbits(&ld, 8
            DEBUGVAR(1,1,"faacDecDecode(): skip CRC"));
    }
#endif

    if (hDecoder->adts_header_present)
    {
        adts.old_format = hDecoder->config.useOldADTSFormat;
        if ((hInfo->error = adts_frame(&adts, &ld)) > 0)
            goto error;

        /* MPEG2 does byte_alignment() here,
         * but ADTS header is always multiple of 8 bits in MPEG2
         * so not needed to actually do it.
         */
    }

#ifdef ANALYSIS
    dbg_count = 0;
#endif

    /* decode the complete bitstream */
    raw_data_block(hDecoder, hInfo, &ld, pce, drc);

    ch_ele = hDecoder->fr_ch_ele;
    channels = hDecoder->fr_channels;

    if (hInfo->error > 0)
        goto error;


    /* no more bit reading after this */
    bitsconsumed = faad_get_processed_bits(&ld);
    hInfo->bytesconsumed = bit2byte(bitsconsumed);
    if (ld.error)
    {
        hInfo->error = 14;
        goto error;
    }
    faad_endbits(&ld);

#ifdef DRM
#ifdef SBR_DEC
    if ((hDecoder->sbr_present_flag == 1) && (hDecoder->object_type == DRM_ER_LC))
    {
        int32_t i;

        if (bitsconsumed + 8 > buffer_size*8)
        {
            hInfo->error = 14;
            goto error;
        }

        hDecoder->sbr_used[0] = 1;

        if (!hDecoder->sbr[0])
            hDecoder->sbr[0] = sbrDecodeInit(hDecoder->frameLength, 1);

        /* Reverse bit reading of SBR data in DRM audio frame */
        revbuffer = (uint8_t*)malloc(buffer_size*sizeof(uint8_t));
        prevbufstart = revbuffer;
        pbufend = &buffer[buffer_size - 1];
        for (i = 0; i < buffer_size; i++)
            *prevbufstart++ = tabFlipbits[*pbufend--];

        /* Set SBR data */
        hDecoder->sbr[0]->data = revbuffer;
        /* consider 8 bits from AAC-CRC */
        hDecoder->sbr[0]->data_size_bits = buffer_size*8 - bitsconsumed - 8;
        hDecoder->sbr[0]->data_size =
            bit2byte(hDecoder->sbr[0]->data_size_bits + 8);

        hDecoder->sbr[0]->lcstereo_flag = hDecoder->lcstereo_flag;

        hDecoder->sbr[0]->sample_rate = get_sample_rate(hDecoder->sf_index);
        hDecoder->sbr[0]->sample_rate *= 2;

        hDecoder->sbr[0]->id_aac = hDecoder->element_id[0];
    }
#endif
#endif

    if (!hDecoder->adts_header_present && !hDecoder->adif_header_present)
    {
        if (channels != hDecoder->channelConfiguration)
            hDecoder->channelConfiguration = channels;

        if (channels == 8) /* 7.1 */
            hDecoder->channelConfiguration = 7;
        if (channels == 7) /* not a standard channelConfiguration */
            hDecoder->channelConfiguration = 0;
    }

    if ((channels == 5 || channels == 6) && hDecoder->config.downMatrix)
    {
        hDecoder->downMatrix = 1;
        output_channels = 2;
    } else {
        output_channels = channels;
    }

    /* Make a channel configuration based on either a PCE or a channelConfiguration */
    create_channel_config(hDecoder, hInfo);

    /* number of samples in this frame */
    hInfo->samples = frame_len*output_channels;
    /* number of channels in this frame */
    hInfo->channels = output_channels;
    /* samplerate */
    hInfo->samplerate = get_sample_rate(hDecoder->sf_index);
    /* object type */
    hInfo->object_type = hDecoder->object_type;
    /* sbr */
    hInfo->sbr = NO_SBR;
    /* header type */
    hInfo->header_type = RAW;
    if (hDecoder->adif_header_present)
        hInfo->header_type = ADIF;
    if (hDecoder->adts_header_present)
        hInfo->header_type = ADTS;

    /* check if frame has channel elements */
    if (channels == 0)
    {
        hDecoder->frame++;
        return NULL;
    }

    /* allocate the buffer for the final samples */
    if (hDecoder->sample_buffer == NULL)
    {
        static const uint8_t str[] = { sizeof(int16_t), sizeof(int32_t), sizeof(int32_t),
            sizeof(float32_t), sizeof(double), sizeof(int16_t), sizeof(int16_t),
            sizeof(int16_t), sizeof(int16_t), 0, 0, 0
        };
        uint8_t stride = str[outputFormat-1];
#ifdef SBR_DEC
        if ((hDecoder->sbr_present_flag == 1) || (hDecoder->forceUpSampling == 1))
            stride = 2 * stride;
#endif
        hDecoder->sample_buffer = malloc(frame_len*channels*stride);
    }

    sample_buffer = hDecoder->sample_buffer;

#ifdef SBR_DEC
    if ((hDecoder->sbr_present_flag == 1) || (hDecoder->forceUpSampling == 1))
    {
        uint8_t i, ch = 0;
        for (i = 0; i < ch_ele; i++)
        {
            /* following case can happen when forceUpSampling == 1 */
            if (hDecoder->sbr[i] == NULL)
            {
                hDecoder->sbr[i] = sbrDecodeInit(hDecoder->frameLength
#ifdef DRM
                    , 0
#endif
                    );
                hDecoder->sbr[i]->data = NULL;
                hDecoder->sbr[i]->data_size = 0;
                hDecoder->sbr[i]->id_aac = hDecoder->element_id[i];
            }

            /* Allocate space for SBR output */
            if (hDecoder->time_out2[ch] == NULL)
            {
                hDecoder->time_out2[ch] = (real_t*)malloc(hDecoder->frameLength*2*sizeof(real_t));
                memset(hDecoder->time_out2[ch], 0, hDecoder->frameLength*2*sizeof(real_t));
            }

            if (hDecoder->sbr[i]->id_aac == ID_CPE)
            {
                /* space for 2 channels needed */
                if (hDecoder->time_out2[ch+1] == NULL)
                {
                    hDecoder->time_out2[ch+1] = (real_t*)malloc(hDecoder->frameLength*2*sizeof(real_t));
                    memset(hDecoder->time_out2[ch+1], 0, hDecoder->frameLength*2*sizeof(real_t));
                }

                memcpy(time_out2[ch],
                    time_out[ch], frame_len*sizeof(real_t));
                memcpy(time_out2[ch+1],
                    time_out[ch+1], frame_len*sizeof(real_t));
                sbrDecodeFrame(hDecoder->sbr[i],
                    time_out2[ch], time_out2[ch+1],
                    hDecoder->postSeekResetFlag, hDecoder->forceUpSampling);
                ch += 2;
            } else {
                memcpy(time_out2[ch],
                    time_out[ch], frame_len*sizeof(real_t));
                sbrDecodeFrame(hDecoder->sbr[i],
                    time_out2[ch], NULL,
                    hDecoder->postSeekResetFlag, hDecoder->forceUpSampling);
                ch++;
            }
        }
        frame_len *= 2;
        hInfo->samples *= 2;
        hInfo->samplerate *= 2;
        /* sbr */
        if (hDecoder->sbr_present_flag == 1)
        {
            hInfo->object_type = HE_AAC;
            hInfo->sbr = SBR_UPSAMPLED;
        } else {
            hInfo->sbr = NO_SBR_UPSAMPLED;
        }

        sample_buffer = output_to_PCM(hDecoder, time_out2, sample_buffer,
            output_channels, frame_len, outputFormat);
    } else {
#endif
        sample_buffer = output_to_PCM(hDecoder, time_out, sample_buffer,
            output_channels, frame_len, outputFormat);
#ifdef SBR_DEC
    }
#endif

    hDecoder->postSeekResetFlag = 0;

    hDecoder->frame++;
#ifdef LD_DEC
    if (object_type != LD)
    {
#endif
        if (hDecoder->frame <= 1)
            hInfo->samples = 0;
#ifdef LD_DEC
    } else {
        /* LD encoders will give lower delay */
        if (hDecoder->frame <= 0)
            hInfo->samples = 0;
    }
#endif

    /* cleanup */
#ifdef ANALYSIS
    fflush(stdout);
#endif

    return sample_buffer;

error:
    /* cleanup */
#ifdef ANALYSIS
    fflush(stdout);
#endif

    return NULL;
}
