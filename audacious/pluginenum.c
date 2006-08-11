/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "pluginenum.h"

#include <glib.h>
#include <gmodule.h>
#include <glib/gprintf.h>
#include <string.h>

#include "controlsocket.h"
#include "main.h"
#include "mainwin.h"
#include "playback.h"
#include "playlist.h"
#include "util.h"

#include "effect.h"
#include "general.h"
#include "input.h"
#include "output.h"
#include "visualization.h"

const gchar *plugin_dir_list[] = {
    PLUGINSUBS,
    NULL
};

GHashTable *plugin_matrix = NULL;
GList *lowlevel_list = NULL;

static gint
inputlist_compare_func(gconstpointer a, gconstpointer b)
{
    const InputPlugin *ap = a, *bp = b;
    return strcasecmp(ap->description, bp->description);
}

static gint
outputlist_compare_func(gconstpointer a, gconstpointer b)
{
    const OutputPlugin *ap = a, *bp = b;
    return strcasecmp(ap->description, bp->description);
}

static gint
effectlist_compare_func(gconstpointer a, gconstpointer b)
{
    const EffectPlugin *ap = a, *bp = b;
    return strcasecmp(ap->description, bp->description);
}

static gint
generallist_compare_func(gconstpointer a, gconstpointer b)
{
    const GeneralPlugin *ap = a, *bp = b;
    return strcasecmp(ap->description, bp->description);
}

static gint
vislist_compare_func(gconstpointer a, gconstpointer b)
{
    const VisPlugin *ap = a, *bp = b;
    return strcasecmp(ap->description, bp->description);
}

static gboolean
plugin_is_duplicate(const gchar * filename)
{
    GList *l;
    const gchar *basename = g_basename(filename);

    /* FIXME: messy stuff */

    for (l = ip_data.input_list; l; l = g_list_next(l))
        if (!strcmp(basename, g_basename(INPUT_PLUGIN(l->data)->filename)))
            return TRUE;

    for (l = op_data.output_list; l; l = g_list_next(l))
        if (!strcmp(basename, g_basename(OUTPUT_PLUGIN(l->data)->filename)))
            return TRUE;

    for (l = ep_data.effect_list; l; l = g_list_next(l))
        if (!strcmp(basename, g_basename(EFFECT_PLUGIN(l->data)->filename)))
            return TRUE;

    for (l = gp_data.general_list; l; l = g_list_next(l))
        if (!strcmp(basename, g_basename(GENERAL_PLUGIN(l->data)->filename)))
            return TRUE;

    for (l = vp_data.vis_list; l; l = g_list_next(l))
        if (!strcmp(basename, g_basename(VIS_PLUGIN(l->data)->filename)))
            return TRUE;

    for (l = lowlevel_list; l; l = g_list_next(l))
        if (!strcmp(basename, g_basename(VIS_PLUGIN(l->data)->filename)))
            return TRUE;

    return FALSE;
}


#define PLUGIN_GET_INFO(x) ((PluginGetInfoFunc)(x))()
typedef Plugin * (*PluginGetInfoFunc) (void);

static void
input_plugin_init(Plugin * plugin)
{
    InputPlugin *p = INPUT_PLUGIN(plugin);

    p->get_vis_type = input_get_vis_type;
    p->add_vis_pcm = input_add_vis_pcm;
    
    /* Pretty const casts courtesy of XMMS's plugin.h legacy. Anyone
       else thinks we could use a CONST macro to solve the warnings?
       - descender */
    p->set_info = (void (*)(gchar *, gint, gint, gint, gint)) playlist_set_info;
    p->set_info_text = (void (*)(gchar *)) input_set_info_text;
    p->set_status_buffering = (void (*)(gboolean)) input_set_status_buffering;     

    ip_data.input_list = g_list_append(ip_data.input_list, p);
    
    g_hash_table_replace(plugin_matrix, g_path_get_basename(p->filename),
                         GINT_TO_POINTER(1));
}

static void
output_plugin_init(Plugin * plugin)
{
    OutputPlugin *p = OUTPUT_PLUGIN(plugin);
    op_data.output_list = g_list_append(op_data.output_list, p);    
}

static void
effect_plugin_init(Plugin * plugin)
{
    EffectPlugin *p = EFFECT_PLUGIN(plugin);
    ep_data.effect_list = g_list_append(ep_data.effect_list, p);
}

static void
general_plugin_init(Plugin * plugin)
{
    GeneralPlugin *p = GENERAL_PLUGIN(plugin);
    p->xmms_session = ctrlsocket_get_session_id();
    gp_data.general_list = g_list_append(gp_data.general_list, p);
}

static void
vis_plugin_init(Plugin * plugin)
{
    VisPlugin *p = VIS_PLUGIN(plugin);
    p->xmms_session = ctrlsocket_get_session_id();
    p->disable_plugin = vis_disable_plugin;
    vp_data.vis_list = g_list_append(vp_data.vis_list, p);
}

