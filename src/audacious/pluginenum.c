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

#include "pluginenum.h"

#include "vfs_buffer.h"
#include "vfs_buffered_file.h"

const gchar *plugin_dir_list[] = {
    PLUGINSUBS,
    NULL
};

/*****************************************************************/

static struct _AudaciousFuncTableV1 _aud_papi_v1 = {
    .vfs_fopen = vfs_fopen,
    .vfs_fclose = vfs_fclose,
    .vfs_dup = vfs_dup,
    .vfs_fread = vfs_fread,
    .vfs_fwrite = vfs_fwrite,
    .vfs_getc = vfs_getc,
    .vfs_ungetc = vfs_ungetc,
    .vfs_fgets = vfs_fgets,
    .vfs_fseek = vfs_fseek,
    .vfs_rewind = vfs_rewind,
    .vfs_ftell = vfs_ftell,
    .vfs_feof = vfs_feof,
    .vfs_file_test = vfs_file_test,
    .vfs_is_writeable = vfs_is_writeable,
    .vfs_truncate = vfs_truncate,
    .vfs_fsize = vfs_fsize,
    .vfs_get_metadata = vfs_get_metadata,
    .vfs_fprintf = vfs_fprintf,
    .vfs_register_transport = vfs_register_transport,
    .vfs_file_get_contents = vfs_file_get_contents,
    .vfs_is_remote = vfs_is_remote,
    .vfs_is_streaming = vfs_is_streaming,

    .vfs_buffer_new = vfs_buffer_new,
    .vfs_buffer_new_from_string = vfs_buffer_new_from_string,

    .vfs_buffered_file_new_from_uri = vfs_buffered_file_new_from_uri,
    .vfs_buffered_file_release_live_fd = vfs_buffered_file_release_live_fd,

    .cfg_db_open = cfg_db_open,
    .cfg_db_close = cfg_db_close,

    .cfg_db_get_string = cfg_db_get_string,
    .cfg_db_get_int = cfg_db_get_int,
    .cfg_db_get_bool = cfg_db_get_bool,
    .cfg_db_get_float = cfg_db_get_float,
    .cfg_db_get_double = cfg_db_get_double,

    .cfg_db_set_string = cfg_db_set_string,
    .cfg_db_set_int = cfg_db_set_int,
    .cfg_db_set_bool = cfg_db_set_bool,
    .cfg_db_set_float = cfg_db_set_float,
    .cfg_db_set_double = cfg_db_set_double,

    .cfg_db_unset_key = cfg_db_unset_key,

    .tuple_new = tuple_new,
    .tuple_new_from_filename = tuple_new_from_filename,
    .tuple_associate_string = tuple_associate_string,
    .tuple_associate_int = tuple_associate_int,
    .tuple_disassociate = tuple_disassociate,
    .tuple_get_value_type = tuple_get_value_type,
    .tuple_get_string = tuple_get_string,
    .tuple_get_int = tuple_get_int,

    .tuple_formatter_process_string = tuple_formatter_process_string,
    .tuple_formatter_process_function = tuple_formatter_process_function,
    .tuple_formatter_process_construct = tuple_formatter_process_construct,
    .tuple_formatter_process_expr = tuple_formatter_process_expr,
    .tuple_formatter_register_function = tuple_formatter_register_function,
    .tuple_formatter_register_expression = tuple_formatter_register_expression,
    .tuple_formatter_make_title_string = tuple_formatter_make_title_string,

    .mime_get_plugin = mime_get_plugin,
    .mime_set_plugin = mime_set_plugin,

    .uri_get_plugin = uri_get_plugin,
    .uri_set_plugin = uri_set_plugin,

    .util_info_dialog = util_info_dialog,
};

/*****************************************************************/

GList *lowlevel_list = NULL;
extern GList *vfs_transports;

mowgli_dictionary_t *plugin_dict = NULL;

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
    gchar *base_filename = g_path_get_basename(filename);

    if (mowgli_dictionary_retrieve(plugin_dict, base_filename) != NULL)
    {
        g_free(base_filename);
        return TRUE;
    }

    g_free(base_filename);

    return FALSE;
}

gboolean
plugin_is_enabled(const gchar *filename)
{
    Plugin *plugin = mowgli_dictionary_retrieve(plugin_dict, filename);

    if (!plugin)
        return FALSE;

    return plugin->enabled;
}

