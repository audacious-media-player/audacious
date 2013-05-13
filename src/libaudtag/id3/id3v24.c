/*
 * id3v24.c
 * Copyright 2009-2011 Paula Stanciu, Tony Vroon, John Lindgren,
 *                     Mikael Magnusson, and Michał Lipski
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libaudcore/audstrings.h>

#include "id3-common.h"
#include "id3v24.h"
#include "../util.h"

enum
{
    ID3_ALBUM = 0,
    ID3_TITLE,
    ID3_COMPOSER,
    ID3_COPYRIGHT,
    ID3_DATE,
    ID3_LENGTH,
    ID3_ARTIST,
    ID3_TRACKNR,
    ID3_YEAR,
    ID3_GENRE,
    ID3_COMMENT,
    ID3_PRIVATE,
    ID3_ENCODER,
    ID3_RECORDING_TIME,
    ID3_TXXX,
    ID3_RVA2,
    ID3_TAGS_NO
};

static const char * id3_frames[ID3_TAGS_NO] = {"TALB", "TIT2", "TCOM", "TCOP",
 "TDAT", "TLEN", "TPE1", "TRCK", "TYER", "TCON", "COMM", "PRIV", "TSSE", "TDRC",
 "TXXX", "RVA2"};

static const unsigned char PRIMARY_CLASS_MUSIC[16] = {0xBC, 0x7D, 0x60, 0xD1, 0x23,
 0xE3, 0xE2, 0x4B, 0x86, 0xA1, 0x48, 0xA4, 0x2A, 0x28, 0x44, 0x1E};
static const unsigned char PRIMARY_CLASS_AUDIO[16] = {0x29, 0x0F, 0xCD, 0x01, 0x4E,
 0xDA, 0x57, 0x41, 0x89, 0x7B, 0x62, 0x75, 0xD5, 0x0C, 0x4F, 0x11};
static const unsigned char SECONDARY_CLASS_AUDIOBOOK[16] = {0xEB, 0x6B, 0x23, 0xE0,
 0x81, 0xC2, 0xDE, 0x4E, 0xA3, 0x6D, 0x7A, 0xF7, 0x6A, 0x3D, 0x45, 0xB5};
static const unsigned char SECONDARY_CLASS_SPOKENWORD[16] = {0x13, 0x2A, 0x17, 0x3A,
 0xD9, 0x2B, 0x31, 0x48, 0x83, 0x5B, 0x11, 0x4F, 0x6A, 0x95, 0x94, 0x3F};
static const unsigned char SECONDARY_CLASS_NEWS[16] = {0x9B, 0xDB, 0x77, 0x66, 0xA0,
 0xE5, 0x63, 0x40, 0xA1, 0xAD, 0xAC, 0xEB, 0x52, 0x84, 0x0C, 0xF1};
static const unsigned char SECONDARY_CLASS_TALKSHOW[16] = {0x67, 0x4A, 0x82, 0x1B,
 0x80, 0x3F, 0x3E, 0x4E, 0x9C, 0xDE, 0xF7, 0x36, 0x1B, 0x0F, 0x5F, 0x1B};
static const unsigned char SECONDARY_CLASS_GAMES_CLIP[16] = {0x68, 0x33, 0x03, 0x00,
 0x09, 0x50, 0xC3, 0x4A, 0xA8, 0x20, 0x5D, 0x2D, 0x09, 0xA4, 0xE7, 0xC1};
static const unsigned char SECONDARY_CLASS_GAMES_SONG[16] = {0x31, 0xF7, 0x4F, 0xF2,
 0xFC, 0x96, 0x0F, 0x4D, 0xA2, 0xF5, 0x5A, 0x34, 0x83, 0x68, 0x2B, 0x1A};

#pragma pack(push) /* must be byte-aligned */
#pragma pack(1)
typedef struct
{
    char magic[3];
    unsigned char version;
    unsigned char revision;
    unsigned char flags;
    uint32_t size;
}
ID3v2Header;

typedef struct
{
    char key[4];
    uint32_t size;
    uint16_t flags;
}
ID3v2FrameHeader;
#pragma pack(pop)

