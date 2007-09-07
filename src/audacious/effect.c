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

static gint
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

static void
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

static EffectPlugin pseudo_effect_plugin = {
    NULL,
    NULL,
    "XMMS Multiple Effects Support",
    NULL,
    NULL,
    NULL,
    NULL,
    TRUE,
    effect_do_mod_samples,
    effect_do_query_format
};

/* get_current_effect_plugin() and effects_enabled() are still to be used by 
 * output plugins as they were when we only supported one effects plugin at
 * a time. We now had a pseudo-effects-plugin that chains all the enabled
 * plugins. -- Jakdaw */

EffectPlugin *
get_current_effect_plugin(void)
{
    return &pseudo_effect_plugin;
}

gboolean
effects_enabled(void)
{
    return TRUE;
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

gboolean
effect_enabled(int i)
{
    return (g_list_find
            (ep_data.enabled_list,
             (EffectPlugin *) g_list_nth(ep_data.effect_list,
                                         i)->data) ? TRUE : FALSE);
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
