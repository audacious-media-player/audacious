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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
 */
#ifndef WAV_H
#define WAV_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>

#include <sys/types.h>

#include <libaudacious/vfs.h>
#include "audacious/plugin.h"

#define	WAVE_FORMAT_UNKNOWN		(0x0000)
#define	WAVE_FORMAT_PCM			(0x0001)
#define	WAVE_FORMAT_ADPCM		(0x0002)
#define	WAVE_FORMAT_ALAW		(0x0006)
#define	WAVE_FORMAT_MULAW		(0x0007)
#define	WAVE_FORMAT_OKI_ADPCM		(0x0010)
#define	WAVE_FORMAT_DIGISTD		(0x0015)
#define	WAVE_FORMAT_DIGIFIX		(0x0016)
#define	IBM_FORMAT_MULAW         	(0x0101)
#define	IBM_FORMAT_ALAW			(0x0102)
#define	IBM_FORMAT_ADPCM         	(0x0103)

extern InputPlugin wav_ip;

typedef struct {
    VFSFile *file;
    short format_tag, channels, block_align, bits_per_sample, eof;
    long samples_per_sec, avg_bytes_per_sec;
    unsigned long position, length;
    int seek_to, data_offset, going;
    pid_t pid;
} WaveFile;

static void wav_init(void);
static int is_our_file(char *filename);
static void play_file(char *filename);
static void stop(void);
static void seek(int time);
static void wav_pause(short p);
static int get_time(void);
static void get_song_info(char *filename, char **title, int *length);

#endif
