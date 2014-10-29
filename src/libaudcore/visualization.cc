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

#include "interface.h"
#include "internal.h"

#include <assert.h>
#include <string.h>

#include "plugin.h"
#include "plugins.h"
#include "runtime.h"

static Index<VisFunc> vis_funcs[AUD_VIS_TYPES];
static Index<VisPlugin *> vis_plugins[AUD_VIS_TYPES];

static int running = false;
static int num_enabled = 0;

EXPORT void aud_vis_func_add (int type, VisFunc func)
{
    assert (type >= 0 && type < AUD_VIS_TYPES);
    vis_funcs[type].append (func);

    num_enabled ++;
    if (num_enabled == 1)
        vis_runner_enable (true);
}

EXPORT void aud_vis_func_remove (VisFunc func)
{
    int num_disabled = 0;

    auto is_match = [&] (VisFunc func2)
    {
        if (func2 != func)
            return false;

        num_disabled ++;
        return true;
    };

    for (int type = 0; type < AUD_VIS_TYPES; type ++)
        vis_funcs[type].remove_if (is_match, true);

    num_enabled -= num_disabled;
    if (! num_enabled)
        vis_runner_enable (false);
}

void vis_send_clear (void)
{
    for (VisFunc func : vis_funcs[AUD_VIS_TYPE_CLEAR])
        ((VisClearFunc) func) ();

    for (int type = 0; type < AUD_VIS_TYPES; type ++)
    {
        for (VisPlugin * vp : vis_plugins[type])
            vp->clear ();
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
    auto is_active = [] (int type)
        { return vis_funcs[type].len () || vis_plugins[type].len (); };

    float mono[512];
    float freq[256];

    if (is_active (AUD_VIS_TYPE_MONO_PCM) || is_active (AUD_VIS_TYPE_FREQ))
        pcm_to_mono (data, mono, channels);
    if (is_active (AUD_VIS_TYPE_FREQ))
        calc_freq (mono, freq);

    for (VisFunc func : vis_funcs[AUD_VIS_TYPE_MONO_PCM])
        ((VisMonoPCMFunc) func) (mono);
    for (VisPlugin * vp : vis_plugins[AUD_VIS_TYPE_MONO_PCM])
        vp->render_mono_pcm (mono);

    for (VisFunc func : vis_funcs[AUD_VIS_TYPE_MULTI_PCM])
        ((VisMultiPCMFunc) func) (data, channels);
    for (VisPlugin * vp : vis_plugins[AUD_VIS_TYPE_MULTI_PCM])
        vp->render_multi_pcm (data, channels);

    for (VisFunc func : vis_funcs[AUD_VIS_TYPE_FREQ])
        ((VisFreqFunc) func) (freq);
    for (VisPlugin * vp : vis_plugins[AUD_VIS_TYPE_FREQ])
        vp->render_freq (freq);
}

static bool vis_load (PluginHandle * plugin)
{
    AUDINFO ("Activating %s.\n", aud_plugin_get_name (plugin));
    VisPlugin * header = (VisPlugin *) aud_plugin_get_header (plugin);
    if (! header)
        return false;

    assert (header->vis_type >= 0 && header->vis_type < AUD_VIS_TYPES);
    vis_plugins[header->vis_type].append (header);

    num_enabled ++;
    if (num_enabled == 1)
        vis_runner_enable (true);

    return true;
}

static void vis_unload (PluginHandle * plugin)
{
    AUDINFO ("Deactivating %s.\n", aud_plugin_get_name (plugin));
    VisPlugin * header = (VisPlugin *) aud_plugin_get_header (plugin);
    if (! header)
        return;

    header->clear ();

    int num_disabled = 0;

    auto is_match = [&] (VisPlugin * header2)
    {
        if (header2 != header)
            return false;

        num_disabled ++;
        return true;
    };

    assert (header->vis_type >= 0 && header->vis_type < AUD_VIS_TYPES);
    vis_plugins[header->vis_type].remove_if (is_match, true);

    num_enabled -= num_disabled;
    if (! num_enabled)
        vis_runner_enable (false);
}

void vis_activate (bool activate)
{
    if (! activate == ! running)
        return;

    for (PluginHandle * plugin : aud_plugin_list (PLUGIN_TYPE_VIS))
    {
        if (! aud_plugin_get_enabled (plugin))
            continue;

        if (activate)
            vis_load (plugin);
        else
            vis_unload (plugin);
    }

    running = activate;
}

bool vis_plugin_start (PluginHandle * plugin)
{
    VisPlugin * vp = (VisPlugin *) aud_plugin_get_header (plugin);
    if (! vp || ! vp->init ())
        return false;

    if (running)
        vis_load (plugin);

    return true;
}

void vis_plugin_stop (PluginHandle * plugin)
{
    VisPlugin * vp = (VisPlugin *) aud_plugin_get_header (plugin);
    if (! vp)
        return;

    if (running)
        vis_unload (plugin);

    vp->cleanup ();
}
