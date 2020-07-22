/*
 * archive_reader.cc
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

#ifdef USE_LIBARCHIVE

#include "archive_reader.h"
#include "audstrings.h"
#include "runtime.h"

EXPORT ArchiveReader::ArchiveReader(VFSFile && archive_file) :
    m_archive_file (archive_file)
{
}

EXPORT Index<String> ArchiveReader::read_folder()
{
    Index<String> files;
    archive * a = nullptr;
    archive_entry * entry = nullptr;

    // if fseek returns non-zero, bail
    if (m_archive_file.fseek (0, VFS_SEEK_SET))
        return files;

    a = archive_read_new ();
    archive_read_support_filter_all (a);
    archive_read_support_format_all (a);

    archive_read_open (a, this, nullptr, reader, nullptr);

    while (archive_read_next_header (a, & entry) == ARCHIVE_OK)
    {
        files.append (String (archive_entry_pathname (entry)));
    }

    archive_read_free (a);

    return files;
}

ssize_t ArchiveReader::reader(archive * a, void * client_data, const void ** buff)
{
    constexpr int pagesize = 4096;

    ArchiveReader * ar = (ArchiveReader *) client_data;

    ar->m_buf.clear ();
    ar->m_buf.insert (0, pagesize);

    auto size = ar->m_archive_file.fread (ar->m_buf.begin (), 1, pagesize);
    ar->m_buf.remove (size, -1);

    * buff = ar->m_buf.begin ();

    return size;
}

EXPORT VFSFile ArchiveReader::open(const char * path)
{
    return VFSFile (path, read_file (path));
}

EXPORT VFSArchiveReaderImpl * ArchiveReader::read_file(const char * path)
{
    archive * a = nullptr;
    archive_entry * entry = nullptr;

    // if fseek returns non-zero, bail
    if (m_archive_file.fseek (0, VFS_SEEK_SET))
        return nullptr;

    a = archive_read_new ();
    archive_read_support_filter_all (a);
    archive_read_support_format_all (a);

    archive_read_open (a, this, nullptr, reader, nullptr);

    while (archive_read_next_header (a, & entry) == ARCHIVE_OK)
    {
        if (! str_compare (archive_entry_pathname (entry), path))
            return new VFSArchiveReaderImpl (a, entry);
    }

    // on error, cleanup and return nullptr
    archive_read_free (a);
    return nullptr;
}

VFSArchiveReaderImpl::VFSArchiveReaderImpl(archive * a, archive_entry * entry) :
    m_archive (a),
    m_archive_entry (entry),
    m_pos (0),
    m_eof (false)
{
    m_size = archive_entry_size (m_archive_entry);
}

VFSArchiveReaderImpl::~VFSArchiveReaderImpl()
{
    if (m_archive)
        archive_read_free (m_archive);

    m_archive = nullptr;
}

int64_t VFSArchiveReaderImpl::fsize ()
{
    return m_size;
}

int64_t VFSArchiveReaderImpl::ftell ()
{
    return m_pos;
}

bool VFSArchiveReaderImpl::feof ()
{
    return m_eof;
}

int VFSArchiveReaderImpl::ftruncate (int64_t)
{
    return 0;
}

int VFSArchiveReaderImpl::fflush ()
{
    return 0;
}

int64_t VFSArchiveReaderImpl::fwrite (const void * ptr, int64_t size, int64_t nmemb)
{
    return 0;
}

int VFSArchiveReaderImpl::fseek (int64_t offset, VFSSeekType whence)
{
    AUDERR("<%p> implement me!\n", this);
    return -1;
}

int64_t VFSArchiveReaderImpl::fread (void * ptr, int64_t size, int64_t nmemb)
{
    auto ret = archive_read_data (m_archive, ptr, (size * nmemb));
    if (ret < ARCHIVE_OK)
        return -1;

    if (! ret)
        m_eof = true;

    m_pos += ret;

    return ret;
}

#endif // USE_LIBARCHIVE
