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
#ifndef OSX_H
#define OSX_H

#include "config.h"

#include <gtk/gtk.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#elif defined(HAVE_MACHINE_SOUNDCARD_H)
#include <machine/soundcard.h>
#endif

#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "audacious/plugin.h"

#ifdef WORDS_BIGENDIAN
# define IS_BIG_ENDIAN TRUE
#else
# define IS_BIG_ENDIAN FALSE
#endif

#define OUTPUT_BUFSIZE (4096 * 8)

extern OutputPlugin op;


struct format_info 
{
	union 
	{
		AFormat xmms;
		int osx;
	} format;


	int frequency;
	int channels;
	int bps;
};


typedef struct
{
	gint audio_device;
	gint mixer_device;
	gint buffer_size;
	gint prebuffer;
	gboolean use_master;
	gboolean use_alt_audio_device, use_alt_mixer_device;
	gchar *alt_audio_device, *alt_mixer_device;
}
OSXConfig;

extern OSXConfig osx_cfg;

void osx_init(void);
void osx_about(void);
void osx_configure(void);

void osx_get_volume(int *l, int *r);
void osx_set_volume(int l, int r);

int osx_playing(void);
int osx_free(void);
void osx_write(void *ptr, int length);
void osx_close(void);
void osx_flush(int time);
void osx_pause(short p);
int osx_open(AFormat fmt, int rate, int nch);
int osx_get_output_time(void);
int osx_get_written_time(void);
void osx_set_audio_params(void);

void* osx_get_convert_buffer(size_t size);
int (*osx_get_convert_func(int output, int input))(void **, int);

#endif
