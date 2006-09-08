/*  Audacious - the quite insane multimedia player.
 *  Copyright (C) 2005-2006  Audacious team
 *
 *  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "hints.h"

#include <glib.h>
#include <gtk/gtk.h>

#include "equalizer.h"
#include "mainwin.h"
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