typedef struct
{
    char key[5];
    unsigned char * data;
    int size;
}
GenericFrame;

#define ID3_HEADER_SYNCSAFE             0x80
#define ID3_HEADER_HAS_EXTENDED_HEADER  0x40
#define ID3_HEADER_HAS_FOOTER           0x10

#define ID3_FRAME_HAS_GROUP   0x0040
#define ID3_FRAME_COMPRESSED  0x0008
#define ID3_FRAME_ENCRYPTED   0x0004
#define ID3_FRAME_SYNCSAFE    0x0002
#define ID3_FRAME_HAS_LENGTH  0x0001

/* str_unref() may be a macro */
static void str_unref_cb (void * str)
{
    str_unref (str);
}

static bool_t skip_extended_header_3 (VFSFile * handle, int * _size)
{
    uint32_t size;

    if (vfs_fread (& size, 1, 4, handle) != 4)
        return FALSE;

    size = GUINT32_FROM_BE (size);

    TAGDBG ("Found v2.3 extended header, size = %d.\n", (int) size);

    if (vfs_fseek (handle, size, SEEK_CUR))
        return FALSE;

    * _size = 4 + size;
    return TRUE;
}

static bool_t skip_extended_header_4 (VFSFile * handle, int * _size)
{
    uint32_t size;

    if (vfs_fread (& size, 1, 4, handle) != 4)
        return FALSE;

    size = unsyncsafe32 (GUINT32_FROM_BE (size));

    TAGDBG ("Found v2.4 extended header, size = %d.\n", (int) size);

    if (vfs_fseek (handle, size - 4, SEEK_CUR))
        return FALSE;

    * _size = size;
    return TRUE;
}

static bool_t validate_header (ID3v2Header * header, bool_t is_footer)
{
    if (memcmp (header->magic, is_footer ? "3DI" : "ID3", 3))
        return FALSE;

    if ((header->version != 3 && header->version != 4) || header->revision != 0)
        return FALSE;

    header->size = unsyncsafe32 (GUINT32_FROM_BE (header->size));

    TAGDBG ("Found ID3v2 %s:\n", is_footer ? "footer" : "header");
    TAGDBG (" magic = %.3s\n", header->magic);
    TAGDBG (" version = %d\n", (int) header->version);
    TAGDBG (" revision = %d\n", (int) header->revision);
    TAGDBG (" flags = %x\n", (int) header->flags);
    TAGDBG (" size = %d\n", (int) header->size);
    return TRUE;
}

static bool_t read_header (VFSFile * handle, int * version, bool_t *
 syncsafe, int64_t * offset, int * header_size, int * data_size, int *
 footer_size)
{
    ID3v2Header header, footer;

    if (vfs_fseek (handle, 0, SEEK_SET))
        return FALSE;

    if (vfs_fread (& header, 1, sizeof (ID3v2Header), handle) != sizeof
     (ID3v2Header))
        return FALSE;

    if (validate_header (& header, FALSE))
    {
        * offset = 0;
        * version = header.version;
        * header_size = sizeof (ID3v2Header);
        * data_size = header.size;

        if (header.flags & ID3_HEADER_HAS_FOOTER)
        {
            if (vfs_fseek (handle, header.size, SEEK_CUR))
                return FALSE;

            if (vfs_fread (& footer, 1, sizeof (ID3v2Header), handle) != sizeof
             (ID3v2Header))
                return FALSE;

            if (! validate_header (& footer, TRUE))
                return FALSE;

            * footer_size = sizeof (ID3v2Header);
        }
        else
            * footer_size = 0;
    }
    else
    {
        int64_t end = vfs_fsize (handle);

        if (end < 0)
            return FALSE;

        if (vfs_fseek (handle, end - sizeof (ID3v2Header), SEEK_SET))
            return FALSE;

        if (vfs_fread (& footer, 1, sizeof (ID3v2Header), handle) != sizeof
         (ID3v2Header))
            return FALSE;

        if (! validate_header (& footer, TRUE))
            return FALSE;

        * offset = end - 2 * sizeof (ID3v2Header) - footer.size;
        * version = footer.version;
        * header_size = sizeof (ID3v2Header);
        * data_size = footer.size;
        * footer_size = sizeof (ID3v2Header);

        if (vfs_fseek (handle, * offset, SEEK_SET))
            return FALSE;

        if (vfs_fread (& header, 1, sizeof (ID3v2Header), handle) != sizeof
         (ID3v2Header))
            return FALSE;

        if (! validate_header (& header, FALSE))
            return FALSE;
    }

    * syncsafe = (header.flags & ID3_HEADER_SYNCSAFE) ? TRUE : FALSE;

    if (header.flags & ID3_HEADER_HAS_EXTENDED_HEADER)
    {
        int extended_size = 0;

        if (header.version == 3)
        {
            if (! skip_extended_header_3 (handle, & extended_size))
                return FALSE;
        }
        else if (header.version == 4)
        {
            if (! skip_extended_header_4 (handle, & extended_size))
                return FALSE;
        }

        * header_size += extended_size;
        * data_size -= extended_size;
    }

    TAGDBG ("Offset = %d, header size = %d, data size = %d, footer size = "
     "%d.\n", (int) * offset, * header_size, * data_size, * footer_size);

    return TRUE;
}

