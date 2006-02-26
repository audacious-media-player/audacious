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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/bmp_vfs.h"
#include "include/itunes.h"
#include "include/endian.h"
#include "../fmt.h"
#include "../config.h"
#include "include/unicode.h"
#define BUFFER_SIZE 4096
#define NAME_ATOM	((0xa9 << 24) | ('n' << 16) | ('a' << 8) | ('m' << 0))
#define ARTIST_ATOM	((0xa9 << 24) | ('A' << 16) | ('R' << 8) | ('T' << 0))
#define ALBUM_ATOM	((0xa9 << 24) | ('a' << 16) | ('l' << 8) | ('b' << 0))
#define YEAR_ATOM	((0xa9 << 24) | ('d' << 16) | ('a' << 8) | ('y' << 0))
#define GENRE_ATOM	(('g' << 24) | ('n' << 16) | ('r' << 8) | ('e' << 0))
#define TRACK_ATOM	(('t' << 24) | ('r' << 16) | ('k' << 8) | ('n' << 0))
#define DISC_ATOM	(('d' << 24) | ('i' << 16) | ('s' << 8) | ('k' << 0))
#define COPYRIGHT_ATOM	(('c' << 24) | ('p' << 16) | ('r' << 8) | ('t' << 0))

static itunes_t *readAtoms(VFSFile *fp)
{
	itunes_t *itunes = calloc(sizeof(itunes_t), 1);
	unsigned char *tag_buffer, *bp, cToInt[4];
	int size, meta_size;
	
	vfs_fread(cToInt, 1, 4, fp);
	size = be2int(cToInt) - 4;
	tag_buffer = malloc(size);
	vfs_fread(tag_buffer, 1, size, fp);
	bp = tag_buffer;
	bp += 8;
	while(bp - tag_buffer < size)
	{
		unsigned char **tag_data = NULL;
		switch(tagid2int(bp))
		{
			case NAME_ATOM:
				tag_data = &itunes->title;
				break;
			case ARTIST_ATOM:
				tag_data = &itunes->artist;
				break;
			case ALBUM_ATOM:
				tag_data = &itunes->album;
				break;
			/*
			 * Genre is weird.  I don't know how user input genres
			 * work. Use at your own risk. Is terminated with an
			 * extra 0.
			 */
			case GENRE_ATOM:
				tag_data = &itunes->genre;
				break;
			case YEAR_ATOM:
				tag_data = &itunes->year;
				break;
			case COPYRIGHT_ATOM:
				tag_data = &itunes->copyright;
				break;
		}
		if(	tagid2int(bp) == NAME_ATOM ||
			tagid2int(bp) == ARTIST_ATOM ||
			tagid2int(bp) == ALBUM_ATOM ||
			tagid2int(bp) == GENRE_ATOM ||
			tagid2int(bp) == YEAR_ATOM ||
			tagid2int(bp) == COPYRIGHT_ATOM)
		{
			bp += 4;
			memcpy(cToInt, bp, 4);
			meta_size = be2int(cToInt) - 16;
			bp += 16;
			*tag_data = realloc(*tag_data, meta_size + 1);
			*(*tag_data + meta_size) = '\0';
			strncpy((char*)*tag_data, (char*)bp, meta_size);
			bp += meta_size;
		}
		/*
		 * Track number is easier, but unverified.
		 * Disc number is similar.  Doesn't skip at end though.
		 *
		 * Assuming max of what a char can hold.
		 */
		else if(tagid2int(bp) == TRACK_ATOM ||
			tagid2int(bp) == DISC_ATOM)
		{
			unsigned char *tag_num_data, *tag_num_max_data, skip;
			if(tagid2int(bp) == TRACK_ATOM)
			{
				tag_num_data = &itunes->track;
				tag_num_max_data = &itunes->maxtrack;
				skip = 2;
			}
			else if(tagid2int(bp) == DISC_ATOM)
			{
				tag_num_data = &itunes->disc;
				tag_num_max_data = &itunes->maxdisc;
				skip = 0;
			}
			bp += 4;
			memcpy(cToInt, bp, 4);
			meta_size = be2int(cToInt) - 16;
			bp += 19;
			*tag_num_data = *bp;
			bp += 2;
			*tag_num_max_data = *bp;
			bp += 1 + skip;
		}
		/*
		 * TODO:
		 *
		 * I'd like to handle rtng (Rating) but I don't know how.
		 * What are the xxIDs? aART?
		 * Where is Grouping, BPM, Composer?
		 *
		 * In any case, do not handle anything else.
		 */
		else
		{
			bp -= 4;
			memcpy(cToInt, bp, 4);
			meta_size = be2int(cToInt);
			bp += meta_size;
		}
		bp += 4;
	}
	
	free(tag_buffer);
	
	return itunes;
}

