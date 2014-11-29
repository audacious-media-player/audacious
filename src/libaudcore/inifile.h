/*
 * inifile.h
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

#ifndef LIBAUDCORE_INIFILE_H
#define LIBAUDCORE_INIFILE_H

class VFSFile;

class IniParser
{
public:
    virtual ~IniParser () {}

    void parse (VFSFile & file);

protected:
    virtual void handle_heading (const char * heading) = 0;
    virtual void handle_entry (const char * key, const char * value) = 0;
};

bool inifile_write_heading (VFSFile & file, const char * heading)
 __attribute__ ((warn_unused_result));
bool inifile_write_entry (VFSFile & file, const char * key, const char * value)
 __attribute__ ((warn_unused_result));

#endif /* LIBAUDCORE_INIFILE_H */
