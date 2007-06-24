/*
 * audacious: Cross-platform multimedia player.
 * logger.c: Event logging.
 *
 * Copyright (c) 2005-2007 Audacious development team.
 * Copyright (c) 2003-2005 BMP development team.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "logger.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include "main.h"


#define LOG_ALL_LEVELS \
    (G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION)


struct _LogHandler {
    gchar *domain;
    GLogLevelFlags level;
    guint id;
};

typedef struct _LogHandler LogHandler;


static FILE *bmp_log_file = NULL;

G_LOCK_DEFINE_STATIC(bmp_log_file);

static LogHandler log_handlers[] = {
    {NULL, LOG_ALL_LEVELS, 0},
    {"Glib", LOG_ALL_LEVELS, 0},
    {"Gtk", LOG_ALL_LEVELS, 0}
};

static guint log_handler_count = G_N_ELEMENTS(log_handlers);


static const gchar *
get_timestamp_str(void)
{
    time_t current_time = time(NULL);
    return ctime(&current_time);
}

static size_t
get_filesize(const gchar *filename)
{
    struct stat info;

    if (stat(filename, &info))
        return 0;

    return info.st_size;
}

static void
log_to_file(const gchar * domain, GLogLevelFlags level,
            const gchar * message, gpointer data)
{
    FILE *file = (FILE *) data;

    if (!file) {
        g_printerr(G_STRLOC ": file is NULL!\n");
        return;
    }

    G_LOCK(bmp_log_file);

    if (domain)
        g_fprintf(file, "(%s) ", domain);
    
    if (message)
        g_fprintf(file, "%s\n", message);
    else
        g_fprintf(file, "message is NULL!\n");

    fflush(file);

    G_UNLOCK(bmp_log_file);
}

gboolean
bmp_logger_start(const gchar * filename)
{
    guint i;

    g_return_val_if_fail(filename != NULL, FALSE);

    /* truncate file when size limit is reached */
    if (get_filesize(filename) < BMP_LOGGER_FILE_MAX_SIZE)
        bmp_log_file = fopen(filename, "at");
    else
        bmp_log_file = fopen(filename, "w+t");

    if (!bmp_log_file) {
        g_printerr(_("Unable to create log file (%s)!\n"), filename);
        return FALSE;
    }

    for (i = 0; i < log_handler_count; i++) {
        log_handlers[i].id = g_log_set_handler(log_handlers[i].domain,
                                               log_handlers[i].level,
                                               log_to_file, bmp_log_file);
    }

    g_message("\n** LOGGING STARTED AT %s", get_timestamp_str());

    return TRUE;
}

void
bmp_logger_stop(void)
{
    guint i;

    if (!bmp_log_file)
        return;

    g_message("\n** LOGGING STOPPED AT %s", get_timestamp_str());

    for (i = 0; i < log_handler_count; i++)
        g_log_remove_handler(log_handlers[i].domain, log_handlers[i].id);

    fclose(bmp_log_file);
    bmp_log_file = NULL;
}
