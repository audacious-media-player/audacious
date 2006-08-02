/*
 * libxmmsstandard - XMMS plugin.
 * Copyright (C) 2000-2001 Konstantin Laevsky <debleek63@yahoo.com>
 *
 * audacious port of the voice removal code from libxmmsstandard
 * by Thomas Cort <linuxgeek@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA
 *
 */

#include <gtk/gtk.h>
#include <audacious/plugin.h>
#include <libaudacious/configdb.h>

#include "../../../config.h"

#define PLUGIN_NAME "voice_removal " PACKAGE_VERSION

static int apply_effect (gpointer *d, gint length, AFormat afmt,
			gint srate, gint nch);

static EffectPlugin xmms_plugin = {
	NULL,
	NULL,
	PLUGIN_NAME,
	NULL,
	NULL,
	NULL,
	NULL,
	apply_effect,
	NULL
};

EffectPlugin *get_eplugin_info (void) {
	return &xmms_plugin;
}

static int apply_effect (gpointer *d, gint length, AFormat afmt,
			gint srate, gint nch) {
	int x;
	int left, right;
	gint16 *dataptr = (gint16 *) * d;

	if (!((afmt == FMT_S16_NE) ||
		(afmt == FMT_S16_LE && G_BYTE_ORDER == G_LITTLE_ENDIAN) ||
		(afmt == FMT_S16_BE && G_BYTE_ORDER == G_BIG_ENDIAN))   ||
		(nch != 2)) {
		return length;
	}

	for (x = 0; x < length; x += 4) {
		left  = CLAMP(dataptr[1] - dataptr[0], -32768, 32767);
		right = CLAMP(dataptr[0] - dataptr[1], -32768, 32767);
		dataptr[0] = left;
		dataptr[1] = right;
		dataptr += 2;
	}

	return (length);
}
