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

#ifndef _LV_AUDIO_H
#define _LV_AUDIO_H

#include <libvisual/lv_fft.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define VISUAL_AUDIO(obj)				(VISUAL_CHECK_CAST ((obj), 0, VisAudio))

typedef struct _VisAudio VisAudio;

/**
 * The VisAudio structure contains the sample and extra information
 * about the sample like a 256 bands analyzer, sound energy and
 * in the future BPM detection.
 *
 * @see visual_audio_new
 */
struct _VisAudio {
	VisObject	 object;			/**< The VisObject data. */

	short		 plugpcm[2][512];		/**< PCM data that comes from the input plugin
							 * or a callback function. */
	short		 pcm[3][512];			/**< PCM data that should be used within plugins
							 * pcm[0][x] is the left channel, pcm[1][x] is the right
							 * channel and pcm[2][x] is an average of both channels. */
	short		 freq[3][256];			/**< Frequency data as a 256 bands analyzer, with the channels
							 * like with the pcm element. */
	short		 freqnorm[3][256];		/**< Frequency data like the freq member, however this time the bands
							 * are normalized. */
	VisFFTState	*fft_state;			/**< Private member that contains context information for the FFT engine. */

	short int	 bpmhistory[1024][6];		/**< Private member for BPM detection, not implemented right now. */
	short int	 bpmdata[1024][6];		/**< Private member for BPM detection, not implemented right now. */
	short int	 bpmenergy[6];			/**< Private member for BPM detection, not implemented right now. */
	int		 energy;			/**< Audio energy level. */
};

VisAudio *visual_audio_new (void);
int visual_audio_analyze (VisAudio *audio);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LV_AUDIO_H */
