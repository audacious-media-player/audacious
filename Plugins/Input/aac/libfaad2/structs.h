/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
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
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: structs.h,v 1.42 2004/09/08 09:43:11 gcp Exp $
**/

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "cfft.h"
#ifdef SBR_DEC
#include "sbr_dec.h"
#endif

#define MAX_CHANNELS        64
#define MAX_SYNTAX_ELEMENTS 48
#define MAX_WINDOW_GROUPS    8
#define MAX_SFB             51
#define MAX_LTP_SFB         40
#define MAX_LTP_SFB_S        8

/* used to save the prediction state */
typedef struct {
    signed short r[2];
    signed short COR[2];
    signed short VAR[2];
} pred_state;

typedef struct {
    unsigned short N;
    cfft_info *cfft;
    complex_t *sincos;
#ifdef PROFILE
    int64_t cycles;
    int64_t fft_cycles;
#endif
} mdct_info;

typedef struct
{
    const real_t *long_window[2];
    const real_t *short_window[2];
#ifdef LD_DEC
    const real_t *ld_window[2];
#endif

    mdct_info *mdct256;
#ifdef LD_DEC
    mdct_info *mdct1024;
#endif
    mdct_info *mdct2048;
#ifdef PROFILE
    int64_t cycles;
#endif
} fb_info;

typedef struct
{
    unsigned char present;

    unsigned char num_bands;
    unsigned char pce_instance_tag;
    unsigned char excluded_chns_present;
    unsigned char band_top[17];
    unsigned char prog_ref_level;
    unsigned char dyn_rng_sgn[17];
    unsigned char dyn_rng_ctl[17];
    unsigned char exclude_mask[MAX_CHANNELS];
    unsigned char additional_excluded_chns[MAX_CHANNELS];

    real_t ctrl1;
    real_t ctrl2;
} drc_info;

typedef struct
{
    unsigned char element_instance_tag;
    unsigned char object_type;
    unsigned char sf_index;
    unsigned char num_front_channel_elements;
    unsigned char num_side_channel_elements;
    unsigned char num_back_channel_elements;
    unsigned char num_lfe_channel_elements;
    unsigned char num_assoc_data_elements;
    unsigned char num_valid_cc_elements;
    unsigned char mono_mixdown_present;
    unsigned char mono_mixdown_element_number;
    unsigned char stereo_mixdown_present;
    unsigned char stereo_mixdown_element_number;
    unsigned char matrix_mixdown_idx_present;
    unsigned char pseudo_surround_enable;
    unsigned char matrix_mixdown_idx;
    unsigned char front_element_is_cpe[16];
    unsigned char front_element_tag_select[16];
    unsigned char side_element_is_cpe[16];
    unsigned char side_element_tag_select[16];
    unsigned char back_element_is_cpe[16];
    unsigned char back_element_tag_select[16];
    unsigned char lfe_element_tag_select[16];
    unsigned char assoc_data_element_tag_select[16];
    unsigned char cc_element_is_ind_sw[16];
    unsigned char valid_cc_element_tag_select[16];

    unsigned char channels;

    unsigned char comment_field_bytes;
    unsigned char comment_field_data[257];

    /* extra added values */
    unsigned char num_front_channels;
    unsigned char num_side_channels;
    unsigned char num_back_channels;
    unsigned char num_lfe_channels;
    unsigned char sce_channel[16];
    unsigned char cpe_channel[16];
} program_config;

typedef struct
{
    unsigned short syncword;
    unsigned char id;
    unsigned char layer;
    unsigned char protection_absent;
    unsigned char profile;
    unsigned char sf_index;
    unsigned char private_bit;
    unsigned char channel_configuration;
    unsigned char original;
    unsigned char home;
    unsigned char emphasis;
    unsigned char copyright_identification_bit;
    unsigned char copyright_identification_start;
    unsigned short aac_frame_length;
    unsigned short adts_buffer_fullness;
    unsigned char no_raw_data_blocks_in_frame;
    unsigned short crc_check;

    /* control param */
    unsigned char old_format;
} adts_header;

typedef struct
{
    unsigned char copyright_id_present;
    signed char copyright_id[10];
    unsigned char original_copy;
    unsigned char home;
    unsigned char bitstream_type;
    unsigned long bitrate;
    unsigned char num_program_config_elements;
    unsigned long adif_buffer_fullness;

    /* maximum of 16 PCEs */
    program_config pce[16];
} adif_header;

