/*
 * types.h
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
