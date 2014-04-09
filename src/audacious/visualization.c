/*
 * visualization.c
 * Copyright 2010-2011 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <glib.h>
#include <string.h>

#include <libaudcore/runtime.h>

#include "fft.h"
#include "misc.h"
#include "plugin.h"
#include "visualization.h"
#include "vis_runner.h"

static GList * vis_funcs[AUD_VIS_TYPES];

typedef struct {
    PluginHandle * plugin;
    VisPlugin * header;
} LoadedVis;

static int running = FALSE;
static GList * loaded_vis_plugins = NULL;

void vis_func_add (int type, VisFunc func)
{
    g_return_if_fail (type >= 0 && type < AUD_VIS_TYPES);
    vis_funcs[type] = g_list_prepend (vis_funcs[type], (void *) func);

    vis_runner_enable (TRUE);
}

void vis_func_remove (VisFunc func)
{
    bool_t disable = TRUE;

    for (int i = 0; i < AUD_VIS_TYPES; i ++)
    {
        vis_funcs[i] = g_list_remove_all (vis_funcs[i], (void *) func);
        if (vis_funcs[i])
            disable = FALSE;
    }

    if (disable)
        vis_runner_enable (FALSE);
}

void vis_send_clear (void)
{
    for (GList * node = vis_funcs[AUD_VIS_TYPE_CLEAR]; node; node = node->next)
    {
        void (* func) (void) = (void (*) (void)) node->data;
        func ();
    }
}

static void pcm_to_mono (const float * data, float * mono, int channels)
{
    if (channels == 1)
        memcpy (mono, data, sizeof (float) * 512);
    else
    {
        float * set = mono;
        while (set < & mono[512])
        {
            * set ++ = (data[0] + data[1]) / 2;
            data += channels;
        }
    }
}

void vis_send_audio (const float * data, int channels)
{
    float mono[512];
    float freq[256];

    if (vis_funcs[AUD_VIS_TYPE_MONO_PCM] || vis_funcs[AUD_VIS_TYPE_FREQ])
        pcm_to_mono (data, mono, channels);
    if (vis_funcs[AUD_VIS_TYPE_FREQ])
        calc_freq (mono, freq);

    for (GList * node = vis_funcs[AUD_VIS_TYPE_MONO_PCM]; node; node = node->next)
    {
        void (* func) (const float *) = (void (*) (const float *)) node->data;
        func (mono);
    }

    for (GList * node = vis_funcs[AUD_VIS_TYPE_MULTI_PCM]; node; node = node->next)
    {
        void (* func) (const float *, int) = (void (*) (const float *, int)) node->data;
        func (data, channels);
    }

    for (GList * node = vis_funcs[AUD_VIS_TYPE_FREQ]; node; node = node->next)
    {
        void (* func) (const float *) = (void (*) (const float *)) node->data;
        func (freq);
    }
}

static int vis_find_cb (LoadedVis * vis, PluginHandle * plugin)
{
    return (vis->plugin == plugin) ? 0 : -1;
}

static void vis_load (PluginHandle * plugin)
{
    GList * node = g_list_find_custom (loaded_vis_plugins, plugin,
     (GCompareFunc) vis_find_cb);
    if (node != NULL)
        return;

    AUDDBG ("Activating %s.\n", plugin_get_name (plugin));
    VisPlugin * header = plugin_get_header (plugin);
    g_return_if_fail (header != NULL);

    LoadedVis * vis = g_slice_new (LoadedVis);
    vis->plugin = plugin;
    vis->header = header;

    if (PLUGIN_HAS_FUNC (header, clear))
        vis_func_add (AUD_VIS_TYPE_CLEAR, (VisFunc) header->clear);
    if (PLUGIN_HAS_FUNC (header, render_mono_pcm))
        vis_func_add (AUD_VIS_TYPE_MONO_PCM, (VisFunc) header->render_mono_pcm);
    if (PLUGIN_HAS_FUNC (header, render_multi_pcm))
        vis_func_add (AUD_VIS_TYPE_MULTI_PCM, (VisFunc) header->render_multi_pcm);
    if (PLUGIN_HAS_FUNC (header, render_freq))
        vis_func_add (AUD_VIS_TYPE_FREQ, (VisFunc) header->render_freq);

    loaded_vis_plugins = g_list_prepend (loaded_vis_plugins, vis);
}

static void vis_unload (PluginHandle * plugin)
{
    GList * node = g_list_find_custom (loaded_vis_plugins, plugin,
     (GCompareFunc) vis_find_cb);
    if (node == NULL)
        return;

    AUDDBG ("Deactivating %s.\n", plugin_get_name (plugin));
    LoadedVis * vis = node->data;
    loaded_vis_plugins = g_list_delete_link (loaded_vis_plugins, node);

    VisPlugin * header = vis->header;
    if (PLUGIN_HAS_FUNC (header, clear))
        vis_func_remove ((VisFunc) header->clear);
    if (PLUGIN_HAS_FUNC (header, render_mono_pcm))
        vis_func_remove ((VisFunc) header->render_mono_pcm);
    if (PLUGIN_HAS_FUNC (header, render_multi_pcm))
        vis_func_remove ((VisFunc) header->render_multi_pcm);
    if (PLUGIN_HAS_FUNC (header, render_freq))
        vis_func_remove ((VisFunc) header->render_freq);

    if (PLUGIN_HAS_FUNC (header, clear))
        header->clear ();

    g_slice_free (LoadedVis, vis);
}

static bool_t vis_init_cb (PluginHandle * plugin)
{
    vis_load (plugin);
    return TRUE;
}

static void vis_cleanup_cb (LoadedVis * vis)
{
    vis_unload (vis->plugin);
}

void vis_activate (bool_t activate)
{
    if (! activate == ! running)
        return;

    if (activate)
        plugin_for_enabled (PLUGIN_TYPE_VIS, (PluginForEachFunc) vis_init_cb, NULL);
    else
        g_list_foreach (loaded_vis_plugins, (GFunc) vis_cleanup_cb, NULL);

    running = activate;
}

bool_t vis_plugin_start (PluginHandle * plugin)
{
    VisPlugin * vp = plugin_get_header (plugin);
    g_return_val_if_fail (vp != NULL, FALSE);

    if (vp->init != NULL && ! vp->init ())
        return FALSE;

    if (running)
        vis_load (plugin);

    return TRUE;
}

void vis_plugin_stop (PluginHandle * plugin)
{
    VisPlugin * vp = plugin_get_header (plugin);
    g_return_if_fail (vp != NULL);

    if (running)
        vis_unload (plugin);

    if (vp->cleanup != NULL)
        vp->cleanup ();
}
