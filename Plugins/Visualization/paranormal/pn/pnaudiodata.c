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

#include <math.h>

#include <config.h>
#include "pnaudiodata.h"

/* Initialization */
static void         pn_audio_data_class_init        (PnAudioDataClass *class);
static void         pn_audio_data_init              (PnAudioData *audio_data,
						     PnAudioDataClass *class);

static GObjectClass *parent_class = NULL;

GType
pn_audio_data_get_type (void)
{
  static GType audio_data_type = 0;

  if (! audio_data_type)
    {
      static const GTypeInfo audio_data_info =
      {
	sizeof (PnAudioDataClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_audio_data_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnAudioData),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_audio_data_init
      };

      /* FIXME: should this be dynamic? */
      audio_data_type = g_type_register_static (PN_TYPE_OBJECT,
					    "PnAudioData",
					    &audio_data_info,
					    0);
    }
  return audio_data_type;
}

static void
pn_audio_data_class_init (PnAudioDataClass *class)
{
  GObjectClass *gobject_class;
  PnObjectClass *object_class;

  gobject_class = (GObjectClass *) class;
  object_class = (PnObjectClass *) class;

  parent_class = g_type_class_peek_parent (class);
}

static void
pn_audio_data_init (PnAudioData *audio_data, PnAudioDataClass *class)
{
  audio_data->stereo = TRUE;

  /* Initialize with a 1-sample and 1-band pcm_ and freq_data, respecitvely */
  audio_data->pcm_samples = 1;
  audio_data->pcm_data[0] = g_new0 (gfloat, 1);
  audio_data->pcm_data[1] = g_new0 (gfloat, 1);
  audio_data->pcm_data[2] = g_new0 (gfloat, 1);
  
  audio_data->freq_bands = 1;
  audio_data->freq_data[0] = g_new0 (gfloat, 1);
  audio_data->freq_data[1] = g_new0 (gfloat, 1);
  audio_data->freq_data[2] = g_new0 (gfloat, 1);
}

/**
 * pn_audio_data_new
 *
 * Creates a new #PnAudioData object.
 *
 * Returns: The new #PnAudioData object
 */
PnAudioData*
pn_audio_data_new (void)
{
  return (PnAudioData *) g_object_new (PN_TYPE_AUDIO_DATA, NULL);
}

/**
 * pn_audio_data_set_stereo
 * @audio_data: a #PnAudioData
 * @stereo: TRUE or FALSE
 *
 * Sets whether @audio_data will use stereo audio data.  If
 * stereo is %FALSE, each channel buffer points to the same memory
 * location;  otherwise they are separate buffers.
 */
void
pn_audio_data_set_stereo (PnAudioData *audio_data, gboolean stereo)
{
  gboolean changed;

  g_return_if_fail (audio_data != NULL);
  g_return_if_fail (PN_IS_AUDIO_DATA (audio_data));

  changed = audio_data->stereo == stereo ? TRUE : FALSE;
  audio_data->stereo = stereo;

  if (changed)
    {
      pn_audio_data_set_pcm_samples (audio_data, audio_data->pcm_samples);
      pn_audio_data_set_freq_bands (audio_data, audio_data->freq_bands);
    }
}

/**
 * pn_audio_data_get_stereo
 * @audio_data: a #PnAudioData
 *
 * Retrieves the whether or not @audio_data contains stereo audio data
 *
 * Returns: %TRUE if it contains stereo data, %FALSE otherwise
 */
gboolean
pn_audio_data_get_stereo (PnAudioData *audio_data)
{
  g_return_val_if_fail (audio_data != NULL, FALSE);
  g_return_val_if_fail (PN_IS_AUDIO_DATA (audio_data), FALSE);

  return audio_data->stereo;
}

/**
 * pn_audio_data_set_pcm_samples
 * @audio_data: a #PnAudioData
 * @samples: the number of samples
 *
 * Sets the number of samples that @audio_data's pcm data
 * contains.
 */ 