static int unsyncsafe (unsigned char * data, int size)
{
    unsigned char * get = data, * set = data;

    while (size --)
    {
        unsigned char c = * set ++ = * get ++;

        if (c == 0xff && size && ! get[0])
        {
            size --;
            get ++;
        }
    }

    return set - data;
}

static bool_t read_frame (VFSFile * handle, int max_size, int version,
 bool_t syncsafe, int * frame_size, char * key, unsigned char * * data, int * size)
{
    ID3v2FrameHeader header;
    int skip = 0;

    if ((max_size -= sizeof (ID3v2FrameHeader)) < 0)
        return FALSE;

    if (vfs_fread (& header, 1, sizeof (ID3v2FrameHeader), handle) != sizeof
     (ID3v2FrameHeader))
        return FALSE;

    if (! header.key[0]) /* padding */
        return FALSE;

    header.size = (version == 3) ? GUINT32_FROM_BE (header.size) : unsyncsafe32
     (GUINT32_FROM_BE (header.size));
    header.flags = GUINT16_FROM_BE (header.flags);

    if (header.size > max_size || header.size == 0)
        return FALSE;

    TAGDBG ("Found frame:\n");
    TAGDBG (" key = %.4s\n", header.key);
    TAGDBG (" size = %d\n", (int) header.size);
    TAGDBG (" flags = %x\n", (int) header.flags);

    * frame_size = sizeof (ID3v2FrameHeader) + header.size;
    g_strlcpy (key, header.key, 5);

    if (header.flags & (ID3_FRAME_COMPRESSED | ID3_FRAME_ENCRYPTED))
    {
        TAGDBG ("Hit compressed/encrypted frame %s.\n", key);
        return FALSE;
    }

    if (header.flags & ID3_FRAME_HAS_GROUP)
        skip ++;
    if (header.flags & ID3_FRAME_HAS_LENGTH)
        skip += 4;

    if ((skip > 0 && vfs_fseek (handle, skip, SEEK_CUR)) || skip >= header.size)
        return FALSE;

    * size = header.size - skip;
    * data = g_malloc (* size);

    if (vfs_fread (* data, 1, * size, handle) != * size)
        return FALSE;

    if (syncsafe || (header.flags & ID3_FRAME_SYNCSAFE))
        * size = unsyncsafe (* data, * size);

    TAGDBG ("Data size = %d.\n", * size);
    return TRUE;
}

static char * decode_text_frame (const unsigned char * data, int size)
{
    return convert_text ((const char *) data + 1, size - 1, data[0], FALSE,
     NULL, NULL);
}

static bool_t decode_comment_frame (const unsigned char * _data, int size, char * *
 lang, char * * type, char * * value)
{
    const char * data = (const char *) _data;
    char * pair, * sep;
    int converted;

    pair = convert_text (data + 4, size - 4, data[0], FALSE, & converted, NULL);

    if (pair == NULL || (sep = memchr (pair, 0, converted)) == NULL)
        return FALSE;

    * lang = g_strndup (data + 1, 3);
    * type = g_strdup (pair);
    * value = g_strdup (sep + 1);

    g_free (pair);
    return TRUE;
}

