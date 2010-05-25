/*
 * Copyright 2009 Paula Stanciu
 * Copyright 2010 John Lindgren
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

#include <glib.h>

#include <libaudcore/audstrings.h>

#include "id3v2.h"
#include "../util.h"

enum
{
    ID3_ALBUM = 0,
    ID3_TITLE,
    ID3_COMPOSER,
    ID3_COPYRIGHT,
    ID3_DATE,
    ID3_TIME,
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

static const gchar * id3_frames[ID3_TAGS_NO] = {"TALB","TIT2","TCOM", "TCOP",
 "TDAT", "TIME", "TLEN", "TPE1", "TRCK", "TYER","TCON", "COMM", "PRIV", "TSSE",
 "TDRC", "TXXX", "RVA2"};

static const guchar PRIMARY_CLASS_MUSIC[16] = {0xBC, 0x7D, 0x60, 0xD1, 0x23,
 0xE3, 0xE2, 0x4B, 0x86, 0xA1, 0x48, 0xA4, 0x2A, 0x28, 0x44, 0x1E};
static const guchar PRIMARY_CLASS_AUDIO[16] = {0x29, 0x0F, 0xCD, 0x01, 0x4E,
 0xDA, 0x57, 0x41, 0x89, 0x7B, 0x62, 0x75, 0xD5, 0x0C, 0x4F, 0x11};
static const guchar SECONDARY_CLASS_AUDIOBOOK[16] = {0xEB, 0x6B, 0x23, 0xE0,
 0x81, 0xC2, 0xDE, 0x4E, 0xA3, 0x6D, 0x7A, 0xF7, 0x6A, 0x3D, 0x45, 0xB5};
static const guchar SECONDARY_CLASS_SPOKENWORD[16] = {0x13, 0x2A, 0x17, 0x3A,
 0xD9, 0x2B, 0x31, 0x48, 0x83, 0x5B, 0x11, 0x4F, 0x6A, 0x95, 0x94, 0x3F};
static const guchar SECONDARY_CLASS_NEWS[16] = {0x9B, 0xDB, 0x77, 0x66, 0xA0,
 0xE5, 0x63, 0x40, 0xA1, 0xAD, 0xAC, 0xEB, 0x52, 0x84, 0x0C, 0xF1};
static const guchar SECONDARY_CLASS_TALKSHOW[16] = {0x67, 0x4A, 0x82, 0x1B,
 0x80, 0x3F, 0x3E, 0x4E, 0x9C, 0xDE, 0xF7, 0x36, 0x1B, 0x0F, 0x5F, 0x1B};
static const guchar SECONDARY_CLASS_GAMES_CLIP[16] = {0x68, 0x33, 0x03, 0x00,
 0x09, 0x50, 0xC3, 0x4A, 0xA8, 0x20, 0x5D, 0x2D, 0x09, 0xA4, 0xE7, 0xC1};
static const guchar SECONDARY_CLASS_GAMES_SONG[16] = {0x31, 0xF7, 0x4F, 0xF2,
 0xFC, 0x96, 0x0F, 0x4D, 0xA2, 0xF5, 0x5A, 0x34, 0x83, 0x68, 0x2B, 0x1A};

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

typedef struct
{
    ID3v2FrameHeader * header;
    gchar * frame_body;
}
GenericFrame;

#define ID3_FLAG_HAS_EXTENDED_HEADER 0x40
#define ID3_FLAG_HAS_FOOTER          0x10

#define TAG_SIZE 1

static mowgli_dictionary_t * frames = NULL;
static mowgli_list_t * frameIDs = NULL;

static guint32 unsyncsafe32 (guint32 x)
{
    return (x & 0x7f) | ((x & 0x7f00) >> 1) | ((x & 0x7f0000) >> 2) | ((x &
     0x7f000000) >> 3);
}

static guint32 syncsafe32 (guint32 x)
{
    return (x & 0x7f) | ((x & 0x3f80) << 1) | ((x & 0x1fc000) << 2) | ((x &
     0xfe00000) << 3);
}

#define write_syncsafe_int32(x) vfs_fput_be32 (syncsafe32 (x))

static gboolean skip_extended_header_3 (VFSFile * handle, gint * _size)
{
    guint32 size;

    if (vfs_fread (& size, 1, 4, handle) != 4)
        return FALSE;

    size = GUINT32_FROM_BE (size);

    AUDDBG ("Found v2.3 extended header, size = %d.\n", (gint) size);

    if (vfs_fseek (handle, size, SEEK_CUR))
        return FALSE;

    * _size = 4 + size;
    return TRUE;
}

static gboolean skip_extended_header_4 (VFSFile * handle, gint * _size)
{
    guint32 size;

    if (vfs_fread (& size, 1, 4, handle) != 4)
        return FALSE;

    size = unsyncsafe32 (GUINT32_FROM_BE (size));

    AUDDBG ("Found v2.4 extended header, size = %d.\n", (gint) size);

    if (vfs_fseek (handle, size - 4, SEEK_CUR))
        return FALSE;

    * _size = size;
    return TRUE;
}

static gboolean validate_header (ID3v2Header * header, gboolean is_footer)
{
    if (memcmp (header->magic, is_footer ? "3DI" : "ID3", 3))
        return FALSE;

    if ((header->version != 3 && header->version != 4) || header->revision != 0)
        return FALSE;

    header->size = unsyncsafe32 (GUINT32_FROM_BE (header->size));

    AUDDBG ("Found ID3v2 %s:\n", is_footer ? "footer" : "header");
    AUDDBG (" magic = %.3s\n", header->magic);
    AUDDBG (" version = %d\n", (gint) header->version);
    AUDDBG (" revision = %d\n", (gint) header->revision);
    AUDDBG (" flags = %x\n", (gint) header->flags);
    AUDDBG (" size = %d\n", (gint) header->size);

    return TRUE;
}

static gboolean read_header (VFSFile * handle, gint * version, gsize * offset,
 gint * header_size, gint * data_size, gint * footer_size)
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

        if (header.flags & ID3_FLAG_HAS_FOOTER)
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
        gsize end = vfs_fsize (handle);

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

    if (header.flags & ID3_FLAG_HAS_EXTENDED_HEADER)
    {
        gint extended_size = 0;

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

    AUDDBG ("Offset = %d, header size = %d, data size = %d, footer size = "
     "%d.\n", (gint) * offset, * header_size, * data_size, * footer_size);

    return TRUE;
}

static guint32 read_syncsafe_int32 (VFSFile * fd)
{
    guint32 val = read_BEuint32(fd);

    return unsyncsafe32 (val);
}

static ID3v2FrameHeader * readID3v2FrameHeader (VFSFile * fd, gint version)
{
    ID3v2FrameHeader *frameheader = g_new0(ID3v2FrameHeader, 1);
    frameheader->frame_id = read_char_data(fd, 4);

    if (version == 3)
        frameheader->size = read_BEuint32 (fd);
    else
        frameheader->size = read_syncsafe_int32 (fd);

    frameheader->flags = read_LEuint16(fd);
    if ((frameheader->flags & 0x100) == 0x100)
        frameheader->size = read_syncsafe_int32(fd);

    AUDDBG("got frame %s, size %d, flags %x\n", frameheader->frame_id, frameheader->size, frameheader->flags);

    return frameheader;
}

static void free_frame_header (ID3v2FrameHeader * header)
{
    g_free (header->frame_id);
    g_free (header);
}

static gint unsyncsafe (gchar * data, gint size)
{
    gchar * get = data, * set = data;

    while (size --)
    {
        gchar c = * set ++ = * get ++;

        if (c == (gchar) 0xff && size)
        {
            size --;
            get ++;
        }
    }

    return set - data;
}

static gchar * read_text_frame (VFSFile * handle, ID3v2FrameHeader * header)
{
    gint size = header->size;
    gchar data[size];

    if (vfs_fread (data, 1, size, handle) != size)
        return NULL;

    if (header->flags & 0x200)
        size = unsyncsafe (data, size);

    switch (data[0])
    {
    case 0:
        data[size] = '\0';
        return str_to_utf8(data + 1);
    case 1:
        if (data[1] == (gchar) 0xff)
            return g_convert (data + 3, size - 3, "UTF-8", "UTF-16LE", NULL,
             NULL, NULL);
        else
            return g_convert (data + 3, size - 3, "UTF-8", "UTF-16BE", NULL,
             NULL, NULL);
    case 2:
        return g_convert (data + 1, size - 1, "UTF-8", "UTF-16BE", NULL, NULL,
         NULL);
    case 3:
        return g_strndup (data + 1, size - 1);
    default:
        AUDDBG ("Throwing away %i bytes of text due to invalid encoding %d\n",
         size - 1, (gint) data[0]);
        return NULL;
    }
}

static gchar * read_raw_text_frame (VFSFile * handle, ID3v2FrameHeader * header)
{
    gint size = header->size;
    gchar data[size];

    if (vfs_fread (data, 1, size, handle) != size)
        return NULL;

    if (header->flags & 0x200)
        size = unsyncsafe (data, size);

    switch (data[0])
    {
    case 0:
	return g_convert (data + 1, size - 1, "UTF-8", "ISO-8859-1", NULL,
	 NULL, NULL);
    case 1:
        if (data[1] == (gchar) 0xff)
            return g_convert (data + 3, size - 3, "UTF-8", "UTF-16LE", NULL,
             NULL, NULL);
        else
            return g_convert (data + 3, size - 3, "UTF-8", "UTF-16BE", NULL,
             NULL, NULL);
    case 2:
        return g_convert (data + 1, size - 1, "UTF-8", "UTF-16BE", NULL, NULL,
         NULL);
    case 3:
        return g_strndup (data + 1, size - 1);
    default:
        AUDDBG ("Throwing away %i bytes of text due to invalid encoding %d\n",
         size - 1, (gint) data[0]);
        return NULL;
    }
}

static gboolean read_comment_frame (VFSFile * handle, ID3v2FrameHeader * header,
 gchar * * lang, gchar * * type, gchar * * value)
{
    gint size = header->size;
    gchar data[size];
    gchar * pair, * sep;
    gsize converted;

    if (vfs_fread (data, 1, size, handle) != size)
        return FALSE;

    if (header->flags & 0x200)
        size = unsyncsafe (data, size);

    switch (data[0])
    {
    case 0:
        pair = g_convert (data + 4, size - 4, "UTF-8", "ISO-8859-1", NULL,
         & converted, NULL);
        break;
    case 1:
        if (data[4] == (gchar) 0xff)
            pair = g_convert (data + 6, size - 6, "UTF-8", "UTF-16LE", NULL,
             & converted, NULL);
        else
            pair = g_convert (data + 6, size - 6, "UTF-8", "UTF-16BE", NULL,
             & converted, NULL);
        break;
    case 2:
        pair = g_convert (data + 4, size - 4, "UTF-8", "UTF-16BE", NULL,
         & converted, NULL);
        break;
    case 3:
        pair = g_malloc (size - 3);
        memcpy (pair, data + 4, size - 4);
        pair[size - 4] = 0;
        converted = size - 4;
        break;
    default:
        AUDDBG ("Throwing away %i bytes of text due to invalid encoding %d\n",
         size - 4, (gint) data[0]);
        pair = NULL;
        break;
    }

    if (pair == NULL || (sep = memchr (pair, 0, converted)) == NULL)
        return FALSE;

    * lang = g_strndup (data + 1, 3);
    * type = g_strdup (pair);
    * value = g_strdup (sep + 1);

    g_free (pair);
    return TRUE;
}

static void readGenericFrame (VFSFile * fd, GenericFrame * gf, gint version)
{
    gf->header = readID3v2FrameHeader (fd, version);
    gf->frame_body = read_char_data(fd, gf->header->size);

    AUDDBG("got frame %s, size %d\n", gf->header->frame_id, gf->header->size);
}

static void free_generic_frame (GenericFrame * frame)
{
    free_frame_header (frame->header);
    g_free (frame->frame_body);
    g_free (frame);
}

static gboolean isValidFrame (GenericFrame * frame)
{
    if (strlen(frame->header->frame_id) != 0)
        return TRUE;
    else
        return FALSE;
}

static void readAllFrames (VFSFile * fd, gint framesSize, gint version)
{
    int pos = 0;

    AUDDBG("readAllFrames\n");

    while (pos < framesSize)
    {
        GenericFrame *gframe = g_new0(GenericFrame, 1);

        readGenericFrame (fd, gframe, version);

        if (isValidFrame(gframe))
        {
            mowgli_dictionary_add (frames, (void *) gframe->header->frame_id,
             gframe);
            mowgli_node_add ((void *) gframe->header->frame_id,
             mowgli_node_create (), frameIDs);
            pos += 10 + gframe->header->size;
        }
        else
        {
            free_generic_frame (gframe);
            break;
        }
    }

}

static void writeID3FrameHeaderToFile (VFSFile * fd, ID3v2FrameHeader * header)
{
    vfs_fwrite(header->frame_id, 4, 1, fd);
    vfs_fput_be32 (syncsafe32 (header->size), fd);
    vfs_fwrite(&header->flags, 2, 1, fd);
}

static void writeGenericFrame (VFSFile * fd, GenericFrame * frame)
{
    writeID3FrameHeaderToFile(fd, frame->header);
    vfs_fwrite(frame->frame_body, frame->header->size, 1, fd);
}

static guint32 writeAllFramesToFile (VFSFile * fd)
{
    guint32 size = 0;
    mowgli_node_t *n, *tn;
    MOWGLI_LIST_FOREACH_SAFE(n, tn, frameIDs->head)
    {
        GenericFrame *frame = (GenericFrame *) mowgli_dictionary_retrieve(frames, (gchar *) (n->data));
        if (frame)
        {
            writeGenericFrame(fd, frame);
            size += frame->header->size + 10;
        }
    }
    return size;
}

static gboolean write_header (VFSFile * handle, gint size, gboolean is_footer)
{
    ID3v2Header header;

    memcpy (header.magic, is_footer ? "3DI" : "ID3", 3);
    header.version = 4;
    header.revision = 0;
    header.flags = ID3_FLAG_HAS_FOOTER;
    header.size = syncsafe32 (size);
    header.size = GUINT32_TO_BE (header.size);

    return vfs_fwrite (& header, 1, sizeof (ID3v2Header), handle) == sizeof
     (ID3v2Header);
}

static int getFrameID (ID3v2FrameHeader * header)
{
    int i = 0;
    for (i = 0; i < ID3_TAGS_NO; i++)
    {
        if (!strcmp(header->frame_id, id3_frames[i]))
            return i;
    }
    return -1;
}

static void skipFrame (VFSFile * fd, guint32 size)
{
    vfs_fseek(fd, size, SEEK_CUR);
}

static void associate_string (Tuple * tuple, VFSFile * handle, gint field,
 const gchar * customfield, ID3v2FrameHeader * header)
{
    gchar * text = read_text_frame (handle, header);

    if (text == NULL)
        return;

    if (customfield != NULL)
        AUDDBG ("custom field %s = %s\n", customfield, text);
    else
        AUDDBG ("field %i = %s\n", field, text);

    tuple_associate_string (tuple, field, customfield, text);
    g_free (text);
}

static void associate_int (Tuple * tuple, VFSFile * handle, gint field,
 const gchar * customfield, ID3v2FrameHeader * header)
{
    gchar * text = read_text_frame (handle, header);

    if (text == NULL)
        return;

    if (customfield != NULL)
        AUDDBG ("custom field %s = %s\n", customfield, text);
    else
        AUDDBG ("field %i = %s\n", field, text);

    tuple_associate_int (tuple, field, customfield, atoi (text));
    g_free (text);
}

static void decode_private_info(Tuple * tuple, VFSFile * fd, ID3v2FrameHeader * header)
{
    gchar *text = read_char_data(fd, header->size);
    if (!strncmp(text, "WM/", 3))
    {
        gchar *separator = strchr(text, 0);
        if (separator == NULL)
            return;
        gchar * value = separator + 1;
        if (!strncmp(text, "WM/MediaClassPrimaryID", 22))
        {
            if (!memcmp(value, PRIMARY_CLASS_MUSIC, 16))
                tuple_associate_string (tuple, -1, "media-class", "Music");
            if (!memcmp(value, PRIMARY_CLASS_AUDIO, 16))
                tuple_associate_string (tuple, -1, "media-class", "Audio (non-music)");
        } else if (!strncmp(text, "WM/MediaClassSecondaryID", 24))
        {
            if (!memcmp(value, SECONDARY_CLASS_AUDIOBOOK, 16))
                tuple_associate_string (tuple, -1, "media-class", "Audio Book");
            if (!memcmp(value, SECONDARY_CLASS_SPOKENWORD, 16))
                tuple_associate_string (tuple, -1, "media-class", "Spoken Word");
            if (!memcmp(value, SECONDARY_CLASS_NEWS, 16))
                tuple_associate_string (tuple, -1, "media-class", "News");
            if (!memcmp(value, SECONDARY_CLASS_TALKSHOW, 16))
                tuple_associate_string (tuple, -1, "media-class", "Talk Show");
            if (!memcmp(value, SECONDARY_CLASS_GAMES_CLIP, 16))
                tuple_associate_string (tuple, -1, "media-class", "Game Audio (clip)");
            if (!memcmp(value, SECONDARY_CLASS_GAMES_SONG, 16))
                tuple_associate_string (tuple, -1, "media-class", "Game Soundtrack");
        } else {
            AUDDBG("Unrecognised tag %s (Windows Media) ignored\n", text);
        }
    } else {
        AUDDBG("Unable to decode private data, skipping: %s\n", text);
    }
}

static void decode_comment (Tuple * tuple, VFSFile * handle, ID3v2FrameHeader *
 header)
{
    gchar * lang, * type, * value;

    if (! read_comment_frame (handle, header, & lang, & type, & value))
        return;

    AUDDBG ("comment: lang = %s, type = %s, value = %s\n", lang, type, value);

    if (! type[0]) /* blank type == actual comment */
        tuple_associate_string (tuple, FIELD_COMMENT, NULL, value);

    g_free (lang);
    g_free (type);
    g_free (value);
}

