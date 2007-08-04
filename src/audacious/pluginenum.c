/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifndef SHARED_SUFFIX
# define SHARED_SUFFIX G_MODULE_SUFFIX
#endif

#include "pluginenum.h"

#include <glib.h>
#include <gmodule.h>
#include <glib/gprintf.h>
#include <string.h>

#include "main.h"
#include "ui_main.h"
#include "playback.h"
#include "playlist.h"
#include "strings.h"
#include "util.h"

#include "effect.h"
#include "general.h"
#include "input.h"
#include "output.h"
#include "visualization.h"
#include "discovery.h"
const gchar *plugin_dir_list[] = {
    PLUGINSUBS,
    NULL
};

GHashTable *plugin_matrix = NULL;
GList *lowlevel_list = NULL;
extern GList *vfs_transports;

static gint
inputlist_compare_func(gconstpointer a, gconstpointer b)
{
    const InputPlugin *ap = a, *bp = b;
    if(ap->description && bp->description)
        return strcasecmp(ap->description, bp->description);
    else
        return 0;
}

static gint
outputlist_compare_func(gconstpointer a, gconstpointer b)
{
    const OutputPlugin *ap = a, *bp = b;
    if(ap->description && bp->description)
        return strcasecmp(ap->description, bp->description);
    else
        return 0;
}

static gint
effectlist_compare_func(gconstpointer a, gconstpointer b)
{
    const EffectPlugin *ap = a, *bp = b;
    if(ap->description && bp->description)
        return strcasecmp(ap->description, bp->description);
    else
        return 0;
}

static gint
generallist_compare_func(gconstpointer a, gconstpointer b)
{
    const GeneralPlugin *ap = a, *bp = b;
    if(ap->description && bp->description)
        return strcasecmp(ap->description, bp->description);
    else
        return 0;
}

static gint
vislist_compare_func(gconstpointer a, gconstpointer b)
{
    const VisPlugin *ap = a, *bp = b;
    if(ap->description && bp->description)
        return strcasecmp(ap->description, bp->description);
    else
        return 0;
}

static gint
discoverylist_compare_func(gconstpointer a, gconstpointer b)
{
    const DiscoveryPlugin *ap = a, *bp = b;
    if(ap->description && bp->description)
        return strcasecmp(ap->description, bp->description);
    else
        return 0;
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
        if (!strcmp(basename, g_basename(LOWLEVEL_PLUGIN(l->data)->filename)))
            return TRUE;

    for (l = dp_data.discovery_list; l; l = g_list_next(l))
        if (!strcmp(basename, g_basename(DISCOVERY_PLUGIN(l->data)->filename)))
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
    p->set_info = (void (*)(gchar *, gint, gint, gint, gint)) playlist_set_info_old_abi;
    p->set_info_text = input_set_info_text;
    p->set_status_buffering = input_set_status_buffering;

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
    gp_data.general_list = g_list_append(gp_data.general_list, p);
}

static void
vis_plugin_init(Plugin * plugin)
{
    VisPlugin *p = VIS_PLUGIN(plugin);
    p->disable_plugin = vis_disable_plugin;
    vp_data.vis_list = g_list_append(vp_data.vis_list, p);
}

static void
discovery_plugin_init(Plugin * plugin)
{
    DiscoveryPlugin *p = DISCOVERY_PLUGIN(plugin);
    dp_data.discovery_list = g_list_append(dp_data.discovery_list, p);
}

/*******************************************************************/

static void
plugin2_dispose(GModule *module, const gchar *str, ...)
{
    gchar buf[4096];
    va_list va;

    va_start(va, str);
    vsnprintf(buf, 4096, str, va);
    va_end(va);

    g_print("*** %s\n", buf);
    g_module_close(module);
}

