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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#include "fft.h"
#include "input.h"
#include "main.h"
#include "output.h"
#include "playback.h"
#include "pluginenum.h"
#include "strings.h"
#include "tuple.h"
#include "tuple_formatter.h"
#include "ui_main.h"
#include "util.h"
#include "visualization.h"
#include "ui_skinned_playstatus.h"
#include "hook.h"

#include "vfs.h"
#include "vfs_buffer.h"
#include "vfs_buffered_file.h"

G_LOCK_DEFINE_STATIC(vis_mutex);

struct _VisNode {
    gint time;
    gint nch;
    gint length;                /* number of samples per channel */
    gint16 data[2][512];
};

typedef struct _VisNode VisNode;


InputPluginData ip_data = {
    NULL,
    NULL,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    NULL
};

static GList *vis_list = NULL;

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


gboolean
input_is_enabled(const gchar * filename)
{
    gchar *basename = g_path_get_basename(filename);
    gint enabled;

    enabled = GPOINTER_TO_INT(g_hash_table_lookup(plugin_matrix, basename));
    g_free(basename);

    return enabled;
}

static void
disabled_iplugins_foreach_func(const gchar * name,
                               gboolean enabled,
                               GString * list)
{
    g_return_if_fail(list != NULL);

    if (enabled)
        return;

    if (list->len > 0)
        g_string_append(list, ":");

    g_string_append(list, name);
}

gchar *
input_stringify_disabled_list(void)
{
    GString *disabled_list;

    disabled_list = g_string_new("");
    g_hash_table_foreach(plugin_matrix,
                         (GHFunc) disabled_iplugins_foreach_func,
                         disabled_list);

    return g_string_free(disabled_list, FALSE);
}

void
free_vis_data(void)
{
    G_LOCK(vis_mutex);
    g_list_foreach(vis_list, (GFunc) g_free, NULL);
    g_list_free(vis_list);
    vis_list = NULL;
    G_UNLOCK(vis_mutex);
}

static void
convert_to_s16_ne(AFormat fmt, gpointer ptr, gint16 * left,
                  gint16 * right, gint nch, gint max)
{
    gint16 *ptr16;
    guint16 *ptru16;
    guint8 *ptru8;
    gint i;

    switch (fmt) {
    case FMT_U8:
        ptru8 = ptr;
        if (nch == 1)
            for (i = 0; i < max; i++)
                left[i] = ((*ptru8++) ^ 128) << 8;
        else
            for (i = 0; i < max; i++) {
                left[i] = ((*ptru8++) ^ 128) << 8;
                right[i] = ((*ptru8++) ^ 128) << 8;
            }
        break;
    case FMT_S8:
        ptru8 = ptr;
        if (nch == 1)
            for (i = 0; i < max; i++)
                left[i] = (*ptru8++) << 8;
        else
            for (i = 0; i < max; i++) {
                left[i] = (*ptru8++) << 8;
                right[i] = (*ptru8++) << 8;
            }
        break;
    case FMT_U16_LE:
        ptru16 = ptr;
        if (nch == 1)
            for (i = 0; i < max; i++, ptru16++)
                left[i] = GUINT16_FROM_LE(*ptru16) ^ 32768;
        else
            for (i = 0; i < max; i++) {
                left[i] = GUINT16_FROM_LE(*ptru16) ^ 32768;
                ptru16++;
                right[i] = GUINT16_FROM_LE(*ptru16) ^ 32768;
                ptru16++;
            }
        break;
    case FMT_U16_BE:
        ptru16 = ptr;
        if (nch == 1)
            for (i = 0; i < max; i++, ptru16++)
                left[i] = GUINT16_FROM_BE(*ptru16) ^ 32768;
        else
            for (i = 0; i < max; i++) {
                left[i] = GUINT16_FROM_BE(*ptru16) ^ 32768;
                ptru16++;
                right[i] = GUINT16_FROM_BE(*ptru16) ^ 32768;
                ptru16++;
            }
        break;
    case FMT_U16_NE:
        ptru16 = ptr;
        if (nch == 1)
            for (i = 0; i < max; i++)
                left[i] = (*ptru16++) ^ 32768;
        else
            for (i = 0; i < max; i++) {
                left[i] = (*ptru16++) ^ 32768;
                right[i] = (*ptru16++) ^ 32768;
            }
        break;
    case FMT_S16_LE:
        ptr16 = ptr;
        if (nch == 1)
            for (i = 0; i < max; i++, ptr16++)
                left[i] = GINT16_FROM_LE(*ptr16);
        else
            for (i = 0; i < max; i++) {
                left[i] = GINT16_FROM_LE(*ptr16);
                ptr16++;
                right[i] = GINT16_FROM_LE(*ptr16);
                ptr16++;
            }
        break;
    case FMT_S16_BE:
        ptr16 = ptr;
        if (nch == 1)
            for (i = 0; i < max; i++, ptr16++)
                left[i] = GINT16_FROM_BE(*ptr16);
        else
            for (i = 0; i < max; i++) {
                left[i] = GINT16_FROM_BE(*ptr16);
                ptr16++;
                right[i] = GINT16_FROM_BE(*ptr16);
                ptr16++;
            }
        break;
    case FMT_S16_NE:
        ptr16 = ptr;
        if (nch == 1)
            for (i = 0; i < max; i++)
                left[i] = (*ptr16++);
        else
            for (i = 0; i < max; i++) {
                left[i] = (*ptr16++);
                right[i] = (*ptr16++);
            }
        break;
    }
}

