/*
 * strpool.c
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

#include <glib.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"

/* Each string in the pool is allocated with five leading bytes: a 32-bit
 * reference count and a one-byte signature, the '@' character. */

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static GHashTable * table;

static void str_destroy (void * str)
{
    * ((char *) str - 1) = 0;
    free ((char *) str - 5);
}

EXPORT char * str_get (const char * str)
{
    if (! str)
        return NULL;

    char * copy;
    pthread_mutex_lock (& mutex);

    if (! table)
        table = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, str_destroy);

    if ((copy = g_hash_table_lookup (table, str)))
    {
        void * mem = copy - 5;
        (* (int32_t *) mem) ++;
    }
    else
    {
        void * mem = malloc (6 + strlen (str));
        (* (int32_t *) mem) = 1;

        copy = (char *) mem + 5;
        copy[-1] = '@';
        strcpy (copy, str);

        g_hash_table_insert (table, copy, copy);
    }

    pthread_mutex_unlock (& mutex);
    return copy;
}

EXPORT char * str_ref (char * str)
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

EXPORT void str_unref (char * str)
{
    if (! str)
        return;

    pthread_mutex_lock (& mutex);
    STR_CHECK (str);

    void * mem = str - 5;
    if (! -- (* (int32_t *) mem))
        g_hash_table_remove (table, str);

    pthread_mutex_unlock (& mutex);
}

EXPORT char * str_nget (const char * str, int len)
{
    if (memchr (str, 0, len))
        return str_get (str);

    char buf[len + 1];
    memcpy (buf, str, len);
    buf[len] = 0;

    return str_get (buf);
}

EXPORT char * str_printf (const char * format, ...)
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

EXPORT void strpool_abort (char * str)
{
    fprintf (stderr, "String not in pool: %s\n", str);
    abort ();
}

static void str_leaked (void * key, void * str, void * unused)
{
    fprintf (stderr, "String not freed: %s\n", (char *) str);
}

EXPORT void strpool_shutdown (void)
{
    if (! table)
        return;

    g_hash_table_foreach (table, str_leaked, NULL);
    g_hash_table_destroy (table);
    table = NULL;
}
