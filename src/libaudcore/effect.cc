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

#include "drct.h"
#include "list.h"
#include "plugin.h"
#include "plugins.h"
#include "runtime.h"

struct Effect : public ListNode
{
    PluginHandle * plugin;
    int position;
    EffectPlugin * header;
    int channels_returned, rate_returned;
    bool remove_flag;
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static List<Effect> effects;
static int input_channels, input_rate;

void effect_start (int & channels, int & rate)
{
    pthread_mutex_lock (& mutex);

    AUDDBG ("Starting effects.\n");

    effects.clear ();

    input_channels = channels;
    input_rate = rate;

    auto & list = aud_plugin_list (PluginType::Effect);

    for (int i = 0; i < list.len (); i ++)
    {
        PluginHandle * plugin = list[i];
        if (! aud_plugin_get_enabled (plugin))
            continue;

        AUDINFO ("Starting %s at %d channels, %d Hz.\n",
         aud_plugin_get_name (plugin), channels, rate);

        EffectPlugin * header = (EffectPlugin *) aud_plugin_get_header (plugin);
        if (! header)
            continue;

        header->start (channels, rate);

        Effect * effect = new Effect ();
        effect->plugin = plugin;
        effect->position = i;
        effect->header = header;
        effect->channels_returned = channels;
        effect->rate_returned = rate;

        effects.append (effect);
    }

    pthread_mutex_unlock (& mutex);
}

Index<float> & effect_process (Index<float> & data)
{
    Index<float> * cur = & data;
    pthread_mutex_lock (& mutex);

    Effect * e = effects.head ();
    while (e)
    {
        Effect * next = effects.next (e);

        if (e->remove_flag)
        {
            cur = & e->header->finish (* cur, false);

            // simulate end-of-playlist call
            // first save the current data
            Index<float> save = std::move (* cur);
            cur = & e->header->finish (* cur, true);

            // combine the saved and new data
            save.move_from (* cur, 0, -1, -1, true, true);
            * cur = std::move (save);

            effects.remove (e);
            delete e;
        }
        else
            cur = & e->header->process (* cur);

        e = next;
    }

    pthread_mutex_unlock (& mutex);
    return * cur;
}

bool effect_flush (bool force)
{
    bool flushed = true;
    pthread_mutex_lock (& mutex);

    for (Effect * e = effects.head (); e; e = effects.next (e))
    {
        if (! e->header->flush (force) && ! force)
        {
            flushed = false;
            break;
        }
    }

    pthread_mutex_unlock (& mutex);
    return flushed;
}

Index<float> & effect_finish (Index<float> & data, bool end_of_playlist)
{
    Index<float> * cur = & data;
    pthread_mutex_lock (& mutex);

    for (Effect * e = effects.head (); e; e = effects.next (e))
        cur = & e->header->finish (* cur, end_of_playlist);

    pthread_mutex_unlock (& mutex);
    return * cur;
}

int effect_adjust_delay (int delay)
{
    pthread_mutex_lock (& mutex);

    for (Effect * e = effects.tail (); e; e = effects.prev (e))
        delay = e->header->adjust_delay (delay);

    pthread_mutex_unlock (& mutex);
    return delay;
}

static void effect_insert (PluginHandle * plugin, EffectPlugin * header)
{
    int position = aud_plugin_list (PluginType::Effect).find (plugin);

    Effect * prev = nullptr;

    for (Effect * e = effects.head (); e; e = effects.next (e))
    {
        if (e->plugin == plugin)
        {
            e->remove_flag = false;
            return;
        }

        if (e->position > position)
            break;

        prev = e;
    }

    AUDDBG ("Adding %s without reset.\n", aud_plugin_get_name (plugin));

    int channels, rate;
    if (prev)
    {
        AUDDBG ("Adding %s after %s.\n", aud_plugin_get_name (plugin),
         aud_plugin_get_name (prev->plugin));
        channels = prev->channels_returned;
        rate = prev->rate_returned;
    }
    else
    {
        AUDDBG ("Adding %s as first effect.\n", aud_plugin_get_name (plugin));
        channels = input_channels;
        rate = input_rate;
    }

    AUDINFO ("Starting %s at %d channels, %d Hz.\n", aud_plugin_get_name (plugin), channels, rate);
    header->start (channels, rate);

    Effect * effect = new Effect ();
    effect->plugin = plugin;
    effect->position = position;
    effect->header = header;
    effect->channels_returned = channels;
    effect->rate_returned = rate;

    effects.insert_after (prev, effect);
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
        if (! ep)
            return false;

        effect_enable (plugin, ep, true);
    }

    return true;
}

void effect_plugin_stop (PluginHandle * plugin)
{
    if (aud_drct_get_playing ())
    {
        EffectPlugin * ep = (EffectPlugin *) aud_plugin_get_header (plugin);
        if (! ep)
            return;

        effect_enable (plugin, ep, false);
    }
}
