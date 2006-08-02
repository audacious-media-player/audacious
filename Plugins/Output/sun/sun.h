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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "audioio.h"

#include "audacious/plugin.h"
#include "libaudacious/configdb.h"

/* Default path to audio device. */
#ifndef SUN_DEV_AUDIO
#define SUN_DEV_AUDIO "/dev/audio"
#endif

/* Default path to audioctl device */
#ifndef SUN_DEV_AUDIOCTL
#define SUN_DEV_AUDIOCTL "/dev/audioctl"
#endif

/* Default path to mixer device */
#ifndef SUN_DEV_MIXER
#define SUN_DEV_MIXER "/dev/mixer"
#endif

/* Default mixer device to control */
#ifndef SUN_DEFAULT_VOLUME_DEV
#define SUN_DEFAULT_VOLUME_DEV "dac"
#endif

/* Default hardware block size */
#ifndef SUN_DEFAULT_BLOCKSIZE
#define SUN_DEFAULT_BLOCKSIZE 8800
#endif

/* Default `requested' buffer size */
#ifndef SUN_DEFAULT_BUFFER_SIZE
#define SUN_DEFAULT_BUFFER_SIZE 8800
#endif

/* Minimum total buffer size */
#ifndef SUN_MIN_BUFFER_SIZE
#define SUN_MIN_BUFFER_SIZE 14336
#endif

/* Default prebuffering (%) */
#ifndef SUN_DEFAULT_PREBUFFER_SIZE
#define SUN_DEFAULT_PREBUFFER_SIZE 25
#endif

#define SUN_VERSION "0.6"

struct sun_format {
	char	name[16];
	union {
		AFormat	xmms;
		gint	sun;
	} format;
	int	frequency;
	int	channels;
	int	bps;
};

struct sun_audio {
	struct	sun_format *input;
	struct	sun_format *output;
	struct	sun_format *effect;

	gchar	*devaudio;
	gchar	*devaudioctl;
	gchar	*devmixer;
	gchar	*mixer_voldev;

	gint	fd;
	gint	mixerfd;

	gboolean mixer_keepopen;

	gboolean going;
	gboolean paused;
	gboolean unpause, do_pause;

	gint	req_prebuffer_size;
	gint	req_buffer_size;

	pthread_mutex_t	mixer_mutex;
};

struct sun_statsframe {
	int	fd;
	int	active;

	GtkWidget	*mode_label;
	GtkWidget	*blocksize_label;
	GtkWidget	*ooffs_label;
	pthread_mutex_t	audioctl_mutex;
	pthread_mutex_t	active_mutex;
};

extern	OutputPlugin	op;

extern struct sun_audio		audio;
extern struct sun_statsframe	stats_frame;

void	 sun_init(void);
void	 sun_about(void);
void	 sun_configure(void);
void	 sun_cleanup(void);

gint	 sun_open(AFormat, int, int);
void	 sun_write(void *, int);
void	 sun_close(void);
void	 sun_flush(int);
void	 sun_pause(short);
gint	 sun_free(void);
gint	 sun_playing(void);
gint	 sun_output_time(void);
gint	 sun_written_time(void);
void	 sun_get_volume(int *, int *);
void	 sun_set_volume(int, int);
void	*sun_get_convert_buffer(size_t);
int	(*sun_get_convert_func(int, int))(void **, int);

#ifdef WORDS_BIGENDIAN
#define IS_BIG_ENDIAN TRUE
#else
#define IS_BIG_ENDIAN FALSE
#endif

