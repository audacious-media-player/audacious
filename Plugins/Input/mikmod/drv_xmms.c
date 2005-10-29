
/*      MikMod sound library
   (c) 1998 Miodrag Vallat and others - see file AUTHORS for complete list

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Modified 2/1/99 for xmms by J. Nick Koston (BlueDraco)
 */

/*==============================================================================

  $Id: drv_xmms.c,v 1.4 2002/04/27 18:47:08 havard Exp $

  Output data to xmms

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include "libaudacious/util.h"

#include "mikmod-plugin.h"

static int buffer_size;

static SBYTE *audiobuffer = NULL;
extern MIKMODConfig mikmod_cfg;
static short audio_open = FALSE;
gboolean mikmod_xmms_audio_error = FALSE;
extern gboolean mikmod_going;

static BOOL xmms_IsThere(void)
{
	return 1;
}

static BOOL xmms_Init(void)
{
	AFormat fmt;
	int nch;

	buffer_size = 512;
	if (!mikmod_cfg.force8bit)
		buffer_size *= 2;
	if (!mikmod_cfg.force_mono)
		buffer_size *= 2;
	if (!(audiobuffer = (SBYTE *) g_malloc0(buffer_size)))
		return 1;
	
	fmt = (md_mode & DMODE_16BITS) ? FMT_S16_NE : FMT_U8;
	nch = (md_mode & DMODE_STEREO) ? 2 : 1;

	if (audio_open)
		mikmod_ip.output->close_audio();

	if (!mikmod_ip.output->open_audio(fmt, md_mixfreq, nch))
	{
		mikmod_xmms_audio_error = TRUE;
		return 1;
	}
	audio_open = TRUE;

	return VC_Init();
}

static void xmms_Exit(void)
{
	VC_Exit();
	if (audiobuffer)
	{
		g_free(audiobuffer);
		audiobuffer = NULL;
	}
	if (audio_open)
	{
		mikmod_ip.output->close_audio();
		audio_open = FALSE;
	}

}

static void xmms_Update(void)
{
	gint length;

	length = VC_WriteBytes((SBYTE *) audiobuffer, buffer_size);
	mikmod_ip.add_vis_pcm(mikmod_ip.output->written_time(), mikmod_cfg.force8bit ? FMT_U8 : FMT_S16_NE, mikmod_cfg.force_mono ? 1 : 2, length, audiobuffer);
	
	while(mikmod_ip.output->buffer_free() < length && mikmod_going)
		xmms_usleep(10000);
	if(mikmod_going)
		mikmod_ip.output->write_audio(audiobuffer, length);

}

static BOOL xmms_Reset(void)
{
	VC_Exit();
	return VC_Init();
}

MDRIVER drv_xmms =
{
	NULL,
	"xmms",
	"xmms output driver v1.0",
	0, 255,
#if (LIBMIKMOD_VERSION > 0x030106)
        "xmms",
        NULL,
#endif
        xmms_IsThere, 
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	xmms_Init,
	xmms_Exit,
	xmms_Reset,
	VC_SetNumVoices,
	VC_PlayStart,
	VC_PlayStop,
	xmms_Update,
	NULL,
	VC_VoiceSetVolume,
	VC_VoiceGetVolume,
	VC_VoiceSetFrequency,
	VC_VoiceGetFrequency,
	VC_VoiceSetPanning,
	VC_VoiceGetPanning,
	VC_VoicePlay,
	VC_VoiceStop,
	VC_VoiceStopped,
	VC_VoiceGetPosition,
	VC_VoiceRealVolume

};
