/*
 * Copyright 2009 Paula Stanciu
 * Copyright 2010 John Lindgren
 * Copyright 2010 Tony Vroon
 * Copyright 2010 William Pitcock
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

#include "id3-common.h"
#include "id3v22.h"
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
    ID3_ENCODER,
    ID3_TXX,
    ID3_RVA,
    ID3_FUCKO_ARTIST,
    ID3_TAGS_NO
};

static const gchar * id3_frames[ID3_TAGS_NO] = {"TAL", "TT2", "TCM", "TCR",
"TDA", "TLE", "TPE", "TRK", "TYE", "TCO", "COM", "TSS", "TXX", "RVA", "TP1"};

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

typedef struct
{
    guchar key[3];
    guchar size[3];
}
ID3v2FrameHeader;
#pragma pack(pop)

typedef struct
{
    gchar key[5];
    guchar * data;
    gint size;
}
GenericFrame;

#define ID3_HEADER_SYNCSAFE             0x40
#define ID3_HEADER_COMPRESSED           0x20

#define TAG_SIZE 1

#define write_syncsafe_int32(x) vfs_fput_be32 (syncsafe32 (x))

static gboolean validate_header (ID3v2Header * header)
{
    if (memcmp (header->magic, "ID3", 3))
        return FALSE;

    if ((header->version != 2))
        return FALSE;

    header->size = unsyncsafe32(GUINT32_FROM_BE(header->size));

    AUDDBG ("Found ID3v2 header:\n");
    AUDDBG (" magic = %.3s\n", header->magic);
    AUDDBG (" version = %d\n", (gint) header->version);
    AUDDBG (" revision = %d\n", (gint) header->revision);
    AUDDBG (" flags = %x\n", (gint) header->flags);
    AUDDBG (" size = %d\n", (gint) header->size);
    return TRUE;
}

static gboolean read_header (VFSFile * handle, gint * version, gboolean *
 syncsafe, gsize * offset, gint * header_size, gint * data_size)
{
    ID3v2Header header;

    if (vfs_fseek (handle, 0, SEEK_SET))
        return FALSE;

    if (vfs_fread (& header, 1, sizeof (ID3v2Header), handle) != sizeof
     (ID3v2Header))
        return FALSE;

    if (validate_header (& header))
    {
        * offset = 0;
        * version = header.version;
        * header_size = sizeof (ID3v2Header);
        * data_size = header.size;
    } else return FALSE;

    * syncsafe = (header.flags & ID3_HEADER_SYNCSAFE) ? TRUE : FALSE;

    AUDDBG ("Offset = %d, header size = %d, data size = %d\n",
     (gint) * offset, * header_size, * data_size);

    return TRUE;
}

static gboolean read_frame (VFSFile * handle, gint max_size, gint version,
 gboolean syncsafe, gint * frame_size, gchar * key, guchar * * data, gint * size)
{
    ID3v2FrameHeader header;
    gint skip = 0, i;
    guint32 hdrsz = 0;

    if ((max_size -= sizeof (ID3v2FrameHeader)) < 0)
        return FALSE;

    if (vfs_fread (& header, 1, sizeof (ID3v2FrameHeader), handle) != sizeof
     (ID3v2FrameHeader))
        return FALSE;

    if (! header.key[0]) /* padding */
        return FALSE;

    for (i = 0; i < 3; i++)
    {
        hdrsz |= (guint32) header.size[i] << ((2 - i) * 8);
        AUDDBG("header.size[%d] = %d hdrsz %d slot %d\n", i, header.size[i], hdrsz, 2 - i);
    }

//    hdrsz = GUINT32_TO_BE(hdrsz);
    if (hdrsz > max_size || hdrsz == 0)
        return FALSE;

    AUDDBG ("Found frame:\n");
    AUDDBG (" key = %.3s\n", header.key);
    AUDDBG (" size = %d\n", (gint) hdrsz);

    * frame_size = sizeof (ID3v2FrameHeader) + hdrsz;
    sprintf (key, "%.3s", header.key);

    if (skip > 0 && vfs_fseek (handle, skip, SEEK_CUR))
        return FALSE;

    * size = hdrsz - skip;
    * data = g_malloc (* size);

    if (vfs_fread (* data, 1, * size, handle) != * size)
        return FALSE;

    AUDDBG ("Data size = %d.\n", * size);
    return TRUE;
}

