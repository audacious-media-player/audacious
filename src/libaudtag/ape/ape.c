/*
 * ape.c
 * Copyright 2010 John Lindgren
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

/* TODO:
 * - Support updating files that have their tag at the beginning?
 */

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libaudcore/vfs.h>

#include "ape.h"

#pragma pack(push) /* must be byte-aligned */
#pragma pack(1)
typedef struct
{
    char magic[8];
    uint32_t version; /* LE */
    uint32_t length; /* LE */
    uint32_t items; /* LE */
    uint32_t flags; /* LE */
    uint64_t reserved;
}
APEHeader;
#pragma pack(pop)

typedef struct
{
    char * key, * value;
}
ValuePair;

#define APE_FLAG_HAS_HEADER (1 << 31)
#define APE_FLAG_HAS_NO_FOOTER (1 << 30)
#define APE_FLAG_IS_HEADER (1 << 29)

static bool_t ape_read_header (VFSFile * handle, APEHeader * header)
{
    if (vfs_fread (header, 1, sizeof (APEHeader), handle) != sizeof (APEHeader))
        return FALSE;

    if (strncmp (header->magic, "APETAGEX", 8))
        return FALSE;

    header->version = GUINT32_FROM_LE (header->version);
    header->length = GUINT32_FROM_LE (header->length);
    header->items = GUINT32_FROM_LE (header->items);
    header->flags = GUINT32_FROM_LE (header->flags);

    if (header->length < sizeof (APEHeader))
        return FALSE;

    return TRUE;
}

static bool_t ape_find_header (VFSFile * handle, APEHeader * header, int *
 start, int * length, int * data_start, int * data_length)
{
    APEHeader secondary;

    if (vfs_fseek (handle, 0, SEEK_SET))
        return FALSE;

    if (ape_read_header (handle, header))
    {
        TAGDBG ("Found header at 0, length = %d, version = %d.\n", (int)
         header->length, (int) header->version);
        * start = 0;
        * length = header->length;
        * data_start = sizeof (APEHeader);
        * data_length = header->length - sizeof (APEHeader);

        if (! (header->flags & APE_FLAG_HAS_HEADER) || ! (header->flags &
         APE_FLAG_IS_HEADER))
        {
            TAGDBG ("Invalid header flags (%u).\n", (unsigned int) header->flags);
            return FALSE;
        }

        if (! (header->flags & APE_FLAG_HAS_NO_FOOTER))
        {
            if (vfs_fseek (handle, header->length, SEEK_CUR))
                return FALSE;

            if (! ape_read_header (handle, & secondary))
            {
                TAGDBG ("Expected footer, but found none.\n");
                return FALSE;
            }

            * length += sizeof (APEHeader);
        }

        return TRUE;
    }

    if (vfs_fseek (handle, -(int) sizeof (APEHeader), SEEK_END))
        return FALSE;

    if (ape_read_header (handle, header))
    {
        TAGDBG ("Found footer at %d, length = %d, version = %d.\n", (int)
         vfs_ftell (handle) - (int) sizeof (APEHeader), (int) header->length,
         (int) header->version);
        * start = vfs_ftell (handle) - header->length;
        * length = header->length;
        * data_start = vfs_ftell (handle) - header->length;
        * data_length = header->length - sizeof (APEHeader);

        if ((header->flags & APE_FLAG_HAS_NO_FOOTER) || (header->flags &
         APE_FLAG_IS_HEADER))
        {
            TAGDBG ("Invalid footer flags (%u).\n", (unsigned int) header->flags);
            return FALSE;
        }

        if (header->flags & APE_FLAG_HAS_HEADER)
        {
            if (vfs_fseek (handle, -(int) header->length - sizeof (APEHeader),
             SEEK_CUR))
                return FALSE;

            if (! ape_read_header (handle, & secondary))
            {
                TAGDBG ("Expected header, but found none.\n");
                return FALSE;
            }

            * start -= sizeof (APEHeader);
            * length += sizeof (APEHeader);
        }

        return TRUE;
    }

    TAGDBG ("No header found.\n");
    return FALSE;
}

static bool_t ape_is_our_file (VFSFile * handle)
{
    APEHeader header;
    int start, length, data_start, data_length;

    return ape_find_header (handle, & header, & start, & length, & data_start,
     & data_length);
}

static ValuePair * ape_read_item (void * * data, int length)
{
    uint32_t * header = * data;
    char * value;
    ValuePair * pair;

    if (length < 8)
    {
        TAGDBG ("Expected item, but only %d bytes remain in tag.\n", length);
        return NULL;
    }

    value = memchr ((char *) (* data) + 8, 0, length - 8);

    if (value == NULL)
    {
        TAGDBG ("Unterminated item key (max length = %d).\n", length - 8);
        return NULL;
    }

    value ++;

    if (header[0] > (char *) (* data) + length - value)
    {
        TAGDBG ("Item value of length %d, but only %d bytes remain in tag.\n",
         (int) header[0], (int) ((char *) (* data) + length - value));
        return NULL;
    }

    pair = g_malloc (sizeof (ValuePair));
    pair->key = g_strdup ((char *) (* data) + 8);
    pair->value = g_strndup (value, header[0]);

    * data = value + header[0];

    return pair;
}

