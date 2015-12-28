/*
 * vfs_local.h
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

#ifndef LIBAUDCORE_VFS_LOCAL_H
#define LIBAUDCORE_VFS_LOCAL_H

#include "plugin.h"

class LocalTransport : public TransportPlugin
{
public:
    constexpr LocalTransport () : TransportPlugin (PluginInfo (), nullptr) {}

    VFSImpl * fopen (const char * filename, const char * mode, String & error);
    VFSFileTest test_file (const char * filename, VFSFileTest test, String & error);
    Index<String> read_folder (const char * filename, String & error);
};

class StdinTransport : public TransportPlugin
{
public:
    constexpr StdinTransport () : TransportPlugin (PluginInfo (), nullptr) {}

    VFSImpl * fopen (const char * filename, const char * mode, String & error);
};

VFSImpl * vfs_tmpfile (String & error);

#endif /* LIBAUDCORE_VFS_LOCAL_H */
