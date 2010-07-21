/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2010  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team
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

#include <glib.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include <libaudcore/hook.h>

#include "debug.h"
#include "fft.h"
#include "interface.h"
#include "misc.h"
#include "playback.h"
#include "plugin.h"
#include "plugins.h"
#include "visualization.h"

typedef struct {
    PluginHandle * plugin;
    VisPlugin * header;
    GtkWidget * widget;
    gboolean started;
} LoadedVis;

static GList * loaded_vis_plugins = NULL;

void calc_stereo_pcm (VisPCMData dest, const VisPCMData src, gint nch)
{
    memcpy(dest[0], src[0], 512 * sizeof(gint16));
    if (nch == 1)
        memcpy(dest[1], src[0], 512 * sizeof(gint16));
    else
        memcpy(dest[1], src[1], 512 * sizeof(gint16));
}

void calc_mono_pcm (VisPCMData dest, const VisPCMData src, gint nch)
{
    gint i;
    gint16 *d;
    const gint16 *sl, *sr;

    if (nch == 1)
        memcpy(dest[0], src[0], 512 * sizeof(gint16));
    else {
        d = dest[0];
        sl = src[0];
        sr = src[1];
        for (i = 0; i < 512; i++) {
            *(d++) = (*(sl++) + *(sr++)) >> 1;
        }
    }
}

static void calc_freq (gint16 * dest, const gint16 * src)
{
    static fft_state *state = NULL;
    gfloat tmp_out[257];
    gint i;

    if (!state)
        state = fft_init();

    fft_perform(src, tmp_out, state);

    for (i = 0; i < 256; i++)
        dest[i] = ((gint) sqrt(tmp_out[i + 1])) >> 8;
}

void calc_mono_freq (VisFreqData dest, const VisPCMData src, gint nch)
{
    gint i;
    gint16 *d, tmp[512];
    const gint16 *sl, *sr;

    if (nch == 1)
        calc_freq(dest[0], src[0]);
    else {
        d = tmp;
        sl = src[0];
        sr = src[1];
        for (i = 0; i < 512; i++) {
            *(d++) = (*(sl++) + *(sr++)) >> 1;
        }
        calc_freq(dest[0], tmp);
    }
}

void calc_stereo_freq (VisFreqData dest, const VisPCMData src, gint nch)
{
    calc_freq(dest[0], src[0]);

    if (nch == 2)
        calc_freq(dest[1], src[1]);
    else
        memcpy(dest[1], dest[0], 256 * sizeof(gint16));
}

static void send_audio (const VisNode * vis_node)
{
    gint16 mono_freq[2][256], stereo_freq[2][256];
    gboolean mono_freq_calced = FALSE, stereo_freq_calced = FALSE;
    gint16 mono_pcm[2][512], stereo_pcm[2][512];
    gboolean mono_pcm_calced = FALSE, stereo_pcm_calced = FALSE;

    for (GList * node = loaded_vis_plugins; node != NULL; node = node->next)
    {
        VisPlugin * vp = ((LoadedVis *) node->data)->header;

        if (vp->num_pcm_chs_wanted > 0 && vp->render_pcm) {
            if (vp->num_pcm_chs_wanted == 1) {
                if (!mono_pcm_calced) {
                    calc_mono_pcm(mono_pcm, vis_node->data, vis_node->nch);
                    mono_pcm_calced = TRUE;
                }
                vp->render_pcm(mono_pcm);
            }
            else {
                if (!stereo_pcm_calced) {
                    calc_stereo_pcm(stereo_pcm, vis_node->data, vis_node->nch);
                    stereo_pcm_calced = TRUE;
                }
                vp->render_pcm(stereo_pcm);
            }
        }
        if (vp->num_freq_chs_wanted > 0 && vp->render_freq) {
            if (vp->num_freq_chs_wanted == 1) {
                if (!mono_freq_calced) {
                    calc_mono_freq(mono_freq, vis_node->data, vis_node->nch);
                    mono_freq_calced = TRUE;
                }
                vp->render_freq(mono_freq);
            }
            else {
                if (!stereo_freq_calced) {
                    calc_stereo_freq(stereo_freq, vis_node->data, vis_node->nch);
                    stereo_freq_calced = TRUE;
                }
                vp->render_freq(stereo_freq);
            }
        }
    }
}

