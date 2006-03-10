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

#include "include/wma.h"
#include "include/endian.h"
#include "../fmt.h"
#include "../config.h"
#include "include/unicode.h"
#define WMA_GUID	(unsigned char [16]) \
		      {	0x30, 0x26, 0xB2, 0x75, \
			0x8E, 0x66, \
			0xCF, 0x11, \
			0xA6, 0xD9, \
			0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C }
#define CONTENT_GUID	(unsigned char [16]) \
		      {	0x33, 0x26, 0xB2, 0x75, \
			0x8E, 0x66, \
			0xCF, 0x11, \
			0xA6, 0xD9, \
			0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C }
#define EXTENDED_GUID	(unsigned char [16]) \
		      {	0x40, 0xA4, 0xD0, 0xD2, \
			0x07, 0xE3, \
			0xD2, 0x11, \
			0x97, 0xF0, \
			0x00, 0xA0, 0xC9, 0x5E, 0xA8, 0x50 }
#define BUFFER_SIZE 4096

static wma_t *readAttributes(VFSFile *fp, int pos)
{
	wma_t *wma = calloc(sizeof(wma_t), 1);
	attribute_t *attribute;
	unsigned char *tag_buffer = NULL, *bp, cToInt[8], *data = NULL;
	int title_size, author_size, copyright_size, desc_size, rating_size,
		size, primary_items;
	unsigned int i;

	vfs_fseek(fp, pos, SEEK_SET);
	vfs_fread(cToInt, 1, 8, fp);
	/* Yes, it's 64 bits in size, but I'm lazy and don't want to adjust. */
	size = le2int(cToInt);
	tag_buffer = malloc(size - 24);
	vfs_fread(tag_buffer, 1, size - 24, fp);
	bp = tag_buffer;
	title_size = le2short(bp);
	bp += 2;
	author_size = le2short(bp);
	bp += 2;
	copyright_size = le2short(bp);
	bp += 2;
	desc_size = le2short(bp);
	bp += 2;
	rating_size = le2short(bp);
	bp += 2;
	if(title_size > 0)
	{
		attribute = calloc(sizeof(attribute_t), 1);
		wma->items = realloc(wma->items,
				(wma->numitems + 1) * sizeof(attribute_t *));
		attribute->name = (unsigned char*)strdup("Title");
		data = malloc(title_size);
		memcpy(data, bp, title_size);
		bp += title_size;
		utf16le_to_utf8(data, title_size, &attribute->data);
		attribute->type = 0;
		wma->items[wma->numitems++] = attribute;
		free(data);
	}
	if(author_size > 0)
	{
		attribute = calloc(sizeof(attribute_t), 1);
		wma->items = realloc(wma->items,
				(wma->numitems + 1) * sizeof(attribute_t *));
		attribute->name = (unsigned char*)strdup("Author");
		data = malloc(author_size);
		memcpy(data, bp, author_size);
		bp += author_size;
		utf16le_to_utf8(data, author_size, &attribute->data);
		attribute->type = 0;
		wma->items[wma->numitems++] = attribute;
		free(data);
	}
	if(copyright_size > 0)
	{
		attribute = calloc(sizeof(attribute_t), 1);
		wma->items = realloc(wma->items,
				(wma->numitems + 1) * sizeof(attribute_t *));
		attribute->name = (unsigned char*)strdup("Copyright");
		data = malloc(copyright_size);
		memcpy(data, bp, copyright_size);
		bp += copyright_size;
		utf16le_to_utf8(data, copyright_size, &attribute->data);
		attribute->type = 0;
		wma->items[wma->numitems++] = attribute;
		free(data);
	}
	if(desc_size > 0)
	{
		attribute = calloc(sizeof(attribute_t), 1);
		wma->items = realloc(wma->items,
				(wma->numitems + 1) * sizeof(attribute_t *));
		attribute->name = (unsigned char*)strdup("Description");
		data = malloc(desc_size);
		memcpy(data, bp, desc_size);
		bp += desc_size;
		utf16le_to_utf8(data, desc_size, &attribute->data);
		attribute->type = 0;
		wma->items[wma->numitems++] = attribute;
		free(data);
	}
	if(rating_size > 0)
	{
		attribute = calloc(sizeof(attribute_t), 1);
		wma->items = realloc(wma->items,
				(wma->numitems + 1) * sizeof(attribute_t *));
		attribute->name = (unsigned char*)strdup("Rating");
		data = malloc(rating_size);
		memcpy(data, bp, rating_size);
		bp += rating_size;
		utf16le_to_utf8(data, desc_size, &attribute->data);
		attribute->type = 0;
		wma->items[wma->numitems++] = attribute;
		free(data);
	}
	primary_items = wma->numitems;

	vfs_fread(tag_buffer, 16, 1, fp);
	if(memcmp(tag_buffer, EXTENDED_GUID, 16))
	{
		free(tag_buffer);
		return wma;
	}
	vfs_fread(cToInt, 8, 1, fp);
	/*
	 * Another 64-bit breakage. If you've got that large an amount of
	 * metadata, you've got a problem.
	 */
	size = le2int(cToInt);
	tag_buffer = realloc(tag_buffer, size);
	vfs_fread(tag_buffer, size, 1, fp);
	bp = tag_buffer;
	memcpy(cToInt, bp, 2);
	wma->numitems += le2short(cToInt);
	wma->items = realloc(wma->items,
			wma->numitems * sizeof(attribute_t *));
	bp += 2;
	for(i = primary_items; i < wma->numitems; i++)
	{
		int type;

		attribute = calloc(sizeof(attribute_t), 1);

		memcpy(cToInt, bp, 2);
		size = le2short(cToInt);
		bp += 2;
		data = malloc(size);
		memcpy(data, bp, size);
		utf16le_to_utf8(data, size, &attribute->name);
		bp += size;
		memcpy(cToInt, bp, 2);
		type = le2short(cToInt);
		attribute->type = type;
		bp += 2;
		memcpy(cToInt, bp, 2);
		size = le2short(cToInt);
		bp += 2;
		data = realloc(data, size);
		memcpy(data, bp, size);
		switch(type)
		{
			/* Type 0 is Little-endian UTF16 */
			case 0:
				utf16le_to_utf8(data, size, &attribute->data);
				break;
			/*
			 * Type 1 is binary
			 * Type 2 is boolean
			 * Type 3 is 32-bit integer (signed?)
			 * Type 4 is double
			 */
			case 1:
			case 2:
			case 3:
			case 4:
			default:
				attribute->data = malloc(size);
				memcpy(attribute->data, data, size);
		}
		bp += size;
		
		wma->items[i] = attribute;		
	}

	free(tag_buffer);
	
	return wma;
}

