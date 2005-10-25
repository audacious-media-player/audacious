/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "lv_common.h"
#include "lv_audio.h"

static int audio_dtor (VisObject *object);

static int audio_band_total (VisAudio *audio, int begin, int end);
static int audio_band_energy (VisAudio *audio, int band, int length);

static int audio_dtor (VisObject *object)
{
	VisAudio *audio = VISUAL_AUDIO (object);

	visual_object_unref (VISUAL_OBJECT (audio->fft_state));

	return VISUAL_OK;
}

static int audio_band_total (VisAudio *audio, int begin, int end)
{
	int bpmtotal = 0;
	int i;

	for (i = begin; i < end; i++)
		bpmtotal += audio->freq[2][i];

	if (bpmtotal > 0)
		return bpmtotal / (end - begin);
	else
		return 0;
}

static int audio_band_energy (VisAudio *audio, int band, int length)
{
	int energytotal = 0;
	int i;

	for (i = 0; i < length; i++)
		energytotal += audio->bpmhistory[i][band];

	if (energytotal > 0)
		return energytotal / length;
	else
		return 0;
}

/**
 * @defgroup VisAudio VisAudio
 * @{
 */

/**
 * Creates a new VisAudio structure.
 *
 * @return A newly allocated VisAudio, or NULL on failure.
 */
VisAudio *visual_audio_new ()
{
	VisAudio *audio;

	audio = visual_mem_new0 (VisAudio, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (audio), TRUE, audio_dtor);

	return audio;
}

/**
 * This function analyzes the VisAudio, the FFT frequency magic gets done here, also
 * the audio energy is calculated and some other magic to provide the developer more
 * information about the current sample and the stream.
 *
 * For every sample that is being retrieved this needs to be called. However keep in mind
 * that the VisBin runs it automaticly.
 *
 * @param audio Pointer to a VisAudio that needs to be analyzed.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_AUDIO_NULL on failure.
 */
int visual_audio_analyze (VisAudio *audio)
{
        float tmp_out[256];
	double scale;
	int i, j, y;

	visual_log_return_val_if_fail (audio != NULL, -VISUAL_ERROR_AUDIO_NULL);

	/* Load the pcm data */
	for (i = 0; i < 512; i++) {
		audio->pcm[0][i] = audio->plugpcm[0][i];
		audio->pcm[1][i] = audio->plugpcm[1][i];
		audio->pcm[2][i] = (audio->plugpcm[0][i] + audio->plugpcm[1][i]) >> 1;
	}

	/* Initialize fft if not yet initialized */
	if (audio->fft_state == NULL)
		audio->fft_state = visual_fft_init ();

	/* FFT analyze the pcm data */
	visual_fft_perform (audio->plugpcm[0], tmp_out, audio->fft_state);
		
	for (i = 0; i < 256; i++)
		audio->freq[0][i] = ((int) sqrt (tmp_out[i + 1])) >> 8;

	visual_fft_perform (audio->plugpcm[1], tmp_out, audio->fft_state);

	for (i = 0; i < 256; i++)
		audio->freq[1][i] = ((int) sqrt (tmp_out[i + 1])) >> 8;

	for (i = 0; i < 256; i++)
		audio->freq[2][i] = (audio->freq[0][i] + audio->freq[1][i]) >> 1;

	/* Normalized frequency analyzer */
	/** @todo FIXME Not sure if this is totally correct */
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 256; j++) {
			/* (Height / log (256)) */
			scale = 256 / log (256);

			y = audio->freq[i][j];
			y = log (y) * scale;

			if (y < 0)
				y = 0;

			audio->freqnorm[i][j] = y;
		}
	}

	
	/* BPM stuff, used for the audio energy only right now */
	for (i = 1023; i > 0; i--) {
		visual_mem_copy (&audio->bpmhistory[i], &audio->bpmhistory[i - 1], 6 * sizeof (short int));
		visual_mem_copy (&audio->bpmdata[i], &audio->bpmdata[i - 1], 6 * sizeof (short int));
	}

	/* Calculate the audio energy */
	audio->energy = 0;
	
	for (i = 0; i < 6; i++)	{
		audio->bpmhistory[0][i] = audio_band_total (audio, i * 2, (i * 2) + 3);
		audio->bpmenergy[i] = audio_band_energy (audio, i, 10);

		audio->bpmdata[0][i] = audio->bpmhistory[0][i] - audio->bpmenergy[i];

		audio->energy += audio_band_energy(audio, i, 50);
	}

	audio->energy >>= 7;

	if (audio->energy > 100)
		audio->energy = 100;

	return VISUAL_OK;
}

/**
 * @}
 */
