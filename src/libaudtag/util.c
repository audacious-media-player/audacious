/*
 * util.c
 * Copyright 2009-2014 Paula Stanciu, Tony Vroon, and John Lindgren
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
#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <libaudcore/audstrings.h>

#include "util.h"

const char *convert_numericgenre_to_text(int numericgenre)
{
    const struct
    {
        int numericgenre;
        const char *genre;
    }
    table[] =
    {
        {GENRE_BLUES, "Blues"},
        {GENRE_CLASSIC_ROCK, "Classic Rock"},
        {GENRE_COUNTRY, "Country"},
        {GENRE_DANCE, "Dance"},
        {GENRE_DISCO, "Disco"},
        {GENRE_FUNK, "Funk"},
        {GENRE_GRUNGE, "Grunge"},
        {GENRE_HIPHOP, "Hip-Hop"},
        {GENRE_JAZZ, "Jazz"},
        {GENRE_METAL, "Metal"},
        {GENRE_NEW_AGE, "New Age"},
        {GENRE_OLDIES, "Oldies"},
        {GENRE_OTHER, "Other"},
        {GENRE_POP, "Pop"},
        {GENRE_R_B, "R&B"},
        {GENRE_RAP, "Rap"},
        {GENRE_REGGAE, "Reggae"},
        {GENRE_ROCK, "Rock"},
        {GENRE_TECHNO, "Techno"},
        {GENRE_INDUSTRIAL, "Industrial"},
        {GENRE_ALTERNATIVE, "Alternative"},
        {GENRE_SKA, "Ska"},
        {GENRE_DEATH_METAL, "Death Metal"},
        {GENRE_PRANKS, "Pranks"},
        {GENRE_SOUNDTRACK, "Soundtrack"},
        {GENRE_EURO_TECHNO, "Euro-Techno"},
        {GENRE_AMBIENT, "Ambient"},
        {GENRE_TRIP_HOP, "Trip-Hop"},
        {GENRE_VOCAL, "Vocal"},
        {GENRE_JAZZ_FUNK, "Jazz+Funk"},
        {GENRE_FUSION, "Fusion"},
        {GENRE_TRANCE, "Trance"},
        {GENRE_CLASSICAL, "Classical"},
        {GENRE_INSTRUMENTAL, "Instrumental"},
        {GENRE_ACID, "Acid"},
        {GENRE_HOUSE, "House"},
        {GENRE_GAME, "Game"},
        {GENRE_SOUND_CLIP, "Sound Clip"},
        {GENRE_GOSPEL, "Gospel"},
        {GENRE_NOISE, "Noise"},
        {GENRE_ALTERNROCK, "AlternRock"},
        {GENRE_BASS, "Bass"},
        {GENRE_SOUL, "Soul"},
        {GENRE_PUNK, "Punk"},
        {GENRE_SPACE, "Space"},
        {GENRE_MEDITATIVE, "Meditative"},
        {GENRE_INSTRUMENTAL_POP, "Instrumental Pop"},
        {GENRE_INSTRUMENTAL_ROCK, "Instrumental Rock"},
        {GENRE_ETHNIC, "Ethnic"},
        {GENRE_GOTHIC, "Gothic"},
        {GENRE_DARKWAVE, "Darkwave"},
        {GENRE_TECHNO_INDUSTRIAL, "Techno-Industrial"},
        {GENRE_ELECTRONIC, "Electronic"},
        {GENRE_POP_FOLK, "Pop-Folk"},
        {GENRE_EURODANCE, "Eurodance"},
        {GENRE_DREAM, "Dream"},
        {GENRE_SOUTHERN_ROCK, "Southern Rock"},
        {GENRE_COMEDY, "Comedy"},
        {GENRE_CULT, "Cult"},
        {GENRE_GANGSTA, "Gangsta"},
        {GENRE_TOP40, "Top 40"},
        {GENRE_CHRISTIAN_RAP, "Christian Rap"},
        {GENRE_POP_FUNK, "Pop/Funk"},
        {GENRE_JUNGLE, "Jungle"},
        {GENRE_NATIVE_AMERICAN, "Native American"},
        {GENRE_CABARET, "Cabaret"},
        {GENRE_NEW_WAVE, "New Wave"},
        {GENRE_PSYCHEDELIC, "Psychedelic"},
        {GENRE_RAVE, "Rave"},
        {GENRE_SHOWTUNES, "Showtunes"},
        {GENRE_TRAILER, "Trailer"},
        {GENRE_LO_FI, "Lo-Fi"},
        {GENRE_TRIBAL, "Tribal"},
        {GENRE_ACID_PUNK, "Acid Punk"},
        {GENRE_ACID_JAZZ, "Acid Jazz"},
        {GENRE_POLKA, "Polka"},
        {GENRE_RETRO, "Retro"},
        {GENRE_MUSICAL, "Musical"},
        {GENRE_ROCK_ROLL, "Rock & Roll"},
        {GENRE_HARD_ROCK, "Hard Rock"},
        {GENRE_FOLK, "Folk"},
        {GENRE_FOLK_ROCK, "Folk-Rock"},
        {GENRE_NATIONAL_FOLK, "National Folk"},
        {GENRE_SWING, "Swing"},
        {GENRE_FAST_FUSION, "Fast Fusion"},
        {GENRE_BEBOB, "Bebob"},
        {GENRE_LATIN, "Latin"},
        {GENRE_REVIVAL, "Revival"},
        {GENRE_CELTIC, "Celtic"},
        {GENRE_BLUEGRASS, "Bluegrass"},
        {GENRE_AVANTGARDE, "Avantgarde"},
        {GENRE_GOTHIC_ROCK, "Gothic Rock"},
        {GENRE_PROGRESSIVE_ROCK, "Progressive Rock"},
        {GENRE_PSYCHEDELIC_ROCK, "Psychedelic Rock"},
        {GENRE_SYMPHONIC_ROCK, "Symphonic Rock"},
        {GENRE_SLOW_ROCK, "Slow Rock"},
        {GENRE_BIG_BAND, "Big Band"},
        {GENRE_CHORUS, "Chorus"},
        {GENRE_EASY_LISTENING, "Easy Listening"},
        {GENRE_ACOUSTIC, "Acoustic"},
        {GENRE_HUMOUR, "Humour"},
        {GENRE_SPEECH, "Speech"},
        {GENRE_CHANSON, "Chanson"},
        {GENRE_OPERA, "Opera"},
        {GENRE_CHAMBER_MUSIC, "Chamber Music"},
        {GENRE_SONATA, "Sonata"},
        {GENRE_SYMPHONY, "Symphony"},
        {GENRE_BOOTY_BASS, "Booty Bass"},
        {GENRE_PRIMUS, "Primus"},
        {GENRE_PORN_GROOVE, "Porn Groove"},
        {GENRE_SATIRE, "Satire"},
        {GENRE_SLOW_JAM, "Slow Jam"},
        {GENRE_CLUB, "Club"},
        {GENRE_TANGO, "Tango"},
        {GENRE_SAMBA, "Samba"},
        {GENRE_FOLKLORE, "Folklore"},
        {GENRE_BALLAD, "Ballad"},
        {GENRE_POWER_BALLAD, "Power Ballad"},
        {GENRE_RHYTHMIC_SOUL, "Rhythmic Soul"},
        {GENRE_FREESTYLE, "Freestyle"},
        {GENRE_DUET, "Duet"},
        {GENRE_PUNK_ROCK, "Punk Rock"},
        {GENRE_DRUM_SOLO, "Drum Solo"},
        {GENRE_A_CAPELLA, "A capella"},
        {GENRE_EURO_HOUSE, "Euro-House"},
    };

    int count;

    for (count = 0; count < ARRAY_LEN (table); count++)
    {
        if (table[count].numericgenre == numericgenre)
            return table[count].genre;
    }

    return "Unknown";
}

uint32_t unsyncsafe32 (uint32_t x)
{
    return (x & 0x7f) | ((x & 0x7f00) >> 1) | ((x & 0x7f0000) >> 2) | ((x &
     0x7f000000) >> 3);
}

uint32_t syncsafe32 (uint32_t x)
{
    return (x & 0x7f) | ((x & 0x3f80) << 1) | ((x & 0x1fc000) << 2) | ((x &
     0xfe00000) << 3);
}

bool_t open_temp_file_for (TempFile * temp, VFSFile * file)
{
    char * template = filename_build (g_get_tmp_dir (), "audacious-temp-XXXXXX");
    SCOPY (tempname, template);
    str_unref (template);

    temp->fd = g_mkstemp (tempname);
    if (temp->fd < 0)
        return FALSE;

    temp->name = str_get (tempname);

    return TRUE;
}

bool_t copy_region_to_temp_file (TempFile * temp, VFSFile * file, int64_t offset, int64_t size)
{
    if (vfs_fseek (file, offset, SEEK_SET) < 0)
        return FALSE;

    char buf[16384];

    while (size < 0 || size > 0)
    {
        int64_t readsize;

        if (size > 0)
        {
            readsize = MIN (size, sizeof buf);
            if (vfs_fread (buf, 1, readsize, file) != readsize)
                return FALSE;

            size -= readsize;
        }
        else
        {
            /* negative size means copy to EOF */
            readsize = vfs_fread (buf, 1, sizeof buf, file);
            if (! readsize)
                break;
        }

        int64_t written = 0;
        while (written < readsize)
        {
            int64_t writesize = write (temp->fd, buf + written, readsize - written);
            if (writesize <= 0)
                return FALSE;

            written += writesize;
        }
    }

    return TRUE;
}

bool_t replace_with_temp_file (TempFile * temp, VFSFile * file)
{
    if (lseek (temp->fd, 0, SEEK_SET) < 0)
        return FALSE;

    if (vfs_fseek (file, 0, SEEK_SET) < 0)
        return FALSE;

    if (vfs_ftruncate (file, 0) < 0)
        return FALSE;

    char buf[16384];

    while (1)
    {
        int64_t readsize = read (temp->fd, buf, sizeof buf);
        if (readsize < 0)
            return FALSE;

        if (readsize == 0)
            break;

        if (vfs_fwrite (buf, 1, readsize, file) != readsize)
            return FALSE;
    }

    close (temp->fd);
    g_unlink (temp->name);

    return TRUE;
}
