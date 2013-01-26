/*
 * config.c
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
#include <stdio.h>
#include <string.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>

#include "main.h"
#include "misc.h"

#define DEFAULT_SECTION "audacious"

static const char * const core_defaults[] = {

 /* general */
 "advance_on_delete", "FALSE",
 "clear_playlist", "TRUE",
 "open_to_temporary", "TRUE",
 "resume_playback_on_startup", "FALSE",
 "show_interface", "TRUE",

 /* equalizer */
 "eqpreset_default_file", "",
 "eqpreset_extension", "",
 "equalizer_active", "FALSE",
 "equalizer_autoload", "FALSE",
 "equalizer_bands", "0,0,0,0,0,0,0,0,0,0",
 "equalizer_preamp", "0",

 /* info popup / info window */
 "cover_name_exclude", "back",
 "cover_name_include", "album,cover,front,folder",
 "filepopup_delay", "5",
 "filepopup_showprogressbar", "TRUE",
 "recurse_for_cover", "FALSE",
 "recurse_for_cover_depth", "0",
 "show_filepopup_for_tuple", "TRUE",
 "use_file_cover", "FALSE",

 /* network */
 "use_proxy", "FALSE",
 "use_proxy_auth", "FALSE",

 /* output */
 "default_gain", "0",
 "enable_replay_gain", "TRUE",
 "enable_clipping_prevention", "TRUE",
 "output_bit_depth", "16",
 "output_buffer_size", "500",
 "replay_gain_album", "FALSE",
 "replay_gain_preamp", "0",
 "soft_clipping", "FALSE",
 "software_volume_control", "FALSE",
 "sw_volume_left", "100",
 "sw_volume_right", "100",

 /* playback */
 "no_playlist_advance", "FALSE",
 "repeat", "FALSE",
 "shuffle", "FALSE",
 "stop_after_current_song", "FALSE",

 /* playlist */
#ifdef _WIN32
 "convert_backslash", "TRUE",
#else
 "convert_backslash", "FALSE",
#endif
 "generic_title_format", "${?artist:${artist} - }${?album:${album} - }${title}",
 "leading_zero", "FALSE",
 "metadata_on_play", "FALSE",
 "show_numbers_in_pl", "FALSE",

 NULL};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static GHashTable * defaults;
static GKeyFile * keyfile;
static bool_t modified;

/* str_unref() may be a macro */
static void str_unref_cb (void * str)
{
    str_unref (str);
}

void config_load (void)
{
    g_return_if_fail (! defaults && ! keyfile);
    pthread_mutex_lock (& mutex);

    defaults = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
     (GDestroyNotify) g_hash_table_destroy);
    keyfile = g_key_file_new ();

    char * path = g_strdup_printf ("%s/config", get_path (AUD_PATH_USER_DIR));
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
    pthread_mutex_unlock (& mutex);

    config_set_defaults (NULL, core_defaults);
}

void config_save (void)
{
    g_return_if_fail (defaults && keyfile);
    pthread_mutex_lock (& mutex);

    if (! modified)
    {
        pthread_mutex_unlock (& mutex);
        return;
    }

    char * path = g_strdup_printf ("%s/config", get_path (AUD_PATH_USER_DIR));
    char * data = g_key_file_to_data (keyfile, NULL, NULL);

    GError * error = NULL;
    if (! g_file_set_contents (path, data, -1, & error))
    {
        fprintf (stderr, "Error saving config: %s\n", error->message);
        g_error_free (error);
    }

    g_free (data);
    g_free (path);

    modified = FALSE;
    pthread_mutex_unlock (& mutex);
}

void config_cleanup (void)
{
    g_return_if_fail (defaults && keyfile);
    pthread_mutex_lock (& mutex);

    g_key_file_free (keyfile);
    keyfile = NULL;
    g_hash_table_destroy (defaults);
    defaults = NULL;

    pthread_mutex_unlock (& mutex);
}

