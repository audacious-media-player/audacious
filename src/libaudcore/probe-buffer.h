/*
 * probe-buffer.h
 * Copyright 2015 John Lindgren
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

#ifndef LIBAUDCORE_PROBE_BUFFER_H
#define LIBAUDCORE_PROBE_BUFFER_H

/* The buffer is only for the beginning of the file, to allow detecting the file
 * type without reading from the file repeatedly.  Whenever buffering is active,
 * m_at indicates the virtual file position, while the real file position is at
 * the end of the buffer.  Once it is necessary to read outside the buffer, m_at
 * is set to -1, the buffer memory is released, and all file operations are done
 * on the real file.  Seeking to the beginning of the file activates buffering
 * again.
 *
 * Optionally, reads can be restricted to the bufferable area.  In this case,
 * reads will stop short at the end of the bufferable area, and seeks outside
 * the bufferable area (and any type of SEEK_END) will fail.  However, the real
 * file size is still reported.
 */

#include "vfs.h"

class ProbeBuffer : public VFSImpl
{
public:
    ProbeBuffer (const char * filename, VFSImpl * file);
    ~ProbeBuffer ();

    int64_t fread (void * ptr, int64_t size, int64_t nmemb);
    int fseek (int64_t offset, VFSSeekType whence);

    int64_t ftell ();
    int64_t fsize ();
    bool feof ();

    int64_t fwrite (const void * ptr, int64_t size, int64_t nmemb);
    int ftruncate (int64_t length);
    int fflush ();

    String get_metadata (const char * field);

    void set_limit_to_buffer (bool limit)
        { m_limited = limit; }

private:
    void increase_buffer (int64_t size);
    void release_buffer ();

    String m_filename;
    SmartPtr<VFSImpl> m_file;
    char * m_buffer = nullptr;
    int m_filled = 0, m_at = 0;
    bool m_limited = false;
};

#endif // LIBAUDCORE_PROBE_BUFFER_H
