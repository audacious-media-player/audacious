/*
 * parse.h
 * Copyright 2016 John Lindgren
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

#ifndef LIBAUDCORE_PARSE_H
#define LIBAUDCORE_PARSE_H

#include <stdio.h>
#include "objects.h"

// simplistic key-value file parser
// used for playlist-state and plugin-registry files
class TextParser
{
public:
    TextParser (FILE * file) :
        m_file (file)
        { next (); }

    void next ();
    bool eof () const
        { return ! m_val; }

    bool get_int (const char * key, int & val) const;
    String get_str (const char * key) const;

private:
    FILE * m_file;
    char * m_val;
    char m_key[512];
};

#endif // LIBAUDCORE_PARSE_H