#ifdef LTP_DEC
typedef struct
{
    unsigned char last_band;
    unsigned char data_present;
    unsigned short lag;
    unsigned char lag_update;
    unsigned char coef;
    unsigned char long_used[MAX_SFB];
    unsigned char short_used[8];
    unsigned char short_lag_present[8];
    unsigned char short_lag[8];
} ltp_info;
#endif

#ifdef MAIN_DEC
typedef struct
{
    unsigned char limit;
    unsigned char predictor_reset;
    unsigned char predictor_reset_group_number;
    unsigned char prediction_used[MAX_SFB];
} pred_info;
#endif

typedef struct
{
    unsigned char number_pulse;
    unsigned char pulse_start_sfb;
    unsigned char pulse_offset[4];
    unsigned char pulse_amp[4];
} pulse_info;

typedef struct
{
    unsigned char n_filt[8];
    unsigned char coef_res[8];
    unsigned char length[8][4];
    unsigned char order[8][4];
    unsigned char direction[8][4];
    unsigned char coef_compress[8][4];
    unsigned char coef[8][4][32];
} tns_info;

#ifdef SSR_DEC
typedef struct
{
    unsigned char max_band;

    unsigned char adjust_num[4][8];
    unsigned char alevcode[4][8][8];
    unsigned char aloccode[4][8][8];
} ssr_info;
#endif

typedef struct
{
    unsigned char max_sfb;

    unsigned char num_swb;
    unsigned char num_window_groups;
    unsigned char num_windows;
    unsigned char window_sequence;
    unsigned char window_group_length[8];
    unsigned char window_shape;
    unsigned char scale_factor_grouping;
    unsigned short sect_sfb_offset[8][15*8];
    unsigned short swb_offset[52];

    unsigned char sect_cb[8][15*8];
    unsigned short sect_start[8][15*8];
    unsigned short sect_end[8][15*8];
    unsigned char sfb_cb[8][8*15];
    unsigned char num_sec[8]; /* number of sections in a group */

    unsigned char global_gain;
    signed short scale_factors[8][51]; /* [0..255] */

    unsigned char ms_mask_present;
    unsigned char ms_used[MAX_WINDOW_GROUPS][MAX_SFB];

    unsigned char noise_used;

    unsigned char pulse_data_present;
    unsigned char tns_data_present;
    unsigned char gain_control_data_present;
    unsigned char predictor_data_present;

    pulse_info pul;
    tns_info tns;
#ifdef MAIN_DEC
    pred_info pred;
#endif
#ifdef LTP_DEC
    ltp_info ltp;
    ltp_info ltp2;
#endif
#ifdef SSR_DEC
    ssr_info ssr;
#endif

#ifdef ERROR_RESILIENCE
    /* ER HCR data */
    unsigned short length_of_reordered_spectral_data;
    unsigned char length_of_longest_codeword;
    /* ER RLVC data */
    unsigned char sf_concealment;
    unsigned char rev_global_gain;
    unsigned short length_of_rvlc_sf;
    unsigned short dpcm_noise_nrg;
    unsigned char sf_escapes_present;
    unsigned char length_of_rvlc_escapes;
    unsigned short dpcm_noise_last_position;
#endif
} ic_stream; /* individual channel stream */

typedef struct
{
    unsigned char channel;
    signed short paired_channel;

    unsigned char element_instance_tag;
    unsigned char common_window;

    ic_stream ics1;
    ic_stream ics2;
} element; /* syntax element (SCE, CPE, LFE) */

typedef struct mp4AudioSpecificConfig
{
    /* Audio Specific Info */
    unsigned char objectTypeIndex;
    unsigned char samplingFrequencyIndex;
    unsigned long samplingFrequency;
    unsigned char channelsConfiguration;

    /* GA Specific Info */
    unsigned char frameLengthFlag;
    unsigned char dependsOnCoreCoder;
    unsigned short coreCoderDelay;
    unsigned char extensionFlag;
    unsigned char aacSectionDataResilienceFlag;
    unsigned char aacScalefactorDataResilienceFlag;
    unsigned char aacSpectralDataResilienceFlag;
    unsigned char epConfig;

    signed char sbr_present_flag;
    unsigned char forceUpSampling;
    unsigned char downSampledSBR;
} mp4AudioSpecificConfig;

