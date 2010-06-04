/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#ifndef AUDACIOUS_EFFECT_H
#define AUDACIOUS_EFFECT_H

#include <glib.h>

#include "audacious/plugin.h"

typedef struct _EffectPluginData EffectPluginData;

struct _EffectPluginData {
    GList *effect_list;
    GList *enabled_list;
};

GList *get_effect_list(void);
GList *get_effect_enabled_list(void);
void effect_enable_plugin(EffectPlugin *ep, gboolean enable);
gchar *effect_stringify_enabled_list(void);
void effect_enable_from_stringified_list(const gchar * list);
gint effect_do_mod_samples(gpointer * data, gint length, AFormat fmt,
	gint srate, gint nch);
void effect_do_query_format(AFormat * fmt, gint * rate, gint * nch);
void effect_flow(FlowContext *context);

extern EffectPluginData ep_data;

void new_effect_start (gint * channels, gint * rate);
void new_effect_process (gfloat * * data, gint * samples);
void new_effect_flush (void);
void new_effect_finish (gfloat * * data, gint * samples);
gint new_effect_decoder_to_output_time (gint time);
gint new_effect_output_to_decoder_time (gint time);

#endif /* AUDACIOUS_EFFECT_H */
