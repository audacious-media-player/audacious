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
#include <libaudcore/stringpool.h>

#include "misc.h"

#define DEFAULT_SECTION "audacious"

static const gchar * core_defaults[] = {

 /* general */
 "advance_on_delete", "FALSE",
 "clear_playlist", "TRUE",
 "open_to_temporary", "FALSE",
 "resume_playback_on_startup", "FALSE",

 /* equalizer */
 "eqpreset_default_file", "",
 "eqpreset_extension", "",
 "equalizer_active", "FALSE",
 "equalizer_autoload", "FALSE",
 "equalizer_bands", "0,0,0,0,0,0,0,0,0,0",
 "equalizer_preamp", "0",

 /* info popup / info window */
 "cover_name_exclude", "back",
 "cover_name_include", "album,folder",
 "filepopup_delay", "5",
 "filepopup_showprogressbar", "TRUE",
 "recurse_for_cover", "FALSE",
 "recurse_for_cover_depth", "0",
 "show_filepopup_for_tuple", "TRUE",
 "use_file_cover", "FALSE",

 /* playback */
 "no_playlist_advance", "FALSE",
 "repeat", "FALSE",
 "shuffle", "FALSE",
 "stop_after_current_song", "FALSE",

 NULL};

static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
static GHashTable * defaults;
static GKeyFile * keyfile;
static gboolean modified;

void config_load (void)
{
    g_return_if_fail (! defaults && ! keyfile);
    g_static_mutex_lock (& mutex);

    defaults = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
     (GDestroyNotify) g_hash_table_destroy);
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

    config_set_defaults (NULL, core_defaults);
}

void config_save (void)
{
    g_return_if_fail (defaults && keyfile);
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
    g_return_if_fail (defaults && keyfile);
    g_static_mutex_lock (& mutex);

    g_key_file_free (keyfile);
    keyfile = NULL;
    g_hash_table_destroy (defaults);
    defaults = NULL;

    g_static_mutex_unlock (& mutex);
}

void config_clear_section (const gchar * section)
{
    g_return_if_fail (defaults && keyfile);
    g_static_mutex_lock (& mutex);

    if (! section)
        section = DEFAULT_SECTION;

    if (g_key_file_has_group (keyfile, section))
    {
        g_key_file_remove_group (keyfile, section, NULL);
        modified = TRUE;
    }

    g_static_mutex_unlock (& mutex);
}

void config_set_defaults (const gchar * section, const gchar * const * entries)
{
    g_return_if_fail (defaults && keyfile);
    g_static_mutex_lock (& mutex);

    if (! section)
        section = DEFAULT_SECTION;

    GHashTable * table = g_hash_table_lookup (defaults, section);
    if (! table)
    {
        table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
         (GDestroyNotify) stringpool_unref);
        g_hash_table_replace (defaults, g_strdup (section), table);
    }

    while (1)
    {
        const gchar * name = * entries ++;
        const gchar * value = * entries ++;
        if (! name || ! value)
            break;

        g_hash_table_replace (table, g_strdup (name), stringpool_get ((gchar *) value, FALSE));
    }

    g_static_mutex_unlock (& mutex);
}

static const gchar * get_default (const gchar * section, const gchar * name)
{
    GHashTable * table = g_hash_table_lookup (defaults, section);
    const gchar * def = table ? g_hash_table_lookup (table, name) : NULL;
    return def ? def : "";
}

void set_string (const gchar * section, const gchar * name, const gchar * value)
{
    g_return_if_fail (defaults && keyfile);
    g_return_if_fail (name && value);
    g_static_mutex_lock (& mutex);

    if (! section)
        section = DEFAULT_SECTION;

    gchar * old = g_key_file_get_value (keyfile, section, name, NULL);
    if (old && ! strcmp (old, value))
    {
        g_free (old);
        g_static_mutex_unlock (& mutex);
        return;
    }
    g_free (old);

    const gchar * def = get_default (section, name);
    if (! strcmp (value, def))
        g_key_file_remove_key (keyfile, section, name, NULL);
    else
        g_key_file_set_value (keyfile, section, name, value);

    if (! strcmp (section, DEFAULT_SECTION))
    {
        gchar * event = g_strdup_printf ("set %s", name);
        event_queue (event, NULL);
        g_free (event);
    }

    modified = TRUE;
    g_static_mutex_unlock (& mutex);
}

gchar * get_string (const gchar * section, const gchar * name)
{
    g_return_val_if_fail (defaults && keyfile, g_strdup (""));
    g_return_val_if_fail (name, g_strdup (""));
    g_static_mutex_lock (& mutex);

    if (! section)
        section = DEFAULT_SECTION;

    gchar * value = g_key_file_get_string (keyfile, section, name, NULL);
    if (! value)
        value = g_strdup (get_default (section, name));

    g_static_mutex_unlock (& mutex);
    return value;
}

void set_bool (const gchar * section, const gchar * name, gboolean value)
{
    set_string (section, name, value ? "TRUE" : "FALSE");
}

gboolean get_bool (const gchar * section, const gchar * name)
{
    gchar * string = get_string (section, name);
    gboolean value = ! strcmp (string, "TRUE");
    g_free (string);
    return value;
}

void set_int (const gchar * section, const gchar * name, gint value)
{
    gchar * string = int_to_string (value);
    g_return_if_fail (string);
    set_string (section, name, string);
    g_free (string);
}

gint get_int (const gchar * section, const gchar * name)
{
    gint value = 0;
    gchar * string = get_string (section, name);
    string_to_int (string, & value);
    g_free (string);
    return value;
}

void set_double (const gchar * section, const gchar * name, gdouble value)
{
    gchar * string = double_to_string (value);
    g_return_if_fail (string);
    set_string (section, name, string);
    g_free (string);
}

gdouble get_double (const gchar * section, const gchar * name)
{
    gdouble value = 0;
    gchar * string = get_string (section, name);
    string_to_double (string, & value);
    g_free (string);
    return value;
}

/* configdb compatibility hack -- do not use */
gboolean xxx_config_is_set (const gchar * section, const gchar * name)
{
    g_return_val_if_fail (defaults && keyfile, FALSE);
    g_return_val_if_fail (name, FALSE);
    g_static_mutex_lock (& mutex);

    if (! section)
        section = DEFAULT_SECTION;

    gboolean is_set = g_key_file_has_key (keyfile, section, name, NULL);

    g_static_mutex_unlock (& mutex);
    return is_set;
}
