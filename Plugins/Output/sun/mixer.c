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
#include "mixer.h"

int sun_mixer_open(void)
{
	if (pthread_mutex_lock(&audio.mixer_mutex) != 0)
		return -1;

    	if (!audio.mixer_keepopen || audio.mixerfd < 1)
	{
		audio.mixerfd = open(audio.devmixer, O_RDWR);
		if (audio.mixerfd < 0)
			perror(audio.devmixer);
	}
	return 0;
}

void sun_mixer_close(void)
{
	if (!audio.mixer_keepopen)
	{
		close(audio.mixerfd);
		audio.mixerfd = 0;
	}
	pthread_mutex_unlock(&audio.mixer_mutex);
}

int sun_mixer_get_dev(int fd, int *dev, char *id)
{
	mixer_devinfo_t info;

	for (info.index = 0; ioctl(fd, AUDIO_MIXER_DEVINFO, &info) >= 0;
	     info.index++)
	{
		if (!strcmp(id, info.label.name))
		{
			*dev = info.index;
			return 0;
		}
	}
	return -1;
}

void sun_get_volume(int *l, int *r)
{
	mixer_ctrl_t mixer;

	if (sun_mixer_open() < 0)
	{
		*l = 0;
		*r = 0;
		return;
	}

	if ((sun_mixer_get_dev(audio.mixerfd, &mixer.dev,
	    audio.mixer_voldev) < 0))
		goto closemixer;

	mixer.type = AUDIO_MIXER_VALUE;
	if (audio.output != NULL)
		mixer.un.value.num_channels = audio.output->channels;
	else
		mixer.un.value.num_channels = 2;

	if (ioctl(audio.mixerfd, AUDIO_MIXER_READ, &mixer) < 0)
		goto closemixer;
	*l = (mixer.un.value.level[AUDIO_MIXER_LEVEL_LEFT] * 100) / 255;
	if (mixer.un.value.num_channels > 1)
		*r = (mixer.un.value.level[AUDIO_MIXER_LEVEL_RIGHT] * 100) / 255;
	else
		*r = *l;

 closemixer:
	sun_mixer_close();
}

void sun_set_volume(int l, int r)
{
	mixer_ctrl_t mixer;

	if (sun_mixer_open() < 0)
		return;

	if ((sun_mixer_get_dev(audio.mixerfd, &mixer.dev,
			       audio.mixer_voldev) < 0))
	{
		if (!audio.mixer_keepopen)
			close(audio.mixerfd);
		return;
	}
	mixer.type = AUDIO_MIXER_VALUE;
	if (audio.output != NULL)
		mixer.un.value.num_channels = audio.output->channels;
	else
		mixer.un.value.num_channels = 2;
	mixer.un.value.level[AUDIO_MIXER_LEVEL_LEFT] = (l * 255) / 100;
	if (mixer.un.value.num_channels > 1)
		mixer.un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = (r * 255) / 100;

	if (ioctl(audio.mixerfd, AUDIO_MIXER_WRITE, &mixer) < 0)
	{
		if (!audio.mixer_keepopen)
			close(audio.mixerfd);
		return;
	}
	sun_mixer_close();
}
