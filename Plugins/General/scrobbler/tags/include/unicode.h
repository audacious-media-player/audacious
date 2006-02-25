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

#ifndef UNICODE_H
#define UNICODE_H 1
wchar_t *utf8_to_wchar(unsigned char *, size_t);
unsigned char *wchar_to_utf8(wchar_t *, size_t);
void iso88591_to_utf8(unsigned char *, size_t, unsigned char **);
void utf16bom_to_utf8(unsigned char *, size_t, unsigned char **);
void utf16be_to_utf8(unsigned char *, size_t, unsigned char **);
void utf16le_to_utf8(unsigned char *, size_t, unsigned char **);
#endif
