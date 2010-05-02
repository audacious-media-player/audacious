/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
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

#include "visualization.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "fft.h"
#include "input.h"
#include "main.h"
#include "playback.h"
#include "pluginenum.h"
#include "plugin.h"
#include "vis_runner.h"

VisPluginData vp_data = {
    NULL,
    NULL,
    FALSE
};

static void send_audio (void * hook_data, void * user_data);

GList *
get_vis_list(void)
{
    return vp_data.vis_list;
}

GList *
get_vis_enabled_list(void)
{
    return vp_data.enabled_list;
}

void
vis_disable_plugin(VisPlugin * vp)
{
    vis_enable_plugin(vp, FALSE);
}

void
vis_playback_start(void)
{
    GList *node;
    VisPlugin *vp;

    if (vp_data.playback_started)
        return;

    for (node = vp_data.enabled_list; node; node = g_list_next(node)) {
        vp = node->data;
        if (vp->playback_start)
        {
            plugin_set_current((Plugin *)vp);
            vp->playback_start();
        }
    }
    vp_data.playback_started = TRUE;
}

void
vis_playback_stop(void)
{
    GList *node;
    VisPlugin *vp;

    if (!vp_data.playback_started)
        return;

    for (node = vp_data.enabled_list; node; node = g_list_next(node)) {
        vp = node->data;
        if (vp->playback_stop)
        {
            plugin_set_current((Plugin *)vp);
            vp->playback_stop();
        }
    }
    vp_data.playback_started = FALSE;
}

void
vis_enable_plugin(VisPlugin *vp, gboolean enable)
{
    if (!vp)
        return;

    if (enable && !vp->enabled)
    {
        if (vp_data.enabled_list == NULL)
            vis_runner_add_hook(send_audio, 0);

        vp_data.enabled_list = g_list_append(vp_data.enabled_list, vp);
        if (vp->init)
        {
            plugin_set_current((Plugin *)vp);
            vp->init();

            if (vp->get_widget != NULL)
            {
                GtkWidget *w = vp->get_widget();

                interface_run_gtk_plugin(w, vp->description);
            }
        }
        if (playback_get_playing() && vp->playback_start)
        {
            plugin_set_current((Plugin *)vp);
            vp->playback_start();
        }
    }
    else if (!enable && vp->enabled) {
        vp_data.enabled_list = g_list_remove(vp_data.enabled_list, vp);
        if (playback_get_playing() && vp->playback_stop)
        {
            plugin_set_current((Plugin *)vp);
            vp->playback_stop();
        }
        if (vp->get_widget != NULL)
        {
            GtkWidget *w = vp->get_widget();

            interface_stop_gtk_plugin(w);
        }
        if (vp->cleanup)
        {
            plugin_set_current((Plugin *)vp);
            vp->cleanup();
        }

        if (vp_data.enabled_list == NULL)
            vis_runner_remove_hook(send_audio);
    }

    vp->enabled = enable;
}

gchar *
vis_stringify_enabled_list(void)
{
    gchar *enalist = NULL, *tmp, *tmp2;
    GList *node = vp_data.enabled_list;

    if (g_list_length(node)) {
        enalist = g_path_get_basename(VIS_PLUGIN(node->data)->filename);
        for (node = g_list_next(node); node != NULL; node = g_list_next(node)) {
            tmp = enalist;
            tmp2 = g_path_get_basename(VIS_PLUGIN(node->data)->filename);
            enalist = g_strconcat(tmp, ",", tmp2, NULL);
            g_free(tmp);
            g_free(tmp2);
        }
    }
    return enalist;
}

static gboolean
vis_queued_enable_cb(gpointer vp)
{
    VisPlugin *p = vp;

    vis_enable_plugin(p, TRUE);

    return FALSE;
}

void
vis_enable_from_stringified_list(gchar * list)
{
    gchar **plugins, *base;
    GList *node;
    gint i;

    if (!list || !strcmp(list, ""))
        return;
    plugins = g_strsplit(list, ",", 0);
    for (i = 0; plugins[i]; i++) {
        for (node = vp_data.vis_list; node != NULL; node = g_list_next(node)) {
            base = g_path_get_basename(VIS_PLUGIN(node->data)->filename);

            /* don't start the vis plugin until our eventloop and interface is up. --nenolod */
            if (!strcmp(plugins[i], base))
                g_idle_add(vis_queued_enable_cb, node->data);

            g_free(base);
        }
    }
    g_strfreev(plugins);
}

void
calc_stereo_pcm(gint16 dest[2][512], gint16 src[2][512], gint nch)
{
    memcpy(dest[0], src[0], 512 * sizeof(gint16));
    if (nch == 1)
        memcpy(dest[1], src[0], 512 * sizeof(gint16));
    else
        memcpy(dest[1], src[1], 512 * sizeof(gint16));
}

void
calc_mono_pcm(gint16 dest[2][512], gint16 src[2][512], gint nch)
{
    gint i;
    gint16 *d, *sl, *sr;

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

static void
calc_freq(gint16 * dest, gint16 * src)
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

void
calc_mono_freq(gint16 dest[2][256], gint16 src[2][512], gint nch)
{
    gint i;
    gint16 *d, *sl, *sr, tmp[512];

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

void
calc_stereo_freq(gint16 dest[2][256], gint16 src[2][512], gint nch)
{
    calc_freq(dest[0], src[0]);

    if (nch == 2)
        calc_freq(dest[1], src[1]);
    else
        memcpy(dest[1], dest[0], 256 * sizeof(gint16));
}

static void send_audio (void * hook_data, void * user_data)
{
    VisNode * vis_node = hook_data;
    int16_t (* pcm_data) [512] = vis_node->data;
    int nch = vis_node->nch;
    GList *node = vp_data.enabled_list;
    VisPlugin *vp;
    gint16 mono_freq[2][256], stereo_freq[2][256];
    gboolean mono_freq_calced = FALSE, stereo_freq_calced = FALSE;
    gint16 mono_pcm[2][512], stereo_pcm[2][512];
    gboolean mono_pcm_calced = FALSE, stereo_pcm_calced = FALSE;

    while (node) {
        vp = node->data;
        if (vp->num_pcm_chs_wanted > 0 && vp->render_pcm) {
            if (vp->num_pcm_chs_wanted == 1) {
                if (!mono_pcm_calced) {
                    calc_mono_pcm(mono_pcm, pcm_data, nch);
                    mono_pcm_calced = TRUE;
                }
                plugin_set_current((Plugin *)vp);
                vp->render_pcm(mono_pcm);
            }
            else {
                if (!stereo_pcm_calced) {
                    calc_stereo_pcm(stereo_pcm, pcm_data, nch);
                    stereo_pcm_calced = TRUE;
                }
                plugin_set_current((Plugin *)vp);
                vp->render_pcm(stereo_pcm);
            }
        }
        if (vp->num_freq_chs_wanted > 0 && vp->render_freq) {
            if (vp->num_freq_chs_wanted == 1) {
                if (!mono_freq_calced) {
                    calc_mono_freq(mono_freq, pcm_data, nch);
                    mono_freq_calced = TRUE;
                }
                plugin_set_current((Plugin *)vp);
                vp->render_freq(mono_freq);
            }
            else {
                if (!stereo_freq_calced) {
                    calc_stereo_freq(stereo_freq, pcm_data, nch);
                    stereo_freq_calced = TRUE;
                }
                plugin_set_current((Plugin *)vp);
                vp->render_freq(stereo_freq);
            }
        }
        node = g_list_next(node);
    }
}
