/*

    libTiMidity -- MIDI to WAVE converter library
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>
    Copyright (C) 2004 Konstantin Korikov <lostclus@ua.fm>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef TIMIDITY_H
#define TIMIDITY_H

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define LIBTIMIDITY_VERSION_MAJOR 0L
#define LIBTIMIDITY_VERSION_MINOR 1L
#define LIBTIMIDITY_PATCHLEVEL    0L

#define LIBTIMIDITY_VERSION \
        ((LIBTIMIDITY_VERSION_MAJOR<<16)| \
         (LIBTIMIDITY_VERSION_MINOR<< 8)| \
         (LIBTIMIDITY_PATCHLEVEL))

/* Audio format flags (defaults to LSB byte order)
 */
#define MID_AUDIO_U8        0x0008	/* Unsigned 8-bit samples */
#define MID_AUDIO_S8        0x8008	/* Signed 8-bit samples */
#define MID_AUDIO_U16LSB    0x0010	/* Unsigned 16-bit samples */
#define MID_AUDIO_S16LSB    0x8010	/* Signed 16-bit samples */
#define MID_AUDIO_U16MSB    0x1010	/* As above, but big-endian byte order */
#define MID_AUDIO_S16MSB    0x9010	/* As above, but big-endian byte order */
#define MID_AUDIO_U16       MID_AUDIO_U16LSB
#define MID_AUDIO_S16       MID_AUDIO_S16LSB

/* Core Library Types
 */
  typedef unsigned char uint8;
  typedef signed char sint8;
  typedef unsigned short uint16;
  typedef signed short sint16;
  typedef unsigned int uint32;
  typedef signed int sint32;

  typedef size_t (*MidIStreamReadFunc) (void *ctx, void *ptr, size_t size,
					size_t nmemb);
  typedef int (*MidIStreamCloseFunc) (void *ctx);

  typedef struct _MidIStream MidIStream;
  typedef struct _MidDLSPatches MidDLSPatches;
  typedef struct _MidSong MidSong;

  typedef struct _MidSongOptions MidSongOptions;
  struct _MidSongOptions
  {
    sint32 rate;	  /* DSP frequency -- samples per second */
    uint16 format;	  /* Audio data format */
    uint8 channels;	  /* Number of channels: 1 mono, 2 stereo */
    uint16 buffer_size;	  /* Sample buffer size in samples */
  };

  typedef enum
  {
    MID_SONG_TEXT = 0,
    MID_SONG_COPYRIGHT = 1
  } MidSongMetaId;


/* Core Library Functions
 * ======================
 */

/* Initialize the library. If config_file is NULL
 * search for configuratin file in default directories
 */
  extern int mid_init (char *config_file);

/* Initialize the library without reading any
 * configuratin file
 */
  extern int mid_init_no_config (void);

/* Shutdown the library
 */
  extern void mid_exit (void);


/* Input Stream Functions
 * ======================
 */

/* Create input stream from a file name
 */
  extern MidIStream *mid_istream_open_file (const char *file);

/* Create input stream from a file pointer
 */
  extern MidIStream *mid_istream_open_fp (FILE * fp, int autoclose);

/* Create input stream from memory
 */
  extern MidIStream *mid_istream_open_mem (void *mem, size_t size,
					   int autofree);

/* Create custom input stream
 */
  extern MidIStream *mid_istream_open_callbacks (MidIStreamReadFunc read,
						 MidIStreamCloseFunc close,
						 void *context);

/* Read data from input stream
 */
  extern size_t mid_istream_read (MidIStream * stream, void *ptr, size_t size,
				  size_t nmemb);

/* Skip data from input stream
 */
  extern void mid_istream_skip (MidIStream * stream, size_t len);

/* Close and destroy input stream
 */
  extern int mid_istream_close (MidIStream * stream);


/* DLS Pathes Functions
 * ====================
 */

/* Load DLS patches
 */
  extern MidDLSPatches *mid_dlspatches_load (MidIStream * stream);

/* Destroy DLS patches
 */
  extern void mid_dlspatches_free (MidDLSPatches * patches);


/* MIDI Song Functions
 * ===================
 */

/* Load MIDI song
 */
  extern MidSong *mid_song_load (MidIStream * stream,
				 MidSongOptions * options);

/* Load MIDI song with specified DLS pathes
 */
  extern MidSong *mid_song_load_dls (MidIStream * stream,
				     MidDLSPatches * patches,
				     MidSongOptions * options);

/* Set song amplification value
 */
  extern void mid_song_set_volume (MidSong * song, int volume);

/* Seek song to the start position and initialize conversion
 */
  extern void mid_song_start (MidSong * song);

/* Read WAVE data
 */
  extern size_t mid_song_read_wave (MidSong * song, void *ptr, size_t size);

/* Seek song to specified offset in millseconds
 */
  extern void mid_song_seek (MidSong * song, uint32 ms);

/* Get total song time in millseconds
 */
  extern uint32 mid_song_get_total_time (MidSong * song);

/* Get current song time in millseconds
 */
  extern uint32 mid_song_get_time (MidSong * song);

/* Get song meta data. Return NULL if no meta data found
 */
  extern char *mid_song_get_meta (MidSong * song, MidSongMetaId what);

/* Destroy song
 */
  extern void mid_song_free (MidSong * song);

#ifdef __cplusplus
}
#endif
#endif				/* TIMIDITY_H */
