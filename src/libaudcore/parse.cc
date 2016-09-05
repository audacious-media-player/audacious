/*
 * parse.cc
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

#include "parse.h"
#include <string.h>

void TextParser::next ()
{
    m_val = nullptr;

    if (! fgets (m_key, sizeof m_key, m_file))
        return;

    char * space = strchr (m_key, ' ');
    if (! space)
        return;

    * space = 0;
    m_val = space + 1;

    char * newline = strchr (m_val, '\n');
    if (newline)
        * newline = 0;
}

bool TextParser::get_int (const char * key, int & val) const
{
    return (m_val && ! strcmp (m_key, key) && sscanf (m_val, "%d", & val) == 1);
}

String TextParser::get_str (const char * key) const
{
    return (m_val && ! strcmp (m_key, key)) ? String (m_val) : String ();
}

