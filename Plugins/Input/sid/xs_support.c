/*  
   XMMS-SID - SIDPlay input plugin for X MultiMedia System (XMMS)

   Miscellaneous support functions
   
   Programmed and designed by Matti 'ccr' Hamalainen <ccr@tnsp.org>
   (C) Copyright 1999-2005 Tecnic Software productions (TNSP)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
*/
#include "xmms-sid.h"
#include "xs_support.h"
#include <ctype.h>


/* Bigendian file reading functions
 */
guint16 xs_rd_be16(FILE * f)
{
	return (((guint16) fgetc(f)) << 8) | ((guint16) fgetc(f));
}


guint32 xs_rd_be32(FILE * f)
{
	return (((guint32) fgetc(f)) << 24) |
	    (((guint32) fgetc(f)) << 16) | (((guint32) fgetc(f)) << 8) | ((guint32) fgetc(f));
}


size_t xs_rd_str(FILE * f, gchar * s, size_t l)
{
	return fread(s, sizeof(gchar), l, f);
}


/* Copy a string
 */
gchar *xs_strncpy(gchar * pDest, gchar * pSource, size_t n)
{
	gchar *s, *d;
	size_t i;

	/* Check the string pointers */
	if (!pSource || !pDest)
		return pDest;

	/* Copy to the destination */
	i = n;
	s = pSource;
	d = pDest;
	while (*s && (i > 0)) {
		*(d++) = *(s++);
		i--;
	}

	/* Fill rest of space with zeros */
	while (i > 0) {
		*(d++) = 0;
		i--;
	}

	/* Ensure that last is always zero */
	pDest[n - 1] = 0;

	return pDest;
}


/* Copy a given string over in *ppResult.
 */
gint xs_pstrcpy(gchar ** ppResult, const gchar * pStr)
{
	/* Check the string pointers */
	if (!ppResult || !pStr)
		return -1;

	/* Allocate memory for destination */
	if (*ppResult)
		g_free(*ppResult);
	*ppResult = (gchar *) g_malloc(strlen(pStr) + 1);
	if (!*ppResult)
		return -2;

	/* Copy to the destination */
	strcpy(*ppResult, pStr);

	return 0;
}


/* Concatenates a given string into string pointed by *ppResult.
 */
gint xs_pstrcat(gchar ** ppResult, const gchar * pStr)
{
	/* Check the string pointers */
	if (!ppResult || !pStr)
		return -1;

	if (*ppResult != NULL) {
		*ppResult = (gchar *) g_realloc(*ppResult, strlen(*ppResult) + strlen(pStr) + 1);
		if (*ppResult == NULL)
			return -1;
		strcat(*ppResult, pStr);
	} else {
		*ppResult = (gchar *) g_malloc(strlen(pStr) + 1);
		if (*ppResult == NULL)
			return -1;
		strcpy(*ppResult, pStr);
	}

	return 0;
}


/* Concatenate a given string up to given dest size or \n.
 * If size max is reached, change the end to "..."
 */
void xs_pnstrcat(gchar * pDest, size_t iSize, gchar * pStr)
{
	size_t i, n;
	gchar *s, *d;

	d = pDest;
	i = 0;
	while (*d && (i < iSize)) {
		i++;
		d++;
	}

	s = pStr;
	while (*s && (*s != '\n') && (i < iSize)) {
		*d = *s;
		d++;
		s++;
		i++;
	}

	*d = 0;

	if (i >= iSize) {
		i--;
		d--;
		n = 3;
		while ((i > 0) && (n > 0)) {
			*d = '.';
			d--;
			i--;
			n--;
		}
	}
}


/* Locate character in string
 */
gchar *xs_strrchr(gchar * pcStr, gchar ch)
{
	gchar *lastPos = NULL;

	while (*pcStr) {
		if (*pcStr == ch)
			lastPos = pcStr;
		pcStr++;
	}

	return lastPos;
}


void xs_findnext(gchar * pcStr, guint * piPos)
{
	while (pcStr[*piPos] && isspace(pcStr[*piPos]))
		(*piPos)++;
}


void xs_findeol(gchar * pcStr, guint * piPos)
{
	while (pcStr[*piPos] && (pcStr[*piPos] != '\n') && (pcStr[*piPos] != '\r'))
		(*piPos)++;
}


void xs_findnum(gchar * pcStr, guint * piPos)
{
	while (pcStr[*piPos] && isdigit(pcStr[*piPos]))
		(*piPos)++;
}


#ifndef HAVE_MEMSET
void *xs_memset(void *p, int c, size_t n)
{
	gchar *dp;

	dp = (gchar *) p;
	while (n--) {
		*dp = (gchar) c;
		n--;
	}

	return p;
}
#endif
