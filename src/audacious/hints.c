/*
 * audacious: Cross-platform multimedia player.
 * hints.c: Support for NetWM hints.
 *
 * Copyright (c) 2005-2007 Audacious development team.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "hints.h"

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

