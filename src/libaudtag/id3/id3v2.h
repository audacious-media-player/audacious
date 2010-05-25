/*
 * Copyright 2009 Paula Stanciu
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#ifndef ID3_H

#define ID3_H

#include <libaudcore/audstrings.h>
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>
#include "../tag_module.h"

#define PRIMARY_CLASS_MUSIC (unsigned char [16]) { 0xBC, 0x7D, 0x60, 0xD1, 0x23, 0xE3, 0xE2, 0x4B, 0x86, 0xA1, 0x48, 0xA4, 0x2A, 0x28, 0x44, 0x1E }
#define PRIMARY_CLASS_AUDIO (unsigned char [16]) { 0x29, 0x0F, 0xCD, 0x01, 0x4E, 0xDA, 0x57, 0x41, 0x89, 0x7B, 0x62, 0x75, 0xD5, 0x0C, 0x4F, 0x11 }
#define SECONDARY_CLASS_AUDIOBOOK (unsigned char [16]) { 0xEB, 0x6B, 0x23, 0xE0, 0x81, 0xC2, 0xDE, 0x4E, 0xA3, 0x6D, 0x7A, 0xF7, 0x6A, 0x3D, 0x45, 0xB5 }
#define SECONDARY_CLASS_SPOKENWORD (unsigned char [16]) { 0x13, 0x2A, 0x17, 0x3A, 0xD9, 0x2B, 0x31, 0x48, 0x83, 0x5B, 0x11, 0x4F, 0x6A, 0x95, 0x94, 0x3F }
#define SECONDARY_CLASS_NEWS (unsigned char [16]) { 0x9B, 0xDB, 0x77, 0x66, 0xA0, 0xE5, 0x63, 0x40, 0xA1, 0xAD, 0xAC, 0xEB, 0x52, 0x84, 0x0C, 0xF1 }
#define SECONDARY_CLASS_TALKSHOW (unsigned char [16]) { 0x67, 0x4A, 0x82, 0x1B, 0x80, 0x3F, 0x3E, 0x4E, 0x9C, 0xDE, 0xF7, 0x36, 0x1B, 0x0F, 0x5F, 0x1B }
#define SECONDARY_CLASS_GAMES_CLIP (unsigned char [16]) { 0x68, 0x33, 0x03, 0x00, 0x09, 0x50, 0xC3, 0x4A, 0xA8, 0x20, 0x5D, 0x2D, 0x09, 0xA4, 0xE7, 0xC1 }
#define SECONDARY_CLASS_GAMES_SONG (unsigned char [16]) { 0x31, 0xF7, 0x4F, 0xF2, 0xFC, 0x96, 0x0F, 0x4D, 0xA2, 0xF5, 0x5A, 0x34, 0x83, 0x68, 0x2B, 0x1A }

#pragma pack(push) /* must be byte-aligned */
#pragma pack(1)
typedef struct
{
    gchar magic[3];
    guchar version;
    guchar revision;
    guchar flags;
    guint32 size;
}
ID3v2Header;
#pragma pack(pop)

typedef struct
{
    gchar * frame_id;
    guint32 size;
    guint16 flags;
}
ID3v2FrameHeader;

typedef struct genericframe
{
    ID3v2FrameHeader *header;
    gchar*           frame_body;
}GenericFrame;

guint32 read_syncsafe_int32(VFSFile *fd);

gchar* readFrameBody(VFSFile *fd,int size);

void write_ASCII(VFSFile *fd, int size, gchar* value);

void write_utf8(VFSFile *fd, int size,gchar* value);

guint32 writeAllFramesToFile(VFSFile *fd);

void writeID3FrameHeaderToFile(VFSFile *fd, ID3v2FrameHeader *header);

void writeGenericFrame(VFSFile *fd,GenericFrame *frame);

gboolean isExtendedHeader(ID3v2Header *header);

int getFrameID(ID3v2FrameHeader *header);

void skipFrame(VFSFile *fd, guint32 size);

gboolean isValidFrame(GenericFrame *frame);

extern tag_module_t id3v2;
#endif
