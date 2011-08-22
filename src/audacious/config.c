/*
 * config.c
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

#include <glib.h>
#include <stdio.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/eventqueue.h>

#include "misc.h"

#define DEFAULT_SECTION "audacious"

static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
static GKeyFile * keyfile;
static gboolean modified;

void config_load (void)
{
    g_return_if_fail (! keyfile);
    g_static_mutex_lock (& mutex);

    keyfile = g_key_file_new ();
    gchar * path = g_strdup_printf ("%s/config", get_path (AUD_PATH_USER_DIR));

    if (g_file_test (path, G_FILE_TEST_EXISTS))
    {
        GError * error = NULL;
        if (! g_key_file_load_from_file (keyfile, path, 0, & error))
        {
            fprintf (stderr, "Error loading config: %s\n", error->message);
            g_error_free (error);
        }
    }

    g_free (path);
    modified = FALSE;

    g_static_mutex_unlock (& mutex);
}

void config_save (void)
{
    g_return_if_fail (keyfile);
    g_static_mutex_lock (& mutex);

    if (! modified)
    {
        g_static_mutex_unlock (& mutex);
        return;
    }

    gchar * path = g_strdup_printf ("%s/config", get_path (AUD_PATH_USER_DIR));
    gchar * data = g_key_file_to_data (keyfile, NULL, NULL);

    GError * error = NULL;
    if (! g_file_set_contents (path, data, -1, & error))
    {
        fprintf (stderr, "Error saving config: %s\n", error->message);
        g_error_free (error);
    }

    g_free (data);
    g_free (path);
    modified = FALSE;

    g_static_mutex_unlock (& mutex);
}

void config_cleanup (void)
{
    g_return_if_fail (keyfile);
    g_static_mutex_lock (& mutex);

    g_key_file_free (keyfile);
    keyfile = NULL;

    g_static_mutex_unlock (& mutex);
}

void set_string (const gchar * section, const gchar * name, const gchar * value)
{
    g_return_if_fail (keyfile);
    g_static_mutex_lock (& mutex);

    if (! section)
        section = DEFAULT_SECTION;

    gchar * old = g_key_file_get_value (keyfile, section, name, NULL);
    if (old && ! strcmp (old, value))
    {
        g_static_mutex_unlock (& mutex);
        return;
    }

    g_key_file_set_value (keyfile, section, name, value);
    modified = TRUE;

    if (! strcmp (section, DEFAULT_SECTION))
    {
        gchar * event = g_strdup_printf ("set %s", name);
        event_queue (event, NULL);
        g_free (event);
    }

    g_static_mutex_unlock (& mutex);
}

gboolean get_string (const gchar * section, const gchar * name, gchar * * addr)
{
    g_return_val_if_fail (keyfile, FALSE);
    g_static_mutex_lock (& mutex);

    if (! section)
        section = DEFAULT_SECTION;

    gchar * value = g_key_file_get_string (keyfile, section, name, NULL);
    if (! value)
    {
        g_static_mutex_unlock (& mutex);
        return FALSE;
    }

    * addr = value;

    g_static_mutex_unlock (& mutex);
    return TRUE;
}

void set_bool (const gchar * section, const gchar * name, gboolean value)
{
    set_string (section, name, value ? "TRUE" : "FALSE");
}

gboolean get_bool (const gchar * section, const gchar * name, gboolean * addr)
{
    gchar * string;
    if (! get_string (section, name, & string))
        return FALSE;

    gboolean success = TRUE;

    if (! strcmp (string, "TRUE"))
        * addr = TRUE;
    else if (! strcmp (string, "FALSE"))
        * addr = FALSE;
    else
        success = FALSE;

    g_free (string);
    return success;
}

void set_int (const gchar * section, const gchar * name, gint value)
{
    gchar * string = int_to_string (value);
    g_return_if_fail (string);
    set_string (section, name, string);
}

gboolean get_int (const gchar * section, const gchar * name, gint * addr)
{
    gchar * string;
    if (! get_string (section, name, & string))
        return FALSE;

    gboolean success = string_to_int (string, addr);
    g_free (string);
    return success;
}

void set_double (const gchar * section, const gchar * name, gdouble value)
{
    gchar * string = double_to_string (value);
    g_return_if_fail (string);
    set_string (section, name, string);
}

gboolean get_double (const gchar * section, const gchar * name, gdouble * addr)
{
    gchar * string;
    if (! get_string (section, name, & string))
        return FALSE;

    gboolean success = string_to_double (string, addr);
    g_free (string);
    return success;
}