InputVisType
input_get_vis_type()
{
    return INPUT_VIS_OFF;
}

void
input_add_vis_pcm(gint time, AFormat fmt, gint nch, gint length, gpointer ptr)
{
    VisNode *vis_node;
    gint max;

    max = length / nch;
    if (fmt == FMT_U16_LE || fmt == FMT_U16_BE || fmt == FMT_U16_NE ||
        fmt == FMT_S16_LE || fmt == FMT_S16_BE || fmt == FMT_S16_NE)
        max /= 2;
    max = CLAMP(max, 0, 512);

    vis_node = g_new0(VisNode, 1);
    vis_node->time = time;
    vis_node->nch = nch;
    vis_node->length = max;
    convert_to_s16_ne(fmt, ptr, vis_node->data[0], vis_node->data[1], nch,
                      max);

    G_LOCK(vis_mutex);
    vis_list = g_list_append(vis_list, vis_node);
    G_UNLOCK(vis_mutex);
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
 */
ProbeResult *
input_check_file(const gchar * filename, gboolean show_warning)
{
    VFSFile *fd;
    GList *node;
    InputPlugin *ip;
    gchar *filename_proxy;
    gint ret = 1;
    gchar *ext, *tmp, *tmp_uri;
    gboolean use_ext_filter;
    gchar *mimetype;
    ProbeResult *pr = NULL;

    filename_proxy = g_strdup(filename);

    /* Some URIs will end in ?<subsong> to determine the subsong requested. */
    tmp_uri = g_strdup(filename);

    tmp = strrchr(tmp_uri, '?');

    if (tmp != NULL && g_ascii_isdigit(*(tmp + 1)))
        *tmp = '\0';

    /* Check for plugins with custom URI:// strings */
    /* cue:// cdda:// tone:// tact:// */
    if ((ip = uri_get_plugin(filename)) != NULL &&
        input_is_enabled(ip->filename) == TRUE)
    {
        if (ip->is_our_file != NULL)
            ret = ip->is_our_file(filename_proxy);
        else
            ret = 0;
        if (ret > 0)
        {
            g_free(filename_proxy);
            pr = g_new0(ProbeResult, 1);
            pr->ip = ip;
            return pr;
        }
        g_free(filename_proxy);
        return NULL;
    }

    /* CD-Audio uses cdda:// dummy paths, no filedescriptor handling for it */
    /* also cuesheet uses cue:// */
/*
    if (!g_strncasecmp(filename, "cue://", 6)) {
        for (node = get_input_list(); node != NULL; node = g_list_next(node))
        {
            ip = INPUT_PLUGIN(node->data);
            if (!ip || !input_is_enabled(ip->filename))
                continue;
            if (ip->is_our_file != NULL)
                ret = ip->is_our_file(filename_proxy);
            else
                ret = 0;
            if (ret > 0)
            {
                g_free(filename_proxy);
                pr = g_new0(ProbeResult, 1);
                pr->ip = ip;
                return pr;
            }
        }
        g_free(filename_proxy);
        return NULL;
    }
*/
    fd = vfs_buffered_file_new_from_uri(tmp_uri);
    g_free(tmp_uri);

    if (!fd) {
        printf("Unable to read from %s, giving up.\n", filename_proxy);
        g_free(filename_proxy);
        return NULL;
    }

    ext = strrchr(filename_proxy, '.') + 1;

    use_ext_filter =
        (fd != NULL && (!g_strncasecmp(filename, "/", 1) ||
                        !g_strncasecmp(filename, "file://", 7))) ? TRUE : FALSE;

    mimetype = vfs_get_metadata(fd, "content-type");
    if ((ip = mime_get_plugin(mimetype)) != NULL &&
	input_is_enabled(ip->filename) == TRUE)
    {
        if (ip->probe_for_tuple != NULL)
        {
            Tuple *tuple = ip->probe_for_tuple(filename_proxy, fd);

            if (tuple != NULL)
            {
                g_free(filename_proxy);
                vfs_fclose(fd);

                pr = g_new0(ProbeResult, 1);
                pr->ip = ip;
                pr->tuple = tuple;
                tuple_associate_int(pr->tuple, "mtime", input_get_mtime(filename_proxy));

                return pr;
            }
        }
        else if (fd && ip->is_our_file_from_vfs != NULL)
        {
            ret = ip->is_our_file_from_vfs(filename_proxy, fd);

            if (ret > 0)
            {
                g_free(filename_proxy);
                vfs_fclose(fd);

                pr = g_new0(ProbeResult, 1);
                pr->ip = ip;

                return pr;
            }
        }
        else if (ip->is_our_file != NULL)
        {
            ret = ip->is_our_file(filename_proxy);

            if (ret > 0)
            {
                g_free(filename_proxy);
                vfs_fclose(fd);

                pr = g_new0(ProbeResult, 1);
                pr->ip = ip;

                return pr;
            }
        }
    }

    for (node = get_input_list(); node != NULL; node = g_list_next(node))
    {
        ip = INPUT_PLUGIN(node->data);

        if (!ip || !input_is_enabled(ip->filename))
            continue;

        vfs_rewind(fd);

        if (cfg.use_extension_probing == TRUE && ip->vfs_extensions != NULL &&
            ext != NULL && ext != (gpointer) 0x1 && use_ext_filter == TRUE)
        {
            gint i;
            gboolean is_our_ext = FALSE;

            for (i = 0; ip->vfs_extensions[i] != NULL; i++)
            {
                if (str_has_prefix_nocase(ext, ip->vfs_extensions[i]))
                {
                    is_our_ext = TRUE;
                    break;
                }
            }

            /* not a plugin that supports this extension */
            if (is_our_ext == FALSE) 
                continue;
        }

        if (fd && ip->probe_for_tuple != NULL)
        {
            Tuple *tuple = ip->probe_for_tuple(filename_proxy, fd);

            if (tuple != NULL)
            {
                g_free(filename_proxy);
                vfs_fclose(fd);

                pr = g_new0(ProbeResult, 1);
                pr->ip = ip;
                pr->tuple = tuple;
                tuple_associate_int(pr->tuple, "mtime", input_get_mtime(filename_proxy));

                return pr;
            }
        }
        else if (fd && ip->is_our_file_from_vfs != NULL)
        {
            ret = ip->is_our_file_from_vfs(filename_proxy, fd);

            if (ret > 0)
            {
                g_free(filename_proxy);
                vfs_fclose(fd);

                pr = g_new0(ProbeResult, 1);
                pr->ip = ip;

                return pr;
            }
        }
        else if (ip->is_our_file != NULL)
        {
            ret = ip->is_our_file(filename_proxy);

            if (ret > 0)
            {
                g_free(filename_proxy);
                vfs_fclose(fd);

                pr = g_new0(ProbeResult, 1);
                pr->ip = ip;

                return pr;
            }
        }

        if (ret <= -1)
            break;
    }

    g_free(filename_proxy);

    vfs_fclose(fd);

    return NULL;
}


void
input_set_eq(gint on, gfloat preamp, gfloat * bands)
{
    InputPlayback *playback;

    if (!ip_data.playing)
        return;

    if ((playback = get_current_input_playback()) == NULL)
        return;

    if (playback->plugin->set_eq)
        playback->plugin->set_eq(on, preamp, bands);
}

Tuple *
input_get_song_tuple(const gchar * filename)
{
    InputPlugin *ip = NULL;
    Tuple *input;
    gchar *ext = NULL;
    gchar *filename_proxy;
    ProbeResult *pr;

    if (filename == NULL)
    return NULL;

    filename_proxy = g_strdup(filename);

    pr = input_check_file(filename_proxy, FALSE);

    if (!pr)
        return NULL;

    ip = pr->ip;

    g_free(pr);

    if (ip && ip->get_song_tuple)
        input = ip->get_song_tuple(filename_proxy);
    else
    {
        gchar *tmp;

        input = tuple_new();

        tmp = g_strdup(filename);
        if ((ext = strrchr(tmp, '.')))
            *ext = '\0';

        tuple_associate_string(input, "file-name", g_path_get_basename(tmp));

        if (ext)
            tuple_associate_string(input, "file-ext", ext + 1);

        tuple_associate_string(input, "file-path", g_path_get_dirname(tmp));
        tuple_associate_int(input, "length", -1);

        g_free(tmp);
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
        ip->file_info_box(filename_proxy);
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

        if (!input_is_enabled(ip->filename))
            continue;

        if ((result = ip->scan_dir(path_proxy)))
            break;
    }

    g_free(path_proxy);

    return result;
}

void
input_get_volume(gint * l, gint * r)
{
    InputPlayback *playback;

    *l = -1;
    *r = -1;
    if (playback_get_playing()) {
        if ((playback = get_current_input_playback()) != NULL &&
            playback->plugin->get_volume &&
            playback->plugin->get_volume(l, r))
            return;
    }
    output_get_volume(l, r);
}

void
input_set_volume(gint l, gint r)
{
    InputPlayback *playback;

    gint h_vol[2];

    h_vol[0] = l;
    h_vol[1] = r;
    hook_call("volume set", h_vol);
    
    if (playback_get_playing() &&
        (playback = get_current_input_playback()) != NULL &&
        playback->plugin->set_volume &&
        playback->plugin->set_volume(l, r))
        return;

    output_set_volume(l, r);
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
        g_free(vis);
    }
    else
        vis_send_data(NULL, 0, 0);
}

/* FIXME: move this somewhere else */
void
input_set_info_text(gchar *text)
{
    gchar *title = g_strdup(text);
    event_queue("title change", title);
}

void
input_set_status_buffering(gboolean status)
{
    if (!playback_get_playing())
        return;

    if (!get_current_input_playback())
        return;

    ip_data.buffering = status;

    g_return_if_fail(mainwin_playstatus != NULL);

    if (ip_data.buffering == TRUE && mainwin_playstatus != NULL && UI_SKINNED_PLAYSTATUS(mainwin_playstatus)->status == STATUS_STOP)
        UI_SKINNED_PLAYSTATUS(mainwin_playstatus)->status = STATUS_PLAY;

    ui_skinned_playstatus_set_buffering(mainwin_playstatus, ip_data.buffering);
}

void
input_about(gint index)
{
    InputPlugin *ip;

    ip = g_list_nth(ip_data.input_list, index)->data;
    if (ip && ip->about)
        ip->about();
}

void
input_configure(gint index)
{
    InputPlugin *ip;

    ip = g_list_nth(ip_data.input_list, index)->data;
    if (ip && ip->configure)
        ip->configure();
}