void
pn_audio_data_set_pcm_samples (PnAudioData *audio_data, guint samples)
{
  g_return_if_fail (audio_data != NULL);
  g_return_if_fail (PN_IS_AUDIO_DATA (audio_data));
  g_return_if_fail (samples > 0);

  if (audio_data->pcm_data[0])
    {
      g_free (audio_data->pcm_data[0]);
      if (audio_data->stereo)
	{
	  g_free (audio_data->pcm_data[1]);
	  g_free (audio_data->pcm_data[2]);
	}
    }

  audio_data->pcm_samples = samples;
  audio_data->pcm_data[0] = g_new0 (gfloat, samples);
  if (audio_data->stereo)
    {
      audio_data->pcm_data[1] = g_new0 (gfloat, samples);
      audio_data->pcm_data[2] = g_new0 (gfloat, samples);
    }
  else
    audio_data->pcm_data[1] = audio_data->pcm_data[2] = audio_data->pcm_data[0];
}
      
/**
 * pn_audio_data_get_pcm_samples
 * @audio_data: a #PnAudioData
 *
 * Retrieves the number of samples that @audio_data's pcm data
 * contains
 *
 * Returns: The number of samples
 */
guint
pn_audio_data_get_pcm_samples (PnAudioData *audio_data)
{
  g_return_val_if_fail (audio_data != NULL, 0);
  g_return_val_if_fail (PN_IS_AUDIO_DATA (audio_data), 0);

  return audio_data->pcm_samples;
}  

/**
 * pn_audio_data_set_freq_bands
 * @audio_data: a #PnAudioData
 * @bands: the number of bands
 *
 * Sets the number of bands that @audio_data's frequency data
 * contains.
 */
void
pn_audio_data_set_freq_bands (PnAudioData *audio_data, guint bands)
{
  g_return_if_fail (audio_data != NULL);
  g_return_if_fail (PN_IS_AUDIO_DATA (audio_data));
  g_return_if_fail (bands > 0);

  
  if (audio_data->freq_data[0])
    {
      g_free (audio_data->freq_data[0]);
      if (audio_data->stereo)
	{
	  g_free (audio_data->freq_data[1]);
	  g_free (audio_data->freq_data[2]);
	}
    }

  audio_data->freq_bands = bands;
  audio_data->freq_data[0] = g_new0 (gfloat, bands);
  if (audio_data->stereo)
    {
      audio_data->freq_data[1] = g_new0 (gfloat, bands);
      audio_data->freq_data[2] = g_new0 (gfloat, bands);
    }
  else
    audio_data->freq_data[1] = audio_data->freq_data[2] = audio_data->freq_data[0];
}

/**
 * pn_audio_data_get_freq_bands
 * @audio_data: a #PnAudioData
 *
 * Retrieves the number of bands that @audio_data's frequency data
 * contains
 *
 * Returns: The number of bands
 */
guint
pn_audio_data_get_freq_bands (PnAudioData *audio_data)
{
  g_return_val_if_fail (audio_data != NULL, 0);
  g_return_val_if_fail (PN_IS_AUDIO_DATA (audio_data), 0);

  return audio_data->freq_bands;
}  

/**
 * pn_audio_data_get_volume
 * @audio_data: a #PnAudioData
 *
 * Retrieves the volume level (from 0.0 to 1.0) of the audio frame
 * contained within a #PnAudioData object.
 *
 * Returns: The volume level
 */
gfloat
pn_audio_data_get_volume (PnAudioData *audio_data)
{
  g_return_val_if_fail (audio_data != NULL, 0.0);
  g_return_val_if_fail (PN_IS_AUDIO_DATA (audio_data), 0.0);

  return audio_data->volume;
}

/**
 * pn_audio_data_update
 * @audio_data: a #PnAudioData
 *
 * Updates the information about the audio data frame in a #PnAudioData.
 * This function should be called after all updates to pcm_data or freq_data.
 */
void
pn_audio_data_update (PnAudioData *audio_data)
{
  guint i;

  g_return_if_fail (audio_data != NULL);
  g_return_if_fail (PN_IS_AUDIO_DATA (audio_data));

  /* Get the volume */
  audio_data->volume = 0.0;
  for (i=0; i<audio_data->pcm_samples; i++)
    audio_data->volume = MAX (audio_data->volume, fabs (audio_data->pcm_data[PN_CHANNEL_LEFT][i]));
}
