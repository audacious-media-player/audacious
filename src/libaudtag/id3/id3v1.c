/*
 * id3v1.c
 * Copyright 2013 John Lindgren
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <libaudcore/audstrings.h>

#include "../tag_module.h"
#include "../util.h"

#include "id3v1.h"

#pragma pack(push)
#pragma pack(1)

typedef struct {
    char header[3];
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    char comment[30];
    unsigned char genre;
} ID3v1Tag;

typedef struct {
    char header[4];
    char title[60];
    char artist[60];
    char album[60];
    unsigned char speed;
    char genre[30];
    char start[6];
    char end[6];
} ID3v1Ext;

#pragma pack(pop)

static bool_t read_id3v1_tag (VFSFile * file, ID3v1Tag * tag)
{
    if (vfs_fseek (file, -sizeof (ID3v1Tag), SEEK_END) < 0)
        return FALSE;
    if (vfs_fread (tag, 1, sizeof (ID3v1Tag), file) != sizeof (ID3v1Tag))
        return FALSE;

    return ! strncmp (tag->header, "TAG", 3);
}

static bool_t read_id3v1_ext (VFSFile * file, ID3v1Ext * ext)
{
    if (vfs_fseek (file, -(sizeof (ID3v1Ext) + sizeof (ID3v1Tag)), SEEK_END) < 0)
        return FALSE;
    if (vfs_fread (ext, 1, sizeof (ID3v1Ext), file) != sizeof (ID3v1Ext))
        return FALSE;

    return ! strncmp (ext->header, "TAG+", 4);
}

static bool_t id3v1_can_handle_file (VFSFile * file)
{
    ID3v1Tag tag;
    return read_id3v1_tag (file, & tag);
}

static bool_t combine_string (Tuple * tuple, int field, const char * str1,
 int size1, const char * str2, int size2)
{
    char str[size1 + size2 + 1];
    
    memset (str, 0, sizeof str);
    strncpy (str, str1, size1);
    strncpy (str + strlen (str), str2, size2);

    g_strchomp (str);

    if (! str[0])
        return FALSE;

    char * utf8 = str_to_utf8 (str, -1);
    if (! utf8)
        return FALSE;

    tuple_set_str (tuple, field, utf8);

    str_unref (utf8);
    return TRUE;
}

static bool_t id3v1_read_tag (Tuple * tuple, VFSFile * file)
{
    ID3v1Tag tag;
    ID3v1Ext ext;

    if (! read_id3v1_tag (file, & tag))
        return FALSE;

    if (! read_id3v1_ext (file, & ext))
        memset (& ext, 0, sizeof (ID3v1Ext));

    combine_string (tuple, FIELD_TITLE, tag.title, sizeof tag.title, ext.title, sizeof ext.title);
    combine_string (tuple, FIELD_ARTIST, tag.artist, sizeof tag.artist, ext.artist, sizeof ext.artist);
    combine_string (tuple, FIELD_ALBUM, tag.album, sizeof tag.album, ext.album, sizeof ext.album);
    combine_string (tuple, FIELD_COMMENT, tag.comment, sizeof tag.comment, NULL, 0);

    SNCOPY (year, tag.year, 4);
    if (atoi (year))
        tuple_set_int (tuple, FIELD_YEAR, atoi (year));

    if (! tag.comment[28] && tag.comment[29])
        tuple_set_int (tuple, FIELD_TRACK_NUMBER, (unsigned char) tag.comment[29]);

    if (! combine_string (tuple, FIELD_GENRE, ext.genre, sizeof ext.genre, NULL, 0))
        tuple_set_str (tuple, FIELD_GENRE, convert_numericgenre_to_text (tag.genre));

    return TRUE;
}

tag_module_t id3v1 = {
    .name = "ID3v1",
    .can_handle_file = id3v1_can_handle_file,
    .read_tag = id3v1_read_tag,
};
