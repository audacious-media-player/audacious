/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2008  Audacious development team
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

/* #define AUD_DEBUG */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#include <mowgli.h>

#include "fft.h"
#include "input.h"
#include "main.h"
#include "output.h"
#include "playback.h"
#include "pluginenum.h"
#include "strings.h"
#include "tuple.h"
#include "tuple_formatter.h"
#include "util.h"
#include "visualization.h"
#include "hook.h"

#include "vfs.h"
#include "vfs_buffer.h"
#include "vfs_buffered_file.h"

#include "libSAD.h"

G_LOCK_DEFINE_STATIC(vis_mutex);

InputPluginData ip_data = {
    NULL,
    NULL,
    FALSE,
    FALSE,
    FALSE,
    NULL
};

static GList *vis_list = NULL;
static int volume_l = -1, volume_r = -1;
static SAD_dither_t *sad_state = NULL;
static gint sad_nch = -1;
static AFormat sad_fmt = -1;

InputPlayback *
get_current_input_playback(void)
{
    return ip_data.current_input_playback;
}

void
set_current_input_playback(InputPlayback * ip)
{
    ip_data.current_input_playback = ip;
}

GList *
get_input_list(void)
{
    return ip_data.input_list;
}

/*
 * TODO: move this to utility as something like
 *   plugin_generate_list(GList *plugin_list, gboolean enabled_state)
 *
 *   -nenolod
 */
gchar *
input_stringify_disabled_list(void)
{
    GList *node;
    GString *list = g_string_new("");

    MOWGLI_ITER_FOREACH(node, ip_data.input_list)
    {
        Plugin *plugin = (Plugin *) node->data;
        gchar *filename;

        if (plugin->enabled)
            continue;

        filename = g_path_get_basename(plugin->filename);

        if (list->len > 0)
            g_string_append(list, ":");

        g_string_append(list, filename);
        g_free(filename);
    }

    return g_string_free(list, FALSE);
}

void
free_vis_data(void)
{
    GList *iter;

    G_LOCK(vis_mutex);

    MOWGLI_ITER_FOREACH(iter, vis_list)
        g_slice_free(VisNode, iter->data);

    g_list_free(vis_list);
    vis_list = NULL;

    G_UNLOCK(vis_mutex);
}

InputVisType
input_get_vis_type()
{
    return INPUT_VIS_OFF;
}

static SAD_dither_t*
init_sad(AFormat fmt, gint nch)
{
    gint ret;

    SAD_buffer_format in, out;
    in.sample_format = sadfmt_from_afmt(fmt);
    in.fracbits = FMT_FRACBITS(fmt);
    in.channels = nch;
    in.channels_order = SAD_CHORDER_INTERLEAVED;
    in.samplerate = 0;
    
    out.sample_format = SAD_SAMPLE_S16;
    out.fracbits = 0;
    out.channels = nch;
    out.channels_order = SAD_CHORDER_SEPARATED; /* sic! --asphyx */
    out.samplerate = 0;

    AUDDBG("fmt=%d, nch=%d\n", fmt, nch);
    SAD_dither_t *state = SAD_dither_init(&in, &out, &ret);
    if (state != NULL) SAD_dither_set_dither(state, FALSE);
    return state;
}

void
input_add_vis_pcm(gint time, AFormat fmt, gint nch, gint length, gpointer ptr)
{
#if 0
    VisNode *vis_node;
    gint max;
    
    if (nch > 2) return;

    if (sad_state == NULL || nch != sad_nch || fmt != sad_fmt) {
        if(sad_state != NULL) SAD_dither_free(sad_state);
        sad_state = init_sad(fmt, nch);
        if(sad_state == NULL) return;
        sad_nch = nch;
        sad_fmt = fmt;
    }

    max = length / nch / FMT_SIZEOF(fmt);
    max = CLAMP(max, 0, 512);

    vis_node = g_slice_new0(VisNode);
    vis_node->time = time;
    vis_node->nch = nch;
    vis_node->length = max;

    gint16 *tmp[2];

    tmp[0] = vis_node->data[0];
    tmp[1] = vis_node->data[1];
    SAD_dither_process_buffer(sad_state, ptr, tmp, max);

    G_LOCK(vis_mutex);
    vis_list = g_list_append(vis_list, vis_node);
    G_UNLOCK(vis_mutex);
#endif
}

void
input_dont_show_warning(GtkObject * object, gpointer user_data)
{
    *((gboolean *) user_data) =
        !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(object));
}

