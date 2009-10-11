/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2009  Audacious development team
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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#include <mowgli.h>

#include "input.h"

InputPluginData ip_data = {
    NULL,
    NULL,
    FALSE,
    FALSE,
    FALSE,
    NULL
};

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

/* do actual probing. this function is called from input_check_file() */
static ProbeResult * input_do_check_file (InputPlugin * ip,
    VFSFile * fd, gchar * filename_proxy)
{
    ProbeResult *pr = NULL;
    gint result = 0;

    g_return_val_if_fail(fd != NULL, NULL);

    pr = g_new0(ProbeResult, 1);
    pr->ip = ip;

    vfs_rewind(fd);

    if (ip->is_our_file_from_vfs != NULL)
    {
        result = ip->is_our_file_from_vfs(filename_proxy, fd);

        if (result > 0)
            return pr;
    }
    else if (ip->is_our_file != NULL)
    {
        result = ip->is_our_file(filename_proxy);

        if (result > 0)
            return pr;
    }

    g_free(pr);

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

ProbeResult * input_check_file (const gchar * filename)
{
    VFSFile *fd;
    GList *node;
    InputPlugin *ip;
    gchar *filename_proxy, *mimetype;
    gint ret = 1;
    gboolean use_ext_filter = FALSE;
    ProbeResult *pr = NULL;
    extern GHashTable *ext_hash;

    /* Some URIs have special subsong identifier to determine the subsong requested. */
    filename_proxy = filename_split_subtune(filename, NULL);

    /* Check for plugins with custom URI:// strings */
    /* cue:// cdda:// tone:// tact:// */
    if ((ip = uri_get_plugin(filename_proxy)) != NULL && ip->enabled) {
        if (ip->is_our_file != NULL) {
            ret = ip->is_our_file(filename_proxy);
        } else
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


    /* Open the file with vfs sub-system.
     * FIXME! XXX! buffered VFS file does not handle mixed seeks and reads/writes
     * correctly! As a temporary workaround, switching to unbuffered ... -ccr
     */
    //fd = vfs_buffered_file_new_from_uri(filename_proxy);
    fd = vfs_fopen(filename_proxy, "rb");

    if (fd == NULL) {
        g_warning("Unable to read from %s, giving up.\n", filename_proxy);
        g_free(filename_proxy);
        return NULL;
    }


    /* Apply mimetype check. note that stdio does not support mimetype check. */
    mimetype = vfs_get_metadata(fd, "content-type");
    if (mimetype != NULL) {
        ip = mime_get_plugin(mimetype);
        g_free(mimetype);
    } else
        ip = NULL;

    if (ip != NULL && ip->enabled) {
        pr = input_do_check_file (ip, fd, filename_proxy);
        if (pr != NULL) {
            g_free(filename_proxy);
            vfs_fclose(fd);
            return pr;
        }
    }


    /* Apply ext_hash check */
    if (cfg.use_extension_probing) {
        use_ext_filter =
            (fd != NULL && (!g_ascii_strncasecmp(filename_proxy, "/", 1) ||
            !g_ascii_strncasecmp(filename_proxy, "file://", 7))) ? TRUE : FALSE;
    }

    if (use_ext_filter) {
        GList **list_hdr = NULL;
        gchar *base, *lext, *ext;
        gchar *tmp2 = g_filename_from_uri(filename_proxy, NULL, NULL);
        gchar *realfn = g_strdup(tmp2 ? tmp2 : filename_proxy);
        g_free(tmp2);

        base = g_path_get_basename(realfn);
        g_free(realfn);
        ext = strrchr(base, '.');

        if (ext) {
            lext = g_ascii_strdown(ext+1, -1);
            list_hdr = g_hash_table_lookup(ext_hash, lext);
            g_free(lext);
        }
        g_free(base);

        if (list_hdr != NULL) {
            for(node = *list_hdr; node != NULL; node = g_list_next(node)) {
                ip = INPUT_PLUGIN(node->data);

                if (!ip || !ip->enabled)
                    continue;

                pr = input_do_check_file (ip, fd, filename_proxy);

                if(pr) {
                    g_free(filename_proxy);
                    vfs_fclose(fd);
                    return pr;
                }
            }
        }
    }


    /* Do full scan when extension match isn't specified. */
    for (node = get_input_list(); node != NULL; node = g_list_next(node)) {
        ip = INPUT_PLUGIN(node->data);

        if (ip == NULL || !ip->enabled)
            continue;

        pr = input_do_check_file (ip, fd, filename_proxy);

        if (pr != NULL) {
            g_free(filename_proxy);
            vfs_fclose(fd);
            return pr;
        }
    }


    /* All probing failed, return NULL. */
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

    pr = input_check_file (filename_proxy);

    if (!pr) {
        g_free(filename_proxy);
        return NULL;
    }

    ip = pr->ip;

    g_free(pr);

    if (ip && ip->get_song_tuple)
        input = ip->get_song_tuple(filename_proxy);
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
                             G_CALLBACK(gtk_widget_destroy),
                             window);
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

    pr = input_check_file (filename_proxy);

    if (!pr)
        return;

    ip = pr->ip;

    g_free(pr);

    if (ip->file_info_box)
        ip->file_info_box(filename_proxy);
    else
        input_general_file_info_box(filename, ip);

    g_free(filename_proxy);
}
void
input_get_volume(gint * l, gint * r)
{
    if (ip_data.playing && ip_data.current_input_playback &&
     ip_data.current_input_playback->plugin->get_volume &&
     ip_data.current_input_playback->plugin->get_volume (l, r))
        return;

    output_get_volume (l, r);
}

void
input_set_volume(gint l, gint r)
{
    gint h_vol[2];

    h_vol[0] = l;
    h_vol[1] = r;
    hook_call("volume set", h_vol);

    if (ip_data.playing && ip_data.current_input_playback &&
     ip_data.current_input_playback->plugin->set_volume &&
     ip_data.current_input_playback->plugin->set_volume (l, r))
        return;

    output_set_volume (l, r);
}
