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
#include "plugin.h"

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
                length = ep->mod_samples(data, length, fmt, srate, nch);
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
                ep->query_format(fmt, rate, nch);
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
enable_effect_plugin(int i, gboolean enable)
{
    GList *node = g_list_nth(ep_data.effect_list, i);
    EffectPlugin *ep;

    if (!node || !(node->data))
        return;
    ep = node->data;
    ep->enabled = enable;

    if (enable && !ep->enabled) {
        ep_data.enabled_list = g_list_append(ep_data.enabled_list, ep);
        if (ep->init)
            ep->init();
    }
    else if (!enable && ep->enabled) {
        ep_data.enabled_list = g_list_remove(ep_data.enabled_list, ep);
        if (ep->cleanup)
            ep->cleanup();
    }
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
            g_strdup(g_basename(((EffectPlugin *) node->data)->filename));
        node = node->next;
        while (node) {
            temp = enalist;
            temp2 =
                g_strdup(g_basename(((EffectPlugin *) node->data)->filename));
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
                if (ep->init)
                    ep->init();
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

    context->len = effect_do_mod_samples(&context->data, context->len,
        context->fmt, context->srate, context->channels);
}

