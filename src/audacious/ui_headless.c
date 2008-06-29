/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2008  Audacious development team
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "interface.h"
#include "playlist.h"
#include "input.h"

static gboolean
aud_headless_iteration(gpointer unused)
{
    free_vis_data();
    return TRUE;
}

static gboolean
_ui_initialize(void)
{
    g_timeout_add(10, aud_headless_iteration, NULL);
    g_main_loop_run(g_main_loop_new(NULL, TRUE));

    return TRUE;
}

static Interface headless_interface = {
    .id = "headless",
    .desc = N_("Headless Interface"),
    .init = _ui_initialize,
};

void
ui_populate_headless_interface(void)
{
    interface_register(&headless_interface);
}