static void vis_start (LoadedVis * vis)
{
    if (vis->started)
        return;
    AUDDBG ("Starting %s.\n", plugin_get_name (vis->plugin));
    if (vis->header->playback_start != NULL)
        vis->header->playback_start ();
    vis->started = TRUE;
}

static void vis_start_all (void)
{
    g_list_foreach (loaded_vis_plugins, (GFunc) vis_start, NULL);
}

static void vis_stop (LoadedVis * vis)
{
    if (! vis->started)
        return;
    AUDDBG ("Stopping %s.\n", plugin_get_name (vis->plugin));
    if (vis->header->playback_stop != NULL)
        vis->header->playback_stop ();
    vis->started = FALSE;
}

static void vis_stop_all (void)
{
    g_list_foreach (loaded_vis_plugins, (GFunc) vis_stop, NULL);
}

static gint vis_find_cb (LoadedVis * vis, PluginHandle * plugin)
{
    return (vis->plugin == plugin) ? 0 : -1;
}

static void vis_load (PluginHandle * plugin)
{
    GList * node = g_list_find_custom (loaded_vis_plugins, plugin,
     (GCompareFunc) vis_find_cb);
    if (node != NULL)
        return;

    AUDDBG ("Loading %s.\n", plugin_get_name (plugin));
    VisPlugin * header = plugin_get_header (plugin);
    g_return_if_fail (header != NULL);

    if (header->init != NULL)
        header->init ();

    LoadedVis * vis = g_slice_new (LoadedVis);
    vis->plugin = plugin;
    vis->header = header;
    vis->widget = NULL;
    vis->started = FALSE;

    if (header->get_widget != NULL)
        vis->widget = header->get_widget ();

    if (vis->widget != NULL)
    {
        AUDDBG ("Adding %s to interface.\n", plugin_get_name (plugin));
        g_signal_connect (vis->widget, "destroy", (GCallback)
         gtk_widget_destroyed, & vis->widget);
        interface_add_plugin_widget (plugin, vis->widget);
    }

    if (playback_get_playing ())
        vis_start (vis);

    if (loaded_vis_plugins == NULL)
        vis_runner_add_hook ((VisHookFunc) send_audio, NULL);

    loaded_vis_plugins = g_list_prepend (loaded_vis_plugins, vis);
}

static void vis_unload (PluginHandle * plugin)
{
    GList * node = g_list_find_custom (loaded_vis_plugins, plugin,
     (GCompareFunc) vis_find_cb);
    if (node == NULL)
        return;

    AUDDBG ("Unloading %s.\n", plugin_get_name (plugin));
    LoadedVis * vis = node->data;
    loaded_vis_plugins = g_list_delete_link (loaded_vis_plugins, node);

    if (loaded_vis_plugins == NULL)
        vis_runner_remove_hook ((VisHookFunc) send_audio);

    if (vis->widget != NULL)
    {
        AUDDBG ("Removing %s from interface.\n", plugin_get_name (plugin));
        interface_remove_plugin_widget (plugin, vis->widget);
        g_return_if_fail (vis->widget == NULL); /* not destroyed? */
    }

    if (vis->header->cleanup != NULL)
        vis->header->cleanup ();

    g_slice_free (LoadedVis, vis);
}

static gboolean vis_init_cb (PluginHandle * plugin)
{
    vis_load (plugin);
    return TRUE;
}

void vis_init (void)
{
    plugin_for_enabled (PLUGIN_TYPE_VIS, (PluginForEachFunc) vis_init_cb, NULL);

    hook_associate ("playback begin", (HookFunction) vis_start_all, NULL);
    hook_associate ("playback stop", (HookFunction) vis_stop_all, NULL);
}

static void vis_cleanup_cb (LoadedVis * vis)
{
    vis_unload (vis->plugin);
}

void vis_cleanup (void)
{
    hook_dissociate ("playback begin", (HookFunction) vis_start_all);
    hook_dissociate ("playback stop", (HookFunction) vis_stop_all);

    g_list_foreach (loaded_vis_plugins, (GFunc) vis_cleanup_cb, NULL);
}

void vis_plugin_enable (PluginHandle * plugin, gboolean enable)
{
    plugin_set_enabled (plugin, enable);

    if (enable)
        vis_load (plugin);
    else
        vis_unload (plugin);
}
