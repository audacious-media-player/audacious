#include "internal.h"
#include "vfs.h"

extern "C" const char * libguess_determine_encoding (const char *, int, const char *)
    { return nullptr; }

bool aud_get_bool (const char *, const char *)
    { return false; }
String aud_get_str (const char *, const char *)
    { return String (""); }
String VFSFile::get_metadata (const char *)
    { return String (); }

size_t misc_bytes_allocated;
