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

#ifndef APE_H
#define APE_H 1
typedef struct
{
	unsigned int len;
	unsigned char *data, *name;
} apefielddata_t;

typedef struct
{
	unsigned int numitems, version;
	apefielddata_t **items;
} ape_t;

#ifndef MAKE_BMP
int findAPE(VFSFile *);
#else
int findAPE(VFSFile *);
#endif
ape_t *readAPE(char *);
void freeAPE(ape_t *);
#endif
