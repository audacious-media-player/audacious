/*
 * id3-common.h
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

#ifndef AUDTAG_ID3_COMMON_H
#define AUDTAG_ID3_COMMON_H

#include <libaudcore/index.h>
#include <libaudcore/tuple.h>

void id3_associate_string (Tuple & tuple, Tuple::Field field, const char * data, int size);
void id3_associate_int (Tuple & tuple, Tuple::Field field, const char * data, int size);
void id3_associate_length (Tuple & tuple, const char * data, int size);
void id3_decode_genre (Tuple & tuple, const char * data, int size);
void id3_decode_comment (Tuple & tuple, const char * data, int size);
void id3_decode_rva (Tuple & tuple, const char * data, int size);
void id3_decode_txxx (Tuple & tuple, const char * data, int size);

Index<char> id3_decode_picture (const char * data, int size);

#endif
