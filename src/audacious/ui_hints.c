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

#include "ui_hints.h"

#include <glib.h>
#include <gtk/gtk.h>

#include "ui_equalizer.h"
#include "ui_main.h"
#include "ui_playlist.h"

#include "platform/smartinclude.h"

void
hint_set_always(gboolean always)
{
    gtk_window_set_keep_above(GTK_WINDOW(mainwin), always);
    gtk_window_set_keep_above(GTK_WINDOW(equalizerwin), always);
    gtk_window_set_keep_above(GTK_WINDOW(playlistwin), always);
}

void
hint_set_sticky(gboolean sticky)
{
    if (sticky) {
        gtk_window_stick(GTK_WINDOW(mainwin));
        gtk_window_stick(GTK_WINDOW(equalizerwin));
        gtk_window_stick(GTK_WINDOW(playlistwin));
    }
    else {
        gtk_window_unstick(GTK_WINDOW(mainwin));
        gtk_window_unstick(GTK_WINDOW(equalizerwin));
        gtk_window_unstick(GTK_WINDOW(playlistwin));
    }
}