int findiTunes(VFSFile *fp)
{
	unsigned char *tag_buffer, *bp, cToInt[4], *start_pos;
	int parent_size, atom_size, pos = 0;
	
	/*
	 * Find the ILST atom and point the file pointer there and return
	 * the atom size.
	 *
	 * Please note that this could easily not work (especially when not
	 * encoded with iTunes) as this is mainly based off of a reference
	 * file encoded by iTunes and the QTFileFormat documentation released
	 * by Apple.  It's not based off any official documentation of M4A.
	 *
	 * As a result of not being based off official documentation, this is
	 * EXTREMELY likely to return 0 (i.e. no data found).
	 *
	 * First we assume that ftyp is the first atom, and M4A is the type.
	 */
	vfs_fread(cToInt, 1, 4, fp);
	atom_size = be2int(cToInt) - 4;
	tag_buffer = malloc(8);
	vfs_fread(tag_buffer, 1, 8, fp);
	if(strncmp((char*)tag_buffer, "ftypM4A ", 8))
	{
		free(tag_buffer);
		return -1;
	}
	vfs_fseek(fp, -8, SEEK_CUR);
	tag_buffer = realloc(tag_buffer, atom_size);
	vfs_fread(tag_buffer, 1, atom_size, fp);
	/* Now keep skipping until we hit a moov container atom */
	while(!feof(fp))
	{
		vfs_fread(cToInt, 1, 4, fp);
		atom_size = be2int(cToInt) - 4;
		tag_buffer = realloc(tag_buffer, atom_size);
		pos = ftell(fp);
		vfs_fread(tag_buffer, 1, atom_size, fp);
		if(!strncmp((char*)tag_buffer, "moov", 4))
			break;
	}
	if(feof(fp))
	{
		free(tag_buffer);
		return -1;
	}
	parent_size = atom_size;
	/* Now looking for child udta atom (NOT in trak) */
	bp = tag_buffer + 4;
	while(bp - tag_buffer < parent_size)
	{
		memcpy(cToInt, bp, 4);
		atom_size = be2int(cToInt) - 4;
		bp += 4;
		if(!strncmp((char*)bp, "udta", 4))
			break;
		bp += atom_size;
	}
	if(strncmp((char*)bp, "udta", 4))
	{
		free(tag_buffer);
		return -1;
	}
	parent_size = atom_size;
	start_pos = bp;
	bp += 4;
	/* Now looking for child meta atom */
	while(bp - start_pos < parent_size)
	{
		memcpy(cToInt, bp, 4);
		atom_size = be2int(cToInt) - 4;
		bp += 4;
		if(!strncmp((char*)bp, "meta", 4))
			break;
		bp += atom_size;
	}
	if(strncmp((char*)bp, "meta", 4))
	{
		free(tag_buffer);
		return -1;
	}
	parent_size = atom_size;
	start_pos = bp;
	bp += 8;
	/* Now looking for child ilst atom */
	while(bp - start_pos < parent_size)
	{
		memcpy(cToInt, bp, 4);
		atom_size = be2int(cToInt) - 4;
		bp += 4;
		if(!strncmp((char*)bp, "ilst", 4))
			break;
		bp += atom_size;
	}
	if(strncmp((char*)bp, "ilst", 4))
	{
		free(tag_buffer);
		return -1;
	}
	bp -= 4;
	vfs_fseek(fp, bp - tag_buffer + pos, SEEK_SET);
	
	free(tag_buffer);
	
	return atom_size;
}

itunes_t *readiTunes(char *filename)
{
	VFSFile *fp;
	itunes_t *itunes;
	int status;
	
	fp = vfs_fopen(filename, "r");

	if(!fp)
	{
		pdebug("Couldn't open file!", META_DEBUG);
		return NULL;
	}
	
	vfs_fseek(fp, 0, SEEK_SET);
	
	pdebug("Searching for tag...", META_DEBUG);
	status = findiTunes(fp);
	if(status == -1)
	{
		vfs_fclose(fp);
		return NULL;
	}
	itunes = readAtoms(fp);
	
	vfs_fclose(fp);
	
	return itunes;
}

void freeiTunes(itunes_t *itunes)
{
	if(itunes->title != NULL)
		free(itunes->title);
	if(itunes->artist != NULL)
		free(itunes->artist);
	if(itunes->album != NULL)
		free(itunes->album);
	if(itunes->year != NULL)
		free(itunes->year);
	if(itunes->genre != NULL)
		free(itunes->genre);
	free(itunes);
}
