/*
 * api.h
 * Copyright 2010 John Lindgren
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

typedef const struct {
    const struct ConfigDBAPI * configdb_api;
    const struct DRCTAPI * drct_api;
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
