/*
 *  Copyright (C) 2001  CubeSoft Communications, Inc.
 *  <http://www.csoft.org>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "sun.h"
#include "libaudacious/configfile.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <errno.h>

struct sun_audio audio;

OutputPlugin sun_op =
{
	NULL,
	NULL,
	NULL,			/* Description */
	sun_init,
	sun_cleanup,
	sun_about,
	sun_configure,
	sun_get_volume,
	sun_set_volume,
	sun_open,
	sun_write,
	sun_close,
	sun_flush,
	sun_pause,
	sun_free,
	sun_playing,
	sun_output_time,
	sun_written_time,
	NULL
};


OutputPlugin * get_oplugin_info(void)
{
	sun_op.description = g_strdup_printf(_("BSD Sun Driver %s"),
					     SUN_VERSION);
	return (&sun_op);
}

void sun_init(void)
{
	ConfigFile *cfgfile;
	char *s;

	memset(&audio, 0, sizeof(struct sun_audio));

	cfgfile = xmms_cfg_open_default_file();
	/* Devices */
	xmms_cfg_read_string(cfgfile, "sun", "audio_devaudio", &audio.devaudio);
	xmms_cfg_read_string(cfgfile, "sun",
			     "audio_devaudioctl", &audio.devaudioctl);
	xmms_cfg_read_string(cfgfile, "sun", "audio_devmixer", &audio.devmixer);

	/* Buffering */
	xmms_cfg_read_int(cfgfile, "sun",
			  "buffer_size", &audio.req_buffer_size);
	xmms_cfg_read_int(cfgfile, "sun",
			  "prebuffer_size", &audio.req_prebuffer_size);

	/* Mixer */
	xmms_cfg_read_string(cfgfile, "sun", "mixer_voldev", &audio.mixer_voldev);
	xmms_cfg_read_boolean(cfgfile, "sun",
			      "mixer_keepopen", &audio.mixer_keepopen);

	xmms_cfg_free(cfgfile);

	/* Audio device path */
	if ((s = getenv("AUDIODEVICE")))
		audio.devaudio = g_strdup(s);
	else if (!audio.devaudio || !strcmp("", audio.devaudio))
		audio.devaudio = g_strdup(SUN_DEV_AUDIO);

	/* Audio control device path */
	if (!audio.devaudioctl || !strcmp("", audio.devaudioctl))
		audio.devaudioctl = g_strdup(SUN_DEV_AUDIOCTL);

	/* Mixer device path */
	if ((s = getenv("MIXERDEVICE")))
		audio.devmixer = g_strdup(s);
	else if (!audio.devmixer || !strcmp("", audio.devmixer))
		audio.devmixer = g_strdup(SUN_DEV_MIXER);

	if (!audio.mixer_voldev || !strcmp("", audio.mixer_voldev))
		audio.mixer_voldev = g_strdup(SUN_DEFAULT_VOLUME_DEV);

	/* Default buffering settings */
	if (!audio.req_buffer_size)
		audio.req_buffer_size = SUN_DEFAULT_BUFFER_SIZE;
	if (!audio.req_prebuffer_size)
		audio.req_prebuffer_size = SUN_DEFAULT_PREBUFFER_SIZE;

	audio.input = NULL;
	audio.output = NULL;
	audio.effect = NULL;

	if (pthread_mutex_init(&audio.mixer_mutex, NULL) != 0)
		perror("mixer_mutex");
}

void sun_cleanup(void)
{
	g_free(audio.devaudio);
	g_free(audio.devaudioctl);
	g_free(audio.devmixer);
	g_free(audio.mixer_voldev);

	if (!pthread_mutex_lock(&audio.mixer_mutex))
	{
		if (audio.mixerfd)
			close(audio.mixerfd);
		pthread_mutex_unlock(&audio.mixer_mutex);
		pthread_mutex_destroy(&audio.mixer_mutex);
	}
}

