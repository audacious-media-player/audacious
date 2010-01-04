/*
 *  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious dvelopment team.
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
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

#include "effect.h"

#include <glib.h>
#include <string.h>

#include "output.h"
#include "plugin.h"
#include "pluginenum.h"

EffectPluginData ep_data = {
    NULL,
    NULL
};

gint
effect_do_mod_samples(gpointer * data, gint length,
                      AFormat fmt, gint srate, gint nch)
{
    GList *l = ep_data.enabled_list;

    while (l) {
        if (l->data) {
            EffectPlugin *ep = l->data;
            if (ep->mod_samples)
	    {
	        plugin_set_current((Plugin *)ep);
                length = ep->mod_samples(data, length, fmt, srate, nch);
	    }
        }
        l = g_list_next(l);
    }

    return length;
}

void
effect_do_query_format(AFormat * fmt, gint * rate, gint * nch)
{
    GList *l = ep_data.enabled_list;

    while (l) {
        if (l->data) {
            EffectPlugin *ep = l->data;
            if (ep->query_format)
	    {
	        plugin_set_current((Plugin *)ep);
                ep->query_format(fmt, rate, nch);
	    }
        }
        l = g_list_next(l);
    }
}

GList *
get_effect_enabled_list(void)
{
    return ep_data.enabled_list;
}

void
effect_enable_plugin(EffectPlugin *ep, gboolean enable)
{
    if (enable && !ep->enabled)
        ep_data.enabled_list = g_list_append(ep_data.enabled_list, ep);
    else if (!enable && ep->enabled)
        ep_data.enabled_list = g_list_remove(ep_data.enabled_list, ep);

    ep->enabled = enable;

    /* new-API effects require playback to be restarted */
    if (ep->start != NULL)
        set_current_output_plugin (current_output_plugin);
}

GList *
get_effect_list(void)
{
    return ep_data.effect_list;
}

gchar *
effect_stringify_enabled_list(void)
{
    gchar *enalist = NULL, *temp, *temp2;
    GList *node = ep_data.enabled_list;

    if (g_list_length(node)) {
        enalist =
            g_path_get_basename(((EffectPlugin *) node->data)->filename);
        node = node->next;
        while (node) {
            temp = enalist;
            temp2 =
                g_path_get_basename(((EffectPlugin *) node->data)->filename);
            enalist = g_strconcat(temp, ",", temp2, NULL);
            g_free(temp);
            g_free(temp2);
            node = node->next;
        }
    }
    return enalist;
}

void
effect_enable_from_stringified_list(const gchar * list)
{
    gchar **plugins, *base;
    GList *node;
    gint i;
    EffectPlugin *ep;

    if (!list || !strcmp(list, ""))
        return;
    plugins = g_strsplit(list, ",", 0);
    for (i = 0; plugins[i]; i++) {
        node = ep_data.effect_list;
        while (node) {
            base =
                g_path_get_basename((char *) ((EffectPlugin *) node->
                                              data)->filename);
            if (!strcmp(plugins[i], base)) {
                ep = node->data;
                ep->enabled = TRUE;
                ep_data.enabled_list =
                    g_list_append(ep_data.enabled_list, ep);
            }
            g_free(base);
            node = node->next;
        }
    }
    g_strfreev(plugins);
}

void
effect_flow(FlowContext *context)
{
    AFormat new_format;
    gint new_rate;
    gint new_nch;

    new_format = context->fmt;
    new_rate = context->srate;
    new_nch = context->channels;

    effect_do_query_format(&new_format, &new_rate, &new_nch);

    if (new_format != context->fmt ||
        new_rate != context->srate ||
        new_nch != context->channels)
#if 0 /* Trying to reopen audio in the middle of a pass_audio call is a
       * horrendous idea. -jlindgren */
    {
        /*
         * The effect plugin changes the stream format. Reopen the
         * audio device.
         */
        if (!output_open_audio(new_format, new_rate, new_nch))
            return;

        context->fmt = new_format;
        context->srate = new_rate;
        context->channels = new_nch;
    }
#else
    return;
#endif

    context->len = effect_do_mod_samples(&context->data, context->len,
        context->fmt, context->srate, context->channels);
}

static GList * new_effect_list = NULL;

void new_effect_start (gint * channels, gint * rate)
{
    GList * node;
    EffectPlugin * effect;

    g_list_free (new_effect_list);
    new_effect_list = NULL;

    /* FIXME: Let the user define in what order the effects are applied. */
    for (node = ep_data.enabled_list; node != NULL; node = node->next)
    {
        effect = node->data;

        if (effect->start != NULL)
            new_effect_list = g_list_prepend (new_effect_list, effect);
        else
            printf ("Please update %s to new API.\n", effect->description);
    }

    new_effect_list = g_list_reverse (new_effect_list);

    for (node = new_effect_list; node != NULL; node = node->next)
    {
        effect = node->data;
        effect->start (channels, rate);
    }
}

void new_effect_process (gfloat * * data, gint * samples)
{
    GList * node;
    EffectPlugin * effect;

    for (node = new_effect_list; node != NULL; node = node->next)
    {
        effect = node->data;
        effect->process (data, samples);
    }
}

void new_effect_flush (void)
{
    GList * node;
    EffectPlugin * effect;

    for (node = new_effect_list; node != NULL; node = node->next)
    {
        effect = node->data;
        effect->flush ();
    }
}

void new_effect_finish (gfloat * * data, gint * samples)
{
    GList * node;
    EffectPlugin * effect;

    for (node = new_effect_list; node != NULL; node = node->next)
    {
        effect = node->data;
        effect->finish (data, samples);
    }
}

gint new_effect_decoder_to_output_time (gint time)
{
    GList * node;
    EffectPlugin * effect;

    for (node = new_effect_list; node != NULL; node = node->next)
    {
        effect = node->data;
        time = effect->decoder_to_output_time (time);
    }

    return time;
}

gint new_effect_output_to_decoder_time (gint time)
{
    GList * node;
    EffectPlugin * effect;

    for (node = g_list_last (new_effect_list); node != NULL; node = node->prev)
    {
        effect = node->data;
        time = effect->output_to_decoder_time (time);
    }

    return time;
}
