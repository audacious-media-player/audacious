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
#include "include/vorbis.h"
#include "include/endian.h"
#include "../fmt.h"
#include "../config.h"
#include "include/unicode.h"
#define BUFFER_SIZE 4096

static vorbis_t *readComments(VFSFile *fp)
{
	vorbis_t *comments = calloc(sizeof(vorbis_t), 1);
	unsigned char cToInt[4];
	int i, lines, j = 0;
	
	vfs_fread(cToInt, 1, 4, fp);
	comments->vendorlen = le2int(cToInt);
	comments->vendor = malloc(comments->vendorlen);
	vfs_fread(comments->vendor, 1, comments->vendorlen, fp);
	vfs_fread(cToInt, 1, 4, fp);
	lines = comments->numitems = le2int(cToInt);
	comments->items = realloc(comments->items,
		(comments->numitems) * sizeof(vorbisfielddata_t *));
	for(i = 0; i < lines; i++)
	{
		unsigned char *data, *dp;
		vorbisfielddata_t *fielddata =
			calloc(sizeof(vorbisfielddata_t), 1);
		
		vfs_fread(cToInt, 1, 4, fp);
		fielddata->len = le2int(cToInt);
		data = malloc(fielddata->len);
		vfs_fread(data, 1, fielddata->len, fp);
		dp = strchr(data, '=');
		if(dp == NULL)
		{
			pdebug("No '=' in comment!", META_DEBUG);
			comments->numitems--;
			free(data);
			continue;
		}
		*dp = '\0';
		dp++;
		fielddata->name = malloc(strlen(data) + 1);
		fielddata->data = malloc(fielddata->len - strlen(data));
		*(fielddata->data + fielddata->len - strlen(data) - 1) = '\0';
		strcpy(fielddata->name, data);
		strncpy(fielddata->data, dp, fielddata->len - strlen(data) - 1);
		
		comments->items[j++] = fielddata;

		free(data);
	}
	
	return comments;
}

int findVorbis(VFSFile *fp)
{
	char tag_id[5] = "";
	unsigned char *tag_buffer, *bp, vorbis_type;
	int status = 0, pos = -1;
	
	vfs_fread(tag_id, 1, 4, fp);
	if(strcmp(tag_id, "OggS"))
		return -1;
	tag_buffer = malloc(23);
	vfs_fread(tag_buffer, 1, 23, fp);
	bp = tag_buffer;
	while(status == 0)
	{
		unsigned char segments, *lacing;
		unsigned int pagelen = 0, i;
		bp += 22;
		segments = *bp;
		lacing = malloc(segments);
		vfs_fread(lacing, 1, segments, fp);
		for(i = 0; i < segments; i++)
			pagelen += lacing[i];
		tag_buffer = realloc(tag_buffer, pagelen);
		vfs_fread(tag_buffer, 1, pagelen, fp);
		bp = tag_buffer;
		for(i = 0; i < segments && status == 0;)
		{
			if(strncmp(bp + 1, "vorbis", 6) == 0)
			{
				vorbis_type = *bp;
				if(vorbis_type == 0x03)
				{
					pos = ftell(fp) - pagelen +
						(bp - tag_buffer);
					status = 1;
				}
			}
			bp += lacing[i++];
		}
		if(status == 1 || feof(fp))
		{
			free(lacing);
			break;
		}
		tag_buffer = realloc(tag_buffer, 27);
		vfs_fread(tag_buffer, 1, 27, fp);
		bp = tag_buffer + 4;
		free(lacing);
	}

	free(tag_buffer);
	
	if(feof(fp))
		return -1;
	else
		return pos;
}

int findFlac(VFSFile *fp)
{
	unsigned char tag_id[5] = "";
	int pos;
	
	vfs_fread(tag_id, 1, 4, fp);
	if(strcmp(tag_id, "fLaC"))
		return 0;
	while(1)
	{
		vfs_fread(tag_id, 1, 4, fp);
		if((tag_id[0] & 0x7F) == 4)
			return 1;
		else if((tag_id[0] & 0x80) == 0x80)
			return 0;
		else if(feof(fp))
			return 0;
		else
		{
			pos = flac2int(tag_id);
			vfs_fseek(fp, pos, SEEK_CUR);
		}
	}
}

int findOggFlac(VFSFile *fp)
{
	char tag_id[5] = "";
	unsigned char *tag_buffer, *bp;
	int status = 0, pos = -1;
	
	vfs_fread(tag_id, 1, 4, fp);
	if(strcmp(tag_id, "OggS"))
		return -1;
	/* I assume first page always has only "fLaC" */
	tag_buffer = malloc(28);
	vfs_fread(tag_buffer, 1, 28, fp);
	bp = tag_buffer + 24;
	if(strncmp(bp, "fLaC", 4))
	{
		free(tag_buffer);
		return -1;
	}
	tag_buffer = realloc(tag_buffer, 27);
	vfs_fread(tag_buffer, 1, 27, fp);
	bp = tag_buffer + 4;
	while(status == 0)
	{
		unsigned char segments, *lacing = NULL;
		unsigned int pagelen = 0, i;
		bp += 22;
		segments = *bp;
		lacing = realloc(lacing, segments);
		vfs_fread(lacing, 1, segments, fp);
		for(i = 0; i < segments; i++)
			pagelen += lacing[i];
		tag_buffer = realloc(tag_buffer, pagelen);
		vfs_fread(tag_buffer, 1, pagelen, fp);
		bp = tag_buffer;
		for(i = 0; i < segments && status == 0;)
		{
			if((bp[0] & 0x7F) == 4)
			{
				pos = ftell(fp) - pagelen +
					(bp - tag_buffer);
				status = 1;
			}
			else if((tag_id[0] & 0x80) == 0x80)
			{
				free(tag_buffer);
				free(lacing);
				return -1;
			}
			else
				bp += lacing[i++];
		}
		if(status == 1 || feof(fp))
			break;
		tag_buffer = realloc(tag_buffer, 27);
		vfs_fread(tag_buffer, 1, 27, fp);
		bp = tag_buffer + 4;
		free(lacing);
	}
	
	free(tag_buffer);
	
	if(feof(fp))
		return -1;
	else
		return pos;
}

