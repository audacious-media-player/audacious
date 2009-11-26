/*
 * audio.c
 * Copyright 2009 John Lindgren
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

#include <glib.h>

#include "audio.h"

void audio_amplify (gfloat * data, gint channels, gint frames, gfloat * factors)
{
    gfloat * end = data + channels * frames;
    gint channel;

    while (data < end)
    {
        for (channel = 0; channel < channels; channel ++)
        {
            * data = * data * factors[channel];
            data ++;
        }
    }
}