static time_t
input_get_mtime(const gchar *filename)
{
    struct stat buf;
    gint rv;
    gchar *realfn = NULL;

    /* stat() does not accept file:// --yaz */
    realfn = g_filename_from_uri(filename, NULL, NULL);
    rv = stat(realfn ? realfn : filename, &buf);
    g_free(realfn); realfn = NULL;

    if (rv == 0) {
        return buf.st_mtime;
    } else {
        return 0; //error
    }
}


/* do actual probing. this function is called from input_check_file() */
static ProbeResult *
input_do_check_file(InputPlugin *ip, VFSFile *fd, gchar *filename_proxy, gboolean loading)
{
    ProbeResult *pr = NULL;
    gint result = 0;

    g_return_val_if_fail(fd != NULL, NULL);

    vfs_rewind(fd);

    /* some input plugins provide probe_for_tuple() only. */
    if ( (ip->probe_for_tuple && !ip->is_our_file_from_vfs && !ip->is_our_file) ||
         (ip->probe_for_tuple && ip->have_subtune == TRUE) ||
         (ip->probe_for_tuple && (cfg.use_pl_metadata && (!loading || (loading && cfg.get_info_on_load)))) ) {

        plugin_set_current((Plugin *)ip);
        Tuple *tuple = ip->probe_for_tuple(filename_proxy, fd);

        if (tuple != NULL) {
            pr = g_new0(ProbeResult, 1);
            pr->ip = ip;
            pr->tuple = tuple;
            tuple_associate_int(pr->tuple, FIELD_MTIME, NULL, input_get_mtime(filename_proxy));

            return pr;
        }
    }

    else if (ip->is_our_file_from_vfs != NULL) {
	plugin_set_current((Plugin *)ip);
        result = ip->is_our_file_from_vfs(filename_proxy, fd);

        if (result > 0) {
            pr = g_new0(ProbeResult, 1);
            pr->ip = ip;

            return pr;
        }
    }

    else if (ip->is_our_file != NULL) {
	plugin_set_current((Plugin *)ip);
        result = ip->is_our_file(filename_proxy);

        if (result > 0) {
            pr = g_new0(ProbeResult, 1);
            pr->ip = ip;

            return pr;
        }
    }

    return NULL;
}


/*
 * input_check_file()
 *
 * Inputs:
 *       filename to check recursively against input plugins
 *       whether or not to show an error
 *
 * Outputs:
 *       pointer to input plugin which can handle this file
 *       otherwise, NULL
 *
 *       (the previous code returned a boolean of whether or not we can
 *        play the file... even WORSE for performance)
 *
 * Side Effects:
 *       various input plugins open the file and probe it
 *       -- this can have very ugly effects performance wise on streams
 *
 * --nenolod, Dec 31 2005
 *
 * Rewritten to use NewVFS probing, semantics are still basically the same.
 *
 * --nenolod, Dec  5 2006
 *
 * Adapted to use the NewVFS extension probing system if enabled.
 *
 * --nenolod, Dec 12 2006
 *
 * Adapted to use the mimetype system.
 *
 * --nenolod, Jul  9 2007
 *
 * Adapted to return ProbeResult structure.
 *
 * --nenolod, Jul 20 2007
 *
 * Make use of ext_hash to avoid full scan in input list.
 *
 * --yaz, Nov 16 2007
 */

