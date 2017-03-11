/*
 * playlist-data.cc
 * Copyright 2017 John Lindgren
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

#include "playlist-data.h"

#include "runtime.h"
#include "tuple-compiler.h"

TupleCompiler Entry::s_compiler;
bool Entry::s_use_fallbacks = false;

void Entry::update_formatting () // static
{
    s_compiler.compile (aud_get_str (nullptr, "generic_title_format"));
    s_use_fallbacks = aud_get_bool (nullptr, "metadata_fallbacks");
}

void Entry::cleanup () // static
{
    s_compiler.reset ();
}

void Entry::format ()
{
    tuple.delete_fallbacks ();

    if (s_use_fallbacks)
        tuple.generate_fallbacks ();
    else
        tuple.generate_title ();

    s_compiler.format (tuple);
}

void Entry::set_tuple (Tuple && new_tuple)
{
    /* Since 3.8, cuesheet entries are handled differently.  The entry filename
     * points to the .cue file, and the path to the actual audio file is stored
     * in the Tuple::AudioFile.  If Tuple::AudioFile is not set, then assume
     * that the playlist was created by an older version of Audacious, and
     * revert to the former behavior (don't refresh this entry). */
    if (tuple.is_set (Tuple::StartTime) && ! tuple.is_set (Tuple::AudioFile))
        return;

    error = String ();

    if (! new_tuple.valid ())
        new_tuple.set_filename (filename);

    length = aud::max (0, new_tuple.get_int (Tuple::Length));
    tuple = std::move (new_tuple);

    format ();
}

Entry::Entry (PlaylistAddItem && item) :
    filename (item.filename),
    decoder (item.decoder),
    number (-1),
    length (0),
    shuffle_num (0),
    selected (false),
    queued (false)
{
    set_tuple (std::move (item.tuple));
}

Entry::~Entry ()
{
    pl_signal_entry_deleted (this);
}