void
plugin2_process(PluginHeader *header, GModule *module, const gchar *filename)
{
    InputPlugin **ip_iter;
    OutputPlugin **op_iter;
    EffectPlugin **ep_iter;
    GeneralPlugin **gp_iter;
    VisPlugin **vp_iter;
    DiscoveryPlugin **dp_iter;


    if (header->magic != PLUGIN_MAGIC)
        return plugin2_dispose(module, "plugin <%s> discarded, invalid module magic", filename);

    if (header->api_version != __AUDACIOUS_PLUGIN_API__)
        return plugin2_dispose(module, "plugin <%s> discarded, wanting API version %d, we implement API version %d",
                               filename, header->api_version, __AUDACIOUS_PLUGIN_API__);

    if (header->init)
        header->init();

    header->priv_assoc = g_new0(Plugin, 1);
    header->priv_assoc->handle = module;
    header->priv_assoc->filename = g_strdup(filename);

    if (header->ip_list)
    {
        for (ip_iter = header->ip_list; *ip_iter != NULL; ip_iter++)
        {
            PLUGIN(*ip_iter)->filename = g_strdup(filename);
            g_print("plugin2 '%s' provides InputPlugin <%p>\n", filename, *ip_iter);
            input_plugin_init(PLUGIN(*ip_iter));
        }
    }

    if (header->op_list)
    {
        for (op_iter = header->op_list; *op_iter != NULL; op_iter++)
        {
            PLUGIN(*op_iter)->filename = g_strdup(filename);
            g_print("plugin2 '%s' provides OutputPlugin <%p>\n", filename, *op_iter);
            output_plugin_init(PLUGIN(*op_iter));
        }
    }

    if (header->ep_list)
    {
        for (ep_iter = header->ep_list; *ep_iter != NULL; ep_iter++)
        {
            PLUGIN(*ep_iter)->filename = g_strdup(filename);
            g_print("plugin2 '%s' provides EffectPlugin <%p>\n", filename, *ep_iter);
            effect_plugin_init(PLUGIN(*ep_iter));
        }
    }

    if (header->gp_list)
    {
        for (gp_iter = header->gp_list; *gp_iter != NULL; gp_iter++)
        {
            PLUGIN(*gp_iter)->filename = g_strdup(filename);
            g_print("plugin2 '%s' provides GeneralPlugin <%p>\n", filename, *gp_iter);
            general_plugin_init(PLUGIN(*gp_iter));
        }
    }

    if (header->vp_list)
    {
        for (vp_iter = header->vp_list; *vp_iter != NULL; vp_iter++)
        {
            PLUGIN(*vp_iter)->filename = g_strdup(filename);
            g_print("plugin2 '%s' provides VisPlugin <%p>\n", filename, *vp_iter);
            vis_plugin_init(PLUGIN(*vp_iter));
        }
    }

    if (header->dp_list)
    {
        for (dp_iter = header->dp_list; *dp_iter != NULL; dp_iter++)
        {
            PLUGIN(*dp_iter)->filename = g_strdup(filename);
            g_print("plugin2 '%s' provides DiscoveryPlugin <%p>\n", filename, *dp_iter);
            discovery_plugin_init(PLUGIN(*dp_iter));
        }
    }

}

void
plugin2_unload(PluginHeader *header)
{
    GModule *module;

    g_return_if_fail(header->priv_assoc != NULL);

    module = header->priv_assoc->handle;

    g_free(header->priv_assoc->filename);
    g_free(header->priv_assoc);

    if (header->fini)
        header->fini();

    g_module_close(module);
}

/******************************************************************/

static void
add_plugin(const gchar * filename)
{
    GModule *module;
    gpointer func;

    if (plugin_is_duplicate(filename))
        return;

    g_message("Loaded plugin (%s)", filename);

    if (!(module = g_module_open(filename, G_MODULE_BIND_LOCAL))) {
        printf("Failed to load plugin (%s): %s\n", 
                  filename, g_module_error());
        return;
    }

    /* v2 plugin loading */
    if (g_module_symbol(module, "get_plugin_info", &func))
    {
        PluginHeader *(*header_func_p)() = func;
        PluginHeader *header;

        /* this should never happen. */
        g_return_if_fail((header = header_func_p()) != NULL);

        plugin2_process(header, module, filename);
        return;
    }

    printf("Invalid plugin (%s)\n", filename);
    g_module_close(module);
}

static gboolean
scan_plugin_func(const gchar * path, const gchar * basename, gpointer data)
{
    if (!str_has_suffix_nocase(basename, SHARED_SUFFIX))
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
    DiscoveryPlugin *dp;
    gint dirsel = 0, i = 0;

    if (!g_module_supported()) {
        report_error("Module loading not supported! Plugins will not be loaded.\n");
        return;
    }

    plugin_matrix = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                                          NULL);

