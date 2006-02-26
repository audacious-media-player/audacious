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
#include "include/ape.h"
#include "include/endian.h"
#include "../fmt.h"
#include "../config.h"
#include "include/unicode.h"
#define BUFFER_SIZE 4096

static ape_t *readItems(VFSFile *fp, int version)
{
	ape_t *ape = calloc(sizeof(ape_t), 1);
	unsigned char *tag_buffer = NULL, *bp, cToInt[4];
	int size, start;
	unsigned int i;
	
	ape->version = version;

	/*
	 * Now for the actual reading.
	 * APE2 and APE1 tags are identical for all the structure we care about.
	 */

	vfs_fread(cToInt, 1, 4, fp);
	size = le2int(cToInt);
	vfs_fread(cToInt, 1, 4, fp);
	ape->numitems = le2int(cToInt);
	/* pdebug(fmt_vastr("tag size: %d", size));
	pdebug(fmt_vastr("items: %d", ape->numitems)); */
	vfs_fread(cToInt, 1, 4, fp);
	/* Is it a footer?  Header? */
	if((cToInt[3] & 0x20) == 0x0 || version == 1000)
		start = 8 - size;
	else
		start = 8;
	vfs_fseek(fp, start, SEEK_CUR);

	tag_buffer = realloc(tag_buffer, size);
	vfs_fread(tag_buffer, 1, size, fp);
	bp = tag_buffer;
	
	ape->items = realloc(ape->items,
			(ape->numitems) * sizeof(apefielddata_t *));
	
	for(i = 0; i < ape->numitems && strncmp((char*)bp, "APETAGEX", 8) != 0; i++)
	{
		apefielddata_t *field = calloc(sizeof(apefielddata_t), 1);
		
		memcpy(cToInt, bp, 4);		
		bp += 8;		
		field->len = le2int(cToInt);
		field->name = (unsigned char*)strdup((char*)bp);
		bp += strlen((char*)bp) + 1;
		field->data = malloc(field->len + 1);
		memcpy(field->data, bp, field->len);
		*(field->data + field->len) = '\0';
		bp += field->len;
		
		ape->items[i] = field;		
	}
	if(i < ape->numitems && strncmp((char*)bp, "APETAGEX", 8) == 0)
	{
		ape->numitems = i;
		ape->items = realloc(ape->items,
			(ape->numitems) * sizeof(apefielddata_t *));
	}
		

	free(tag_buffer);
	
	return ape;
}

int findAPE(VFSFile *fp)
{
	unsigned char *tag_buffer, *bp, cToInt[4];
	int pb = 0, status = 0, pos = 0, i, ape_version;
	
	/* Find the tag header or footer and point the file pointer there. */
	tag_buffer = malloc(BUFFER_SIZE);
	pb += vfs_fread(tag_buffer, 1, BUFFER_SIZE, fp);
	bp = tag_buffer;
	while(status == 0)
	{
		for(i = 0; i < BUFFER_SIZE - 8 && status == 0; i++)
		{
			bp++;
			if(!strncmp((char*)bp, "APETAGEX", 8))
				status = 1;
		}
		if(status == 1 || feof(fp))
			break;
		memmove(tag_buffer, tag_buffer + BUFFER_SIZE - 7, 7);
		pos += BUFFER_SIZE - 7;
		pb += vfs_fread(tag_buffer + 7, 1, BUFFER_SIZE - 7, fp);
		bp = tag_buffer;
	}
	if(status == 1)
	{
		vfs_fseek(fp, pos + (bp - tag_buffer) + 8, SEEK_SET);

		free(tag_buffer);
		
		/* Get the tag version. */
		vfs_fread(cToInt, 1, 4, fp);
		ape_version = le2int(cToInt);
		pdebug(ape_version == 1000 ? "Found APE1 tag..." :
			ape_version == 2000 ? "Found APE2 tag..." :
			"Found unknown APE tag...", META_DEBUG);
		
		return ape_version;
	}
	else
	{
		free(tag_buffer);
		return 0;
	}
}

ape_t *readAPE(char *filename)
{
	VFSFile *fp;
	ape_t *ape;
	int status;
	
	fp = vfs_fopen(filename, "r");

	if(!fp)
	{
		pdebug("Couldn't open file!", META_DEBUG);
		return NULL;
	}
	
	vfs_fseek(fp, 0, SEEK_SET);
	
	pdebug("Searching for tag...", META_DEBUG);
	status = findAPE(fp);
	if(status == 0)
	{
		vfs_fclose(fp);
		return NULL;
	}
	ape = readItems(fp, status);
	
	vfs_fclose(fp);
	
	return ape;
}

void freeAPE(ape_t *ape)
{
	unsigned int i;
	
	for(i = 0; i < ape->numitems; i++)
	{
		apefielddata_t *field;
		
		field = ape->items[i];
		free(field->name);
		free(field->data);
		free(field);
	}
	free(ape->items);
	free(ape);
}
