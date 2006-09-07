/*  XMMS - Cross-platform multimedia player
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "coreaudio.h"
#include <errno.h>
#include <CoreAudio/CoreAudio.h>

extern AudioDeviceID device_id;
extern gboolean playing_flag;
extern float left_volume, right_volume;

void osx_get_volume(int *l, int *r)
{
	*l = left_volume * 100;
	*r = right_volume * 100;

#if 0
	float  volume;
	UInt32 size;
	AudioDeviceID temp_device_id;

	size = sizeof(float);

	AudioDeviceGetProperty(device_id,1,0,kAudioDevicePropertyVolumeScalar,&size,&volume);

	volume = volume * 100;

	*r = volume;
	*l = volume;
#endif
}


void osx_set_volume(int l, int r)
{
	left_volume = (float)l / 100.0;
	right_volume = (float) r / 100.0;


#if 0
	int fd, v, cmd, devs;
	gchar *devname;

	Boolean writeable_flag; 

	if (AudioDeviceGetPropertyInfo(device_id,1,false,kAudioDevicePropertyVolumeScalar,NULL,&writeable_flag))
	{
		printf("could not get property info for volume write\n");
	}
	else 
	{
		if (writeable_flag)
		{
			float  volume;

			volume = l / 100.0;
			AudioDeviceSetProperty(device_id,NULL,1,0,kAudioDevicePropertyVolumeScalar,sizeof(float),&volume);
		}
		else
		{
			printf("volume property is not writeable\n");
		}
	}
#endif
}


