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

/* common events. */
static gint update_vis_timeout_source = 0;

static gboolean
interface_common_update_vis(gpointer user_data)
{
    input_update_vis(playback_get_time());
    return TRUE;
}

static void
interface_common_playback_seek(gpointer user_data, gpointer hook_data)
{
    free_vis_data();
}

static void
interface_common_playback_begin(gpointer user_data, gpointer hook_data)
{
    /* update vis info about 100 times a second */
    free_vis_data();
    update_vis_timeout_source =
        g_timeout_add(10, (GSourceFunc) interface_common_update_vis, NULL);
}

static void
interface_common_playback_stop(gpointer user_data, gpointer hook_data)
{
    if (update_vis_timeout_source) {
        g_source_remove(update_vis_timeout_source);
        update_vis_timeout_source = 0;
    }
}

static void
interface_common_hooks_associate(void)
{
    hook_associate("playback begin", (HookFunction) interface_common_playback_begin, NULL);
    hook_associate("playback stop", (HookFunction) interface_common_playback_stop, NULL);
    hook_associate("playback seek", (HookFunction) interface_common_playback_seek, NULL);
}

static void
interface_common_hooks_dissociate(void)
{
    hook_dissociate("playback begin", (HookFunction) interface_common_playback_begin);
    hook_dissociate("playback stop", (HookFunction) interface_common_playback_stop);
    hook_dissociate("playback seek", (HookFunction) interface_common_playback_seek);
}

/* interface abstraction layer */
static mowgli_dictionary_t *interface_dict_ = NULL;
static Interface *current_interface = NULL;

static InterfaceOps interface_ops = {
    .create_prefs_window = create_prefs_window,
    .show_prefs_window = show_prefs_window,
    .hide_prefs_window = hide_prefs_window,

    .filebrowser_show = run_filebrowser,
    .urlopener_show = show_add_url_window,
    .jump_to_track_show = ui_jump_to_track,
    .aboutwin_show = show_about_window,
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

    /* do common initialization */
    interface_common_hooks_associate();

    if (playback_get_playing ())
        interface_common_playback_begin (0, 0);

    i->init();
}

void
interface_destroy(Interface *i)
{
    /* do common cleanups */
    interface_common_hooks_dissociate();

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
