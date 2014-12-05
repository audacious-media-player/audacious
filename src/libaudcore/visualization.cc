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

#include <string.h>

#include "plugin.h"
#include "plugins.h"
#include "runtime.h"

static Index<Visualizer *> visualizers;

static int running = false;
static int num_enabled = 0;

EXPORT void aud_visualizer_add (Visualizer * vis)
{
    visualizers.append (vis);

    num_enabled ++;
    if (num_enabled == 1)
        vis_runner_enable (true);
}

EXPORT void aud_visualizer_remove (Visualizer * vis)
{
    int num_disabled = 0;

    auto is_match = [&] (Visualizer * vis2)
    {
        if (vis2 != vis)
            return false;

        num_disabled ++;
        return true;
    };

    visualizers.remove_if (is_match, true);

    num_enabled -= num_disabled;
    if (! num_enabled)
        vis_runner_enable (false);
}

void vis_send_clear ()
{
    for (Visualizer * vis : visualizers)
        vis->clear ();
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
    auto is_active = [] (int type_mask)
    {
        for (Visualizer * vis : visualizers)
        {
            if ((vis->type_mask & type_mask))
                return true;
        }

        return false;
    };

    float mono[512];
    float freq[256];

    if (is_active (Visualizer::MonoPCM | Visualizer::Freq))
        pcm_to_mono (data, mono, channels);
    if (is_active (Visualizer::Freq))
        calc_freq (mono, freq);

    for (Visualizer * vis : visualizers)
    {
        if ((vis->type_mask & Visualizer::MonoPCM))
            vis->render_mono_pcm (mono);
        if ((vis->type_mask & Visualizer::MultiPCM))
            vis->render_multi_pcm (data, channels);
        if ((vis->type_mask & Visualizer::Freq))
            vis->render_freq (freq);
    }
}

static bool vis_load (PluginHandle * plugin)
{
    AUDINFO ("Activating %s.\n", aud_plugin_get_name (plugin));
    VisPlugin * header = (VisPlugin *) aud_plugin_get_header (plugin);
    if (! header)
        return false;

    aud_visualizer_add (header);
    return true;
}

static void vis_unload (PluginHandle * plugin)
{
    AUDINFO ("Deactivating %s.\n", aud_plugin_get_name (plugin));
    VisPlugin * header = (VisPlugin *) aud_plugin_get_header (plugin);
    if (! header)
        return;

    header->clear ();
    aud_visualizer_remove (header);
}

void vis_activate (bool activate)
{
    if (! activate == ! running)
        return;

    for (PluginHandle * plugin : aud_plugin_list (PluginType::Vis))
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
