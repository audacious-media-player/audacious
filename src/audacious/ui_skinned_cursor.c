/*
 * audacious: Cross-platform multimedia player.
 * ui_skinned_cursor.c: Common code for skinned UI cursors.        
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

#include "platform/smartinclude.h"

#include <gtk/gtkmain.h>
#include <glib-object.h>
#include <glib/gmacros.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkwindow.h>

#include "main.h"
#include "dock.h"
#include "ui_skinned_window.h"
#include "ui_skinned_cursor.h"

void
ui_skinned_cursor_set(GtkWidget * window)
{
    static GdkCursor *cursor = NULL;

    if (window == NULL)
    {
        if (cursor != NULL)
        {
            gdk_cursor_unref(cursor);
            cursor = NULL;
        }

        return;
    }

    if (cursor == NULL)
        cursor = gdk_cursor_new(GDK_LEFT_PTR);

    gdk_window_set_cursor(window->window, cursor);
}