static void free_frame (GenericFrame * frame)
{
    g_free (frame->data);
    g_free (frame);
}

static void free_frame_list (GList * list)
{
    for (GList * node = list; node; node = node->next)
        free_frame (node->data);

    g_list_free (list);
}

static void read_all_frames (VFSFile * handle, int version, bool_t syncsafe,
 int data_size, GHashTable * dict)
{
    int pos;

    for (pos = 0; pos < data_size; )
    {
        int frame_size, size;
        char key[5];
        unsigned char * data;
        GenericFrame * frame;

        if (! read_frame (handle, data_size - pos, version, syncsafe,
         & frame_size, key, & data, & size))
            break;

        pos += frame_size;

        frame = g_malloc (sizeof (GenericFrame));
        strcpy (frame->key, key);
        frame->data = data;
        frame->size = size;

        void * key2, * list = NULL;

        if (g_hash_table_lookup_extended (dict, key, & key2, & list))
            g_hash_table_steal (dict, key);
        else
            key2 = str_get (key);

        list = g_list_append (list, frame);
        g_hash_table_insert (dict, key2, list);
    }
}

static bool_t write_frame (VFSFile * handle, GenericFrame * frame, int *
 frame_size)
{
    TAGDBG ("Writing frame %s, size %d\n", frame->key, frame->size);

    ID3v2FrameHeader header;

    memcpy (header.key, frame->key, 4);
    header.size = syncsafe32 (frame->size);
    header.size = GUINT32_TO_BE (header.size);
    header.flags = 0;

    if (vfs_fwrite (& header, 1, sizeof (ID3v2FrameHeader), handle) != sizeof
     (ID3v2FrameHeader))
        return FALSE;

    if (vfs_fwrite (frame->data, 1, frame->size, handle) != frame->size)
        return FALSE;

    * frame_size = sizeof (ID3v2FrameHeader) + frame->size;
    return TRUE;
}

typedef struct {
    VFSFile * file;
    int written_size;
} WriteState;

static void write_frame_list (void * key, void * list, void * user)
{
    WriteState * state = user;

    for (GList * node = list; node; node = node->next)
    {
        int size;
        if (write_frame (state->file, node->data, & size))
            state->written_size += size;
    }
}

static int write_all_frames (VFSFile * handle, GHashTable * dict)
{
    WriteState state = {handle, 0};
    g_hash_table_foreach (dict, write_frame_list, & state);

    TAGDBG ("Total frame bytes written = %d.\n", state.written_size);
    return state.written_size;
}

static bool_t write_header (VFSFile * handle, int size, bool_t is_footer)
{
    ID3v2Header header;

    memcpy (header.magic, is_footer ? "3DI" : "ID3", 3);
    header.version = 4;
    header.revision = 0;
    header.flags = ID3_HEADER_HAS_FOOTER;
    header.size = syncsafe32 (size);
    header.size = GUINT32_TO_BE (header.size);

    return vfs_fwrite (& header, 1, sizeof (ID3v2Header), handle) == sizeof
     (ID3v2Header);
}

static int get_frame_id (const char * key)
{
    int id;

    for (id = 0; id < ID3_TAGS_NO; id ++)
    {
        if (! strcmp (key, id3_frames[id]))
            return id;
    }

    return -1;
}

static void associate_string (Tuple * tuple, int field, const char *
 customfield, const unsigned char * data, int size)
{
    char * text = decode_text_frame (data, size);

    if (text == NULL || ! text[0])
    {
        g_free (text);
        return;
    }

    if (customfield != NULL)
        TAGDBG ("Custom field %s = %s.\n", customfield, text);
    else
        TAGDBG ("Field %i = %s.\n", field, text);

    tuple_set_str (tuple, field, customfield, text);
    g_free (text);
}

