/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
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

#include "platform/smartinclude.h"

#include <gtk/gtkmain.h>
#include <glib-object.h>
#include <glib/gmacros.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkwindow.h>

#include "main.h"
#include "ui_dock.h"
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