static GList * ape_read_items (VFSFile * handle)
{
    GList * list = NULL;
    APEHeader header;
    int start, length, data_start, data_length;
    void * data, * item;

    if (! ape_find_header (handle, & header, & start, & length, & data_start,
     & data_length))
        return NULL;

    if (vfs_fseek (handle, data_start, SEEK_SET))
        return NULL;

    data = g_malloc (data_length);

    if (vfs_fread (data, 1, data_length, handle) != data_length)
    {
        g_free (data);
        return NULL;
    }

    TAGDBG ("Reading %d items:\n", header.items);
    item = data;

    while (header.items --)
    {
        ValuePair * pair = ape_read_item (& item, (char *) data + data_length -
         (char *) item);

        if (pair == NULL)
            break;

        TAGDBG ("Read: %s = %s.\n", pair->key, pair->value);
        list = g_list_prepend (list, pair);
    }

    g_free (data);
    return g_list_reverse (list);
}

static void free_tag_list (GList * list)
{
    while (list != NULL)
    {
        g_free (((ValuePair *) list->data)->key);
        g_free (((ValuePair *) list->data)->value);
        g_free (list->data);
        list = g_list_delete_link (list, list);
    }
}

static void parse_gain_text (const char * text, int * value, int * unit)
{
    int sign = 1;

    * value = 0;
    * unit = 1;

    if (* text == '-')
    {
        sign = -1;
        text ++;
    }

    while (* text >= '0' && * text <= '9')
    {
        * value = * value * 10 + (* text - '0');
        text ++;
    }

    if (* text == '.')
    {
        text ++;

        while (* text >= '0' && * text <= '9' && * value < G_MAXINT / 10)
        {
            * value = * value * 10 + (* text - '0');
            * unit = * unit * 10;
            text ++;
        }
    }

    * value = * value * sign;
}

static void set_gain_info (Tuple * tuple, int field, int unit_field,
 const char * text)
{
    int value, unit;

    parse_gain_text (text, & value, & unit);

    if (tuple_get_value_type (tuple, unit_field, NULL) == TUPLE_INT)
        value = value * (int64_t) tuple_get_int (tuple, unit_field, NULL) / unit;
    else
        tuple_set_int (tuple, unit_field, NULL, unit);

    tuple_set_int (tuple, field, NULL, value);
}

static bool_t ape_read_tag (Tuple * tuple, VFSFile * handle)
{
    GList * list = ape_read_items (handle), * node;

    for (node = list; node != NULL; node = node->next)
    {
        char * key = ((ValuePair *) node->data)->key;
        char * value = ((ValuePair *) node->data)->value;

        if (! strcmp (key, "Artist"))
            tuple_set_str (tuple, FIELD_ARTIST, NULL, value);
        else if (! strcmp (key, "Title"))
            tuple_set_str (tuple, FIELD_TITLE, NULL, value);
        else if (! strcmp (key, "Album"))
            tuple_set_str (tuple, FIELD_ALBUM, NULL, value);
        else if (! strcmp (key, "Comment"))
            tuple_set_str (tuple, FIELD_COMMENT, NULL, value);
        else if (! strcmp (key, "Genre"))
            tuple_set_str (tuple, FIELD_GENRE, NULL, value);
        else if (! strcmp (key, "Track"))
            tuple_set_int (tuple, FIELD_TRACK_NUMBER, NULL, atoi (value));
        else if (! strcmp (key, "Year"))
            tuple_set_int (tuple, FIELD_YEAR, NULL, atoi (value));
        else if (! g_ascii_strcasecmp (key, "REPLAYGAIN_TRACK_GAIN"))
            set_gain_info (tuple, FIELD_GAIN_TRACK_GAIN, FIELD_GAIN_GAIN_UNIT,
             value);
        else if (! g_ascii_strcasecmp (key, "REPLAYGAIN_TRACK_PEAK"))
            set_gain_info (tuple, FIELD_GAIN_TRACK_PEAK, FIELD_GAIN_PEAK_UNIT,
             value);
        else if (! g_ascii_strcasecmp (key, "REPLAYGAIN_ALBUM_GAIN"))
            set_gain_info (tuple, FIELD_GAIN_ALBUM_GAIN, FIELD_GAIN_GAIN_UNIT,
             value);
        else if (! g_ascii_strcasecmp (key, "REPLAYGAIN_ALBUM_PEAK"))
            set_gain_info (tuple, FIELD_GAIN_ALBUM_PEAK, FIELD_GAIN_PEAK_UNIT,
             value);
    }

    free_tag_list (list);
    return TRUE;
}