int findWMA(VFSFile *fp)
{
	unsigned char *tag_buffer, *bp;
	
	tag_buffer = malloc(BUFFER_SIZE);
	vfs_fread(tag_buffer, 1, BUFFER_SIZE, fp);
	bp = tag_buffer;
	if(memcmp(bp, WMA_GUID, 16))
	{
		free(tag_buffer);
		return -1;
	}
	bp += 30;
	if(memcmp(bp, CONTENT_GUID, 16))
	{
		free(tag_buffer);
		return -1;
	}
	/*
	 * It's stupid to reject if no Extended Content GUID
	 * is found...  This code is in here in case though...
	 *
	bp += 16;
	memcpy(cToInt, bp, 8);
	size = le2long(cToInt);
	bp += size - 16;
	if(!memcmp(bp, EXTENDED_GUID, 16))
	{
		free(tag_buffer);
		return 0;
	}
	*/
	free(tag_buffer);
	return bp - tag_buffer + 16;
}

wma_t *readWMA(char *filename)
{
	VFSFile *fp;
	wma_t *wma;
	int status;
	
	fp = vfs_fopen(filename, "r");

	if(!fp)
	{
		pdebug("Couldn't open file!", META_DEBUG);
		return NULL;
	}
	
	vfs_fseek(fp, 0, SEEK_SET);
	
	pdebug("Searching for tag...", META_DEBUG);
	status = findWMA(fp);
	if(status == 0)
	{
		vfs_fclose(fp);
		return NULL;
	}
	wma = readAttributes(fp, status);
	
	vfs_fclose(fp);
	
	return wma;
}

void freeWMA(wma_t *wma)
{
	unsigned int i;
	
	for(i = 0; i < wma->numitems; i++)
	{
		attribute_t *attribute;
		
		attribute = wma->items[i];
		free(attribute->name);
		free(attribute->data);
		free(attribute);
	}
	free(wma->items);
	free(wma);
}
