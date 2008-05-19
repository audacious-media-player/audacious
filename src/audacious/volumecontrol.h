/*
 * Audacious
 * Copyright (c) 2007 William Pitcock
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#include <glib.h>

#include "flow.h"

#ifndef AUDACIOUS_VOLUMECONTROL_H
#define AUDACIOUS_VOLUMECONTROL_H

void volumecontrol_pad_audio(gpointer data, gint length, AFormat fmt, 
    gint channels);
void volumecontrol_get_volume_state(gint *l, gint *r);
void volumecontrol_set_volume_state(gint l, gint r);
void volumecontrol_flow(FlowContext *context);

#endif /* AUDACIOUS_VOLUMECONTROL_H */