typedef struct NeAACDecConfiguration
{
    unsigned char defObjectType;
    unsigned long defSampleRate;
    unsigned char outputFormat;
    unsigned char downMatrix;
    unsigned char useOldADTSFormat;
    unsigned char dontUpSampleImplicitSBR;
} NeAACDecConfiguration, *NeAACDecConfigurationPtr;

typedef struct NeAACDecFrameInfo
{
    unsigned long bytesconsumed;
    unsigned long samples;
    unsigned char channels;
    unsigned char error;
    unsigned long samplerate;

    /* SBR: 0: off, 1: on; normal, 2: on; downsampled */
    unsigned char sbr;

    /* MPEG-4 ObjectType */
    unsigned char object_type;

    /* AAC header type; MP4 will be signalled as RAW also */
    unsigned char header_type;

    /* multichannel configuration */
    unsigned char num_front_channels;
    unsigned char num_side_channels;
    unsigned char num_back_channels;
    unsigned char num_lfe_channels;
    unsigned char channel_position[MAX_CHANNELS];

    /* PS: 0: off, 1: on */
    unsigned char ps;
} NeAACDecFrameInfo;

typedef struct
{
    unsigned char adts_header_present;
    unsigned char adif_header_present;
    unsigned char sf_index;
    unsigned char object_type;
    unsigned char channelConfiguration;
#ifdef ERROR_RESILIENCE
    unsigned char aacSectionDataResilienceFlag;
    unsigned char aacScalefactorDataResilienceFlag;
    unsigned char aacSpectralDataResilienceFlag;
#endif
    unsigned short frameLength;
    unsigned char postSeekResetFlag;

    unsigned long frame;

    unsigned char downMatrix;
    unsigned char upMatrix;
    unsigned char first_syn_ele;
    unsigned char has_lfe;
    /* number of channels in current frame */
    unsigned char fr_channels;
    /* number of elements in current frame */
    unsigned char fr_ch_ele;

    /* element_output_channels:
       determines the number of channels the element will output
    */
    unsigned char element_output_channels[MAX_SYNTAX_ELEMENTS];
    /* element_alloced:
       determines whether the data needed for the element is allocated or not
    */
    unsigned char element_alloced[MAX_SYNTAX_ELEMENTS];
    /* alloced_channels:
       determines the number of channels where output data is allocated for
    */
    unsigned char alloced_channels;

    /* output data buffer */
    void *sample_buffer;

    unsigned char window_shape_prev[MAX_CHANNELS];
#ifdef LTP_DEC
    unsigned short ltp_lag[MAX_CHANNELS];
#endif
    fb_info *fb;
    drc_info *drc;

    real_t *time_out[MAX_CHANNELS];
    real_t *fb_intermed[MAX_CHANNELS];

#ifdef SBR_DEC
    signed char sbr_present_flag;
    signed char forceUpSampling;
    signed char downSampledSBR;
    /* determines whether SBR data is allocated for the gives element */
    unsigned char sbr_alloced[MAX_SYNTAX_ELEMENTS];

    sbr_info *sbr[MAX_SYNTAX_ELEMENTS];
#endif
#if (defined(PS_DEC) || defined(DRM_PS))
    unsigned char ps_used[MAX_SYNTAX_ELEMENTS];
    unsigned char ps_used_global;
#endif

#ifdef SSR_DEC
    real_t *ssr_overlap[MAX_CHANNELS];
    real_t *prev_fmd[MAX_CHANNELS];
    real_t ipqf_buffer[MAX_CHANNELS][4][96/4];
#endif

#ifdef MAIN_DEC
    pred_state *pred_stat[MAX_CHANNELS];
#endif
#ifdef LTP_DEC
    signed short *lt_pred_stat[MAX_CHANNELS];
#endif

    /* Program Config Element */
    unsigned char pce_set;
    program_config pce;
    unsigned char element_id[MAX_CHANNELS];
    unsigned char internal_channel[MAX_CHANNELS];

    /* Configuration data */
    NeAACDecConfiguration config;

#ifdef PROFILE
    int64_t cycles;
    int64_t spectral_cycles;
    int64_t output_cycles;
    int64_t scalefac_cycles;
    int64_t requant_cycles;
#endif
} NeAACDecStruct, *NeAACDecHandle;



#ifdef __cplusplus
}
#endif
#endif