/* if loading is TRUE, tuple probing can be skipped as regards configuration. */
ProbeResult *
input_check_file(const gchar *filename, gboolean loading)
{
    VFSFile *fd;
    GList *node;
    InputPlugin *ip;
    gchar *filename_proxy;
    gint ret = 1;
    gchar *ext, *tmp, *tmp_uri;
    gboolean use_ext_filter = FALSE;
    gchar *mimetype;
    ProbeResult *pr = NULL;
    GList **list_hdr = NULL;
    extern GHashTable *ext_hash;

    /* Some URIs will end in ?<subsong> to determine the subsong requested. */
    tmp_uri = g_strdup(filename);
    tmp = strrchr(tmp_uri, '?');

    if (tmp && g_ascii_isdigit(*(tmp + 1)))
        *tmp = '\0';

    filename_proxy = g_strdup(tmp_uri);
    g_free(tmp_uri);

    /* Check for plugins with custom URI:// strings */
    /* cue:// cdda:// tone:// tact:// */
    if ((ip = uri_get_plugin(filename_proxy)) != NULL && ip->enabled) {
        if (ip->is_our_file != NULL)
	{
	    plugin_set_current((Plugin *)ip);
            ret = ip->is_our_file(filename_proxy);
	}
        else
            ret = 0;
        if (ret > 0) {
            g_free(filename_proxy);
            pr = g_new0(ProbeResult, 1);
            pr->ip = ip;
            return pr;
        }
        g_free(filename_proxy);
        return NULL;
    }


    // open the file with vfs sub-system
    fd = vfs_buffered_file_new_from_uri(filename_proxy);

    if (!fd) {
        printf("Unable to read from %s, giving up.\n", filename_proxy);
        g_free(filename_proxy);
        return NULL;
    }


    // apply mimetype check. note that stdio does not support mimetype check.
    mimetype = vfs_get_metadata(fd, "content-type");
    if (mimetype) {
        ip = mime_get_plugin(mimetype);
        g_free(mimetype);
    } else
        ip = NULL;
    
    if (ip && ip->enabled) {
        pr = input_do_check_file(ip, fd, filename_proxy, loading);
        if (pr) {
            g_free(filename_proxy);
            vfs_fclose(fd);
            return pr;
        }
    }


    // apply ext_hash check
    if(cfg.use_extension_probing) {
        use_ext_filter =
            (fd && (!g_strncasecmp(filename_proxy, "/", 1) ||
                    !g_strncasecmp(filename_proxy, "file://", 7))) ? TRUE : FALSE;
    }

    if(use_ext_filter) {
        gchar *base, *lext;
        gchar *tmp2 = g_filename_from_uri(filename_proxy, NULL, NULL);
        gchar *realfn = g_strdup(tmp2 ? tmp2 : filename_proxy);
        g_free(tmp2);

        base = g_path_get_basename(realfn);
        g_free(realfn);
        ext = strrchr(base, '.');
    
        if(ext) {
            lext = g_ascii_strdown(ext+1, -1);
            list_hdr = g_hash_table_lookup(ext_hash, lext);
            g_free(lext);
        }
        g_free(base);

        if(list_hdr) {
            for(node = *list_hdr; node != NULL; node = g_list_next(node)) {
                ip = INPUT_PLUGIN(node->data);

                if (!ip || !ip->enabled)
                    continue;

                pr = input_do_check_file(ip, fd, filename_proxy, loading);

                if(pr) {
                    g_free(filename_proxy);
                    vfs_fclose(fd);
                    return pr;
                }
            }
        }

        g_free(filename_proxy);
        vfs_fclose(fd);
        return NULL; // no plugin found.
    }


    // do full scan when extension match isn't specified.
    for (node = get_input_list(); node != NULL; node = g_list_next(node)) {
        ip = INPUT_PLUGIN(node->data);

        if (!ip || !ip->enabled)
            continue;

        pr = input_do_check_file(ip, fd, filename_proxy, loading);

        if(pr) {
            g_free(filename_proxy);
            vfs_fclose(fd);
            return pr;
        }
    }


    // all probing failed. return NULL
    g_free(filename_proxy);
    vfs_fclose(fd);
    return NULL;
}

Tuple *
input_get_song_tuple(const gchar * filename)
{
    InputPlugin *ip = NULL;
    Tuple *input = NULL;
    gchar *ext = NULL;
    gchar *filename_proxy;
    ProbeResult *pr;

    if (filename == NULL)
        return NULL;

    filename_proxy = g_strdup(filename);

    pr = input_check_file(filename_proxy, FALSE);

    if (!pr) {
        g_free(filename_proxy);
        return NULL;
    }

    ip = pr->ip;

    g_free(pr);

    if (ip && ip->get_song_tuple)
    {
        plugin_set_current((Plugin *)ip);
        input = ip->get_song_tuple(filename_proxy);
    }
    else
    {
        gchar *scratch;

        scratch = uri_to_display_basename(filename);
        tuple_associate_string(input, FIELD_FILE_NAME, NULL, scratch);
        g_free(scratch);

        scratch = uri_to_display_dirname(filename);
        tuple_associate_string(input, FIELD_FILE_PATH, NULL, scratch);
        g_free(scratch);

        ext = strrchr(filename, '.');
        if (ext != NULL) {
            ++ext;
            tuple_associate_string(input, FIELD_FILE_EXT, NULL, ext);
        }

        tuple_associate_int(input, FIELD_LENGTH, NULL, -1);
    }

    g_free(filename_proxy);
    return input;
}

