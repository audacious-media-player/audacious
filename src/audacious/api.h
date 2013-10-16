/*
 * api.h
 * Copyright 2010-2013 John Lindgren
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

#ifndef AUDACIOUS_API_H
#define AUDACIOUS_API_H

/* API version.  Plugins are marked with this number at compile time.
 *
 * _AUD_PLUGIN_VERSION is the current version; _AUD_PLUGIN_VERSION_MIN is
 * the oldest one we are backward compatible with.  Plugins marked older than
 * _AUD_PLUGIN_VERSION_MIN or newer than _AUD_PLUGIN_VERSION are not loaded.
 *
 * Before releases that add new pointers to the end of the API tables, increment
 * _AUD_PLUGIN_VERSION but leave _AUD_PLUGIN_VERSION_MIN the same.
 *
 * Before releases that break backward compatibility (e.g. remove pointers from
 * the API tables), increment _AUD_PLUGIN_VERSION *and* set
 * _AUD_PLUGIN_VERSION_MIN to the same value. */

#define _AUD_PLUGIN_VERSION_MIN 45 /* 3.5-devel */
#define _AUD_PLUGIN_VERSION     45 /* 3.5-devel */

typedef const struct {
    const struct ConfigDBAPI * configdb_api;
    const struct DRCTAPI * drct_api;
    const struct InputAPI * input_api;
    const struct MiscAPI * misc_api;
    const struct PlaylistAPI * playlist_api;
    const struct PluginsAPI * plugins_api;
    char * verbose;
} AudAPITable;

#ifdef _AUDACIOUS_CORE
extern char verbose;
#else
extern AudAPITable * _aud_api_table;
#endif

#endif
