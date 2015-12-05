/*
 * util.h
 * Copyright 2009-2010 Paula Stanciu and John Lindgren
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

#ifndef TAGUTIL_H
#define TAGUTIL_H

#include <stdint.h>

enum {
    GENRE_BLUES = 0,
    GENRE_CLASSIC_ROCK,
    GENRE_COUNTRY,
    GENRE_DANCE,
    GENRE_DISCO,
    GENRE_FUNK,
    GENRE_GRUNGE,
    GENRE_HIPHOP,
    GENRE_JAZZ,
    GENRE_METAL,
    GENRE_NEW_AGE,
    GENRE_OLDIES,
    GENRE_OTHER,
    GENRE_POP,
    GENRE_R_B,
    GENRE_RAP,
    GENRE_REGGAE,
    GENRE_ROCK,
    GENRE_TECHNO,
    GENRE_INDUSTRIAL,
    GENRE_ALTERNATIVE,
    GENRE_SKA,
    GENRE_DEATH_METAL,
    GENRE_PRANKS,
    GENRE_SOUNDTRACK,
    GENRE_EURO_TECHNO,
    GENRE_AMBIENT,
    GENRE_TRIP_HOP,
    GENRE_VOCAL,
    GENRE_JAZZ_FUNK,
    GENRE_FUSION,
    GENRE_TRANCE,
    GENRE_CLASSICAL,
    GENRE_INSTRUMENTAL,
    GENRE_ACID,
    GENRE_HOUSE,
    GENRE_GAME,
    GENRE_SOUND_CLIP,
    GENRE_GOSPEL,
    GENRE_NOISE,
    GENRE_ALTERNROCK,
    GENRE_BASS,
    GENRE_SOUL,
    GENRE_PUNK,
    GENRE_SPACE,
    GENRE_MEDITATIVE,
    GENRE_INSTRUMENTAL_POP,
    GENRE_INSTRUMENTAL_ROCK,
    GENRE_ETHNIC,
    GENRE_GOTHIC,
    GENRE_DARKWAVE,
    GENRE_TECHNO_INDUSTRIAL,
    GENRE_ELECTRONIC,
    GENRE_POP_FOLK,
    GENRE_EURODANCE,
    GENRE_DREAM,
    GENRE_SOUTHERN_ROCK,
    GENRE_COMEDY,
    GENRE_CULT,
    GENRE_GANGSTA,
    GENRE_TOP40,
    GENRE_CHRISTIAN_RAP,
    GENRE_POP_FUNK,
    GENRE_JUNGLE,
    GENRE_NATIVE_AMERICAN,
    GENRE_CABARET,
    GENRE_NEW_WAVE,
    GENRE_PSYCHEDELIC,
    GENRE_RAVE,
    GENRE_SHOWTUNES,
    GENRE_TRAILER,
    GENRE_LO_FI,
    GENRE_TRIBAL,
    GENRE_ACID_PUNK,
    GENRE_ACID_JAZZ,
    GENRE_POLKA,
    GENRE_RETRO,
    GENRE_MUSICAL,
    GENRE_ROCK_ROLL,
    GENRE_HARD_ROCK,
    GENRE_FOLK,
    GENRE_FOLK_ROCK,
    GENRE_NATIONAL_FOLK,
    GENRE_SWING,
    GENRE_FAST_FUSION,
    GENRE_BEBOB,
    GENRE_LATIN,
    GENRE_REVIVAL,
    GENRE_CELTIC,
    GENRE_BLUEGRASS,
    GENRE_AVANTGARDE,
    GENRE_GOTHIC_ROCK,
    GENRE_PROGRESSIVE_ROCK,
    GENRE_PSYCHEDELIC_ROCK,
    GENRE_SYMPHONIC_ROCK,
    GENRE_SLOW_ROCK,
    GENRE_BIG_BAND,
    GENRE_CHORUS,
    GENRE_EASY_LISTENING,
    GENRE_ACOUSTIC,
    GENRE_HUMOUR,
    GENRE_SPEECH,
    GENRE_CHANSON,
    GENRE_OPERA,
    GENRE_CHAMBER_MUSIC,
    GENRE_SONATA,
    GENRE_SYMPHONY,
    GENRE_BOOTY_BASS,
    GENRE_PRIMUS,
    GENRE_PORN_GROOVE,
    GENRE_SATIRE,
    GENRE_SLOW_JAM,
    GENRE_CLUB,
    GENRE_TANGO,
    GENRE_SAMBA,
    GENRE_FOLKLORE,
    GENRE_BALLAD,
    GENRE_POWER_BALLAD,
    GENRE_RHYTHMIC_SOUL,
    GENRE_FREESTYLE,
    GENRE_DUET,
    GENRE_PUNK_ROCK,
    GENRE_DRUM_SOLO,
    GENRE_A_CAPELLA,
    GENRE_EURO_HOUSE
};

const char *convert_numericgenre_to_text(int numericgenre);

uint32_t unsyncsafe32 (uint32_t x);
uint32_t syncsafe32 (uint32_t x);

#endif /* TAGUTIL_H */
