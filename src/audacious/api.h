/*
 * api.h
 * Copyright 2010 John Lindgren
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

#ifndef AUDACIOUS_API_H
#define AUDACIOUS_API_H

typedef const struct {
    const struct ConfigDBAPI * configdb_api;
    const struct DRCTAPI * drct_api;
    const struct MiscAPI * misc_api;
    const struct PlaylistAPI * playlist_api;
    const struct PluginsAPI * plugins_api;
    struct _AudConfig * cfg;
} AudAPITable;

#ifndef _AUDACIOUS_CORE
extern AudAPITable * _aud_api_table;
#endif

#endif
