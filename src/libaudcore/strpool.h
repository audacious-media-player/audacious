/*
 * strpool.h
 * Copyright 2011 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#ifndef AUDACIOUS_STRPOOL_H
#define AUDACIOUS_STRPOOL_H

/* Simple sanity check to catch (1) strings that are still in use after their
 * reference count has dropped to zero and (2) strings that should have been
 * pooled but never were.  If the check fails, the program is aborted. */
#define STR_CHECK(str) do {if ((str) && (str)[-1] != '@') strpool_abort ();} while (0)

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
 * memory used by <str>.  Returns NULL for convenience.  If <str> is NULL,
 * simply returns NULL with no side effects. */
char * str_unref (char * str);

/* Calls sprintf internally, then pools the produced string with str_get(). */
char * str_printf (const char * format, ...);

/* Used by STR_CHECK; should not be called directly. */
void strpool_abort (void);

/* Releases all memory used by the string pool.  If strings remain in the pool,
 * a warning may be printed to stderr in order to reveal memory leaks. */
void strpool_shutdown (void);

#endif