static void associate_int (Tuple * tuple, int field, const char *
 customfield, const unsigned char * data, int size)
{
    char * text = decode_text_frame (data, size);

    if (text == NULL || atoi (text) < 1)
    {
        g_free (text);
        return;
    }

    if (customfield != NULL)
        TAGDBG ("Custom field %s = %s.\n", customfield, text);
    else
        TAGDBG ("Field %i = %s.\n", field, text);

    tuple_set_int (tuple, field, customfield, atoi (text));
    g_free (text);
}

static void decode_private_info (Tuple * tuple, const unsigned char * data, int size)
{
    char * text = g_strndup ((const char *) data, size);

    if (!strncmp(text, "WM/", 3))
    {
        char *separator = strchr(text, 0);
        if (separator == NULL)
            goto DONE;

        char * value = separator + 1;
        if (!strncmp(text, "WM/MediaClassPrimaryID", 22))
        {
            if (!memcmp(value, PRIMARY_CLASS_MUSIC, 16))
                tuple_set_str (tuple, -1, "media-class", "Music");
            if (!memcmp(value, PRIMARY_CLASS_AUDIO, 16))
                tuple_set_str (tuple, -1, "media-class", "Audio (non-music)");
        } else if (!strncmp(text, "WM/MediaClassSecondaryID", 24))
        {
            if (!memcmp(value, SECONDARY_CLASS_AUDIOBOOK, 16))
                tuple_set_str (tuple, -1, "media-class", "Audio Book");
            if (!memcmp(value, SECONDARY_CLASS_SPOKENWORD, 16))
                tuple_set_str (tuple, -1, "media-class", "Spoken Word");
            if (!memcmp(value, SECONDARY_CLASS_NEWS, 16))
                tuple_set_str (tuple, -1, "media-class", "News");
            if (!memcmp(value, SECONDARY_CLASS_TALKSHOW, 16))
                tuple_set_str (tuple, -1, "media-class", "Talk Show");
            if (!memcmp(value, SECONDARY_CLASS_GAMES_CLIP, 16))
                tuple_set_str (tuple, -1, "media-class", "Game Audio (clip)");
            if (!memcmp(value, SECONDARY_CLASS_GAMES_SONG, 16))
                tuple_set_str (tuple, -1, "media-class", "Game Soundtrack");
        } else {
            TAGDBG("Unrecognised tag %s (Windows Media) ignored\n", text);
        }
    } else {
        TAGDBG("Unable to decode private data, skipping: %s\n", text);
    }

DONE:
    g_free (text);
}

static void decode_comment (Tuple * tuple, const unsigned char * data, int size)
{
    char * lang, * type, * value;

    if (! decode_comment_frame (data, size, & lang, & type, & value))
        return;

    TAGDBG ("Comment: lang = %s, type = %s, value = %s.\n", lang, type, value);

    if (! type[0]) /* blank type == actual comment */
        tuple_set_str (tuple, FIELD_COMMENT, NULL, value);

    g_free (lang);
    g_free (type);
    g_free (value);
}

static bool_t decode_rva2_block (const unsigned char * * _data, int * _size, int *
 channel, int * adjustment, int * adjustment_unit, int * peak, int *
 peak_unit)
{
    const unsigned char * data = * _data;
    int size = * _size;
    int peak_bits;

    if (size < 4)
        return FALSE;

    * channel = data[0];
    * adjustment = (char) data[1]; /* first byte is signed */
    * adjustment = (* adjustment << 8) | data[2];
    * adjustment_unit = 512;
    peak_bits = data[3];

    data += 4;
    size -= 4;

    TAGDBG ("RVA2 block: channel = %d, adjustment = %d/%d, peak bits = %d\n",
     * channel, * adjustment, * adjustment_unit, peak_bits);

    if (peak_bits > 0 && peak_bits < sizeof (int) * 8)
    {
        int bytes = (peak_bits + 7) / 8;
        int count;

        if (bytes > size)
            return FALSE;

        * peak = 0;
        * peak_unit = 1 << peak_bits;

        for (count = 0; count < bytes; count ++)
            * peak = (* peak << 8) | data[count];

        data += bytes;
        size -= count;

        TAGDBG ("RVA2 block: peak = %d/%d\n", * peak, * peak_unit);
    }
    else
    {
        * peak = 0;
        * peak_unit = 0;
    }

    * _data = data;
    * _size = size;
    return TRUE;
}

