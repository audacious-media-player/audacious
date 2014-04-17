/*
 * runtime.h
 * Copyright 2014 John Lindgren
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

#ifndef LIBAUDCORE_RUNTIME_H
#define LIBAUDCORE_RUNTIME_H

#include <stdio.h>

#include "core.h"

enum {
    AUD_PATH_BIN_DIR,
    AUD_PATH_DATA_DIR,
    AUD_PATH_PLUGIN_DIR,
    AUD_PATH_LOCALE_DIR,
    AUD_PATH_DESKTOP_FILE,
    AUD_PATH_ICON_FILE,
    AUD_PATH_USER_DIR,
    AUD_PATH_PLAYLISTS_DIR,
    AUD_PATH_COUNT
};

enum {
    OUTPUT_RESET_EFFECTS_ONLY,
    OUTPUT_RESET_SOFT,
    OUTPUT_RESET_HARD
};

void aud_init_paths (void);
void aud_cleanup_paths (void);

const char * aud_get_path (int id);

void aud_set_headless_mode (bool_t headless);
bool_t aud_get_headless_mode (void);

void aud_set_verbose_mode (bool_t verbose);
bool_t aud_get_verbose_mode (void);

#define AUDDBG(...) do { \
    if (aud_get_verbose_mode ()) { \
        printf ("%s:%d [%s]: ", __FILE__, __LINE__, __FUNCTION__); \
        printf (__VA_ARGS__); \
    } \
} while (0)

/* Requires: aud_init_paths() */
void aud_init_i18n (void);

void aud_config_set_defaults (const char * section, const char * const * entries);

void aud_set_str (const char * section, const char * name, const char * value);
char * aud_get_str (const char * section, const char * name);
void aud_set_bool (const char * section, const char * name, bool_t value);
bool_t aud_get_bool (const char * section, const char * name);
void aud_set_int (const char * section, const char * name, int value);
int aud_get_int (const char * section, const char * name);
void aud_set_double (const char * section, const char * name, double value);
double aud_get_double (const char * section, const char * name);

void aud_init (void);
void aud_resume (void);
void aud_run (void);
void aud_quit (void);
void aud_cleanup (void);

const char * aud_history_get (int entry);
void aud_history_add (const char * path);

void aud_output_reset (int type);

#endif