static void
input_general_file_info_box(const gchar * filename, InputPlugin * ip)
{
    GtkWidget *window, *vbox;
    GtkWidget *label, *filename_hbox, *filename_entry;
    GtkWidget *bbox, *cancel;

    gchar *title, *fileinfo, *basename, *iplugin;
    gchar *filename_utf8;
    gchar *realfn = NULL;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    realfn = g_filename_from_uri(filename, NULL, NULL);
    basename = g_path_get_basename(realfn ? realfn : filename);
    fileinfo = filename_to_utf8(basename);
    title = g_strdup_printf(_("audacious: %s"), fileinfo);

    gtk_window_set_title(GTK_WINDOW(window), title);

    g_free(title);
    g_free(fileinfo);
    g_free(basename);

    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    filename_hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), filename_hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Filename:"));
    gtk_box_pack_start(GTK_BOX(filename_hbox), label, FALSE, TRUE, 0);

    filename_entry = gtk_entry_new();
    filename_utf8 = filename_to_utf8(realfn ? realfn : filename);
    g_free(realfn); realfn = NULL;

    gtk_entry_set_text(GTK_ENTRY(filename_entry), filename_utf8);
    gtk_editable_set_editable(GTK_EDITABLE(filename_entry), FALSE);
    gtk_box_pack_start(GTK_BOX(filename_hbox), filename_entry, TRUE, TRUE, 0);

    g_free(filename_utf8);

    if (ip)
        if (ip->description)
            iplugin = ip->description;
        else
            iplugin = ip->filename;
    else
        iplugin = _("No input plugin recognized this file");

    title = g_strdup_printf(_("Input plugin: %s"), iplugin);

    label = gtk_label_new(title);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    g_free(title);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

    cancel = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(G_OBJECT(cancel), "clicked",
                             GTK_SIGNAL_FUNC(gtk_widget_destroy),
                             GTK_OBJECT(window));
    GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);

    gtk_widget_show_all(window);
}

void
input_file_info_box(const gchar * filename)
{
    InputPlugin *ip;
    gchar *filename_proxy;
    ProbeResult *pr;

    filename_proxy = g_strdup(filename);

    pr = input_check_file(filename_proxy, FALSE);

    if (!pr)
        return;

    ip = pr->ip;

    g_free(pr);

    if (ip->file_info_box)
    {
        plugin_set_current((Plugin *)ip);
        ip->file_info_box(filename_proxy);
    }
    else
        input_general_file_info_box(filename, ip);

    input_general_file_info_box(filename, NULL);
    g_free(filename_proxy);
}

GList *
input_scan_dir(const gchar * path)
{
    GList *node, *result = NULL;
    InputPlugin *ip;
    gchar *path_proxy;

    g_return_val_if_fail(path != NULL, NULL);

    if (*path == '/')
        while (path[1] == '/')
            path++;

    path_proxy = g_strdup(path);

    for (node = get_input_list(); node; node = g_list_next(node)) {
        ip = INPUT_PLUGIN(node->data);

        if (!ip)
            continue;

        if (!ip->scan_dir)
            continue;

        if (!ip->enabled)
            continue;

        plugin_set_current((Plugin *)ip);
        if ((result = ip->scan_dir(path_proxy)))
            break;
    }

    g_free(path_proxy);

    return result;
}

void
input_get_volume(gint * l, gint * r)
{
    if (volume_l == -1 || volume_r == -1)
	output_get_volume(&volume_l, &volume_r);

    *l = volume_l;
    *r = volume_r;
}

void
input_set_volume(gint l, gint r)
{
    InputPlayback *playback;

    gint h_vol[2];

    h_vol[0] = l;
    h_vol[1] = r;
    hook_call("volume set", h_vol);

    if (playback_get_playing())
        if ((playback = get_current_input_playback()) != NULL)
	    if (playback->plugin->set_volume != NULL)
	    {
	        plugin_set_current((Plugin *)(playback->plugin));
	        if (playback->plugin->set_volume(l, r))
		    return;
	    }

    output_set_volume(l, r);

    volume_l = l;
    volume_r = r;
}

void
input_update_vis(gint time)
{
    GList *node;
    VisNode *vis = NULL, *visnext = NULL;
    gboolean found = FALSE;

    G_LOCK(vis_mutex);
    node = vis_list;
    while (g_list_next(node) && !found) {
        visnext = g_list_next(node)->data;
        vis = node->data;

        if (vis->time >= time)
            break;

        vis_list = g_list_delete_link(vis_list, node);

        if (visnext->time >= time) {
            found = TRUE;
            break;
        }
        g_free(vis);
        node = vis_list;
    }
    G_UNLOCK(vis_mutex);

    if (found) {
        vis_send_data(vis->data, vis->nch, vis->length);
        hook_call("visualization timeout", vis);
        g_free(vis);
    }
    else
        hook_call("visualization timeout", NULL);
}

/* FIXME: move this somewhere else */
void
input_set_info_text(gchar *text)
{
    gchar *title = g_strdup(text);
    event_queue_with_data_free("title change", title);
}