void
plugin_set_enabled(const gchar *filename, gboolean enabled)
{
    Plugin *plugin = mowgli_dictionary_retrieve(plugin_dict, filename);

    if (!plugin)
        return;

    plugin->enabled = enabled;
}

Plugin *
plugin_get_plugin(const gchar *filename)
{
    return mowgli_dictionary_retrieve(plugin_dict, filename);
}

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
    
    p->enabled = TRUE;

    /* XXX: we need something better than p->filename if plugins
       will eventually provide multiple plugins --nenolod */
    mowgli_dictionary_add(plugin_dict, g_basename(p->filename), p);
}

static void
output_plugin_init(Plugin * plugin)
{
    OutputPlugin *p = OUTPUT_PLUGIN(plugin);
    op_data.output_list = g_list_append(op_data.output_list, p);    

    mowgli_dictionary_add(plugin_dict, g_basename(p->filename), p);
}

static void
effect_plugin_init(Plugin * plugin)
{
    EffectPlugin *p = EFFECT_PLUGIN(plugin);
    ep_data.effect_list = g_list_append(ep_data.effect_list, p);

    mowgli_dictionary_add(plugin_dict, g_basename(p->filename), p);
}

static void
general_plugin_init(Plugin * plugin)
{
    GeneralPlugin *p = GENERAL_PLUGIN(plugin);
    gp_data.general_list = g_list_append(gp_data.general_list, p);

    mowgli_dictionary_add(plugin_dict, g_basename(p->filename), p);
}

static void
vis_plugin_init(Plugin * plugin)
{
    VisPlugin *p = VIS_PLUGIN(plugin);
    p->disable_plugin = vis_disable_plugin;
    vp_data.vis_list = g_list_append(vp_data.vis_list, p);

    mowgli_dictionary_add(plugin_dict, g_basename(p->filename), p);
}

static void
discovery_plugin_init(Plugin * plugin)
{
    DiscoveryPlugin *p = DISCOVERY_PLUGIN(plugin);
    dp_data.discovery_list = g_list_append(dp_data.discovery_list, p);

    mowgli_dictionary_add(plugin_dict, g_basename(p->filename), p);
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

    g_message("*** %s\n", buf);
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
            input_plugin_init(PLUGIN(*ip_iter));
        }
    }

    if (header->op_list)
    {
        for (op_iter = header->op_list; *op_iter != NULL; op_iter++)
        {
            PLUGIN(*op_iter)->filename = g_strdup(filename);
            output_plugin_init(PLUGIN(*op_iter));
        }
    }

    if (header->ep_list)
    {
        for (ep_iter = header->ep_list; *ep_iter != NULL; ep_iter++)
        {
            PLUGIN(*ep_iter)->filename = g_strdup(filename);
            effect_plugin_init(PLUGIN(*ep_iter));
        }
    }

    if (header->gp_list)
    {
        for (gp_iter = header->gp_list; *gp_iter != NULL; gp_iter++)
        {
            PLUGIN(*gp_iter)->filename = g_strdup(filename);
            general_plugin_init(PLUGIN(*gp_iter));
        }
    }

    if (header->vp_list)
    {
        for (vp_iter = header->vp_list; *vp_iter != NULL; vp_iter++)
        {
            PLUGIN(*vp_iter)->filename = g_strdup(filename);
            vis_plugin_init(PLUGIN(*vp_iter));
        }
    }

    if (header->dp_list)
    {
        for (dp_iter = header->dp_list; *dp_iter != NULL; dp_iter++)
        {
            PLUGIN(*dp_iter)->filename = g_strdup(filename);
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
        PluginHeader *(*header_func_p)(struct _AudaciousFuncTableV1 *) = func;
        PluginHeader *header;

        /* this should never happen. */
        g_return_if_fail((header = header_func_p(&_aud_papi_v1)) != NULL);

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

    plugin_dict = mowgli_dictionary_create(g_ascii_strcasecmp);

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
        if (cfg.outputplugin && !strcmp(g_basename(cfg.outputplugin), g_basename(op->filename)))
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
            Plugin *plugintmp = plugin_get_plugin(disabled[i]);
            if (plugintmp)
                INPUT_PLUGIN(plugintmp)->enabled = FALSE;
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

    mowgli_dictionary_destroy(plugin_dict, NULL, NULL);
}
