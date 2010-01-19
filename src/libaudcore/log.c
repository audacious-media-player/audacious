/*
 * Logging mechanisms
 * Copyright (c) 2009 Audacious team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#include "log.h"
#include <stdio.h>
#include <string.h>

/** Logfile creation timestamp format */
#define AUD_LOG_CTIME_FMT   "%c"

/** Logfile entry timestamp format */
#define AUD_LOG_LTIME_FMT   "%H:%M:%S"

/**
 * Global log level setting, this determines what is logged in
 * the logfile. Anything "above" this level will not be logged.
 */
static gint log_level = AUD_LOG_INFO;

/** Global log file handle */
static FILE *log_file = NULL;

/** Mutex for protecting from threaded access */
static GMutex *log_mutex = NULL;

/** Hashtable that contains registered thread information */
static GHashTable *log_thread_hash = NULL;

/** Descriptive names for different log levels */
const gchar *log_level_names[AUD_LOG_ALL] = {
    "none",
    "FATAL",
    "ERROR",
    "warning",
    "info",
    "DEBUG",
    "DEBUG+",
};


/**
 * Create a time/date string based on given format string.
 * Current adjusted local time is used as timestamp.
 *
 * @param[in] fmt Format for time/date string, as described in 'man strftime'.
 * @return Newly allocated string, must be freed with g_free().
 */
static gchar *
aud_log_timestr(const gchar *fmt)
{
    gchar tmp[256] = "";
    time_t stamp = time(NULL);
    struct tm stamp_tm;

    if (stamp >= 0 && localtime_r(&stamp, &stamp_tm) != NULL)
        strftime(tmp, sizeof(tmp), fmt, &stamp_tm);

    return g_strdup(tmp);
}

/**
 * The actual internal function that does the logfile output.
 * Entry is prefixed with timestamp information, log level's name
 * (as defined in #log_level_names), logging context and the message
 * itself. Linefeeds are optional in format string, they are added
 * automatically for compatibility with g_log().
 *
 * @param[in] f File handle to write into.
 * @param[in] ctx Logging context or NULL if no context / global context.
 * @param[in] level Message's log level setting.
 * @param[in] msg Log message string.
 */
static void
aud_log_msg(FILE *f, const gchar *ctx, gint level, const gchar *msg)
{
    gchar *timestamp;
    GThread *thread = g_thread_self();
    gchar *name = (log_thread_hash != NULL) ?
        g_hash_table_lookup(log_thread_hash, thread) : NULL;

    timestamp = aud_log_timestr(AUD_LOG_LTIME_FMT);
    fprintf(f, "%s <", timestamp);
    g_free(timestamp);

    if (name != NULL)
    {
        if (ctx != NULL)
            fprintf(f, "%s|%s", ctx, name);
        else
            fprintf(f, "%s", name);
    }
    else
    {
        fprintf(f, "%s|%p", ctx != NULL ? ctx : "global", (void *) thread);
    }

    fprintf(f, "> [%s]: %s", (level >= 0) ? log_level_names[level] :
        log_level_names[AUD_LOG_INFO], msg);

    /* A small hack here to ease transition from g_log() etc. */
    if (msg[strlen(msg) - 1] != '\n')
        fprintf(f, "\n");

    fflush(f);
}


/**
 * Varargs version of internal non-locked logging function.
 *
 * @param[in] f File handle to write into.
 * @param[in] ctx Logging context or NULL if no context / global context.
 * @param[in] level Message's log level setting.
 * @param[in] fmt Message printf() style format string.
 * @param[in] args Argument structure.
 */
static void
aud_do_logv(FILE *f, const gchar *ctx, gint level, const gchar *fmt, va_list args)
{
    gchar *msg = g_strdup_vprintf(fmt, args);
    aud_log_msg(f, ctx, level, msg);
    g_free(msg);
}

/**
 * Internal message logging function that does take the global lock.
 *
 * @param[in] f File handle to write into.
 * @param[in] ctx Logging context or NULL if no context / global context.
 * @param[in] level Message's log level setting.
 * @param[in] fmt Message printf() style format string.
 * @param[in] ... Printf() style arguments, if any.
 */
static void
aud_do_log(FILE *f, const gchar *ctx, gint level, const gchar *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    aud_do_logv(f, ctx, level, fmt, ap);
    va_end(ap);
}

/**
 * Initialize logging subsystem.
 *
 * @param[in] filename Filename for logfile, or NULL to use stderr.
 * @param[in] mode Open mode for fopen().
 * @param[in] level Default logging level.
 */
gint
aud_log_init(const gchar *filename, const gchar *mode, gint level)
{
    FILE *tmp;
    gchar *timestamp;

    /* Open or set logging file descriptor */
    if (filename != NULL)
    {
        if ((tmp = fopen(filename, mode)) == NULL)
            return -1;
    }
    else
        tmp = NULL;

    /* Create mutex, unless it already is set */
    if (log_mutex != NULL || (log_mutex = g_mutex_new()) == NULL)
    {
        fclose(tmp);
        return -3;
    }

    /* Acquire mutex, set log_file etc. */
    g_mutex_lock(log_mutex);
    if (log_file != NULL)
        fclose(log_file);

    if (tmp == NULL)
        log_file = stderr;
    else
        log_file = tmp;

    log_level = level;

    /* Logging starts here */
    timestamp = aud_log_timestr(AUD_LOG_CTIME_FMT);
    aud_do_log(log_file, NULL, -1, "Logfile opened %s.\n", timestamp);
    g_free(timestamp);

    /* Setup thread context hash table */
    if (log_thread_hash != NULL)
    {
        aud_do_log(log_file, NULL, -1, "Warning, log_thread_hash != NULL (%p)!",
            log_thread_hash);
        g_hash_table_destroy(log_thread_hash);
    }

    log_thread_hash = g_hash_table_new_full(
        g_direct_hash, g_direct_equal, NULL, g_free);

    g_mutex_unlock(log_mutex);
    return 0;
}

