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

#ifndef WMA_H
#define WMA_H 1

#include <libaudacious/vfs.h>

typedef struct
{
	unsigned char *data, *name;
	int type;
} attribute_t;

typedef struct
{
	unsigned int numitems;
	attribute_t **items;
} wma_t;

#ifndef MAKE_BMP
int findWMA(VFSFile *);
#else
int findWMA(VFSFile *);
#endif
wma_t *readWMA(char *);
void freeWMA(wma_t *);
#endif
