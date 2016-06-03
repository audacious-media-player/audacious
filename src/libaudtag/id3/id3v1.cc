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

#include <stdlib.h>
#include <string.h>

#include <glib.h>  /* for g_strchomp */

#include <libaudcore/audstrings.h>
#include <libaudtag/builtin.h>
#include <libaudtag/util.h>

#pragma pack(push)
#pragma pack(1)

struct ID3v1Tag {
    char header[3];
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    char comment[30];
    unsigned char genre;
};

struct ID3v1Ext {
    char header[4];
    char title[60];
    char artist[60];
    char album[60];
    unsigned char speed;
    char genre[30];
    char start[6];
    char end[6];
};

#pragma pack(pop)

namespace audtag {

static bool read_id3v1_tag (VFSFile & file, ID3v1Tag * tag)
{
    if (file.fseek (-sizeof (ID3v1Tag), VFS_SEEK_END) < 0)
        return false;
    if (file.fread (tag, 1, sizeof (ID3v1Tag)) != sizeof (ID3v1Tag))
        return false;

    return ! strncmp (tag->header, "TAG", 3);
}

static bool read_id3v1_ext (VFSFile & file, ID3v1Ext * ext)
{
    if (file.fseek (-(sizeof (ID3v1Ext) + sizeof (ID3v1Tag)), VFS_SEEK_END) < 0)
        return false;
    if (file.fread (ext, 1, sizeof (ID3v1Ext)) != sizeof (ID3v1Ext))
        return false;

    return ! strncmp (ext->header, "TAG+", 4);
}

bool ID3v1TagModule::can_handle_file (VFSFile & file)
{
    ID3v1Tag tag;
    return read_id3v1_tag (file, & tag);
}

static bool combine_string (Tuple & tuple, Tuple::Field field,
 const char * str1, int size1, const char * str2, int size2)
{
    StringBuf str = str_copy (str1, strlen_bounded (str1, size1));
    str.insert (-1, str2, strlen_bounded (str2, size2));

    g_strchomp (str);
    str.resize (strlen (str));

    if (! str.len ())
        return false;

    tuple.set_str (field, str);
    return true;
}

bool ID3v1TagModule::read_tag (VFSFile & file, Tuple & tuple, Index<char> * image)
{
    ID3v1Tag tag;
    ID3v1Ext ext;

    if (! read_id3v1_tag (file, & tag))
        return false;

    if (! read_id3v1_ext (file, & ext))
        memset (& ext, 0, sizeof (ID3v1Ext));

    combine_string (tuple, Tuple::Title, tag.title, sizeof tag.title, ext.title, sizeof ext.title);
    combine_string (tuple, Tuple::Artist, tag.artist, sizeof tag.artist, ext.artist, sizeof ext.artist);
    combine_string (tuple, Tuple::Album, tag.album, sizeof tag.album, ext.album, sizeof ext.album);
    combine_string (tuple, Tuple::Comment, tag.comment, sizeof tag.comment, nullptr, 0);

    StringBuf year = str_copy (tag.year, strlen_bounded (tag.year, 4));
    if (atoi (year))
        tuple.set_int (Tuple::Year, atoi (year));

    if (! tag.comment[28] && tag.comment[29])
        tuple.set_int (Tuple::Track, (unsigned char) tag.comment[29]);

    if (! combine_string (tuple, Tuple::Genre, ext.genre, sizeof ext.genre, nullptr, 0))
    {
        const char * genre = convert_numericgenre_to_text (tag.genre);
        if (genre)
            tuple.set_str (Tuple::Genre, genre);
    }

    return true;
}

}
