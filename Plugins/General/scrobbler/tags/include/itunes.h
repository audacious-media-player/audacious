/*
 *   libmetatag - A media file tag-reader library
 *   Copyright (C) 2004  Pipian
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

#ifndef ITUNES_H
#define ITUNES_H 1
typedef struct
{
	unsigned char	*title, *artist, *album, *genre, *year, *copyright,
			track, maxtrack, disc, maxdisc;
} itunes_t;

#ifndef MAKE_BMP
int findiTunes(VFSFile *);
#else
int findiTunes(VFSFile *);
#endif
itunes_t *readiTunes(char *);
void freeiTunes(itunes_t *);
#endif
