/*
 *   libmetatag - A media file tag-reader library
 *   Copyright (C) 2003, 2004  Pipian
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef VORBIS_H
#define VORBIS_H 1

#include <libaudacious/vfs.h>

#define READ_VORBIS 1
#define READ_FLAC 2
#define READ_OGGFLAC 3
#define READ_SPEEX 4

typedef struct
{
	unsigned int len;
	unsigned char *data, *name;
} vorbisfielddata_t;

typedef struct
{
	unsigned int numitems, vendorlen;
	unsigned char *vendor;
	vorbisfielddata_t **items;
} vorbis_t;

#ifndef MAKE_BMP
int findVorbis(VFSFile *);
int findFlac(VFSFile *);
int findOggFlac(VFSFile *);
int findSpeex(VFSFile *);
#else
int findVorbis(VFSFile *);
int findFlac(VFSFile *);
int findOggFlac(VFSFile *);
int findSpeex(VFSFile *);
#endif
vorbis_t *readVorbis(char *);
vorbis_t *readFlac(char *);
vorbis_t *readOggFlac(char *);
vorbis_t *readSpeex(char *);
void freeVorbis(vorbis_t *);
#endif
