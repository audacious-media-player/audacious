/*
 * strpool.c
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

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "core.h"

#ifdef USE_STRINGPOOL
#include <glib.h>
#endif

/* Each string in the pool is allocated with five leading bytes: a 32-bit
 * reference count and a one-byte signature, the '@' character. */

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef USE_STRINGPOOL
static GHashTable * table;
#endif

static void str_destroy (void * str)
{
    * ((char *) str - 1) = 0;
    free ((char *) str - 5);
}

char * str_get (const char * str)
{
    if (! str)
        return NULL;

    char * copy;
    pthread_mutex_lock (& mutex);

#ifdef USE_STRINGPOOL
    if (! table)
        table = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, str_destroy);

    if ((copy = g_hash_table_lookup (table, str)))
    {
        void * mem = copy - 5;
        (* (int32_t *) mem) ++;
    }
    else
    {
#endif
        void * mem = malloc (6 + strlen (str));
        (* (int32_t *) mem) = 1;

        copy = (char *) mem + 5;
        copy[-1] = '@';
        strcpy (copy, str);

#ifdef USE_STRINGPOOL
        g_hash_table_insert (table, copy, copy);
    }
#endif

    pthread_mutex_unlock (& mutex);
    return copy;
}

char * str_ref (char * str)
{
    if (! str)
        return NULL;

    pthread_mutex_lock (& mutex);
    STR_CHECK (str);

    void * mem = str - 5;
    (* (int32_t *) mem) ++;

    pthread_mutex_unlock (& mutex);
    return str;
}

char * str_unref (char * str)
{
    if (! str)
        return NULL;

    pthread_mutex_lock (& mutex);
    STR_CHECK (str);

    void * mem = str - 5;
    if (! -- (* (int32_t *) mem))
#ifdef USE_STRINGPOOL
        g_hash_table_remove (table, str);
#else
        str_destroy (str);
#endif

    pthread_mutex_unlock (& mutex);
    return NULL;
}

char * str_nget (const char * str, int len)
{
    if (strlen (str) <= len)
        return str_get (str);

    char buf[len + 1];
    memcpy (buf, str, len);
    buf[len] = 0;

    return str_get (buf);
}

char * str_printf (const char * format, ...)
{
    va_list args;

    va_start (args, format);
    int len = vsnprintf (NULL, 0, format, args);
    va_end (args);

    char buf[len + 1];

    va_start (args, format);
    vsnprintf (buf, sizeof buf, format, args);
    va_end (args);

    return str_get (buf);
}

void strpool_abort (void)
{
    fprintf (stderr, "String pool consistency check failed, aborting ...\n");
    abort ();
}

#ifdef USE_STRINGPOOL
static void str_leaked (void * key, void * str, void * unused)
{
    fprintf (stderr, "String not freed: %s\n", (char *) str);
}
#endif

void strpool_shutdown (void)
{
#ifdef USE_STRINGPOOL
    g_hash_table_foreach (table, str_leaked, NULL);
    g_hash_table_destroy (table);
    table = NULL;
#endif
}