/** Internal helper function for #aud_log_close(). */
static void
aud_log_print_hash(gpointer key, gpointer value, gpointer found)
{
    if (*(gboolean *)found == FALSE)
    {
        *(gboolean *)found = TRUE;
        aud_do_log(log_file, NULL, -1,
        "Warning, following lingering log thread contexts found:\n");
    }

    aud_do_log(log_file, NULL, -1, " - %p = '%s'\n", key, value);
}

/**
 * Shut down the logging subsystem. Logfile handle is closed,
 * mutexes and such freed, etc.
 */
void
aud_log_close(void)
{
    GMutex *tmp;
    gchar *timestamp;

    if ((tmp = log_mutex) != NULL)
    {
        g_mutex_lock(tmp);

        if (log_thread_hash != NULL)
        {
            gboolean found = FALSE;
            g_hash_table_foreach(log_thread_hash,
                aud_log_print_hash, &found);

            g_hash_table_destroy(log_thread_hash);
        }
        log_thread_hash = NULL;

        timestamp = aud_log_timestr(AUD_LOG_CTIME_FMT);
        aud_do_log(log_file, NULL, -1, "Logfile closed %s.\n", timestamp);
        g_free(timestamp);

        log_mutex = NULL;

        if (log_file != NULL)
            fflush(log_file);

        if (log_file != stderr)
            fclose(log_file);

        log_file = NULL;
        g_mutex_unlock(tmp);
    }
}

/**
 * Add symbolic name for given thread identifier. The identifier
 * will be used in subsequent log messages originating from the
 * thread.
 *
 * @param[in] thread Pointer to a GThread structure of the thread.
 * @param[in] name String describing the thread.
 */
void
aud_log_add_thread_context(GThread *thread, const gchar *name)
{
    gchar *tmp = g_strdup(name), *old;
    g_mutex_lock(log_mutex);

    old = g_hash_table_lookup(log_thread_hash, thread);
    if (old != NULL)
        aud_do_log(log_file, NULL, AUD_LOG_INFO,
        "Warning, thread %p is already in context ('%s')!\n", thread, old);

    g_hash_table_insert(log_thread_hash, thread, tmp);

    aud_do_log(log_file, NULL, AUD_LOG_INFO,
        "Thread %p name set to '%s'\n", thread, name);

    g_mutex_unlock(log_mutex);
}

/**
 * Removes identifier for thread, if present. If thread had not been
 * added in first place (via #aud_log_add_thread_context()), a warning
 * is logged instead.
 *
 * @param[in] thread Pointer to a GThread structure of the thread.
 */
void
aud_log_delete_thread_context(GThread *thread)
{
    gchar *old;
    g_mutex_lock(log_mutex);

    old = g_hash_table_lookup(log_thread_hash, thread);
    if (old == NULL)
    {
        aud_do_log(log_file, NULL, AUD_LOG_INFO,
        "Warning, thread %p does not exist in context table!\n", thread);
    }
    else
    {
        aud_do_log(log_file, NULL, AUD_LOG_INFO,
        "Thread %p name ('%s') deleted from context table.\n", thread, old);
        g_hash_table_remove(log_thread_hash, thread);
    }

    g_mutex_unlock(log_mutex);
}

/**
 * Write a log entry with variable arguments structure.
 *
 * @param[in] ctx Logging context or NULL if no context / global context.
 * @param[in] level Message's log level setting.
 * @param[in] fmt Message printf() style format string.
 * @param[in] args Argument structure.
 */
void
aud_logv(const gchar *ctx, gint level, const gchar *fmt, va_list args)
{
    if (log_mutex == NULL || log_file == NULL)
        aud_do_log(stderr, ctx, level, fmt, args);
    else
    {
        g_mutex_lock(log_mutex);
        if (level <= log_level)
            aud_do_logv(log_file, ctx, level, fmt, args);
        g_mutex_unlock(log_mutex);
    }
}

/**
 * Write a log entry.
 *
 * @param[in] ctx Logging context or NULL if no context / global context.
 * @param[in] level Message's log level setting.
 * @param[in] fmt Message printf() style format string.
 * @param[in] ... Optional printf() arguments.
 */
void
aud_log(const gchar *ctx, gint level, const gchar *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    aud_logv(ctx, level, fmt, ap);
    va_end(ap);
}

/**
 * Write a log entry with file/func/line information.
 *
 * @param[in] ctx Logging context pointer or NULL if no context / global context.
 * @param[in] level Message's log level setting.
 * @param[in] file Filename (usually obtained through __FILE__ macro)
 * @param[in] func Function name (usually obtained through __FUNCTION__ macro)
 * @param[in] line Linenumber (usually obtained through __LINE__ macro)
 * @param[in] fmt Message printf() style format string.
 * @param[in] ... Optional printf() arguments.
 */
void
aud_log_line(const gchar *ctx, gint level, const gchar *file, const gchar *func,
    gint line, const gchar *fmt, ...)
{
    gchar *msg, *str, *info = g_strdup_printf("(%s:%s:%d) ", file, func, line);
    va_list ap;

    va_start(ap, fmt);
    msg = g_strdup_vprintf(fmt, ap);
    va_end(ap);

    str = g_strconcat(info, msg, NULL);

    if (log_mutex == NULL || log_file == NULL)
        aud_log_msg(stderr, ctx, level, str);
    else
    {
        g_mutex_lock(log_mutex);
        aud_log_msg(log_file, ctx, level, str);
        g_mutex_unlock(log_mutex);
    }

    g_free(info);
    g_free(msg);
    g_free(str);
}
