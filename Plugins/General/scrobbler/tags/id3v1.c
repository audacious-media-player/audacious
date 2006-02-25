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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/bmp_vfs.h"
#include "include/id3v1.h"
#include "include/endian.h"
#include "../fmt.h"
#include "../config.h"
#include "include/unicode.h"
#define BUFFER_SIZE 4096

static void cleanID3v1(unsigned char *data, size_t size)
{
	int i;
	
	for(i = size - 1; i > -1; i--)
	{
		if(data[i] == ' ') data[i] = '\0';
		else break;
	}
}

static void cleanComment(unsigned char *data)
{
	int i;
	
	for(i = 27; i > -1; i--)
	{
		if(data[i] == ' ' || data[i] == '\0') data[i] = '\0';
		else break;
	}
}

int findID3v1(VFSFile *fp)
{
	char tag_id[4] = "";
	
	vfs_fread(tag_id, 1, 3, fp);
	
	if(!strncmp(tag_id, "TAG", 3))
		return 1;
	else
		return 0;
}

id3v1_t *readID3v1(char *filename)
{
	VFSFile *fp;
	id3v1_t *id3v1 = NULL;
	int status;
	
	fp = vfs_fopen(filename, "rb");

	if(!fp)
	{
		pdebug("Couldn't open file!", META_DEBUG);
		return NULL;
	}

	pdebug("Searching for tag...", META_DEBUG);
	vfs_fseek(fp, -128, SEEK_END);
	status = findID3v1(fp);
	if(status)
	{
		unsigned char *data;
		
		id3v1 = calloc(sizeof(id3v1_t), 1);
		data = malloc(31);
		*(data + 30) = '\0';
		vfs_fread(data, 1, 30, fp);
		cleanID3v1(data, 30);
		// id3v1->title = realloc(id3v1->title, 31);
		if(strcmp(data, ""))
			iso88591_to_utf8(data, strlen(data), &id3v1->title);
		else
			id3v1->title = NULL;
		vfs_fread(data, 1, 30, fp);
		cleanID3v1(data, 30);
		if(strcmp(data, ""))
			iso88591_to_utf8(data, strlen(data), &id3v1->artist);
		else
			id3v1->artist = NULL;
		vfs_fread(data, 1, 30, fp);
		cleanID3v1(data, 30);
		if(strcmp(data, ""))
			iso88591_to_utf8(data, strlen(data), &id3v1->album);
		else
			id3v1->album = NULL;
		data = realloc(data, 5);
		*(data + 4) = '\0';
		vfs_fread(data, 1, 4, fp);
		cleanID3v1(data, 4);
		if(strcmp(data, ""))
			iso88591_to_utf8(data, strlen(data), &id3v1->year);
		else
			id3v1->year = NULL;
		data = realloc(data, 31);
		*(data + 30) = '\0';
		vfs_fread(data, 1, 30, fp);
		cleanComment(data);
		id3v1->comment = realloc(id3v1->comment, 31);
		memset(id3v1->comment, 0, 31);
		memcpy(id3v1->comment, data, 30);
		if(data[28] == '\0' && data[29] != '\0')
			id3v1->track = data[29];
		else
			id3v1->track = 255;
		free(data);
		vfs_fread(&id3v1->genre, 1, 1, fp);
	}
	
	vfs_fclose(fp);
	
	return id3v1;
}

void freeID3v1(id3v1_t *id3v1)
{
	if(id3v1->title != NULL)
		free(id3v1->title);
	if(id3v1->artist != NULL)
		free(id3v1->artist);
	if(id3v1->album != NULL)
		free(id3v1->album);
	if(id3v1->year != NULL)
		free(id3v1->year);
	free(id3v1->comment);
	free(id3v1);
}