static void decode_rva2 (Tuple * tuple, const unsigned char * data, int size)
{
    const char * domain;
    int channel, adjustment, adjustment_unit, peak, peak_unit;

    if (memchr (data, 0, size) == NULL)
        return;

    domain = (const char *) data;

    TAGDBG ("RVA2 domain: %s\n", domain);

    size -= strlen (domain) + 1;
    data += strlen (domain) + 1;

    while (size > 0)
    {
        if (! decode_rva2_block (& data, & size, & channel, & adjustment,
         & adjustment_unit, & peak, & peak_unit))
            break;

        if (channel != 1) /* specific channel? */
            continue;

        if (tuple_get_value_type (tuple, FIELD_GAIN_GAIN_UNIT, NULL) ==
         TUPLE_INT)
            adjustment = adjustment * (int64_t) tuple_get_int (tuple,
             FIELD_GAIN_GAIN_UNIT, NULL) / adjustment_unit;
        else
            tuple_set_int (tuple, FIELD_GAIN_GAIN_UNIT, NULL,
             adjustment_unit);

        if (peak_unit)
        {
            if (tuple_get_value_type (tuple, FIELD_GAIN_PEAK_UNIT, NULL) ==
             TUPLE_INT)
                peak = peak * (int64_t) tuple_get_int (tuple,
                 FIELD_GAIN_PEAK_UNIT, NULL) / peak_unit;
            else
                tuple_set_int (tuple, FIELD_GAIN_PEAK_UNIT, NULL,
                 peak_unit);
        }

        if (! g_ascii_strcasecmp (domain, "album"))
        {
            tuple_set_int (tuple, FIELD_GAIN_ALBUM_GAIN, NULL, adjustment);

            if (peak_unit)
                tuple_set_int (tuple, FIELD_GAIN_ALBUM_PEAK, NULL, peak);
        }
        else if (! g_ascii_strcasecmp (domain, "track"))
        {
            tuple_set_int (tuple, FIELD_GAIN_TRACK_GAIN, NULL, adjustment);

            if (peak_unit)
                tuple_set_int (tuple, FIELD_GAIN_TRACK_PEAK, NULL, peak);
        }
    }
}

static void decode_genre (Tuple * tuple, const unsigned char * data, int size)
{
    int numericgenre;
    char * text = decode_text_frame (data, size);

    if (text == NULL)
        return;

    if (text[0] == '(')
        numericgenre = atoi (text + 1);
    else
        numericgenre = atoi (text);

    if (numericgenre > 0)
        tuple_set_str (tuple, FIELD_GENRE, NULL,
         convert_numericgenre_to_text (numericgenre));
    else
        tuple_set_str (tuple, FIELD_GENRE, NULL, text);

    g_free (text);
    return;
}

static GenericFrame * add_generic_frame (int id, int size,
 GHashTable * dict)
{
    GenericFrame * frame = g_malloc (sizeof (GenericFrame));

    strcpy (frame->key, id3_frames[id]);
    frame->data = g_malloc (size);
    frame->size = size;

    GList * list = g_list_append (NULL, frame);
    g_hash_table_insert (dict, str_get (id3_frames[id]), list);

    return frame;
}

static void remove_frame (int id, GHashTable * dict)
{
    TAGDBG ("Deleting frame %s.\n", id3_frames[id]);
    g_hash_table_remove (dict, id3_frames[id]);
}

static void add_text_frame (int id, const char * text, GHashTable * dict)
{
    if (text == NULL)
    {
        remove_frame (id, dict);
        return;
    }

    TAGDBG ("Adding text frame %s = %s.\n", id3_frames[id], text);
    int length = strlen (text);

    GenericFrame * frame = add_generic_frame (id, length + 1, dict);
    frame->data[0] = 3; /* UTF-8 encoding */
    memcpy (frame->data + 1, text, length);
}

