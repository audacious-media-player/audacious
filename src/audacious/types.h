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

#include <glib.h>

#define AUD_EQUALIZER_NBANDS 10
#define EQUALIZER_MAX_GAIN 12

typedef struct _Plugin Plugin;
typedef struct _InputPlugin InputPlugin;
typedef struct _OutputPlugin OutputPlugin;
typedef struct _EffectPlugin EffectPlugin;
typedef struct _GeneralPlugin GeneralPlugin;
typedef struct _VisPlugin VisPlugin;

#define PLUGIN(x) ((Plugin *) (x))
#define INPUT_PLUGIN(x) ((InputPlugin *) (x))
#define OUTPUT_PLUGIN(x) ((OutputPlugin *) (x))
#define EFFECT_PLUGIN(x) ((EffectPlugin *) (x))
#define GENERAL_PLUGIN(x) ((GeneralPlugin *) (x))
#define VIS_PLUGIN(x) ((VisPlugin *) (x))

typedef struct _Interface Interface;
typedef struct _PluginPreferences PluginPreferences;
typedef struct _PreferencesWidget PreferencesWidget;

#endif