static gchar * decode_text_frame (const guchar * data, gint size)
{
    return convert_text ((const gchar *) data + 1, size - 1, data[0], FALSE,
     NULL, NULL);
}

static gboolean decode_comment_frame (const guchar * _data, gint size, gchar * *
 lang, gchar * * type, gchar * * value)
{
    const gchar * data = (const gchar *) _data;
    gchar * pair, * sep;
    gint converted;

    pair = convert_text (data + 4, size - 4, data[0], FALSE, & converted, NULL);

    if (pair == NULL || (sep = memchr (pair, 0, converted)) == NULL)
        return FALSE;

    * lang = g_strndup (data + 1, 3);
    * type = g_strdup (pair);
    * value = g_strdup (sep + 1);

    g_free (pair);
    return TRUE;
}

static gint get_frame_id (const gchar * key)
{
    gint id;

    for (id = 0; id < ID3_TAGS_NO; id ++)
    {
        if (! strcmp (key, id3_frames[id]))
            return id;
    }

    return -1;
}

static void associate_string (Tuple * tuple, gint field, const gchar *
 customfield, const guchar * data, gint size)
{
    gchar * text = decode_text_frame (data, size);

    if (text == NULL)
        return;

    if (customfield != NULL)
        AUDDBG ("Custom field %s = %s.\n", customfield, text);
    else
        AUDDBG ("Field %i = %s.\n", field, text);

    tuple_associate_string (tuple, field, customfield, text);
    g_free (text);
}

static void associate_int (Tuple * tuple, gint field, const gchar *
 customfield, const guchar * data, gint size)
{
    gchar * text = decode_text_frame (data, size);

    if (text == NULL)
        return;

    if (customfield != NULL)
        AUDDBG ("Custom field %s = %s.\n", customfield, text);
    else
        AUDDBG ("Field %i = %s.\n", field, text);

    tuple_associate_int (tuple, field, customfield, atoi (text));
    g_free (text);
}

static void decode_comment (Tuple * tuple, const guchar * data, gint size)
{
    gchar * lang, * type, * value;

    if (! decode_comment_frame (data, size, & lang, & type, & value))
        return;

    AUDDBG ("Comment: lang = %s, type = %s, value = %s.\n", lang, type, value);

    if (! type[0]) /* blank type == actual comment */
        tuple_associate_string (tuple, FIELD_COMMENT, NULL, value);

    g_free (lang);
    g_free (type);
    g_free (value);
}

static void decode_txx (Tuple * tuple, const guchar * data, gint size)
{
    gchar * text = decode_text_frame (data, size);

    if (text == NULL)
        return;

    gchar *separator = strchr(text, 0);

    if (separator == NULL)
        return;

    gchar * value = separator + 1;
    AUDDBG ("TXX: %s = %s.\n", text, value);
    tuple_associate_string (tuple, -1, text, value);

    g_free (text);
}

