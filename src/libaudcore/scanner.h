/*
 * scanner.h
 * Copyright 2012 John Lindgren
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

#ifndef LIBAUDCORE_SCANNER_H
#define LIBAUDCORE_SCANNER_H

#include "index.h"
#include "tuple.h"

#define SCAN_TUPLE (1 << 0)
#define SCAN_IMAGE (1 << 1)

#define SCAN_THREADS 2

struct ScanRequest
{
    typedef void (* Callback) (ScanRequest * request);

    ScanRequest (const char * filename, int flags, Callback callback,
     PluginHandle * decoder = nullptr) :
        filename (filename),
        flags (flags),
        callback (callback),
        decoder (decoder) {}

    const String filename;
    const int flags;
    const Callback callback;

    PluginHandle * decoder;

    Tuple tuple;
    Index<char> image_data;
    String image_file;
    String error;
};

void scanner_init ();
void scanner_request (ScanRequest * request);
void scanner_cleanup ();

#endif
