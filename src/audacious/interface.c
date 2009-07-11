/*
 * Audacious2
 * Copyright (c) 2008 William Pitcock <nenolod@dereferenced.org>
 * Copyright (c) 2008 Tomasz Moń <desowin@gmail.com>
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

#include "interface.h"
#include "playback.h"
#include "playlist.h"
#include "ui_fileopener.h"
#include "ui_urlopener.h"
#include "ui_preferences.h"
#include "ui_jumptotrack.h"
#include "ui_credits.h"

/* interface abstraction layer */
static mowgli_dictionary_t *interface_dict_ = NULL;
static Interface *current_interface = NULL;

static InterfaceOps interface_ops = {
    .create_prefs_window = create_prefs_window,
    .show_prefs_window = show_prefs_window,
    .hide_prefs_window = hide_prefs_window,
    .destroy_prefs_window = destroy_prefs_window,

    .filebrowser_show = run_filebrowser,
    .urlopener_show = show_add_url_window,
    .jump_to_track_show = ui_jump_to_track,
    .aboutwin_show = show_about_window,
};

static InterfaceCbs interface_cbs = {
    .show_prefs_window = NULL,
};

void
interface_register(Interface *i)
{
    if (interface_dict_ == NULL)
        interface_dict_ = mowgli_dictionary_create(g_ascii_strcasecmp);

    mowgli_dictionary_add(interface_dict_, i->id, i);
}

void
interface_deregister(Interface *i)
{
    g_return_if_fail(interface_dict_ != NULL);

    mowgli_dictionary_delete(interface_dict_, i->id);
}

void
interface_run(Interface *i)
{
    current_interface = i;
    i->ops = &interface_ops;

    i->init(&interface_cbs);
}

void
interface_destroy(Interface *i)
{
    if (i->fini != NULL)
        i->fini();
}

Interface *
interface_get(gchar *id)
{
    if (interface_dict_ == NULL)
        return NULL;

    return mowgli_dictionary_retrieve(interface_dict_, id);
}

void
interface_foreach(int (*foreach_cb)(mowgli_dictionary_elem_t *delem, void *privdata), void *privdata)
{
    if (interface_dict_ == NULL)
        return;

    mowgli_dictionary_foreach(interface_dict_, foreach_cb, privdata);
}

const Interface *
interface_get_current(void)
{
    return current_interface;
}

void
interface_show_prefs_window(gboolean show)
{
    if (interface_cbs.show_prefs_window != NULL)
        interface_cbs.show_prefs_window(show);
    else
        g_message("Interface didn't register show_prefs_window function");
}

void
interface_show_prefs_handler(gpointer hook_data, gpointer user_data)
{
    gboolean show = GPOINTER_TO_INT(hook_data);
    interface_show_prefs_window(show);
}

void
register_interface_hooks(void)
{
    hook_associate("prefswin show",
                   (HookFunction) interface_show_prefs_handler,
                   NULL);
}