static void decode_txxx (Tuple * tuple, VFSFile * handle, ID3v2FrameHeader * header)
{
    gchar *text = read_raw_text_frame (handle, header);

    if (text == NULL)
        return;

    gchar *separator = strchr(text, 0);

    if (separator == NULL)
        return;

    gchar * value = separator + 1;
    AUDDBG ("Field '%s' has value '%s'\n", text, value);
    tuple_associate_string (tuple, -1, text, value);

    g_free (text);
}

static gboolean decode_rva2_block (guchar * * _data, gint * _size, gint *
 channel, gint * adjustment, gint * adjustment_unit, gint * peak, gint *
 peak_unit)
{
    guchar * data = * _data;
    gint size = * _size;
    gint peak_bits;

    if (size < 4)
        return FALSE;

    * channel = data[0];
    * adjustment = (gchar) data[1]; /* first byte is signed */
    * adjustment = (* adjustment << 8) | data[2];
    * adjustment_unit = 512;
    peak_bits = data[3];

    data += 4;
    size -= 4;

    AUDDBG ("RVA2 block: channel = %d, adjustment = %d/%d, peak bits = %d\n",
     * channel, * adjustment, * adjustment_unit, peak_bits);

    if (peak_bits > 0 && peak_bits < sizeof (gint) * 8)
    {
        gint bytes = (peak_bits + 7) / 8;
        gint count;

        if (bytes > size)
            return FALSE;

        * peak = 0;
        * peak_unit = 1 << peak_bits;

        for (count = 0; count < bytes; count ++)
            * peak = (* peak << 8) | data[count];

        data += bytes;
        size -= count;

        AUDDBG ("RVA2 block: peak = %d/%d\n", * peak, * peak_unit);
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

static void decode_rva2 (Tuple * tuple, VFSFile * handle, ID3v2FrameHeader *
 header)
{
    gint size = header->size;
    guchar _data[size];
    guchar * data = _data;
    gchar * domain;
    gint channel, adjustment, adjustment_unit, peak, peak_unit;

    if (vfs_fread (data, 1, size, handle) != size)
        return;

    if (memchr (data, 0, size) == NULL)
        return;

    domain = (gchar *) data;

    AUDDBG ("RVA2 domain: %s\n", domain);

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
            adjustment = adjustment * (gint64) tuple_get_int (tuple,
             FIELD_GAIN_GAIN_UNIT, NULL) / adjustment_unit;
        else
            tuple_associate_int (tuple, FIELD_GAIN_GAIN_UNIT, NULL,
             adjustment_unit);

        if (peak_unit)
        {
            if (tuple_get_value_type (tuple, FIELD_GAIN_PEAK_UNIT, NULL) ==
             TUPLE_INT)
                peak = peak * (gint64) tuple_get_int (tuple,
                 FIELD_GAIN_PEAK_UNIT, NULL) / peak_unit;
            else
                tuple_associate_int (tuple, FIELD_GAIN_PEAK_UNIT, NULL,
                 peak_unit);
        }

        if (! strcasecmp (domain, "album"))
        {
            tuple_associate_int (tuple, FIELD_GAIN_ALBUM_GAIN, NULL, adjustment);

            if (peak_unit)
                tuple_associate_int (tuple, FIELD_GAIN_ALBUM_PEAK, NULL, peak);
        }
        else if (! strcasecmp (domain, "track"))
        {
            tuple_associate_int (tuple, FIELD_GAIN_TRACK_GAIN, NULL, adjustment);

            if (peak_unit)
                tuple_associate_int (tuple, FIELD_GAIN_TRACK_PEAK, NULL, peak);
        }
    }
}

static Tuple * decodeGenre (Tuple * tuple, VFSFile * fd, ID3v2FrameHeader header)
{
    gint numericgenre;
    gchar * text = read_text_frame (fd, & header);

    if (text == NULL)
        return tuple;

    if (text[0] == '(')
        numericgenre = atoi (text + 1);
    else
        numericgenre = atoi (text);

    if (numericgenre > 0)
    {
        tuple_associate_string(tuple, FIELD_GENRE, NULL, convert_numericgenre_to_text(numericgenre));
        return tuple;
    }
    tuple_associate_string(tuple, FIELD_GENRE, NULL, text);
    g_free (text);
    return tuple;
}

static GenericFrame * add_generic_frame (gint id, gint size)
{
    GenericFrame * frame = mowgli_dictionary_retrieve (frames, id3_frames[id]);

    if (frame == NULL)
    {
        frame = g_malloc (sizeof (GenericFrame));
        frame->header = g_malloc (sizeof (ID3v2FrameHeader));
        frame->header->frame_id = g_strdup (id3_frames[id]);
        mowgli_dictionary_add (frames, id3_frames[id], frame);
        mowgli_node_add ((void *) id3_frames[id], mowgli_node_create (),
         frameIDs);
    }
    else
        g_free (frame->frame_body);

    frame->header->size = size;
    frame->header->flags = 0;
    frame->frame_body = g_malloc (size);
    return frame;
}

static void add_text_frame (gint id, const gchar * text)
{
    gint length = strlen (text);
    GenericFrame * frame = add_generic_frame (id, length + 1);

    frame->frame_body[0] = 3; /* UTF-8 encoding */
    memcpy (frame->frame_body + 1, text, length);
}

static void add_comment_frame (const gchar * text)
{
    gint length = strlen (text);
    GenericFrame * frame = add_generic_frame (ID3_COMMENT, length + 5);

    frame->frame_body[0] = 3; /* UTF-8 encoding */
    strcpy (frame->frame_body + 1, "eng"); /* well, it *might* be English */
    memcpy (frame->frame_body + 5, text, length);
}

static void add_frameFromTupleStr (Tuple * tuple, int field, int id3_field)
{
    add_text_frame (id3_field, tuple_get_string (tuple, field, NULL));
}

static void add_frameFromTupleInt (Tuple * tuple, int field, int id3_field)
{
    gchar scratch[16];

    snprintf (scratch, sizeof scratch, "%d", tuple_get_int (tuple, field, NULL));
    add_text_frame (id3_field, scratch);
}

static gboolean id3v2_can_handle_file (VFSFile * handle)
{
    gint version, header_size, data_size, footer_size;
    gsize offset;

    return read_header (handle, & version, & offset, & header_size, & data_size,
     & footer_size);
}

static gboolean id3v2_read_tag (Tuple * tuple, VFSFile * f)
{
    gint version, header_size, data_size, footer_size;
    gsize offset;
    gint pos;

    if (! read_header (f, & version, & offset, & header_size, & data_size,
     & footer_size))
        return FALSE;

    for (pos = 0; pos < data_size; )
    {
        ID3v2FrameHeader * frame = readID3v2FrameHeader (f, version);

        if (frame->size == 0)
        {
            free_frame_header (frame);
            break;
        }

        int id = getFrameID(frame);
        pos = pos + frame->size + 10;

        if (pos > data_size)
        {
            free_frame_header (frame);
            break;
        }

        switch (id)
        {
          case ID3_ALBUM:
              associate_string (tuple, f, FIELD_ALBUM, NULL, frame);
              break;
          case ID3_TITLE:
              associate_string (tuple, f, FIELD_TITLE, NULL, frame);
              break;
          case ID3_COMPOSER:
              associate_string (tuple, f, FIELD_COMPOSER, NULL, frame);
              break;
          case ID3_COPYRIGHT:
              associate_string (tuple, f, FIELD_COPYRIGHT, NULL, frame);
              break;
          case ID3_DATE:
              associate_string (tuple, f, FIELD_DATE, NULL, frame);
              break;
          case ID3_TIME:
              associate_int (tuple, f, FIELD_LENGTH, NULL, frame);
              break;
          case ID3_LENGTH:
              associate_int (tuple, f, FIELD_LENGTH, NULL, frame);
              break;
          case ID3_ARTIST:
              associate_string (tuple, f, FIELD_ARTIST, NULL, frame);
              break;
          case ID3_TRACKNR:
              associate_int (tuple, f, FIELD_TRACK_NUMBER, NULL, frame);
              break;
          case ID3_YEAR:
          case ID3_RECORDING_TIME:
              associate_int (tuple, f, FIELD_YEAR, NULL, frame);
              break;
          case ID3_GENRE:
              tuple = decodeGenre(tuple, f, *frame);
              break;
          case ID3_COMMENT:
              decode_comment (tuple, f, frame);
              break;
          case ID3_PRIVATE:
              decode_private_info (tuple, f, frame);
              break;
          case ID3_ENCODER:
              associate_string (tuple, f, -1, "encoder", frame);
              break;
          case ID3_TXXX:
              decode_txxx (tuple, f, frame);
              break;
          case ID3_RVA2:
              decode_rva2 (tuple, f, frame);
              break;
          default:
              AUDDBG("Skipping %i bytes over unsupported ID3 frame %s\n", frame->size, frame->frame_id);
              skipFrame(f, frame->size);
        }

        free_frame_header (frame);
    }

    return TRUE;
}

static void free_frame_cb (mowgli_dictionary_elem_t * element, void * unused)
{
    free_generic_frame (element->data);
}

static void free_frame_dictionary (void)
{
    mowgli_dictionary_destroy (frames, free_frame_cb, NULL);
    frames = NULL;
}

static gboolean id3v2_write_tag (Tuple * tuple, VFSFile * f)
{
    gint version, header_size, data_size, footer_size;
    gsize offset;

    if (! read_header (f, & version, & offset, & header_size, & data_size,
     & footer_size))
        return FALSE;

    if (frameIDs != NULL)
    {
        mowgli_node_t *n, *tn;
        MOWGLI_LIST_FOREACH_SAFE(n, tn, frameIDs->head)
        {
            mowgli_node_delete(n, frameIDs);
        }
    }
    frameIDs = mowgli_list_create();

    //read all frames into generic frames;
    frames = mowgli_dictionary_create(strcasecmp);
    readAllFrames (f, data_size, version);

    //make the new frames from tuple and replace in the dictionary the old frames with the new ones
    if (tuple_get_string(tuple, FIELD_ARTIST, NULL))
        add_frameFromTupleStr(tuple, FIELD_ARTIST, ID3_ARTIST);

    if (tuple_get_string(tuple, FIELD_TITLE, NULL))
        add_frameFromTupleStr(tuple, FIELD_TITLE, ID3_TITLE);

    if (tuple_get_string(tuple, FIELD_ALBUM, NULL))
        add_frameFromTupleStr(tuple, FIELD_ALBUM, ID3_ALBUM);

    if (tuple_get_string (tuple, FIELD_COMMENT, NULL) != NULL)
        add_comment_frame (tuple_get_string (tuple, FIELD_COMMENT, NULL));

    if (tuple_get_string(tuple, FIELD_GENRE, NULL))
        add_frameFromTupleStr(tuple, FIELD_GENRE, ID3_GENRE);

    if (tuple_get_int(tuple, FIELD_YEAR, NULL) != 0)
        add_frameFromTupleInt(tuple, FIELD_YEAR, ID3_YEAR);

    if (tuple_get_int(tuple, FIELD_TRACK_NUMBER, NULL) != 0)
        add_frameFromTupleInt(tuple, FIELD_TRACK_NUMBER, ID3_TRACKNR);

    if (! offset)
    {
        if (! cut_beginning_tag (f, header_size + data_size + footer_size))
            return FALSE;
    }
    else
    {
        if (offset + header_size + data_size + footer_size != vfs_fsize (f))
            return FALSE;

        if (vfs_ftruncate (f, offset))
            return FALSE;
    }

    offset = vfs_fsize (f);

    if (offset < 0 || vfs_fseek (f, offset, SEEK_SET) || ! write_header (f, 0,
     FALSE))
        return FALSE;

    data_size = writeAllFramesToFile (f);
    free_frame_dictionary ();

    if (! write_header (f, data_size, TRUE) || vfs_fseek (f, offset, SEEK_SET)
     || ! write_header (f, data_size, FALSE))
        return FALSE;

    return TRUE;
}

tag_module_t id3v2 =
{
    .name = "ID3v2",
    .can_handle_file = id3v2_can_handle_file,
    .read_tag = id3v2_read_tag,
    .write_tag = id3v2_write_tag,
};
