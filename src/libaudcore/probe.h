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

/* Gets album art for <file> (the URI of a song file) as JPEG or PNG data.  If
 * the album art is not yet loaded, sets *queued to true, returns nullptr, and
 * begins to load the album art in the background.  On completion, the "art
 * ready" hook is called, with <file> as a parameter.  If no album art could be
 * loaded, sets *queued to false and returns nullptr.
 *
 * If you only want to display album art for the currently playing song, you can
 * call this function from the "playback ready" hook without any need for a
 * separate "art ready" handler, since the album art is already at that point. */
const Index<char> * aud_art_request_data (const char * file, bool * queued = nullptr);

/* Similar to art_request_data() but returns the URI of an image file.
 * (A temporary file will be created if necessary.) */
const char * aud_art_request_file (const char * file, bool * queued = nullptr);

/* Releases album art returned by art_request_data() or art_request_file(). */
void aud_art_unref (const char * file);

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
