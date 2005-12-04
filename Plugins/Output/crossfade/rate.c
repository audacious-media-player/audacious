/*
 *  XMMS Crossfade Plugin
 *  Copyright (C) 2000-2004  Peter Eisenlohr <peter@eisenlohr.org>
 *
 *  based on the original OSS Output Plugin
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

/*
 *  Rate conversion for 16bit stereo samples.
 * 
 *  The algorithm (Least Common Multiple Linear Interpolation) was
 *  adapted from the rate conversion code used in
 *
 *    sox-12.16, Copyright 1998  Fabrice Bellard, originally
 *               Copyright 1991  Lance Norskog And Sundry Contributors.
 *
 *
 *  NOTE: As of XMMS-crossfade 0.3.6, the resampling code has been extended
 *        to use libsamplerate (also known as Secret Rabbit Code) for better
 *        quality. See http://www.mega-nerd.com/SRC/ for details.
 *
 *  CREDIT: While I started implementing support on my own, it never reached
 *          a usable state. A working re-implementation of libsamplerate
 *          support has finally been provided by Samuel Krempp.
 *
 *          Many thanks for your work and the patch!
 */


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "crossfade.h"
#include "rate.h"
#include "volume.h"

#include <string.h>
#include <assert.h>
#include <math.h>  /* for ceil */


#ifndef HAVE_LIBSAMPLERATE
static glong gcd(glong m, glong n)
{
  glong r;
  while(1) {
    r = m % n;
    if(r == 0) return n;
    m = n;
    n = r;
  }
}

static glong lcm(gint i, gint j)
{
  return ((glong)i * j) / gcd(i, j);
}
#endif


void rate_init(rate_context_t *rc)
{
  memset(rc, 0, sizeof(*rc));
  volume_init(&rc->vc);
}

void rate_config(rate_context_t *rc, gint in_rate, gint out_rate, int converter_type)
{
  if((in_rate  <= 0) || (in_rate  > 65535) || 
     (out_rate <= 0) || (out_rate > 65535)) {
    DEBUG(("[crossfade] rate_config: "
	   "illegal rates (in=%d, out=%d)!\n", in_rate, out_rate));
    return;
  }

  rate_free(rc);

  rc->in_rate  = in_rate;
  rc->out_rate = out_rate;
#ifdef HAVE_LIBSAMPLERATE
  rc->converter_type = converter_type;
  rc->src_data.src_ratio = out_rate *1.0/ in_rate;
  if (!(rc->src_state = src_new(converter_type, 2, NULL))) {
    DEBUG(("[crossfade] rate_config: error creating converter!\n"));
    return;
  }
  DEBUG(("[crossfade] rate_config: using \"%s\" for %d -> %d rate conversion\n",
	 src_get_name(converter_type), in_rate, out_rate));
  
  src_set_ratio(rc->src_state, rc->src_data.src_ratio);
  rc->src_data.end_of_input = 0;
#else
  rc->lcm_rate = lcm(in_rate, out_rate);
  rc->in_skip  = rc->lcm_rate / in_rate;
  rc->out_skip = rc->lcm_rate / out_rate;
  rc->in_ofs   = 0;
  rc->out_ofs  = 0;
  rc->started  = FALSE;
#endif
  rc->valid    = TRUE;
}

gint realloc_if_needed(gpointer *pointer, gint *size, gint req_size)
{
  gpointer data;
  if(req_size==0)
    return 0;

  if(!*pointer || *size < req_size ) {
    DEBUG(("*** allocation %d bytes\n", req_size));
    if(!(data = g_realloc(*pointer, req_size) ) ) {
      DEBUG(("[crossfade] rate_flow: g_realloc(%d) failed!\n", req_size));
      return -1;
    }
    *pointer = data;
    *size = req_size;
    return req_size;
  }
  else return 0;
}

gint16 final_quantize(volume_context_t *vc, gfloat v, gfloat scale)
{
  if(config->mixer_software)
    v *= scale;

  v = rintf(v);
  if(v > 32767 || v < -32768) {
    ++vc->clips;
    v = CLAMP(v, -32768, 32767);
  }

  return (gint16) v;
}

