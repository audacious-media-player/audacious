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

#ifndef ID3V1_H
#define ID3V1_H 1

typedef struct
{
	unsigned char *title, *artist, *album, *year, *comment, track, genre;
} id3v1_t;

#ifndef MAKE_BMP
int findID3v1(VFSFile *);
#else
int findID3v1(VFSFile *);
#endif
id3v1_t *readID3v1(char *);
void freeID3v1(id3v1_t *);
#endif
