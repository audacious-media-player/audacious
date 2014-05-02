/*
 * core.h
 * Copyright 2011-2012 John Lindgren
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

#ifndef LIBAUDCORE_CORE_H
#define LIBAUDCORE_CORE_H

/* COMMON MACROS */

#undef NULL
#define NULL nullptr

/* "bool_t" means "int" for compatibility with GLib */
#undef bool_t
#define bool_t int

#undef FALSE
#define FALSE ((bool_t) 0)
#undef TRUE
#define TRUE ((bool_t) 1)

#undef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#undef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#undef CLAMP
#define CLAMP(a,min,max) ((a) < (min) ? (min) : (a) > (max) ? (max) : (a))

#define ARRAY_LEN(a) (sizeof (a) / sizeof (a)[0])

/* CORE TYPES */

enum {
 PLUGIN_TYPE_TRANSPORT,
 PLUGIN_TYPE_PLAYLIST,
 PLUGIN_TYPE_INPUT,
 PLUGIN_TYPE_EFFECT,
 PLUGIN_TYPE_OUTPUT,
 PLUGIN_TYPE_VIS,
 PLUGIN_TYPE_GENERAL,
 PLUGIN_TYPE_IFACE,
 PLUGIN_TYPES};

struct Plugin;
struct TransportPlugin;
struct PlaylistPlugin;
struct InputPlugin;
struct EffectPlugin;
struct OutputPlugin;
struct VisPlugin;
struct GeneralPlugin;
struct IfacePlugin;

struct PluginHandle;
struct PluginPreferences;
struct PreferencesWidget;
struct Tuple;
struct VFSFile;

/* STRING POOL */

/* If the pool contains a copy of <str>, increments its reference count.
 * Otherwise, adds a copy of <str> to the pool with a reference count of one.
 * In either case, returns the copy.  Because this copy may be shared by other
 * parts of the code, it should not be modified.  If <str> is NULL, simply
 * returns NULL with no side effects. */
char * str_get (const char * str);

/* Increments the reference count of <str>, where <str> is the address of a
 * string already in the pool.  Faster than calling str_get() a second time.
 * Returns <str> for convenience.  If <str> is NULL, simply returns NULL with no
 * side effects. */
char * str_ref (const char * str);

/* Decrements the reference count of <str>, where <str> is the address of a
 * string in the pool.  If the reference count drops to zero, releases the
 * memory used by <str>.   If <str> is NULL, simply returns NULL with no side
 * effects. */
void str_unref (char * str);

/* Returns the cached hash value of a pooled string (or 0 for NULL). */
unsigned str_hash (const char * str);

/* Checks whether two pooled strings are equal.  Since the pool never contains
 * duplicate strings, this is a simple pointer comparison and thus much faster
 * than strcmp().  NULL is considered equal to NULL but not equal to any string. */
bool_t str_equal (const char * str1, const char * str2);

/* Releases all memory used by the string pool.  If strings remain in the pool,
 * a warning may be printed to stderr in order to reveal memory leaks. */
void strpool_shutdown (void);

#endif /* LIBAUDCORE_CORE_H */
