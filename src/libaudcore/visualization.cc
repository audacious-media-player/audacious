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

static int running = false;

EXPORT void aud_vis_func_add (int type, VisFunc func)
{
    assert (type >= 0 && type < AUD_VIS_TYPES);

    vis_funcs[type].append (func);
    vis_runner_enable (true);
}

EXPORT void aud_vis_func_remove (VisFunc func)
{
    bool disable = true;

    for (int type = 0; type < AUD_VIS_TYPES; type ++)
    {
        Index<VisFunc> & list = vis_funcs[type];

        for (int i = 0; i < list.len ();)
        {
            if (list[i] == func)
                list.remove (i, 1);
            else
                i ++;
        }

        if (list.len ())
            disable = false;
    }

    if (disable)
        vis_runner_enable (false);
}

void vis_send_clear (void)
{
    for (VisFunc func : vis_funcs[AUD_VIS_TYPE_CLEAR])
        ((VisClearFunc) func) ();
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

    if (vis_funcs[AUD_VIS_TYPE_MONO_PCM].len () || vis_funcs[AUD_VIS_TYPE_FREQ].len ())
        pcm_to_mono (data, mono, channels);
    if (vis_funcs[AUD_VIS_TYPE_FREQ].len ())
        calc_freq (mono, freq);

    for (VisFunc func : vis_funcs[AUD_VIS_TYPE_MONO_PCM])
        ((VisMonoPCMFunc) func) (mono);

    for (VisFunc func : vis_funcs[AUD_VIS_TYPE_MULTI_PCM])
        ((VisMultiPCMFunc) func) (data, channels);

    for (VisFunc func : vis_funcs[AUD_VIS_TYPE_FREQ])
        ((VisFreqFunc) func) (freq);
}

static bool vis_load (PluginHandle * plugin)
{
    AUDINFO ("Activating %s.\n", aud_plugin_get_name (plugin));
    VisPlugin * header = (VisPlugin *) aud_plugin_get_header (plugin);
    if (! header)
        return false;

    if (PLUGIN_HAS_FUNC (header, clear))
        aud_vis_func_add (AUD_VIS_TYPE_CLEAR, (VisFunc) header->clear);
    if (PLUGIN_HAS_FUNC (header, render_mono_pcm))
        aud_vis_func_add (AUD_VIS_TYPE_MONO_PCM, (VisFunc) header->render_mono_pcm);
    if (PLUGIN_HAS_FUNC (header, render_multi_pcm))
        aud_vis_func_add (AUD_VIS_TYPE_MULTI_PCM, (VisFunc) header->render_multi_pcm);
    if (PLUGIN_HAS_FUNC (header, render_freq))
        aud_vis_func_add (AUD_VIS_TYPE_FREQ, (VisFunc) header->render_freq);

    return true;
}

static void vis_unload (PluginHandle * plugin)
{
    AUDINFO ("Deactivating %s.\n", aud_plugin_get_name (plugin));
    VisPlugin * header = (VisPlugin *) aud_plugin_get_header (plugin);
    if (! header)
        return;

    if (PLUGIN_HAS_FUNC (header, clear))
        aud_vis_func_remove ((VisFunc) header->clear);
    if (PLUGIN_HAS_FUNC (header, render_mono_pcm))
        aud_vis_func_remove ((VisFunc) header->render_mono_pcm);
    if (PLUGIN_HAS_FUNC (header, render_multi_pcm))
        aud_vis_func_remove ((VisFunc) header->render_multi_pcm);
    if (PLUGIN_HAS_FUNC (header, render_freq))
        aud_vis_func_remove ((VisFunc) header->render_freq);

    if (PLUGIN_HAS_FUNC (header, clear))
        header->clear ();
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
    if (! vp)
        return false;

    if (vp->init && ! vp->init ())
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

    if (vp->cleanup)
        vp->cleanup ();
}
