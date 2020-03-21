/*
 * archive_reader.h
 * Copyright (c) 2020 Ariadne Conill
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
/**
 * @file archive_reader.h
 * Header providing archive reading functionality via VFS.
 */

#ifndef LIBAUDCORE_ARCHIVE_READER_H
#define LIBAUDCORE_ARCHIVE_READER_H

#ifdef USE_LIBARCHIVE

#include <archive.h>
#include <archive_entry.h>
#include <libaudcore/vfs.h>

class VFSArchiveReaderImpl : public VFSImpl
{
public:
    VFSArchiveReaderImpl (archive *, archive_entry *);
    ~VFSArchiveReaderImpl ();

protected:
    int64_t fread (void * ptr, int64_t size, int64_t nmemb);
    int fseek (int64_t offset, VFSSeekType whence);

    int64_t ftell ();
    int64_t fsize ();
    bool feof ();

    int64_t fwrite (const void * ptr, int64_t size, int64_t nmemb);
    int ftruncate(int64_t);
    int fflush();

private:
    archive * m_archive;
    archive_entry * m_archive_entry;
    int64_t m_size;
    int64_t m_pos;
    bool m_eof;
};

class ArchiveReader
{
public:
    ArchiveReader (VFSFile && archive_file);
    Index<String> read_folder ();
    VFSFile open (const char * path);

private:
    VFSFile & m_archive_file;
    Index<char> m_buf;

    VFSArchiveReaderImpl * read_file (const char * path);

    static ssize_t reader(archive * a, void * client_data, const void ** buff);
};

#endif

#endif
