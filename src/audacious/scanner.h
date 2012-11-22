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

#ifndef AUDACIOUS_SCANNER_H
#define AUDACIOUS_SCANNER_H

#include <audacious/types.h>
#include <libaudcore/tuple.h>

#define SCAN_TUPLE (1 << 0)
#define SCAN_IMAGE (1 << 1)

#define SCAN_THREADS 2

struct _ScanRequest;
typedef struct _ScanRequest ScanRequest;

typedef void (* ScanCallback) (ScanRequest * request);

ScanRequest * scan_request (const char * filename, int flags,
 PluginHandle * decoder, ScanCallback callback);

const char * scan_request_get_filename (ScanRequest * request);
PluginHandle * scan_request_get_decoder (ScanRequest * request);
Tuple * scan_request_get_tuple (ScanRequest * request);
void scan_request_get_image_data (ScanRequest * request, void * * data, int64_t * len);
char * scan_request_get_image_file (ScanRequest * request);

void scanner_init (void);
void scanner_cleanup (void);

#endif