void config_clear_section (const char * section)
{
    g_return_if_fail (defaults && keyfile);
    pthread_mutex_lock (& mutex);

    if (! section)
        section = DEFAULT_SECTION;

    if (g_key_file_has_group (keyfile, section))
    {
        g_key_file_remove_group (keyfile, section, NULL);
        modified = TRUE;
    }

    pthread_mutex_unlock (& mutex);
}

void config_set_defaults (const char * section, const char * const * entries)
{
    g_return_if_fail (defaults && keyfile);
    pthread_mutex_lock (& mutex);

    if (! section)
        section = DEFAULT_SECTION;

    GHashTable * table = g_hash_table_lookup (defaults, section);
    if (! table)
    {
        table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, str_unref_cb);
        g_hash_table_replace (defaults, g_strdup (section), table);
    }

    while (1)
    {
        const char * name = * entries ++;
        const char * value = * entries ++;
        if (! name || ! value)
            break;

        g_hash_table_replace (table, g_strdup (name), str_get (value));
    }

    pthread_mutex_unlock (& mutex);
}

static const char * get_default (const char * section, const char * name)
{
    GHashTable * table = g_hash_table_lookup (defaults, section);
    const char * def = table ? g_hash_table_lookup (table, name) : NULL;
    return def ? def : "";
}

void set_string (const char * section, const char * name, const char * value)
{
    g_return_if_fail (defaults && keyfile);
    g_return_if_fail (name && value);
    pthread_mutex_lock (& mutex);

    if (! section)
        section = DEFAULT_SECTION;

    const char * def = get_default (section, name);
    bool_t changed = FALSE;

    if (! strcmp (value, def))
    {
        if (g_key_file_has_key (keyfile, section, name, NULL))
        {
            g_key_file_remove_key (keyfile, section, name, NULL);
            changed = TRUE;
        }
    }
    else
    {
        char * old = g_key_file_has_key (keyfile, section, name, NULL) ?
         g_key_file_get_value (keyfile, section, name, NULL) : NULL;

        if (! old || strcmp (value, old))
        {
            g_key_file_set_value (keyfile, section, name, value);
            changed = TRUE;
        }

        g_free (old);
    }

    if (changed)
    {
        modified = TRUE;

        if (! strcmp (section, DEFAULT_SECTION))
        {
            char * event = g_strdup_printf ("set %s", name);
            event_queue (event, NULL);
            g_free (event);
        }
    }

    pthread_mutex_unlock (& mutex);
}

char * get_string (const char * section, const char * name)
{
    g_return_val_if_fail (defaults && keyfile, g_strdup (""));
    g_return_val_if_fail (name, g_strdup (""));
    pthread_mutex_lock (& mutex);

    if (! section)
        section = DEFAULT_SECTION;

    char * value = g_key_file_has_key (keyfile, section, name, NULL) ?
     g_key_file_get_value (keyfile, section, name, NULL) : NULL;

    if (! value)
        value = g_strdup (get_default (section, name));

    pthread_mutex_unlock (& mutex);
    return value;
}

void set_bool (const char * section, const char * name, bool_t value)
{
    set_string (section, name, value ? "TRUE" : "FALSE");
}

bool_t get_bool (const char * section, const char * name)
{
    char * string = get_string (section, name);
    bool_t value = ! strcmp (string, "TRUE");
    g_free (string);
    return value;
}

void set_int (const char * section, const char * name, int value)
{
    char * string = int_to_string (value);
    g_return_if_fail (string);
    set_string (section, name, string);
    g_free (string);
}

int get_int (const char * section, const char * name)
{
    int value = 0;
    char * string = get_string (section, name);
    string_to_int (string, & value);
    g_free (string);
    return value;
}

void set_double (const char * section, const char * name, double value)
{
    char * string = double_to_string (value);
    g_return_if_fail (string);
    set_string (section, name, string);
    g_free (string);
}

double get_double (const char * section, const char * name)
{
    double value = 0;
    char * string = get_string (section, name);
    string_to_double (string, & value);
    g_free (string);
    return value;
}