static bool_t ape_write_item (VFSFile * handle, const char * key,
 const char * value, int * written_length)
{
    int key_len = strlen (key) + 1;
    int value_len = strlen (value);
    uint32_t header[2];

    TAGDBG ("Write: %s = %s.\n", key, value);

    header[0] = GUINT32_TO_LE (value_len);
    header[1] = 0;

    if (vfs_fwrite (header, 1, 8, handle) != 8)
        return FALSE;

    if (vfs_fwrite (key, 1, key_len, handle) != key_len)
        return FALSE;

    if (vfs_fwrite (value, 1, value_len, handle) != value_len)
        return FALSE;

    * written_length += 8 + key_len + value_len;
    return TRUE;
}

static bool_t write_string_item (const Tuple * tuple, int field, VFSFile *
 handle, const char * key, int * written_length, int * written_items)
{
    char * value = tuple_get_str (tuple, field, NULL);

    if (value == NULL)
        return TRUE;

    bool_t success = ape_write_item (handle, key, value, written_length);

    if (success)
        (* written_items) ++;

    str_unref (value);
    return success;
}

static bool_t write_integer_item (const Tuple * tuple, int field, VFSFile *
 handle, const char * key, int * written_length, int * written_items)
{
    int value = tuple_get_int (tuple, field, NULL);
    char scratch[32];

    if (! value)
        return TRUE;

    snprintf (scratch, sizeof scratch, "%d", value);

    if (! ape_write_item (handle, key, scratch, written_length))
        return FALSE;

    (* written_items) ++;
    return TRUE;
}

static bool_t write_header (int data_length, int items, bool_t is_header,
 VFSFile * handle)
{
    APEHeader header;

    memcpy (header.magic, "APETAGEX", 8);
    header.version = GUINT32_TO_LE (2000);
    header.length = GUINT32_TO_LE (data_length + sizeof (APEHeader));
    header.items = GUINT32_TO_LE (items);
    header.flags = is_header ? GUINT32_TO_LE (APE_FLAG_HAS_HEADER |
     APE_FLAG_IS_HEADER) : GUINT32_TO_LE (APE_FLAG_HAS_HEADER);
    header.reserved = 0;

    return vfs_fwrite (& header, 1, sizeof (APEHeader), handle) == sizeof
     (APEHeader);
}

static bool_t ape_write_tag (const Tuple * tuple, VFSFile * handle)
{
    GList * list = ape_read_items (handle), * node;
    APEHeader header;
    int start, length, data_start, data_length, items;

    if (ape_find_header (handle, & header, & start, & length, & data_start,
     & data_length))
    {
        if (start + length != vfs_fsize (handle))
        {
            TAGDBG ("Writing tags is only supported at end of file.\n");
            goto ERR;
        }

        if (vfs_ftruncate (handle, start))
            goto ERR;
    }
    else
    {
        start = vfs_fsize (handle);

        if (start < 0)
            goto ERR;
    }

    if (vfs_fseek (handle, start, SEEK_SET) || ! write_header (0, 0, TRUE,
     handle))
        goto ERR;

    length = 0;
    items = 0;

    if (! write_string_item (tuple, FIELD_ARTIST, handle, "Artist", & length,
     & items) || ! write_string_item (tuple, FIELD_TITLE, handle, "Title",
     & length, & items) || ! write_string_item (tuple, FIELD_ALBUM, handle,
     "Album", & length, & items) || ! write_string_item (tuple, FIELD_COMMENT,
     handle, "Comment", & length, & items) || ! write_string_item (tuple,
     FIELD_GENRE, handle, "Genre", & length, & items) || ! write_integer_item
     (tuple, FIELD_TRACK_NUMBER, handle, "Track", & length, & items) ||
     ! write_integer_item (tuple, FIELD_YEAR, handle, "Year", & length, & items))
        goto ERR;

    for (node = list; node != NULL; node = node->next)
    {
        char * key = ((ValuePair *) node->data)->key;
        char * value = ((ValuePair *) node->data)->value;

        if (! strcmp (key, "Artist") || ! strcmp (key, "Title") || ! strcmp
         (key, "Album") || ! strcmp (key, "Comment") || ! strcmp (key, "Genre")
         || ! strcmp (key, "Track") || ! strcmp (key, "Year"))
            continue;

        if (! ape_write_item (handle, key, value, & length))
            goto ERR;

        items ++;
    }

    TAGDBG ("Wrote %d items, %d bytes.\n", items, length);

    if (! write_header (length, items, FALSE, handle) || vfs_fseek (handle,
     start, SEEK_SET) || ! write_header (length, items, TRUE, handle))
        goto ERR;

    free_tag_list (list);
    return TRUE;

ERR:
    free_tag_list (list);
    return FALSE;
}

tag_module_t ape =
{
    .name = "APE",
    .type = TAG_TYPE_APE,
    .can_handle_file = ape_is_our_file,
    .read_tag = ape_read_tag,
    .write_tag = ape_write_tag,
};
