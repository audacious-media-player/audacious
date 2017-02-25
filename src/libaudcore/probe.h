/*
 * probe.h
 * Copyright 2014 John Lindgren
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

#ifndef LIBAUDCORE_PROBE_H
#define LIBAUDCORE_PROBE_H

#include <libaudcore/index.h>
#include <libaudcore/objects.h>

class PluginHandle;
class Tuple;
class VFSFile;

/* ====== ALBUM ART API ====== */

/* request format */
enum {
    AUD_ART_DATA = (1 << 0),  /* image data in memory */
    AUD_ART_FILE = (1 << 1)   /* filename of image data on disk */
};

/* opaque type storing art data */
struct AudArtItem;

/* don't use these directly, use AudArtPtr */
const Index<char> * aud_art_data (const AudArtItem * item);
const char * aud_art_file (const AudArtItem * item);
void aud_art_unref (AudArtItem * item);

/* handle for accessing/tracking album art data */
class AudArtPtr : public SmartPtr<AudArtItem, aud_art_unref>
{
public:
    AudArtPtr () : SmartPtr () {}
    explicit AudArtPtr (AudArtItem * ptr) : SmartPtr (ptr) {}

    const Index<char> * data () const
        { return get () ? aud_art_data (get ()) : nullptr; }
    const char * file () const
        { return get () ? aud_art_file (get ()) : nullptr; }
};

/*
 * Gets album art for <file> (the URI of a song file).  The data will be
 * returned in the requested <format> (AUD_ART_DATA or AUD_ART_FILE).
 *
 * This is a non-blocking call.  If the data is not yet loaded, it sets *queued
 * to true, returns a null pointer, and begins to load the data in the back-
 * ground.  On completion, the "art ready" hook is called, with <file> as a
 * parameter.  The data can then be requested again from within the hook.
 *
 * As a special case, album art data for the currently playing song is preloaded
 * by the time the "playback ready" hook is called, so in that case there is no
 * need to implement a separate "art ready" handler.
 *
 * On error, a null pointer is returned and *queued is set to false.
 */
AudArtPtr aud_art_request (const char * file, int format, bool * queued = nullptr);

/* ====== GENERAL PROBING API ====== */

/* The following two functions take an additional VFSFile parameter to allow
 * opening a file, probing for a decoder, and then reading the song metadata
 * without opening the file a second time.  If you don't already have a file
 * handle open, then just pass in a null VFSFile and it will be opened for you. */
PluginHandle * aud_file_find_decoder (const char * filename, bool fast,
 VFSFile & file, String * error = nullptr);
bool aud_file_read_tag (const char * filename, PluginHandle * decoder,
 VFSFile & file, Tuple & tuple, Index<char> * image = nullptr,
 String * error = nullptr);

bool aud_file_can_write_tuple (const char * filename, PluginHandle * decoder);
bool aud_file_write_tuple (const char * filename, PluginHandle * decoder, const Tuple & tuple);
bool aud_custom_infowin (const char * filename, PluginHandle * decoder);

#endif