static void
lowlevel_plugin_init(Plugin * plugin)
{
    LowlevelPlugin *p = LOWLEVEL_PLUGIN(plugin);
    lowlevel_list = g_list_append(lowlevel_list, p);    
}

/* FIXME: Placed here (hopefully) temporarily - descender */

typedef struct {
    const gchar *name;
    const gchar *id;
    void (*init)(Plugin *);
} PluginType;

static PluginType plugin_types[] = {
    { "input"        , "get_iplugin_info", input_plugin_init },
    { "output"       , "get_oplugin_info", output_plugin_init },
    { "effect"       , "get_eplugin_info", effect_plugin_init },
    { "general"      , "get_gplugin_info", general_plugin_init },
    { "visualization", "get_vplugin_info", vis_plugin_init },
    { "lowlevel"     , "get_lplugin_info", lowlevel_plugin_init },
    { NULL, NULL, NULL }
};

static void
add_plugin(const gchar * filename)
{
    PluginType *type;
    GModule *module;
    gpointer func;

    if (plugin_is_duplicate(filename))
        return;

    if (!(module = g_module_open(filename, 0))) {
        printf("Failed to load plugin (%s): %s\n", 
                  filename, g_module_error());
        return;
    }

    for (type = plugin_types; type->name; type++)
    {
        if (g_module_symbol(module, type->id, &func)) {
            Plugin *plugin = PLUGIN_GET_INFO(func);

            plugin->handle = module;
            plugin->filename = g_strdup(filename);
            type->init(PLUGIN_GET_INFO(func));

            return;
        }
    }

    printf("Invalid plugin (%s)\n", filename);
    g_module_close(module);
}

static gboolean
scan_plugin_func(const gchar * path, const gchar * basename, gpointer data)
{
    if (!str_has_suffix_nocase(basename, G_MODULE_SUFFIX))
        return FALSE;

    if (!g_file_test(path, G_FILE_TEST_IS_REGULAR))
        return FALSE;

    add_plugin(path);

    return FALSE;
}

static void
scan_plugins(const gchar * path)
{
    dir_foreach(path, scan_plugin_func, NULL, NULL);
}

void
plugin_system_init(void)
{
    gchar *dir, **disabled;
    GList *node;
    OutputPlugin *op;
    InputPlugin *ip;
    LowlevelPlugin *lp;
    gint dirsel = 0, i = 0;

    if (!g_module_supported()) {
        report_error("Module loading not supported! Plugins will not be loaded.\n");
        return;
    }

    plugin_matrix = g_hash_table_new_full(g_str_hash, g_int_equal, g_free,
                                          NULL);

#ifndef DISABLE_USER_PLUGIN_DIR
    scan_plugins(bmp_paths[BMP_PATH_USER_PLUGIN_DIR]);
    /*
     * This is in a separate loop so if the user puts them in the
     * wrong dir we'll still get them in the right order (home dir
     * first)                                                - Zinx
     */
    while (plugin_dir_list[dirsel]) {
        dir = g_build_filename(bmp_paths[BMP_PATH_USER_PLUGIN_DIR],
                               plugin_dir_list[dirsel++], NULL);
        scan_plugins(dir);
        g_free(dir);
    }
    dirsel = 0;
#endif

    while (plugin_dir_list[dirsel]) {
        dir = g_build_filename(PLUGIN_DIR, plugin_dir_list[dirsel++], NULL);
        scan_plugins(dir);
        g_free(dir);
    }

    op_data.output_list = g_list_sort(op_data.output_list, outputlist_compare_func);
    if (!op_data.current_output_plugin
        && g_list_length(op_data.output_list)) {
        op_data.current_output_plugin = op_data.output_list->data;
    }

    ip_data.input_list = g_list_sort(ip_data.input_list, inputlist_compare_func);

    ep_data.effect_list = g_list_sort(ep_data.effect_list, effectlist_compare_func);
    ep_data.enabled_list = NULL;

    gp_data.general_list = g_list_sort(gp_data.general_list, generallist_compare_func);
    gp_data.enabled_list = NULL;

    vp_data.vis_list = g_list_sort(vp_data.vis_list, vislist_compare_func);
    vp_data.enabled_list = NULL;

    general_enable_from_stringified_list(cfg.enabled_gplugins);
    vis_enable_from_stringified_list(cfg.enabled_vplugins);
    effect_enable_from_stringified_list(cfg.enabled_eplugins);

    g_free(cfg.enabled_gplugins);
    cfg.enabled_gplugins = NULL;

    g_free(cfg.enabled_vplugins);
    cfg.enabled_vplugins = NULL;

    g_free(cfg.enabled_eplugins);
    cfg.enabled_eplugins = NULL;

    for (node = op_data.output_list; node; node = g_list_next(node)) {
        op = OUTPUT_PLUGIN(node->data);
        /*
         * Only test basename to avoid problems when changing
         * prefix.  We will only see one plugin with the same
         * basename, so this is usually what the user want.
         */
        if (!strcmp(g_basename(cfg.outputplugin), g_basename(op->filename)))
            op_data.current_output_plugin = op;
        if (op->init)
            op->init();
    }

    for (node = ip_data.input_list; node; node = g_list_next(node)) {
        ip = INPUT_PLUGIN(node->data);
        if (ip->init)
            ip->init();
    }

    for (node = lowlevel_list; node; node = g_list_next(node)) {
        lp = LOWLEVEL_PLUGIN(node->data);
        if (lp->init)
            lp->init();
    }

    if (cfg.disabled_iplugins) {
        disabled = g_strsplit(cfg.disabled_iplugins, ":", 0);
        while (disabled[i]) {
            g_hash_table_replace(plugin_matrix, disabled[i],
                                 GINT_TO_POINTER(FALSE));
            i++;
        }

        g_free(disabled);

        g_free(cfg.disabled_iplugins);
        cfg.disabled_iplugins = NULL;
    }
}