int findSpeex(VFSFile *fp)
{
	char tag_id[5] = "";
	unsigned char *tag_buffer, *bp, segments, *lacing = NULL;
	unsigned int pagelen = 0, i;
	int pos = -1;
	
	vfs_fread(tag_id, 1, 4, fp);
	if(strcmp(tag_id, "OggS"))
		return -1;
	tag_buffer = malloc(23);
	vfs_fread(tag_buffer, 1, 23, fp);
	bp = tag_buffer + 22;
	segments = *bp;
	lacing = malloc(segments);
	vfs_fread(lacing, 1, segments, fp);
	for(i = 0; i < segments; i++)
		pagelen += lacing[i];
	tag_buffer = realloc(tag_buffer, pagelen);
	vfs_fread(tag_buffer, 1, pagelen, fp);
	bp = tag_buffer;
	if(strncmp(bp, "Speex   ", 8))
	{
		free(lacing);
		free(tag_buffer);
		return -1;
	}
	tag_buffer = realloc(tag_buffer, 27);
	vfs_fread(tag_buffer, 1, 27, fp);
	bp = tag_buffer + 26;
	segments = *bp;
	lacing = realloc(lacing, segments);
	vfs_fread(lacing, 1, segments, fp);
	pos = ftell(fp);
	
	free(tag_buffer);
	free(lacing);
	
	if(feof(fp))
		return -1;
	else
		return pos;
}

vorbis_t *readVorbis(char *filename)
{
	VFSFile *fp;
	vorbis_t *comments;
	int pos;
	
	fp = vfs_fopen(filename, "r");

	if(!fp)
	{
		pdebug("Couldn't open file!", META_DEBUG);
		return NULL;
	}
	
	vfs_fseek(fp, 0, SEEK_SET);
	
	pdebug("Searching for tag...", META_DEBUG);
	pos = findVorbis(fp);
	if(pos < 0)
	{
		vfs_fclose(fp);
		return NULL;
	}
	vfs_fseek(fp, pos + 7, SEEK_SET);
	comments = readComments(fp);
	
	vfs_fclose(fp);
	
	return comments;
}

vorbis_t *readFlac(char *filename)
{
	VFSFile *fp;
	vorbis_t *comments;
	int status;
	
	fp = vfs_fopen(filename, "r");

	if(!fp)
	{
		pdebug("Couldn't open file!", META_DEBUG);
		return NULL;
	}
	
	vfs_fseek(fp, 0, SEEK_SET);
	
	pdebug("Searching for tag...", META_DEBUG);
	status = findFlac(fp);
	if(!status)
	{
		vfs_fclose(fp);
		return NULL;
	}
	comments = readComments(fp);
	
	vfs_fclose(fp);
	
	return comments;
}

vorbis_t *readOggFlac(char *filename)
{
	VFSFile *fp;
	vorbis_t *comments;
	int pos;
	
	fp = vfs_fopen(filename, "r");

	if(!fp)
	{
		pdebug("Couldn't open file!", META_DEBUG);
		return NULL;
	}
	
	vfs_fseek(fp, 0, SEEK_SET);
	
	pdebug("Searching for tag...", META_DEBUG);
	pos = findOggFlac(fp);
	if(pos < 0)
	{
		vfs_fclose(fp);
		return NULL;
	}
	vfs_fseek(fp, pos + 4, SEEK_SET);
	comments = readComments(fp);
	
	vfs_fclose(fp);
	
	return comments;
}

vorbis_t *readSpeex(char *filename)
{
	VFSFile *fp;
	vorbis_t *comments;
	int pos;
	
	fp = vfs_fopen(filename, "r");

	if(!fp)
	{
		pdebug("Couldn't open file!", META_DEBUG);
		return NULL;
	}
	
	vfs_fseek(fp, 0, SEEK_SET);
	
	pdebug("Searching for tag...", META_DEBUG);
	pos = findSpeex(fp);
	if(pos < 0)
	{
		vfs_fclose(fp);
		return NULL;
	}
	vfs_fseek(fp, pos, SEEK_SET);
	comments = readComments(fp);
	
	vfs_fclose(fp);
	
	return comments;
}

void freeVorbis(vorbis_t *comments)
{
	int i;
	
	for(i = 0; i < comments->numitems; i++)
	{
		vorbisfielddata_t *field;
		
		field = comments->items[i];
		free(field->data);
		free(field->name);
		free(field);
	}
	free(comments->items);
	free(comments->vendor);
	free(comments);
}
