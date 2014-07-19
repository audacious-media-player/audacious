/*
 * effect.c
 * Copyright 2010-2012 John Lindgren
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

#include "internal.h"

#include <pthread.h>
#include <glib.h>

#include "drct.h"
#include "list.h"
#include "plugin.h"
#include "plugins.h"
#include "runtime.h"

struct Effect : public ListNode
{
    PluginHandle * plugin;
    EffectPlugin * header;
    int channels_returned, rate_returned;
    bool remove_flag;
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static List<Effect> effects;
static int input_channels, input_rate;

struct EffectStartState {
    int * channels, * rate;
};

static bool effect_start_cb (PluginHandle * plugin, EffectStartState * state)
{
    AUDDBG ("Starting %s at %d channels, %d Hz.\n", aud_plugin_get_name (plugin),
     * state->channels, * state->rate);
    EffectPlugin * header = (EffectPlugin *) aud_plugin_get_header (plugin);
    g_return_val_if_fail (header, true);
    header->start (state->channels, state->rate);

    Effect * effect = new Effect ();
    effect->plugin = plugin;
    effect->header = header;
    effect->channels_returned = * state->channels;
    effect->rate_returned = * state->rate;

    effects.append (effect);
    return true;
}

void effect_start (int * channels, int * rate)
{
    pthread_mutex_lock (& mutex);

    AUDDBG ("Starting effects.\n");

    effects.clear ();

    input_channels = * channels;
    input_rate = * rate;

    EffectStartState state = {channels, rate};
    aud_plugin_for_enabled (PLUGIN_TYPE_EFFECT, (PluginForEachFunc) effect_start_cb, & state);

    pthread_mutex_unlock (& mutex);
}

void effect_process (float * * data, int * samples)
{
    pthread_mutex_lock (& mutex);

    Effect * e = effects.head ();
    while (e)
    {
        Effect * next = effects.next (e);

        if (e->remove_flag)
        {
            /* call finish twice to completely drain buffers */
            e->header->finish (data, samples);
            e->header->finish (data, samples);

            effects.remove (e);
            delete e;
        }
        else
            e->header->process (data, samples);

        e = next;
    }

    pthread_mutex_unlock (& mutex);
}

void effect_flush (void)
{
    pthread_mutex_lock (& mutex);

    for (Effect * e = effects.head (); e; e = effects.next (e))
    {
        if (PLUGIN_HAS_FUNC (e->header, flush))
            e->header->flush ();
    }

    pthread_mutex_unlock (& mutex);
}

void effect_finish (float * * data, int * samples)
{
    pthread_mutex_lock (& mutex);

    for (Effect * e = effects.head (); e; e = effects.next (e))
        e->header->finish (data, samples);

    pthread_mutex_unlock (& mutex);
}

int effect_adjust_delay (int delay)
{
    pthread_mutex_lock (& mutex);

    for (Effect * e = effects.tail (); e; e = effects.prev (e))
    {
        if (PLUGIN_HAS_FUNC (e->header, adjust_delay))
            delay = e->header->adjust_delay (delay);
    }

    pthread_mutex_unlock (& mutex);
    return delay;
}

static void effect_insert (PluginHandle * plugin, EffectPlugin * header)
{
    Effect * prev = nullptr;

    for (Effect * e = effects.head (); e; e = effects.next (e))
    {
        if (e->plugin == plugin)
        {
            e->remove_flag = false;
            return;
        }

        if (aud_plugin_compare (e->plugin, plugin) > 0)
            break;

        prev = e;
    }

    AUDDBG ("Adding %s without reset.\n", aud_plugin_get_name (plugin));
    Effect * effect = new Effect ();
    effect->plugin = plugin;
    effect->header = header;

    effects.insert_after (prev, effect);

    int channels, rate;
    if (prev)
    {
        AUDDBG ("Added %s after %s.\n", aud_plugin_get_name (plugin),
         aud_plugin_get_name (prev->plugin));
        channels = prev->channels_returned;
        rate = prev->rate_returned;
    }
    else
    {
        AUDDBG ("Added %s as first effect.\n", aud_plugin_get_name (plugin));
        channels = input_channels;
        rate = input_rate;
    }

    AUDDBG ("Starting %s at %d channels, %d Hz.\n", aud_plugin_get_name (plugin), channels, rate);
    header->start (& channels, & rate);
    effect->channels_returned = channels;
    effect->rate_returned = rate;
}

static void effect_remove (PluginHandle * plugin)
{
    for (Effect * e = effects.head (); e; e = effects.next (e))
    {
        if (e->plugin == plugin)
        {
            AUDDBG ("Removing %s without reset.\n", aud_plugin_get_name (plugin));
            e->remove_flag = true;
            return;
        }
    }
}

static void effect_enable (PluginHandle * plugin, EffectPlugin * ep, bool enable)
{
    if (ep->preserves_format)
    {
        pthread_mutex_lock (& mutex);

        if (enable)
            effect_insert (plugin, ep);
        else
            effect_remove (plugin);

        pthread_mutex_unlock (& mutex);
    }
    else
    {
        AUDDBG ("Reset to add/remove %s.\n", aud_plugin_get_name (plugin));
        aud_output_reset (OutputReset::EffectsOnly);
    }
}

bool effect_plugin_start (PluginHandle * plugin)
{
    if (aud_drct_get_playing ())
    {
        EffectPlugin * ep = (EffectPlugin *) aud_plugin_get_header (plugin);
        g_return_val_if_fail (ep, false);
        effect_enable (plugin, ep, true);
    }

    return true;
}

void effect_plugin_stop (PluginHandle * plugin)
{
    if (aud_drct_get_playing ())
    {
        EffectPlugin * ep = (EffectPlugin *) aud_plugin_get_header (plugin);
        g_return_if_fail (ep);
        effect_enable (plugin, ep, false);
    }
}