static void add_comment_frame (const char * text, GHashTable * dict)
{
    if (text == NULL)
    {
        remove_frame (ID3_COMMENT, dict);
        return;
    }

    TAGDBG ("Adding comment frame = %s.\n", text);
    int length = strlen (text);
    GenericFrame * frame = add_generic_frame (ID3_COMMENT, length + 5, dict);

    frame->data[0] = 3; /* UTF-8 encoding */
    strcpy ((char *) frame->data + 1, "eng"); /* well, it *might* be English */
    memcpy (frame->data + 5, text, length);
}

static void add_frameFromTupleStr (const Tuple * tuple, int field, int
 id3_field, GHashTable * dict)
{
    char * str = tuple_get_str (tuple, field, NULL);
    add_text_frame (id3_field, str, dict);
    str_unref (str);
}

static void add_frameFromTupleInt (const Tuple * tuple, int field, int
 id3_field, GHashTable * dict)
{
    if (tuple_get_value_type (tuple, field, NULL) != TUPLE_INT)
    {
        remove_frame (id3_field, dict);
        return;
    }

    char scratch[16];
    snprintf (scratch, sizeof scratch, "%d", tuple_get_int (tuple, field, NULL));
    add_text_frame (id3_field, scratch, dict);
}

static bool_t id3v24_can_handle_file (VFSFile * handle)
{
    int version, header_size, data_size, footer_size;
    bool_t syncsafe;
    int64_t offset;

    return read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size, & footer_size);
}

static bool_t id3v24_read_tag (Tuple * tuple, VFSFile * handle)
{
    int version, header_size, data_size, footer_size;
    bool_t syncsafe;
    int64_t offset;
    int pos;

    if (! read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size, & footer_size))
        return FALSE;

    for (pos = 0; pos < data_size; )
    {
        int frame_size, size, id;
        char key[5];
        unsigned char * data;

        if (! read_frame (handle, data_size - pos, version, syncsafe,
         & frame_size, key, & data, & size))
            break;

        id = get_frame_id (key);

        switch (id)
        {
          case ID3_ALBUM:
            associate_string (tuple, FIELD_ALBUM, NULL, data, size);
            break;
          case ID3_TITLE:
            associate_string (tuple, FIELD_TITLE, NULL, data, size);
            break;
          case ID3_COMPOSER:
            associate_string (tuple, FIELD_COMPOSER, NULL, data, size);
            break;
          case ID3_COPYRIGHT:
            associate_string (tuple, FIELD_COPYRIGHT, NULL, data, size);
            break;
          case ID3_DATE:
            associate_string (tuple, FIELD_DATE, NULL, data, size);
            break;
          case ID3_LENGTH:
            associate_int (tuple, FIELD_LENGTH, NULL, data, size);
            break;
          case ID3_ARTIST:
            associate_string (tuple, FIELD_ARTIST, NULL, data, size);
            break;
          case ID3_TRACKNR:
            associate_int (tuple, FIELD_TRACK_NUMBER, NULL, data, size);
            break;
          case ID3_YEAR:
          case ID3_RECORDING_TIME:
            associate_int (tuple, FIELD_YEAR, NULL, data, size);
            break;
          case ID3_GENRE:
            decode_genre (tuple, data, size);
            break;
          case ID3_COMMENT:
            decode_comment (tuple, data, size);
            break;
          case ID3_PRIVATE:
            decode_private_info (tuple, data, size);
            break;
          case ID3_RVA2:
            decode_rva2 (tuple, data, size);
            break;
          default:
            TAGDBG ("Ignoring unsupported ID3 frame %s.\n", key);
            break;
        }

        g_free (data);
        pos += frame_size;
    }

    return TRUE;
}

static bool_t parse_apic (const unsigned char * _data, int size, char * * mime,
 int * type, char * * desc, void * * image_data, int * image_size)
{
    const char * data = (const char *) _data;
    const char * sep, * after;

    if (size < 2 || (sep = memchr (data + 1, 0, size - 2)) == NULL)
        return FALSE;

    if ((* desc = convert_text (sep + 2, data + size - sep - 2, data[0], TRUE,
     NULL, & after)) == NULL)
        return FALSE;

    * mime = g_strdup (data + 1);
    * type = sep[1];
    * image_data = g_memdup (after, data + size - after);
    * image_size = data + size - after;

    TAGDBG ("APIC: mime = %s, type = %d, desc = %s, size = %d.\n", * mime,
     * type, * desc, * image_size);
    return TRUE;
}