gint rate_flow(rate_context_t *rc, gpointer *buffer, gint length)
{
  gint size;
  gfloat volume_scale_l=1, volume_scale_r=1;
  struct timeval tv;
  glong dt;

  /* print clipping warnings at most once every second : */
  gettimeofday(&tv, NULL);
  dt = (tv.tv_sec - rc->vc.tv_last.tv_sec)  * 1000
    + (tv.tv_usec - rc->vc.tv_last.tv_usec) / 1000;
  if(((dt < 0) || (dt > 1000)) && (rc->vc.clips > 0)) {
    g_message("[crossfade] final_quantize: %d samples clipped!", rc->vc.clips);
    rc->vc.clips = 0;
    rc->vc.tv_last = tv;
  }

  /* attenuate 1dB every 4% -> 25dB for 100% */
  if(config->mixer_software) {
    volume_scale_l = volume_compute_factor(config->mixer_vol_left, 25);
    volume_scale_r = volume_compute_factor(config->mixer_vol_right, 25);
  }

  if(rc->in_rate == rc->out_rate) {
    /* trivial case, special code (no actual resampling !) */
    gint16 *int_p = *buffer, *int_p2 = NULL;
    gint i;

    gint error = realloc_if_needed((gpointer*) &rc->data, &rc->size, length);
    assert(error != -1);
    int_p2 = rc->data;

    length /= 4;  /* 2 channels x 2 bytes */
    for(i=0; i<length; i++) {
      gfloat v_l = *(int_p++);
      gfloat v_r = *(int_p++);
      *(int_p2 ++) = final_quantize(&rc->vc, v_l, volume_scale_l); 
      *(int_p2 ++) = final_quantize(&rc->vc, v_r, volume_scale_r); 
    }

    *buffer = rc->data;
    return length*4;
  }

#ifdef HAVE_LIBSAMPLERATE
  gint16 *int_p;
  gfloat *float_p;
  gint i, out_bound, out_len, error = 0;

  assert( length % 4 == 0);
  length /= 4;  /* 2 channels x 2 bytes */
  /* safe upper bound of output length: */
  out_bound = (gint) ceil((rc->src_data.src_ratio + 0.05) * length); 

  /* (re-)allocate our buffers */
  size = length * 2 * sizeof(gfloat);
  error = realloc_if_needed((gpointer*)&rc->src_data.data_in, &rc->src_in_size, size);
  assert(error != -1);

  rc->src_data.input_frames = length;
  rc->src_data.end_of_input = 0;
 
  size = out_bound * 2 * sizeof(gfloat);
  error = realloc_if_needed((gpointer*) &rc->src_data.data_out, &rc->src_out_size, size);
  assert(error != -1);

  rc->src_data.output_frames = out_bound;

  size = out_bound * 2 * sizeof(gint16);
  error = realloc_if_needed((gpointer*) &rc->data, &rc->size, size);
  assert(error != -1);

  /* putting data into the float buffers : */
  float_p = rc->src_data.data_in;
  int_p = *buffer;
  for(i=0; i < 2*length; i++)
    *(float_p++) = (gfloat) *(int_p++) / 32768.;
  assert(float_p == rc->src_data.data_in + length*2);

  /* process the float buffers : */
  error = src_process(rc->src_state, & rc->src_data);
  if(error) {
    g_message("[crossfade] rate_flow : src_error %d (%s)", error, src_strerror(error));
  }
    
  float_p = rc->src_data.data_out;
  int_p = rc->data;
  out_len = rc->src_data.output_frames_gen;
  assert(out_len <= out_bound);
  assert(rc->src_data.input_frames_used == length);

  for(i=0; i<out_len; i++) {
    gfloat v_l = 32768* *(float_p ++);
    gfloat v_r = 32768* *(float_p ++);

    *(int_p ++) = final_quantize(&rc->vc, v_l, volume_scale_l);
    *(int_p ++) = final_quantize(&rc->vc, v_r, volume_scale_r);
  }

  *buffer = rc->data;
  return out_len * 4;

#else /* rustic resampling */

  gpointer data;
  gint isamp, emitted = 0;
  gint16 *out, *in = *buffer;

  /* some sanity checks */
  if(length & 3) {
    DEBUG(("[crossfade] rate_flow: truncating %d bytes!", length & 3));
    length &= -4;
  }
  isamp = length / 4;
  if(isamp <= 0) return 0;
  if(!rc || !rc->valid) return length;
  if(rc->in_skip == rc->out_skip) return length;

  /* (re)allocate buffer */
  size = ((isamp * rc->in_skip) / rc->out_skip + 1) * 4;
  if(!rc->data || (size > rc->size)) {
    if(!(data = g_realloc(rc->data, size))) {
      DEBUG(("[crossfade] rate_flow: g_realloc(%d) failed!\n", size));
      return 0;
    }
    rc->data = data;
    rc->size = size;
  }
  *buffer = out = rc->data;

  /* first sample? */
  if(!rc->started) {
    rc->last_l  = in[0];
    rc->last_r  = in[1];
    rc->started = TRUE;
  }

  /* advance input range to span next output */
  while(((rc->in_ofs + rc->in_skip) <= rc->out_ofs) && (isamp-- > 0)) {
    rc->last_l  = *in++;
    rc->last_r  = *in++;
    rc->in_ofs += rc->in_skip;
  }
  if(isamp == 0) return emitted * 4;

  /* interpolate */
  for(;;) {
    gfloat v_l = rc->last_l + (((gfloat)in[0] - rc->last_l)
			    * (rc->out_ofs - rc->in_ofs)
			    / rc->in_skip);
    
    gfloat v_r = rc->last_r + (((gfloat)in[1] - rc->last_r)
			    * (rc->out_ofs - rc->in_ofs)
			    / rc->in_skip);
    *(out ++) = final_quantize(&rc->vc, v_l, volume_scale_l);
    *(out ++) = final_quantize(&rc->vc, v_r, volume_scale_r);
    /* count emitted samples*/
    emitted++;
    
    /* advance to next output */
    rc->out_ofs += rc->out_skip;
    
    /* advance input range to span next output */
    while((rc->in_ofs + rc->in_skip) <= rc->out_ofs) {
      rc->last_l  = *in++;
      rc->last_r  = *in++;
      rc->in_ofs += rc->in_skip;
      if(--isamp == 0) return emitted * 4;
    }
    
    /* long samples with high LCM's overrun counters! */
    if(rc->out_ofs == rc->in_ofs)
      rc->out_ofs = rc->in_ofs = 0;
  }
  
  return 0;  /* program flow never reaches this point */
#endif
}

void
rate_free(rate_context_t *rc)
{
#ifdef HAVE_LIBSAMPLERATE
  if(rc->src_state)         src_delete(rc->src_state); 
  if(rc->src_data.data_in)  g_free(rc->src_data.data_in);
  if(rc->src_data.data_out) g_free(rc->src_data.data_out); 
#endif
  if(rc->data)              g_free(rc->data);

  memset(rc, 0, sizeof(*rc));
}