void
plugin_system_cleanup(void)
{
    InputPlugin *ip;
    OutputPlugin *op;
    EffectPlugin *ep;
    GeneralPlugin *gp;
    VisPlugin *vp;
    LowlevelPlugin *lp;
    GList *node;

    g_message("Shutting down plugin system");

    if (bmp_playback_get_playing()) {
        ip_data.stop = TRUE;
        bmp_playback_stop();
        ip_data.stop = FALSE;
    }

    for (node = get_input_list(); node; node = g_list_next(node)) {
        ip = INPUT_PLUGIN(node->data);
        if (ip && ip->cleanup) {
            ip->cleanup();
            GDK_THREADS_LEAVE();
            while (g_main_iteration(FALSE));
            GDK_THREADS_ENTER();
        }
        g_module_close(ip->handle);
    }

    if (ip_data.input_list)
        g_list_free(ip_data.input_list);

    for (node = get_output_list(); node; node = g_list_next(node)) {
        op = OUTPUT_PLUGIN(node->data);
        if (op && op->cleanup) {
            op->cleanup();
            GDK_THREADS_LEAVE();
            while (g_main_iteration(FALSE));
            GDK_THREADS_ENTER();
        }
        g_module_close(op->handle);
    }
    
    if (op_data.output_list)
        g_list_free(op_data.output_list);

    for (node = get_effect_list(); node; node = g_list_next(node)) {
        ep = EFFECT_PLUGIN(node->data);
        if (ep && ep->cleanup) {
            ep->cleanup();
            GDK_THREADS_LEAVE();
            while (g_main_iteration(FALSE));
            GDK_THREADS_ENTER();
        }
        g_module_close(ep->handle);
    }

    if (ep_data.effect_list)
        g_list_free(ep_data.effect_list);

    for (node = get_general_enabled_list(); node; node = g_list_next(node)) {
        gp = GENERAL_PLUGIN(node->data);
        enable_general_plugin(g_list_index(gp_data.general_list, gp), FALSE);
    }

    if (gp_data.enabled_list)
        g_list_free(gp_data.enabled_list);

    GDK_THREADS_LEAVE();
    while (g_main_iteration(FALSE));
    GDK_THREADS_ENTER();

    for (node = get_general_list(); node; node = g_list_next(node)) {
        gp = GENERAL_PLUGIN(node->data);
        if (gp && gp->cleanup) {
            gp->cleanup();
            GDK_THREADS_LEAVE();
            while (g_main_iteration(FALSE));
            GDK_THREADS_ENTER();
        }
        g_module_close(gp->handle);
    }

    if (gp_data.general_list)
        g_list_free(gp_data.general_list);

    for (node = get_vis_enabled_list(); node; node = g_list_next(node)) {
        vp = VIS_PLUGIN(node->data);
        enable_vis_plugin(g_list_index(vp_data.vis_list, vp), FALSE);
    }

    if (vp_data.enabled_list)
        g_list_free(vp_data.enabled_list);

    GDK_THREADS_LEAVE();
    while (g_main_iteration(FALSE));
    GDK_THREADS_ENTER();

    for (node = get_vis_list(); node; node = g_list_next(node)) {
        vp = VIS_PLUGIN(node->data);
        if (vp && vp->cleanup) {
            vp->cleanup();
            GDK_THREADS_LEAVE();
            while (g_main_iteration(FALSE));
            GDK_THREADS_ENTER();
        }
        g_module_close(vp->handle);
    }

    if (vp_data.vis_list)
        g_list_free(vp_data.vis_list);

    for (node = lowlevel_list; node; node = g_list_next(node)) {
        lp = LOWLEVEL_PLUGIN(node->data);
        if (lp && lp->cleanup) {
            lp->cleanup();
            GDK_THREADS_LEAVE();
            while (g_main_iteration(FALSE));
            GDK_THREADS_ENTER();
        }
        g_module_close(lp->handle);
    }

    if (lowlevel_list)
        g_list_free(lowlevel_list);
}