static bool_t id3v24_read_image (VFSFile * handle, void * * image_data,
 int64_t * image_size64)
{
    int version, header_size, data_size, footer_size, parsed, image_size = 0;
    bool_t syncsafe;
    int64_t offset;
    bool_t found = FALSE;

    if (! read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size, & footer_size))
        return FALSE;

    for (parsed = 0; parsed < data_size && ! found; )
    {
        int frame_size, size, type;
        char key[5];
        unsigned char * data;
        char * mime, * desc;

        if (! read_frame (handle, data_size - parsed, version, syncsafe,
         & frame_size, key, & data, & size))
            break;

        if (! strcmp (key, "APIC") && parse_apic (data, size, & mime, & type,
         & desc, image_data, & image_size))
        {
            g_free (mime);
            g_free (desc);

            if (type == 3) /* album cover */
                found = TRUE;
            else if (type == 0) /* iTunes */
                found = TRUE;
            else if (*image_data != NULL)
            {
                g_free(*image_data);
                *image_data = NULL;
            }
        }

        g_free (data);
        parsed += frame_size;
    }

    if (found)
        * image_size64 = image_size;

    return found;
}

static bool_t id3v24_write_tag (const Tuple * tuple, VFSFile * f)
{
    int version, header_size, data_size, footer_size;
    bool_t syncsafe;
    int64_t offset;

    if (! read_header (f, & version, & syncsafe, & offset, & header_size,
     & data_size, & footer_size))
        return FALSE;

    //read all frames into generic frames;
    GHashTable * dict = g_hash_table_new_full (g_str_hash, g_str_equal,
     str_unref_cb, (GDestroyNotify) free_frame_list);
    read_all_frames (f, version, syncsafe, data_size, dict);

    //make the new frames from tuple and replace in the dictionary the old frames with the new ones
    add_frameFromTupleStr (tuple, FIELD_TITLE, ID3_TITLE, dict);
    add_frameFromTupleStr (tuple, FIELD_ARTIST, ID3_ARTIST, dict);
    add_frameFromTupleStr (tuple, FIELD_ALBUM, ID3_ALBUM, dict);
    add_frameFromTupleInt (tuple, FIELD_YEAR, ID3_YEAR, dict);
    add_frameFromTupleInt (tuple, FIELD_TRACK_NUMBER, ID3_TRACKNR, dict);
    add_frameFromTupleStr (tuple, FIELD_GENRE, ID3_GENRE, dict);

    char * comment = tuple_get_str (tuple, FIELD_COMMENT, NULL);
    add_comment_frame (comment, dict);
    str_unref (comment);

    if (! offset)
    {
        if (! cut_beginning_tag (f, header_size + data_size + footer_size))
            goto ERR;
    }
    else
    {
        if (offset + header_size + data_size + footer_size != vfs_fsize (f))
            goto ERR;
        if (vfs_ftruncate (f, offset))
            goto ERR;
    }

    offset = vfs_fsize (f);

    if (offset < 0 || vfs_fseek (f, offset, SEEK_SET) || ! write_header (f, 0,
     FALSE))
        goto ERR;

    data_size = write_all_frames (f, dict);

    if (! write_header (f, data_size, TRUE) || vfs_fseek (f, offset, SEEK_SET)
     || ! write_header (f, data_size, FALSE))
        goto ERR;

    g_hash_table_destroy (dict);
    return TRUE;

ERR:
    g_hash_table_destroy (dict);
    return FALSE;
}

tag_module_t id3v24 =
{
    .name = "ID3v2.3/4",
    .can_handle_file = id3v24_can_handle_file,
    .read_tag = id3v24_read_tag,
    .read_image = id3v24_read_image,
    .write_tag = id3v24_write_tag,
};