static gboolean decode_rva_block (const guchar * * _data, gint * _size, gint *
 channel, gint * adjustment, gint * adjustment_unit, gint * peak, gint *
 peak_unit)
{
    const guchar * data = * _data;
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

    AUDDBG ("RVA block: channel = %d, adjustment = %d/%d, peak bits = %d\n",
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

        AUDDBG ("RVA block: peak = %d/%d\n", * peak, * peak_unit);
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

static void decode_rva (Tuple * tuple, const guchar * data, gint size)
{
    const gchar * domain;
    gint channel, adjustment, adjustment_unit, peak, peak_unit;

    if (memchr (data, 0, size) == NULL)
        return;

    domain = (const gchar *) data;

    AUDDBG ("RVA domain: %s\n", domain);

    size -= strlen (domain) + 1;
    data += strlen (domain) + 1;

    while (size > 0)
    {
        if (! decode_rva_block (& data, & size, & channel, & adjustment,
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

static void decode_genre (Tuple * tuple, const guchar * data, gint size)
{
    gint numericgenre;
    gchar * text = decode_text_frame (data, size);

    if (text == NULL)
        return;

    if (text[0] == '(')
        numericgenre = atoi (text + 1);
    else
        numericgenre = atoi (text);

    if (numericgenre > 0)
    {
        tuple_associate_string(tuple, FIELD_GENRE, NULL, convert_numericgenre_to_text(numericgenre));
        return;
    }
    tuple_associate_string(tuple, FIELD_GENRE, NULL, text);
    g_free (text);
    return;
}

static gboolean id3v22_can_handle_file (VFSFile * handle)
{
    gint version, header_size, data_size;
    gboolean syncsafe;
    gsize offset;

    return read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size);
}

gboolean id3v22_read_tag (Tuple * tuple, VFSFile * handle)
{
    gint version, header_size, data_size;
    gboolean syncsafe;
    gsize offset;
    gint pos;

    if (! read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size))
        return FALSE;

    AUDDBG("Reading tags from %i bytes of ID3 data in %s\n", data_size, handle->uri);

    for (pos = 0; pos < data_size; )
    {
        gint frame_size, size, id;
        gchar key[5];
        guchar * data;

        if (! read_frame (handle, data_size - pos, version, syncsafe,
         & frame_size, key, & data, & size))
	{
	    AUDDBG("read_frame failed at pos %i\n", pos);
            break;
	}

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
          case ID3_FUCKO_ARTIST:
          case ID3_ARTIST:
            associate_string (tuple, FIELD_ARTIST, NULL, data, size);
            break;
          case ID3_TRACKNR:
            associate_int (tuple, FIELD_TRACK_NUMBER, NULL, data, size);
            break;
          case ID3_YEAR:
            associate_int (tuple, FIELD_YEAR, NULL, data, size);
            break;
          case ID3_GENRE:
            decode_genre (tuple, data, size);
            break;
          case ID3_COMMENT:
            decode_comment (tuple, data, size);
            break;
          case ID3_ENCODER:
            associate_string (tuple, -1, "encoder", data, size);
            break;
          case ID3_TXX:
            decode_txx (tuple, data, size);
            break;
          case ID3_RVA:
            decode_rva (tuple, data, size);
            break;
          default:
            AUDDBG ("Ignoring unsupported ID3 frame %s.\n", key);
            break;
        }

        g_free (data);
        pos += frame_size;
    }

    return TRUE;
}

static gboolean parse_pic (const guchar * data, gint size, gchar * * mime,
 gint * type, void * * image_data, gint * image_size)
{
    const guchar * sep;
    const guchar * after;

    if (size < 2 || (sep = memchr (data + 1, 0, size - 2)) == NULL)
        return FALSE;

    after = sep + 2;
    * mime = g_strdup ((const gchar *) data + 1);
    * type = sep[1];
    * image_data = g_memdup (after, data + size - after);
    * image_size = data + size - after;

    AUDDBG ("PIC: mime = %s, type = %d, size = %d.\n", * mime,
     * type, * image_size);
    return TRUE;
}

static gboolean id3v22_read_image (VFSFile * handle, void * * image_data, gint *
 image_size)
{
    gint version, header_size, data_size, parsed;
    gboolean syncsafe;
    gsize offset;
    gboolean found = FALSE;

    if (! read_header (handle, & version, & syncsafe, & offset, & header_size,
     & data_size))
        return FALSE;

    for (parsed = 0; parsed < data_size && ! found; )
    {
        gint frame_size, size, type;
        gchar key[5];
        guchar * data;
        gchar * mime;
        gint frame_length;

        if (! read_frame (handle, data_size - parsed, version, syncsafe,
         & frame_size, key, & data, & size))
            break;

        frame_length = size;

        if (! strcmp (key, "PIC") && parse_pic (data, frame_length, & mime, & type,
         image_data, image_size))
        {
            g_free (mime);

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

    return found;
}

static gboolean id3v22_write_tag (const Tuple * tuple, VFSFile * f)
{
    return FALSE;
}

tag_module_t id3v22 =
{
    .name = "ID3v2.2",
    .can_handle_file = id3v22_can_handle_file,
    .read_tag = id3v22_read_tag,
    .read_image = id3v22_read_image,
    .write_tag = id3v22_write_tag,
};
