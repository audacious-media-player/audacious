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

#ifndef METATAGS_H
#define METATAGS_H 1

#include <libaudacious/vfs.h>

#include "wma.h"
#include "id3v2.h"
#include "id3v1.h"
#include "vorbis.h"
#include "itunes.h"
#include "ape.h"
#include "cdaudio.h"

extern const char *genre_list[148];

/*
 * Note: This struct has some signs to determine what tags a file has
 * and it is left to the program to interpret them. The main unsigned chars
 * are merely used as references directly to something in one of the substructs.
 *
 */

typedef struct {
	unsigned char	*artist,
			*title,
			*mb,
			*album,
			*year,
			*track,
			*genre;
	int		has_wma,
			has_id3v1,
			has_id3v2,
			has_ape,
			has_vorbis,
			has_flac,
			has_oggflac,
			has_speex,
			has_itunes,
			has_cdaudio,
			prefer_ape;
	wma_t		*wma;
	id3v1_t		*id3v1;
	id3v2_t		*id3v2;
	ape_t		*ape;
	vorbis_t	*vorbis,
			*flac,
			*oggflac,
			*speex;
	itunes_t	*itunes;
	cdaudio_t	*cdaudio;
} metatag_t;

void get_tag_data(metatag_t *, char *, int);
metatag_t *metatag_new(void);
void metatag_delete(metatag_t *);
#endif
