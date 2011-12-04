/*
 * types.h
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

#ifndef AUDACIOUS_TYPES_H
#define AUDACIOUS_TYPES_H

#define AUD_EQUALIZER_NBANDS 10
#define EQUALIZER_MAX_GAIN 12

enum {
 PLUGIN_TYPE_TRANSPORT,
 PLUGIN_TYPE_PLAYLIST,
 PLUGIN_TYPE_INPUT,
 PLUGIN_TYPE_EFFECT,
 PLUGIN_TYPE_OUTPUT,
 PLUGIN_TYPE_VIS,
 PLUGIN_TYPE_GENERAL,
 PLUGIN_TYPE_IFACE,
 PLUGIN_TYPES};

typedef struct PluginHandle PluginHandle;

typedef const struct _Plugin Plugin;
typedef const struct _TransportPlugin TransportPlugin;
typedef const struct _PlaylistPlugin PlaylistPlugin;
typedef const struct _InputPlugin InputPlugin;
typedef const struct _EffectPlugin EffectPlugin;
typedef const struct _OutputPlugin OutputPlugin;
typedef const struct _VisPlugin VisPlugin;
typedef const struct _GeneralPlugin GeneralPlugin;
typedef const struct _IfacePlugin IfacePlugin;

typedef struct _PluginPreferences PluginPreferences;
typedef struct _PreferencesWidget PreferencesWidget;

typedef struct {
    float track_gain; /* dB */
    float track_peak; /* 0-1 */
    float album_gain; /* dB */
    float album_peak; /* 0-1 */
} ReplayGainInfo;

#endif