#ifndef DISABLE_USER_PLUGIN_DIR
    scan_plugins(bmp_paths[BMP_PATH_USER_PLUGIN_DIR]);
    /*
     * This is in a separate lo
     * DiscoveryPlugin *dpop so if the user puts them in the
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

    dp_data.discovery_list = g_list_sort(dp_data.discovery_list, discoverylist_compare_func);
    dp_data.enabled_list = NULL;


    general_enable_from_stringified_list(cfg.enabled_gplugins);
    vis_enable_from_stringified_list(cfg.enabled_vplugins);
    effect_enable_from_stringified_list(cfg.enabled_eplugins);
    discovery_enable_from_stringified_list(cfg.enabled_dplugins);

    g_free(cfg.enabled_gplugins);
    cfg.enabled_gplugins = NULL;

    g_free(cfg.enabled_vplugins);
    cfg.enabled_vplugins = NULL;

    g_free(cfg.enabled_eplugins);
    cfg.enabled_eplugins = NULL;

    g_free(cfg.enabled_dplugins);
    cfg.enabled_dplugins = NULL;


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

    for (node = dp_data.discovery_list; node; node = g_list_next(node)) {
        dp = DISCOVERY_PLUGIN(node->data);
        if (dp->init)
            dp->init();
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
    DiscoveryPlugin *dp;
    GList *node;

    g_message("Shutting down plugin system");

    if (playback_get_playing()) {
        ip_data.stop = TRUE;
        playback_stop();
        ip_data.stop = FALSE;
    }

    /* FIXME: race condition -nenolod */
    op_data.current_output_plugin = NULL;

    for (node = get_input_list(); node; node = g_list_next(node)) {
        ip = INPUT_PLUGIN(node->data);
        if (ip && ip->cleanup) {
            ip->cleanup();
            GDK_THREADS_LEAVE();
            while (g_main_context_iteration(NULL, FALSE));
            GDK_THREADS_ENTER();
        }

        if (ip->handle)
            g_module_close(ip->handle);
    }

    if (ip_data.input_list != NULL)
    {
        g_list_free(ip_data.input_list);
        ip_data.input_list = NULL;
    }

    for (node = get_output_list(); node; node = g_list_next(node)) {
        op = OUTPUT_PLUGIN(node->data);
        if (op && op->cleanup) {
            op->cleanup();
            GDK_THREADS_LEAVE();
            while (g_main_context_iteration(NULL, FALSE));
            GDK_THREADS_ENTER();
        }

        if (op->handle)
            g_module_close(op->handle);
    }
    
    if (op_data.output_list != NULL)
    {
        g_list_free(op_data.output_list);
        op_data.output_list = NULL;
    }

    for (node = get_effect_list(); node; node = g_list_next(node)) {
        ep = EFFECT_PLUGIN(node->data);
        if (ep && ep->cleanup) {
            ep->cleanup();
            GDK_THREADS_LEAVE();
            while (g_main_context_iteration(NULL, FALSE));
            GDK_THREADS_ENTER();
        }

        if (ep->handle)
            g_module_close(ep->handle);
    }

    if (ep_data.effect_list != NULL)
    {
        g_list_free(ep_data.effect_list);
        ep_data.effect_list = NULL;
    }

    for (node = get_general_list(); node; node = g_list_next(node)) {
        gp = GENERAL_PLUGIN(node->data);
        if (gp && gp->cleanup) {
            gp->cleanup();
            GDK_THREADS_LEAVE();
            while (g_main_context_iteration(NULL, FALSE));
            GDK_THREADS_ENTER();
        }

        if (gp->handle)
            g_module_close(gp->handle);
    }

    if (gp_data.general_list != NULL)
    {
        g_list_free(gp_data.general_list);
        gp_data.general_list = NULL;
    }

    for (node = get_vis_list(); node; node = g_list_next(node)) {
        vp = VIS_PLUGIN(node->data);
        if (vp && vp->cleanup) {
            vp->cleanup();
            GDK_THREADS_LEAVE();
            while (g_main_context_iteration(NULL, FALSE));
            GDK_THREADS_ENTER();
        }

        if (vp->handle)
            g_module_close(vp->handle);
    }

    if (vp_data.vis_list != NULL)
    {
        g_list_free(vp_data.vis_list);
        vp_data.vis_list = NULL;
    }


    for (node = get_discovery_list(); node; node = g_list_next(node)) {
        dp = DISCOVERY_PLUGIN(node->data);
        if (dp && dp->cleanup) {
            dp->cleanup();
            GDK_THREADS_LEAVE();
            while (g_main_context_iteration(NULL, FALSE));
            GDK_THREADS_ENTER();
        }

        if (dp->handle)
            g_module_close(dp->handle);
    }

    if (dp_data.discovery_list != NULL)
    {
        g_list_free(dp_data.discovery_list);
        dp_data.discovery_list = NULL;
    }



    for (node = lowlevel_list; node; node = g_list_next(node)) {
        lp = LOWLEVEL_PLUGIN(node->data);
        if (lp && lp->cleanup) {
            lp->cleanup();
            GDK_THREADS_LEAVE();
            while (g_main_context_iteration(NULL, FALSE));
            GDK_THREADS_ENTER();
        }

        if (lp->handle)
            g_module_close(lp->handle);
    }

    if (lowlevel_list != NULL)
    {
        g_list_free(lowlevel_list);
        lowlevel_list = NULL;
    }

    /* XXX: vfs will crash otherwise. -nenolod */
    if (vfs_transports != NULL)
    {
        g_list_free(vfs_transports);
        vfs_transports = NULL;
    }

    g_hash_table_destroy( plugin_matrix );
}
