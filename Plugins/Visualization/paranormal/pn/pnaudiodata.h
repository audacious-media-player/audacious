/* Paranormal - A highly customizable audio visualization library
 * Copyright (C) 2001  Jamie Gennis <jgennis@mindspring.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __PN_AUDIO_DATA_H__
#define __PN_AUDIO_DATA_H__

#include "pnobject.h"


G_BEGIN_DECLS

typedef enum
{
  PN_CHANNEL_LEFT,
  PN_CHANNEL_CENTER,
  PN_CHANNEL_RIGHT
} PnAudioDataChannels;

#define PN_TYPE_AUDIO_DATA              (pn_audio_data_get_type ())
#define PN_AUDIO_DATA(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_AUDIO_DATA, PnAudioData))
#define PN_AUDIO_DATA_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_AUDIO_DATA, PnAudioDataClass))
#define PN_IS_AUDIO_DATA(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_AUDIO_DATA))
#define PN_IS_AUDIO_DATA_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_AUDIO_DATA))
#define PN_AUDIO_DATA_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_AUDIO_DATA, PnAudioDataClass))
#define PN_AUDIO_DATA_CLASS_TYPE(class) (G_TYPE_FROM_CLASS (class))
#define PN_AUDIO_DATA_CLASS_NAME(class) (g_type_name (PN_AUDIO_DATA_CLASS_TYPE (class)))

#define PN_AUDIO_DATA_PCM_SAMPLES(obj)  (PN_AUDIO_DATA (obj)->pcm_samples)
#define PN_AUDIO_DATA_PCM_DATA(obj,ch)  (PN_AUDIO_DATA (obj)->pcm_data[ch])
#define PN_AUDIO_DATA_FREQ_BANDS(obj)   (PN_AUDIO_DATA (obj)->freq_bands)
#define PN_AUDIO_DATA_FREQ_DATA(obj,ch) (PN_AUDIO_DATA (obj)->freq_data[ch])

typedef struct _PnAudioData        PnAudioData;
typedef struct _PnAudioDataClass   PnAudioDataClass;

struct _PnAudioData
{
  PnObject parent;

  /*< public >*/

  /* PCM and frequency data are floats in the range of 0 to 1 */

  /* PCM Data */
  guint pcm_samples;    /* read-only */
  gfloat *pcm_data[3];  /* read-write */

  /* Frequency Data */
  guint freq_bands;     /* read-only */
  gfloat *freq_data[3]; /* read-write */

  /*< private >*/
  /* If this is TRUE, each channel is separate, otherwise they all point
   * to the same buffer.  Default is TRUE.
   */
  gboolean stereo;

  /* Overall volume of the audio frame */
  gfloat volume;
};

struct _PnAudioDataClass
{
  PnObjectClass parent_class;
};

/* Creators */
GType             pn_audio_data_get_type             (void);
PnAudioData      *pn_audio_data_new                  (void);

/* Accessors */
void              pn_audio_data_set_stereo           (PnAudioData *audio_data,
						      gboolean stereo);
gboolean          pn_audio_data_get_stereo           (PnAudioData *audio_data);
void              pn_audio_data_set_pcm_samples      (PnAudioData *audio_data,
						      guint samples);
guint             pn_audio_data_get_pcm_samples      (PnAudioData *audio_data);
void              pn_audio_data_set_freq_bands       (PnAudioData *audio_data,
						      guint bands);
guint             pn_audio_data_get_freq_bands       (PnAudioData *audio_data);
gfloat            pn_audio_data_get_volume           (PnAudioData *audio_data);

/* Actions */
void              pn_audio_data_update               (PnAudioData *audio_data);

#endif /* __PN_AUDIO_DATA_H__ */
