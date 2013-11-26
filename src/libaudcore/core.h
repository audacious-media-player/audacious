/*
 * core.h
 * Copyright 2011 John Lindgren
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

#undef NULL
#ifdef __cplusplus /* *sigh* */
#define NULL 0
#else
#define NULL ((void *) 0)
#endif

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

#define SPRINTF(s,...) \
 char s[snprintf (NULL, 0, __VA_ARGS__) + 1]; \
 snprintf (s, sizeof s, __VA_ARGS__);

/* Simple sanity check to catch (1) strings that are still in use after their
 * reference count has dropped to zero and (2) strings that should have been
 * pooled but never were.  If the check fails, the program is aborted. */
#define STR_CHECK(str) do {if ((str) && (str)[-1] != '@') strpool_abort (str);} while (0)

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
char * str_ref (char * str);

/* Decrements the reference count of <str>, where <str> is the address of a
 * string in the pool.  If the reference count drops to zero, releases the
 * memory used by <str>.   If <str> is NULL, simply returns NULL with no side
 * effects. */
void str_unref (char * str);

/* Calls str_get() on the first <len> characters of <str>.  If <str> has less
 * than or equal to <len> characters, equivalent to str_get(). */
char * str_nget (const char * str, int len);

/* Calls sprintf() internally, then pools the produced string with str_get(). */
char * str_printf (const char * format, ...);

/* Used by STR_CHECK; should not be called directly. */
void strpool_abort (char * str);

/* Releases all memory used by the string pool.  If strings remain in the pool,
 * a warning may be printed to stderr in order to reveal memory leaks. */
void strpool_shutdown (void);

#endif /* LIBAUDCORE_CORE_H */
