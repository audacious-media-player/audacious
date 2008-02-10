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
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "fft.h"
#include "input.h"
#include "main.h"
#include "playback.h"
#include "pluginenum.h"
#include "plugin.h"
#include "ui_preferences.h"

VisPluginData vp_data = {
    NULL,
    NULL,
    FALSE
};

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
    gint i = g_list_index(vp_data.vis_list, vp);
    enable_vis_plugin(i, FALSE);
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
    GList *node = vp_data.enabled_list;
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
enable_vis_plugin(gint i, gboolean enable)
{
    GList *node = g_list_nth(vp_data.vis_list, i);
    VisPlugin *vp;

    if (!node || !(node->data))
        return;
    vp = node->data;

    if (enable && !vp->enabled) {
        vp_data.enabled_list = g_list_append(vp_data.enabled_list, vp);
        if (vp->init)
	{
            plugin_set_current((Plugin *)vp);
            vp->init();
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
        if (vp->cleanup)
	{
            plugin_set_current((Plugin *)vp);
            vp->cleanup();
	}
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

void
vis_enable_from_stringified_list(gchar * list)
{
    gchar **plugins, *base;
    GList *node;
    gint i;
    VisPlugin *vp;

    if (!list || !strcmp(list, ""))
        return;
    plugins = g_strsplit(list, ",", 0);
    for (i = 0; plugins[i]; i++) {
        for (node = vp_data.vis_list; node != NULL; node = g_list_next(node)) {
            base = g_path_get_basename(VIS_PLUGIN(node->data)->filename);
            if (!strcmp(plugins[i], base)) {
                vp = node->data;
                vp_data.enabled_list =
                    g_list_append(vp_data.enabled_list, vp);
                if (vp->init)
		{
                    plugin_set_current((Plugin *)vp);
                    vp->init();
		}
                if (playback_get_playing() && vp->playback_start)
		{
                    plugin_set_current((Plugin *)vp);
                    vp->playback_start();
		}
                vp->enabled = TRUE;
            }
            g_free(base);
        }
    }
    g_strfreev(plugins);
}

static void
calc_stereo_pcm(gint16 dest[2][512], gint16 src[2][512], gint nch)
{
    memcpy(dest[0], src[0], 512 * sizeof(gint16));
    if (nch == 1)
        memcpy(dest[1], src[0], 512 * sizeof(gint16));
    else
        memcpy(dest[1], src[1], 512 * sizeof(gint16));
}

static void
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

static void
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

static void
calc_stereo_freq(gint16 dest[2][256], gint16 src[2][512], gint nch)
{
    calc_freq(dest[0], src[0]);

    if (nch == 2)
        calc_freq(dest[1], src[1]);
    else
        memcpy(dest[1], dest[0], 256 * sizeof(gint16));
}

void
vis_send_data(gint16 pcm_data[2][512], gint nch, gint length)
{
    GList *node = vp_data.enabled_list;
    VisPlugin *vp;
    gint16 mono_freq[2][256], stereo_freq[2][256];
    gboolean mono_freq_calced = FALSE, stereo_freq_calced = FALSE;
    gint16 mono_pcm[2][512], stereo_pcm[2][512];
    gboolean mono_pcm_calced = FALSE, stereo_pcm_calced = FALSE;
    guint8 intern_vis_data[512];
    gint i;

    if (!pcm_data || nch < 1) {
        if (cfg.vis_type != VIS_OFF) {
            if (cfg.player_shaded && cfg.player_visible)
                ui_svis_timeout_func(mainwin_svis, NULL);
            else
                ui_vis_timeout_func(mainwin_vis, NULL);
        }
        return;
    }

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

    if (cfg.vis_type == VIS_OFF)
        return;

    if (cfg.vis_type == VIS_ANALYZER) {
            /* Spectrum analyzer */
            /* 76 values */
            const gint long_xscale[] =
                { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                17, 18,
                19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
                34,
                35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
                50, 51,
                52, 53, 54, 55, 56, 57, 58, 61, 66, 71, 76, 81, 87, 93,
                100, 107,
                114, 122, 131, 140, 150, 161, 172, 184, 255
            };
            /* 20 values */
            const int short_xscale[] =
                { 0, 1, 2, 3, 4, 5, 6, 7, 8, 11, 15, 20, 27,
                36, 47, 62, 82, 107, 141, 184, 255
            };
            const double y_scale = 3.60673760222;   /* 20.0 / log(256) */
            const int *xscale;
            gint j, y, max;

            if (!mono_freq_calced)
                calc_mono_freq(mono_freq, pcm_data, nch);

            memset(intern_vis_data, 0, 75);

            if (cfg.analyzer_type == ANALYZER_BARS) {
                if (cfg.player_shaded) {
                    max = 13;
                }
                else {
                    max = 19;
                }
                xscale = short_xscale;
            }
            else {
                if (cfg.player_shaded) {
                    max = 37;
                }
                else {
                    max = 75;
                }
                xscale = long_xscale;
            }

            for (i = 0; i < max; i++) {
                for (j = xscale[i], y = 0; j < xscale[i + 1]; j++) {
                    if (mono_freq[0][j] > y)
                        y = mono_freq[0][j];
                }
                y >>= 7;
                if (y != 0) {
                    intern_vis_data[i] = log(y) * y_scale;
                    if (intern_vis_data[i] > 15)
                        intern_vis_data[i] = 15;
                }
                else
                    intern_vis_data[i] = 0;
            }
        
    }
    else if(cfg.vis_type == VIS_VOICEPRINT){
        if (cfg.player_shaded && cfg.player_visible) {
            /* VU */
            gint vu, val;

            if (!stereo_pcm_calced)
                calc_stereo_pcm(stereo_pcm, pcm_data, nch);
            vu = 0;
            for (i = 0; i < 512; i++) {
                val = abs(stereo_pcm[0][i]);
                if (val > vu)
                    vu = val;
            }
            intern_vis_data[0] = (vu * 37) >> 15;
            if (intern_vis_data[0] > 37)
                intern_vis_data[0] = 37;
            if (nch == 2) {
                vu = 0;
                for (i = 0; i < 512; i++) {
                    val = abs(stereo_pcm[1][i]);
                    if (val > vu)
                        vu = val;
                }
                intern_vis_data[1] = (vu * 37) >> 15;
                if (intern_vis_data[1] > 37)
                    intern_vis_data[1] = 37;
            }
            else
                intern_vis_data[1] = intern_vis_data[0];
        }
	else{ /*Voiceprint*/
	  if (!mono_freq_calced)
	    calc_mono_freq(mono_freq, pcm_data, nch);
	  memset(intern_vis_data, 0, 256);
	  /* For the values [0-16] we use the frequency that's 3/2 as much.
	  If we assume the 512 values calculated by calc_mono_freq to cover 0-22kHz linearly
	  we get a range of [0-16] * 3/2 * 22000/512 = [0-1,031] Hz.
	  Most stuff above that is harmonics and we want to utilize the 16 samples we have
	  to the max[tm]
	  */
	  for(i = 0; i < 50 ; i+=3){
	    intern_vis_data[i/3] += (mono_freq[0][i/2] >> 5);
	    
	    /*Boost frequencies above 257Hz a little*/
	    //if(i > 4 * 3)
	    //  intern_vis_data[i/3] += 8;
	  }
	}
    }
    else { /* (cfg.vis_type == VIS_SCOPE) */

        /* Oscilloscope */
        gint pos, step;

        if (!mono_pcm_calced)
            calc_mono_pcm(mono_pcm, pcm_data, nch);

        step = (length << 8) / 74;
        for (i = 0, pos = 0; i < 75; i++, pos += step) {
            intern_vis_data[i] = ((mono_pcm[0][pos >> 8]) >> 12) + 7;
            if (intern_vis_data[i] == 255)
                intern_vis_data[i] = 0;
            else if (intern_vis_data[i] > 12)
                intern_vis_data[i] = 12;
            /* Do not see the point of that? (comparison always false) -larne.
               if (intern_vis_data[i] < 0)
               intern_vis_data[i] = 0; */
        }
    }
    if (cfg.player_shaded && cfg.player_visible)
        ui_svis_timeout_func(mainwin_svis, intern_vis_data);
    else
        ui_vis_timeout_func(mainwin_vis, intern_vis_data);
}

void
vis_flow(FlowContext *context)
{
    input_add_vis_pcm(context->time, context->fmt, context->channels,
        context->len, context->data);
}
