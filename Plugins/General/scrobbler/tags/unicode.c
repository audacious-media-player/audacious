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

#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include "include/endian.h"
#include "include/unicode.h"
#include "audacious/util.h"

wchar_t *utf8_to_wchar(unsigned char *utf, size_t memsize)
{
	size_t i;
	int j = 0;
	wchar_t *mem;

	if (utf == NULL)
		return NULL;

	mem = calloc(sizeof(wchar_t) * (memsize + 1), 1);

	for(i = 0; i < memsize;)
	{
		if(utf[i] < 0x80)
			mem[j++] = utf[i++];
		else if(utf[i] < 0xE0)
		{
			mem[j++] = ((utf[i] & 0x1F) << 6) |
				(utf[i + 1] & 0x3F);
			i += 2;
		}
		else if(utf[i] < 0xF0)
		{
			mem[j++] = ((utf[i] & 0x0F) << 12) |
				((utf[i + 1] & 0x3F) << 6) |
				(utf[i + 2] & 0x3F);
			i += 3;
		}
		else if(utf[i] < 0xF8)
		{
			mem[j++] = ((utf[i] & 0x07) << 18) |
				((utf[i + 1] & 0x3F) << 12) |
				((utf[i + 2] & 0x3F) << 6) |
				(utf[i + 2] & 0x3F);
			i += 4;
		}
		else if(utf[i] < 0xFC)
		{
			mem[j++] = ((utf[i] & 0x03) << 24) |
				((utf[i + 1] & 0x3F) << 18) |
				((utf[i + 2] & 0x3F) << 12) |
				((utf[i + 3] & 0x3F) << 6) |
				(utf[i + 4] & 0x3F);
			i += 5;
		}
		else if(utf[i] >= 0xFC)
		{
			mem[j++] = ((utf[i] & 0x01) << 30) |
				((utf[i + 1] & 0x3F) << 24) |
				((utf[i + 2] & 0x3F) << 18) |
				((utf[i + 3] & 0x3F) << 12) |
				((utf[i + 4] & 0x3F) << 6) |
				(utf[i + 5] & 0x3F);
			i += 6;
		}
	}

	mem = realloc(mem, sizeof(wchar_t) * (j + 1));

	return mem;
}

unsigned char *wchar_to_utf8(wchar_t *wchar, size_t memsize)
{
	size_t i;
	unsigned char *mem, *ptr;

	if (wchar == NULL)
		return NULL;
	
	mem = calloc(memsize * 6 + 1, 1);
	ptr = mem;
	
	for(i = 0; i < memsize; i++)
	{
		if(wchar[i] < 0x80)
		{
			*ptr++ = wchar[i] & 0x7F;
		}
		else if(wchar[i] < 0x800)
		{
			*ptr++ = 0xC0 | ((wchar[i] >> 6) & 0x1F);
			*ptr++ = 0x80 | (wchar[i] & 0x3F);
		}
		else if(wchar[i] < 0x10000)
		{
			*ptr++ = 0xE0 | ((wchar[i] >> 12) & 0x0F);
			*ptr++ = 0x80 | ((wchar[i] >> 6) & 0x3F);
			*ptr++ = 0x80 | (wchar[i] & 0x3F);
		}
		else if(wchar[i] < 0x200000)
		{
			*ptr++ = 0xF0 | ((wchar[i] >> 18) & 0x07);
			*ptr++ = 0x80 | ((wchar[i] >> 12) & 0x3F);
			*ptr++ = 0x80 | ((wchar[i] >> 6) & 0x3F);
			*ptr++ = 0x80 | (wchar[i] & 0x3F);
		}
		else if(wchar[i] < 0x4000000)
		{
			*ptr++ = 0xF8 | ((wchar[i] >> 24) & 0x03);
			*ptr++ = 0x80 | ((wchar[i] >> 18) & 0x3F);
			*ptr++ = 0x80 | ((wchar[i] >> 12) & 0x3F);
			*ptr++ = 0x80 | ((wchar[i] >> 6) & 0x3F);
			*ptr++ = 0x80 | (wchar[i] & 0x3F);
		}
		else if((unsigned long)wchar[i] < 0x80000000)
		{
			*ptr++ = 0xFC | ((wchar[i] >> 30) & 0x01);
			*ptr++ = 0x80 | ((wchar[i] >> 24) & 0x3F);
			*ptr++ = 0x80 | ((wchar[i] >> 18) & 0x3F);
			*ptr++ = 0x80 | ((wchar[i] >> 12) & 0x3F);
			*ptr++ = 0x80 | ((wchar[i] >> 6) & 0x3F);
			*ptr++ = 0x80 | (wchar[i] & 0x3F);
		}
	}
	
	mem = realloc(mem, ptr - mem + 1);
	
	return mem;
}

void iso88591_to_utf8(unsigned char *iso, size_t memsize,
				unsigned char **utf)
{
	*utf = str_to_utf8(iso);
}

#if 0
void iso88591_to_utf8(unsigned char *iso, size_t memsize,
				unsigned char **utf)
{
	size_t i;
	wchar_t *wchar;

	wchar = calloc(sizeof(wchar_t) * (memsize + 1), 1);
	for(i = 0; i < memsize; i++) wchar[i] = iso[i];
	*utf = wchar_to_utf8(wchar, memsize);
	free(wchar);
}
#endif

void utf16bom_to_utf8(unsigned char *utf16, size_t memsize,
				unsigned char **utf)
{
	wchar_t *wchar;
	unsigned char utf16char[2];
	int endian = 0;
	size_t i;

	wchar = calloc(sizeof(wchar_t) * memsize / 2 - 1, 1);
	for(i = 0; i < memsize; i += 2)
	{
		if(i == 0)
		{
			if(utf16[i] == 0xFF) endian = 0;
			else if(utf16[i] == 0xFE) endian = 1;
		}
		else
		{
			utf16char[0] = utf16[i];
			utf16char[1] = utf16[i + 1];
			if(endian == 1)      wchar[i / 2 - 1] = be2short(utf16char);
			else if(endian == 0) wchar[i / 2 - 1] = le2short(utf16char);
		}
	}
	*utf = wchar_to_utf8(wchar, memsize / 2 - 1);
	free(wchar);
}

void utf16be_to_utf8(unsigned char *utf16, size_t memsize,
				unsigned char **utf)
{
	wchar_t *wchar;
	unsigned char utf16char[2];
	size_t i;

	wchar = calloc(sizeof(wchar_t) * (memsize / 2), 1);
	for(i = 0; i < memsize; i += 2)
	{
		utf16char[0] = utf16[i];
		utf16char[1] = utf16[i + 1];
		wchar[i / 2] = be2short(utf16char);
	}
	*utf = wchar_to_utf8(wchar, memsize / 2);
	free(wchar);
}

void utf16le_to_utf8(unsigned char *utf16, size_t memsize,
				unsigned char **utf)
{
	wchar_t *wchar;
	unsigned char utf16char[2];
	size_t i;

	wchar = calloc(sizeof(wchar_t) * (memsize / 2), 1);
	for(i = 0; i < memsize; i += 2)
	{
		utf16char[0] = utf16[i];
		utf16char[1] = utf16[i + 1];
		wchar[i / 2] = le2short(utf16char);
	}
	*utf = wchar_to_utf8(wchar, memsize / 2);
	free(wchar);
}
