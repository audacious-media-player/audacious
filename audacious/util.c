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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define WEIRD_UTF_16_PLAYLIST_ENCODING

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#define NEED_GLADE
#include "util.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <X11/Xlib.h>
//#include <sys/ipc.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_FTS_H
#  include <fts.h>
#endif

#include "glade.h"
#include "input.h"
#include "main.h"
#include "playback.h"
#include "playlist.h"
#include "ui_playlist.h"

#ifdef USE_CHARDET
    #include "../libguess/libguess.h"
#ifdef HAVE_UDET
    #include <libudet_c.h>
#endif
#endif

static GQuark quark_popup_data;


/*
 * escape_shell_chars()
 *
 * Escapes characters that are special to the shell inside double quotes.
 */

gchar *
escape_shell_chars(const gchar * string)
{
    const gchar *special = "$`\"\\";    /* Characters to escape */
    const gchar *in = string;
    gchar *out, *escaped;
    gint num = 0;

    while (*in != '\0')
        if (strchr(special, *in++))
            num++;

    escaped = g_malloc(strlen(string) + num + 1);

    in = string;
    out = escaped;

    while (*in != '\0') {
        if (strchr(special, *in))
            *out++ = '\\';
        *out++ = *in++;
    }
    *out = '\0';

    return escaped;
}


/*
 * find <file> in directory <dirname> or subdirectories.  return
 * pointer to complete filename which has to be freed by calling
 * "g_free()" after use. Returns NULL if file could not be found.
 */

typedef struct {
    const gchar *to_match;
    gchar *match;
    gboolean found;
} FindFileContext;

static gboolean
find_file_func(const gchar * path, const gchar * basename, gpointer data)
{
    FindFileContext *context = data;

    if (strlen(path) > FILENAME_MAX) {
        g_warning("Ignoring path: name too long (%s)", path);
        return TRUE;
    }

    if (g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
        if (!strcasecmp(basename, context->to_match)) {
            context->match = g_strdup(path);
            context->found = TRUE;
            return TRUE;
        }
    }
    else if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
        dir_foreach(path, find_file_func, context, NULL);
        if (context->found)
            return TRUE;
    }

    return FALSE;
}

gchar *
find_file_recursively(const gchar * path, const gchar * filename)
{
    FindFileContext context;

    context.to_match = filename;
    context.match = NULL;
    context.found = FALSE;

    dir_foreach(path, find_file_func, &context, NULL);
    return context.match;
}


typedef enum {
    ARCHIVE_UNKNOWN = 0,
    ARCHIVE_DIR,
    ARCHIVE_TAR,
    ARCHIVE_TGZ,
    ARCHIVE_ZIP,
    ARCHIVE_TBZ2
} ArchiveType;

typedef gchar *(*ArchiveExtractFunc) (const gchar *, const gchar *);

typedef struct {
    ArchiveType type;
    const gchar *ext;
} ArchiveExtensionType;

static ArchiveExtensionType archive_extensions[] = {
    {ARCHIVE_TAR, ".tar"},
    {ARCHIVE_ZIP, ".wsz"},
    {ARCHIVE_ZIP, ".zip"},
    {ARCHIVE_TGZ, ".tar.gz"},
    {ARCHIVE_TGZ, ".tgz"},
    {ARCHIVE_TBZ2, ".tar.bz2"},
    {ARCHIVE_TBZ2, ".bz2"},
    {ARCHIVE_UNKNOWN, NULL}
};

static gchar *archive_extract_tar(const gchar * archive, const gchar * dest);
static gchar *archive_extract_zip(const gchar * archive, const gchar * dest);
static gchar *archive_extract_tgz(const gchar * archive, const gchar * dest);
static gchar *archive_extract_tbz2(const gchar * archive, const gchar * dest);

static ArchiveExtractFunc archive_extract_funcs[] = {
    NULL,
    NULL,
    archive_extract_tar,
    archive_extract_tgz,
    archive_extract_zip,
    archive_extract_tbz2
};


/* FIXME: these functions can be generalised into a function using a
 * command lookup table */

static const gchar *
get_tar_command(void)
{
    static const gchar *command = NULL;

    if (!command) {
        if (!(command = getenv("TARCMD")))
            command = "tar";
    }

    return command;
}

static const gchar *
get_unzip_command(void)
{
    static const gchar *command = NULL;

    if (!command) {
        if (!(command = getenv("UNZIPCMD")))
            command = "unzip";
    }

    return command;
}


static gchar *
archive_extract_tar(const gchar * archive, const gchar * dest)
{
    return g_strdup_printf("%s >/dev/null xf \"%s\" -C %s",
                           get_tar_command(), archive, dest);
}

static gchar *
archive_extract_zip(const gchar * archive, const gchar * dest)
{
    return g_strdup_printf("%s >/dev/null -o -j \"%s\" -d %s",
                           get_unzip_command(), archive, dest);
}

static gchar *
archive_extract_tgz(const gchar * archive, const gchar * dest)
{
    return g_strdup_printf("%s >/dev/null xzf \"%s\" -C %s",
                           get_tar_command(), archive, dest);
}

static gchar *
archive_extract_tbz2(const gchar * archive, const gchar * dest)
{
    return g_strdup_printf("bzip2 -dc \"%s\" | %s >/dev/null xf - -C %s",
                           archive, get_tar_command(), dest);
}


ArchiveType
archive_get_type(const gchar * filename)
{
    gint i = 0;

    if (g_file_test(filename, G_FILE_TEST_IS_DIR))
        return ARCHIVE_DIR;

    while (archive_extensions[i].ext) {
        if (g_str_has_suffix(filename, archive_extensions[i].ext)) {
            return archive_extensions[i].type;
        }
        i++;
    }

    return ARCHIVE_UNKNOWN;
}

gboolean
file_is_archive(const gchar * filename)
{
    return (archive_get_type(filename) > ARCHIVE_DIR);
}

gchar *
archive_basename(const gchar * str)
{
    gint i = 0;

    while (archive_extensions[i].ext) {
        if (str_has_suffix_nocase(str, archive_extensions[i].ext)) {
            const gchar *end = g_strrstr(str, archive_extensions[i].ext);
            if (end) {
                return g_strndup(str, end - str);
            }
            break;
        }
        i++;
    }

    return NULL;
}

/*
   decompress_archive

   Decompresses the archive "filename" to a temporary directory,
   returns the path to the temp dir, or NULL if failed,
   watch out tho, doesn't actually check if the system command succeeds :-|
*/

gchar *
archive_decompress(const gchar * filename)
{
    gchar *tmpdir, *cmd, *escaped_filename;
    ArchiveType type;

    if ((type = archive_get_type(filename)) <= ARCHIVE_DIR)
        return NULL;

    tmpdir = g_build_filename(g_get_tmp_dir(), "bmp.XXXXXXXX", NULL);
    if (!mkdtemp(tmpdir)) {
        g_free(tmpdir);
        g_message("Unable to load skin: Failed to create temporary "
                  "directory: %s", g_strerror(errno));
        return NULL;
    }

    escaped_filename = escape_shell_chars(filename);
    cmd = archive_extract_funcs[type] (escaped_filename, tmpdir);
    g_free(escaped_filename);

    if (!cmd) {
        g_message("extraction function is NULL!");
        g_free(tmpdir);
        return NULL;
    }

    system(cmd);
    g_free(cmd);

    return tmpdir;
}


#ifdef HAVE_FTS_H

void
del_directory(const gchar * dirname)
{
    gchar *const argv[2] = { (gchar *) dirname, NULL };
    FTS *fts;
    FTSENT *p;

    fts = fts_open(argv, FTS_PHYSICAL, (gint(*)())NULL);
    while ((p = fts_read(fts))) {
        switch (p->fts_info) {
        case FTS_D:
            break;
        case FTS_DNR:
        case FTS_ERR:
            break;
        case FTS_DP:
            rmdir(p->fts_accpath);
            break;
        default:
            unlink(p->fts_accpath);
            break;
        }
    }
    fts_close(fts);
}

#else                           /* !HAVE_FTS */

gboolean
del_directory_func(const gchar * path, const gchar * basename,
                   gpointer params)
{
    if (!strcmp(basename, ".") || !strcmp(path, ".."))
        return FALSE;

    if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
        dir_foreach(path, del_directory_func, NULL, NULL);
        rmdir(path);
        return FALSE;
    }

    unlink(path);

    return FALSE;
}

void
del_directory(const gchar * path)
{
    dir_foreach(path, del_directory_func, NULL, NULL);
    rmdir(path);
}

#endif                          /* ifdef HAVE_FTS */

gchar *
read_ini_string(const gchar * filename, const gchar * section,
                const gchar * key)
{
    static gchar *buffer = NULL;
    static gchar *open_buffer = NULL;
    gchar *ret_buffer = NULL;
    gint found_section = 0, len = 0;
    static gsize filesize = 0;
    gsize off = 0;
    gchar *outbuf;
    unsigned char x[] = { 0xff, 0xfe, 0x00 };
    guint counter;

    if (!filename)
        return NULL;

    /*
     * We optimise for the condition that we may be reading from the
     * same ini-file multiple times. This is fairly common; it happens
     * on .pls playlist loads. To do otherwise would take entirely too
     * long, as fstat() can be very slow when done 21,000 times too many.
     *
     * Therefore, we optimise by keeping the last ini file in memory.
     * Yes, this is a memory leak, but it is not too bad, hopefully.
     *      - nenolod
     */
    if (open_buffer == NULL || strcasecmp(filename, open_buffer))
    {
        if (buffer != NULL)
	{
            g_free(buffer);
            buffer = NULL;
        }

	if (open_buffer != NULL)
        {
	    g_free(open_buffer);
            open_buffer = NULL;
        }

        if (!g_file_get_contents(filename, &buffer, &filesize, NULL))
            return NULL;

        open_buffer = g_strdup(filename);
    }

    /*
     * Convert UTF-16 into something useful. Original implementation
     * by incomp@#audacious. Cleanups \nenolod
     */
    if (!memcmp(&buffer[0],&x,2)) {
        outbuf = g_malloc (filesize);	/* it's safe to waste memory. */

        for (counter = 2; counter < filesize; counter += 2)
            if (!memcmp(&buffer[counter+1], &x[2], 1))
                outbuf[(counter-2)/2] = buffer[counter];
            else
		return NULL;

        outbuf[(counter-2)/2] = '\0';

        if ((filesize - 2) / 2 == (counter - 2) / 2) {
            g_free(buffer);
            buffer = outbuf;
        } else {
            g_free(outbuf);
	    return NULL;	/* XXX wrong encoding */
        }
    }

    while (!ret_buffer && off < filesize) {
        while (off < filesize &&
               (buffer[off] == '\r' || buffer[off] == '\n' ||
                buffer[off] == ' ' || buffer[off] == '\t'))
            off++;
        if (off >= filesize)
            break;
        if (buffer[off] == '[') {
            gint slen = strlen(section);
            off++;
            found_section = 0;
            if (off + slen + 1 < filesize &&
                !strncasecmp(section, &buffer[off], slen)) {
                off += slen;
                if (buffer[off] == ']') {
                    off++;
                    found_section = 1;
                }
            }
        }
        else if (found_section && off + strlen(key) < filesize &&
                 !strncasecmp(key, &buffer[off], strlen(key))) {
            off += strlen(key);
            while (off < filesize &&
                   (buffer[off] == ' ' || buffer[off] == '\t'))
                off++;
            if (off >= filesize)
                break;
            if (buffer[off] == '=') {
                off++;
                while (off < filesize &&
                       (buffer[off] == ' ' || buffer[off] == '\t'))
                    off++;
                if (off >= filesize)
                    break;
                len = 0;
                while (off + len < filesize &&
                       buffer[off + len] != '\r' &&
                       buffer[off + len] != '\n' && buffer[off + len] != ';')
                    len++;
                ret_buffer = g_strndup(&buffer[off], len);
                off += len;
            }
        }
        while (off < filesize && buffer[off] != '\r' && buffer[off] != '\n')
            off++;
    }

    return ret_buffer;
}

GArray *
string_to_garray(const gchar * str)
{
    GArray *array;
    gint temp;
    const gchar *ptr = str;
    gchar *endptr;

    array = g_array_new(FALSE, TRUE, sizeof(gint));
    for (;;) {
        temp = strtol(ptr, &endptr, 10);
        if (ptr == endptr)
            break;
        g_array_append_val(array, temp);
        ptr = endptr;
        while (!isdigit(*ptr) && (*ptr) != '\0')
            ptr++;
        if (*ptr == '\0')
            break;
    }
    return (array);
}

GArray *
read_ini_array(const gchar * filename, const gchar * section,
               const gchar * key)
{
    gchar *temp;
    GArray *a;

    if ((temp = read_ini_string(filename, section, key)) == NULL) {
        g_free(temp);
        return NULL;
    }
    a = string_to_garray(temp);
    g_free(temp);
    return a;
}

void
glist_movedown(GList * list)
{
    gpointer temp;

    if (g_list_next(list)) {
        temp = list->data;
        list->data = list->next->data;
        list->next->data = temp;
    }
}

void
glist_moveup(GList * list)
{
    gpointer temp;

    if (g_list_previous(list)) {
        temp = list->data;
        list->data = list->prev->data;
        list->prev->data = temp;
    }
}


void
util_menu_position(GtkMenu * menu, gint * x, gint * y,
                   gboolean * push_in, gpointer data)
{
    GtkRequisition requisition;
    gint screen_width;
    gint screen_height;
    MenuPos *pos = data;

    gtk_widget_size_request(GTK_WIDGET(menu), &requisition);

    screen_width = gdk_screen_width();
    screen_height = gdk_screen_height();

    *x = CLAMP(pos->x - 2, 0, MAX(0, screen_width - requisition.width));
    *y = CLAMP(pos->y - 2, 0, MAX(0, screen_height - requisition.height));
}

static void
util_menu_delete_popup_data(GtkObject * object, GtkItemFactory * ifactory)
{
    gtk_signal_disconnect_by_func(object,
                                  GTK_SIGNAL_FUNC
                                  (util_menu_delete_popup_data), ifactory);
    gtk_object_remove_data_by_id(GTK_OBJECT(ifactory), quark_popup_data);
}


/*
 * util_item_factory_popup[_with_data]() is a replacement for
 * gtk_item_factory_popup[_with_data]().
 *
 * The difference is that the menu is always popped up whithin the
 * screen.  This means it does not neccesarily pop up at (x,y).
 */

void
util_item_factory_popup_with_data(GtkItemFactory * ifactory,
                                  gpointer data,
                                  GtkDestroyNotify destroy, guint x,
                                  guint y, guint mouse_button, guint32 time)
{
    static GQuark quark_user_menu_pos = 0;
    MenuPos *pos;

    if (!quark_user_menu_pos)
        quark_user_menu_pos = g_quark_from_static_string("user_menu_pos");

    if (!quark_popup_data)
        quark_popup_data =
            g_quark_from_static_string("GtkItemFactory-popup-data");

    pos = g_object_get_qdata(G_OBJECT(ifactory), quark_user_menu_pos);
    if (!pos) {
        pos = g_new0(MenuPos, 1);

        g_object_set_qdata_full(G_OBJECT(ifactory->widget),
                                quark_user_menu_pos, pos, g_free);
    }
    pos->x = x;
    pos->y = y;


    if (data != NULL) {
        g_object_set_qdata_full(G_OBJECT(ifactory),
                                quark_popup_data, data, destroy);
        g_signal_connect(G_OBJECT(ifactory->widget),
                         "selection-done",
                         G_CALLBACK(util_menu_delete_popup_data), ifactory);
    }

    gtk_menu_popup(GTK_MENU(ifactory->widget), NULL, NULL,
                   (GtkMenuPositionFunc) util_menu_position,
                   pos, mouse_button, time);
}

void
util_item_factory_popup(GtkItemFactory * ifactory, guint x, guint y,
                        guint mouse_button, guint32 time)
{
    util_item_factory_popup_with_data(ifactory, NULL, NULL, x, y,
                                      mouse_button, time);
}


#define URL_HISTORY_MAX_SIZE 30

static void
util_add_url_callback(GtkWidget * widget,
                      GtkEntry * entry)
{
    const gchar *text;

    text = gtk_entry_get_text(entry);
    if (g_list_find_custom(cfg.url_history, text, (GCompareFunc) strcasecmp))
        return;

    cfg.url_history = g_list_prepend(cfg.url_history, g_strdup(text));

    while (g_list_length(cfg.url_history) > URL_HISTORY_MAX_SIZE) {
        GList *node = g_list_last(cfg.url_history);
        g_free(node->data);
        cfg.url_history = g_list_delete_link(cfg.url_history, node);
    }
}

GtkWidget *
util_add_url_dialog_new(const gchar * caption, GCallback ok_func,
    GCallback enqueue_func)
{
    GtkWidget *win, *vbox, *bbox, *enqueue, *ok, *cancel, *combo, *entry;
    GList *url;

    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), caption);
    gtk_window_set_type_hint(GTK_WINDOW(win), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(win), 400, -1);
    gtk_container_set_border_width(GTK_CONTAINER(win), 12);

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    combo = gtk_combo_box_entry_new_text();
    gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 0);

    entry = gtk_bin_get_child(GTK_BIN(combo));
    gtk_window_set_focus(GTK_WINDOW(win), entry);
    gtk_entry_set_text(GTK_ENTRY(entry), "");

    for (url = cfg.url_history; url; url = g_list_next(url))
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), (const gchar *) url->data);

    g_signal_connect(entry, "activate",
                     G_CALLBACK(util_add_url_callback),
                     entry);
    g_signal_connect(entry, "activate",
                     G_CALLBACK(ok_func),
                     entry);
    g_signal_connect_swapped(entry, "activate",
                             G_CALLBACK(gtk_widget_destroy),
                             win);

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

    ok = gtk_button_new_from_stock(GTK_STOCK_OPEN);
    g_signal_connect(ok, "clicked",
		     G_CALLBACK(util_add_url_callback), entry);
    g_signal_connect(ok, "clicked",
		     G_CALLBACK(ok_func), entry);
    g_signal_connect_swapped(ok, "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             win);
    gtk_box_pack_start(GTK_BOX(bbox), ok, FALSE, FALSE, 0);

    enqueue = gtk_button_new_from_stock(GTK_STOCK_ADD);
    gtk_box_pack_start(GTK_BOX(bbox), enqueue, FALSE, FALSE, 0);

    g_signal_connect(enqueue, "clicked",
                     G_CALLBACK(util_add_url_callback),
                     entry);
    g_signal_connect(enqueue, "clicked",
                     G_CALLBACK(enqueue_func),
                     entry);
    g_signal_connect_swapped(enqueue, "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             win);

    cancel = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_box_pack_start(GTK_BOX(bbox), cancel, FALSE, FALSE, 0);

    g_signal_connect_swapped(cancel, "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             win);

    gtk_widget_show_all(vbox);

    return win;
}

static void
filebrowser_add_files(GtkFileChooser * browser,
                      GSList * files)
{
    GSList *cur;
    gchar *ptr;
    guint ctr = 0;

    if (GTK_IS_WIDGET(mainwin_jtf))
        gtk_widget_set_sensitive(mainwin_jtf, FALSE);

    for (cur = files; cur; cur = g_slist_next(cur)) {

        if (g_file_test(cur->data,G_FILE_TEST_IS_DIR)) {
            playlist_add_dir((const gchar *) cur->data);
        } else {
            playlist_add((const gchar *) cur->data);
        }       

        if (++ctr == 20) {
            playlistwin_update_list();
            ctr = 0;
            while (gtk_events_pending() ) gtk_main_iteration();
        }
    } 

    playlistwin_update_list();

    if (GTK_IS_WIDGET(mainwin_jtf))
        gtk_widget_set_sensitive(mainwin_jtf, TRUE);

#ifdef HAVE_GNOME_VFS
    ptr = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(browser));
#else
    ptr = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(browser));
#endif

    g_free(cfg.filesel_path);
    cfg.filesel_path = ptr;
}

static void
filebrowser_add(GtkFileChooser * browser)
{
    GSList *files;

#ifdef HAVE_GNOME_VFS
    files = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(browser));
#else
    files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(browser));
#endif

    if (!files) {
        return;
    }

    filebrowser_add_files(browser, files);
    g_slist_foreach(files, (GFunc) g_free, NULL);
    g_slist_free(files);
}

static void
filebrowser_play(GtkFileChooser * browser)
{
    GSList *files;

#ifdef HAVE_GNOME_VFS
    files = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(browser));
#else
    files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(browser));
#endif

    if (!files) return;

    playlist_clear();

    filebrowser_add_files(browser, files);
    g_slist_foreach(files, (GFunc) g_free, NULL);
    g_slist_free(files);

    bmp_playback_initiate();
}


static void
_filebrowser_add(GtkWidget *widget,
                 gpointer data)
{
    filebrowser_add(data);
    gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(data));
}

static void
_filebrowser_play(GtkWidget *widget, gpointer data)
{
    filebrowser_play(data);
    gtk_file_chooser_unselect_all(data);
}

#if 0
static void
filebrowser_on_response(GtkFileChooser * browser,
                        gint response,
                        gpointer data)
{
    gtk_widget_hide(GTK_WIDGET(browser));
    switch (response) {
    case GTK_RESPONSE_OK:
        break;
    case GTK_RESPONSE_ACCEPT:
        break;
    case GTK_RESPONSE_CLOSE:
        break;
    }
    gtk_widget_destroy(GTK_WIDGET(browser));

}

#endif

static void
_filebrowser_check_hide_add(GtkWidget * widget,
                            gpointer data)
{
    cfg.close_dialog_add = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void
_filebrowser_check_hide_open(GtkWidget * widget,
                             gpointer data)
{
    cfg.close_dialog_open = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}



static gboolean
filebrowser_on_keypress(GtkWidget * browser,
                        GdkEventKey * event,
                        gpointer data)
{
    if (event->keyval == GDK_Escape) {
        /* FIXME: this crashes BMP for some reason */
        /* g_signal_emit_by_name(browser, "delete-event"); */
        gtk_widget_hide(browser);
        return TRUE;
    }

    return FALSE;
}

static void
_filebrowser_do_hide_add(GtkWidget *widget,
                         gpointer data)
{
    if (cfg.close_dialog_add)
        gtk_widget_hide(data);
}

static void
_filebrowser_do_hide_open(GtkWidget *widget,
                          gpointer data)
{
    if (cfg.close_dialog_open)
        gtk_widget_hide(data);
}

void
util_run_filebrowser(gboolean play_button)
{
    static GladeXML *xml = NULL;
    static GtkWidget *dialog = NULL;
    static GtkWidget *chooser = NULL;
    
    static GtkWidget *button_add;
    static GtkWidget *button_select_all, *button_deselect_all;
    static GtkWidget *toggle;

    static gulong handlerid, handlerid_check, handlerid_do;
    static gulong handlerid_activate, handlerid_do_activate;

    if (!xml) {
        /* FIXME: Creating a file chooser dialog manually using
           libglade because there's no way to stop
           GtkFileChooserDialog from resizing the buttons to the same
           size. The long toggle button title causes the buttons to
           turn unnecessarily elongated and fugly. */

        GtkWidget *alignment;

        xml = glade_xml_new_or_die(_("Add/Open Files dialog"),
                                   DATA_DIR "/glade/addfiles.glade",
                                   NULL, NULL);
        glade_xml_signal_autoconnect(xml);

        dialog = glade_xml_get_widget(xml, "add_files_dialog");

        /* FIXME: Creating file chooser widget here because libglade <= 2.4.0 does
           not support GtkFileChooserWidget */

        chooser = gtk_file_chooser_widget_new(GTK_FILE_CHOOSER_ACTION_OPEN);    
        gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(chooser), TRUE);
        
#ifdef HAVE_GNOME_VFS
        gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(chooser), FALSE);
#endif
        
        if (cfg.filesel_path)
#ifdef HAVE_GNOME_VFS
            gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(chooser),
                                                    cfg.filesel_path);
#else
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser),
                                                cfg.filesel_path);
#endif

        alignment = glade_xml_get_widget(xml, "alignment2");
        gtk_container_add(GTK_CONTAINER(alignment), chooser);

        toggle = glade_xml_get_widget(xml, "close_on_action");
        button_select_all = glade_xml_get_widget(xml, "select_all");
        button_deselect_all = glade_xml_get_widget(xml, "deselect_all");
        button_add = glade_xml_get_widget(xml, "action");

        g_signal_connect_swapped(button_select_all, "clicked", 
                                 G_CALLBACK(gtk_file_chooser_select_all),
                                 chooser);
        g_signal_connect_swapped(button_deselect_all, "clicked",
                                 G_CALLBACK(gtk_file_chooser_unselect_all),
                                 chooser);

        g_signal_connect(dialog, "key_press_event",
                         G_CALLBACK(filebrowser_on_keypress),
                         NULL);

        gtk_widget_show_all(dialog);
    } /* !xml */
    else {
        g_signal_handler_disconnect(button_add, handlerid);
        g_signal_handler_disconnect(toggle, handlerid_check);
        g_signal_handler_disconnect(chooser, handlerid_activate);
        g_signal_handler_disconnect(button_add, handlerid_do);
        g_signal_handler_disconnect(chooser, handlerid_do_activate);
		if (cfg.refresh_file_list) {
            // *sigh* force a refresh
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser),gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chooser)));
		}
    }
    
    if (play_button) {
        gtk_window_set_title(GTK_WINDOW(dialog), _("Open Files"));

        gtk_button_set_label(GTK_BUTTON(button_add), GTK_STOCK_OPEN);

        gtk_button_set_label(GTK_BUTTON(toggle), _("Close dialog on Open"));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), cfg.close_dialog_open);

        handlerid = g_signal_connect(button_add, "clicked", G_CALLBACK(_filebrowser_play), chooser);
        handlerid_check = g_signal_connect(toggle, "toggled", G_CALLBACK(_filebrowser_check_hide_open), NULL);
        handlerid_do = g_signal_connect_after(button_add, "clicked", G_CALLBACK(_filebrowser_do_hide_open), dialog);
        handlerid_activate = g_signal_connect(chooser, "file-activated", G_CALLBACK(_filebrowser_play), chooser);
        handlerid_do_activate = g_signal_connect_after(chooser,"file_activated", G_CALLBACK(_filebrowser_do_hide_open), dialog);
    }
    else {
        gtk_window_set_title(GTK_WINDOW(dialog), _("Add Files"));

        gtk_button_set_label(GTK_BUTTON(button_add), GTK_STOCK_ADD);

        gtk_button_set_label(GTK_BUTTON(toggle), _("Close dialog on Add"));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), cfg.close_dialog_add);

        handlerid = g_signal_connect(button_add, "clicked", G_CALLBACK(_filebrowser_add), chooser);
        handlerid_check = g_signal_connect(toggle, "toggled", G_CALLBACK(_filebrowser_check_hide_add), NULL);
        handlerid_do = g_signal_connect_after(button_add, "clicked", G_CALLBACK(_filebrowser_do_hide_add), dialog);
        handlerid_activate = g_signal_connect(chooser, "file-activated", G_CALLBACK(_filebrowser_add), chooser);
        handlerid_do_activate = g_signal_connect_after(chooser,"file_activated", G_CALLBACK(_filebrowser_do_hide_add), dialog);
    }
    
    gtk_window_present(GTK_WINDOW(dialog));
}

GdkFont *
util_font_load(const gchar * name)
{
    GdkFont *font;
    PangoFontDescription *desc;

    desc = pango_font_description_from_string(name);
    font = gdk_font_from_description(desc);

    return font;
}

#ifdef ENABLE_NLS
gchar *
bmp_menu_translate(const gchar * path, gpointer func_data)
{
    gchar *translation = gettext(path);

    if (!translation || *translation != '/') {
        g_warning("Bad translation for menupath: %s", path);
        translation = (gchar *) path;
    }

    return translation;
}
#endif

void
util_set_cursor(GtkWidget * window)
{
    static GdkCursor *cursor = NULL;

    if (!window) {
        if (cursor) {
            gdk_cursor_unref(cursor);
            cursor = NULL;
        }

        return;
    }

    if (!cursor)
        cursor = gdk_cursor_new(GDK_LEFT_PTR);

    gdk_window_set_cursor(window->window, cursor);
}

/* text_get_extents() taken from The GIMP (C) Spencer Kimball, Peter
 * Mattis et al */
gboolean
text_get_extents(const gchar * fontname,
                 const gchar * text,
                 gint * width, gint * height, gint * ascent, gint * descent)
{
    PangoFontDescription *font_desc;
    PangoLayout *layout;
    PangoRectangle rect;

    g_return_val_if_fail(fontname != NULL, FALSE);
    g_return_val_if_fail(text != NULL, FALSE);

    /* FIXME: resolution */
    layout = gtk_widget_create_pango_layout(GTK_WIDGET(mainwin), text);

    font_desc = pango_font_description_from_string(fontname);
    pango_layout_set_font_description(layout, font_desc);
    pango_font_description_free(font_desc);
    pango_layout_get_pixel_extents(layout, NULL, &rect);

    if (width)
        *width = rect.width;
    if (height)
        *height = rect.height;

    if (ascent || descent) {
        PangoLayoutIter *iter;
        PangoLayoutLine *line;

        iter = pango_layout_get_iter(layout);
        line = pango_layout_iter_get_line(iter);
        pango_layout_iter_free(iter);

        pango_layout_line_get_pixel_extents(line, NULL, &rect);

        if (ascent)
            *ascent = PANGO_ASCENT(rect);
        if (descent)
            *descent = -PANGO_DESCENT(rect);
    }

    g_object_unref(layout);

    return TRUE;
}

/* counts number of digits in a gint */
guint
gint_count_digits(gint n)
{
    guint count = 0;

    n = ABS(n);
    do {
        count++;
        n /= 10;
    } while (n > 0);

    return count;
}

static gchar *
str_twenty_to_space(gchar * str)
{
    gchar *match, *match_end;

    g_return_val_if_fail(str != NULL, NULL);

    while ((match = strstr(str, "%20"))) {
        match_end = match + 3;
        *match++ = ' ';
        while (*match_end)
            *match++ = *match_end++;
        *match = 0;
    }

    return str;
}

static gchar *
str_replace_char(gchar * str, gchar old, gchar new)
{
    gchar *match;

    g_return_val_if_fail(str != NULL, NULL);

    match = str;
    while ((match = strchr(match, old)))
        *match = new;

    return str;
}

gchar *
str_append(gchar * str, const gchar * add_str)
{
    return str_replace(str, g_strconcat(str, add_str, NULL));
}

gchar *
str_replace(gchar * str, gchar * new_str)
{
    g_free(str);
    return new_str;
}

void
str_replace_in(gchar ** str, gchar * new_str)
{
    *str = str_replace(*str, new_str);
}


gboolean
str_has_prefix_nocase(const gchar * str, const gchar * prefix)
{
    return (strncasecmp(str, prefix, strlen(prefix)) == 0);
}

gboolean
str_has_suffix_nocase(const gchar * str, const gchar * suffix)
{
    return (strcasecmp(str + strlen(str) - strlen(suffix), suffix) == 0);
}

gboolean
str_has_suffixes_nocase(const gchar * str, gchar * const *suffixes)
{
    gchar *const *suffix;

    g_return_val_if_fail(str != NULL, FALSE);
    g_return_val_if_fail(suffixes != NULL, FALSE);

    for (suffix = suffixes; *suffix; suffix++)
        if (str_has_suffix_nocase(str, *suffix))
            return TRUE;

    return FALSE;
}

gchar *
str_to_utf8_fallback(const gchar * str)
{
    gchar *out_str, *convert_str, *chr;

    /* NULL in NULL out */
    if (!str)
        return NULL;

    convert_str = g_strdup(str);
    for (chr = convert_str; *chr; chr++) {
        if (*chr & 0x80)
            *chr = '?';
    }

    out_str = g_strconcat(convert_str, _("  (invalid UTF-8)"), NULL);
    g_free(convert_str);

    return out_str;
}

gchar *
filename_to_utf8(const gchar * filename)
{
    gchar *out_str;

    /* NULL in NULL out */
    if (!filename)
        return NULL;

    if ((out_str = g_filename_to_utf8(filename, -1, NULL, NULL, NULL)))
        return out_str;

    return str_to_utf8_fallback(filename);
}

gchar *
str_to_utf8(const gchar * str)
{
    gchar *out_str;

    /* NULL in NULL out */
    if (!str)
        return NULL;

    /* Note: Currently, playlist calls this function repeatedly, even
     * if the string is already converted into utf-8.
     * chardet_to_utf8() would convert a valid utf-8 string into a
     * different utf-8 string, if fallback encodings were supplied and
     * the given string could be treated as a string in one of fallback
     * encodings. To avoid this, the order of evaluation has been
     * changed. (It might cause a drawback?)
     */
    /* already UTF-8? */
    if (g_utf8_validate(str, -1, NULL))
        return g_strdup(str);

    /* chardet encoding detector */
    if ((out_str = chardet_to_utf8(str, strlen(str), NULL, NULL, NULL)))
        return out_str;

    /* assume encoding associated with locale */
    if ((out_str = g_locale_to_utf8(str, -1, NULL, NULL, NULL)))
        return out_str;

    /* all else fails, we mask off character codes >= 128,
       replace with '?' */
    return str_to_utf8_fallback(str);
}


const gchar *
str_skip_chars(const gchar * str, const gchar * chars)
{
    while (strchr(chars, *str))
        str++;
    return str;
}

gchar *
convert_title_text(gchar * title)
{
    g_return_val_if_fail(title != NULL, NULL);

    if (cfg.convert_underscore)
        str_replace_char(title, '_', ' ');

    if (cfg.convert_twenty)
        str_twenty_to_space(title);

    return title;
}


gboolean
dir_foreach(const gchar * path, DirForeachFunc function,
            gpointer user_data, GError ** error)
{
    GError *error_out = NULL;
    GDir *dir;
    const gchar *entry;
    gchar *entry_fullpath;

    if (!(dir = g_dir_open(path, 0, &error_out))) {
        g_propagate_error(error, error_out);
        return FALSE;
    }

    while ((entry = g_dir_read_name(dir))) {
        entry_fullpath = g_build_filename(path, entry, NULL);

        if ((*function) (entry_fullpath, entry, user_data)) {
            g_free(entry_fullpath);
            break;
        }

        g_free(entry_fullpath);
    }

    g_dir_close(dir);

    return TRUE;
}

GtkWidget *
make_filebrowser(const gchar * title,
                 gboolean save)
{
    GtkWidget *dialog;
    GtkWidget *button;
    GtkWidget *button_close;

    g_return_val_if_fail(title != NULL, NULL);

    dialog = gtk_file_chooser_dialog_new(title, GTK_WINDOW(mainwin),
                                         GTK_FILE_CHOOSER_ACTION_OPEN, NULL, NULL);
    if (save)
        gtk_file_chooser_set_action(GTK_FILE_CHOOSER(dialog),
                                    GTK_FILE_CHOOSER_ACTION_SAVE);

    if (!save)
        gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

    g_signal_connect(dialog, "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &dialog);

#ifdef HAVE_GNOME_VFS
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), FALSE);
#endif

    button_close = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_REJECT);
    gtk_button_set_use_stock(GTK_BUTTON(button_close), TRUE);
    GTK_WIDGET_SET_FLAGS(button_close, GTK_CAN_DEFAULT);
    g_signal_connect_swapped(button_close, "clicked",
                             G_CALLBACK(gtk_widget_destroy), dialog);

    button = gtk_dialog_add_button(GTK_DIALOG(dialog), save ?
                                   GTK_STOCK_SAVE : GTK_STOCK_OPEN,
                                   GTK_RESPONSE_ACCEPT);
    gtk_button_set_use_stock(GTK_BUTTON(button), TRUE);
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_window_set_default(GTK_WINDOW(dialog), button);

    gtk_widget_show(dialog);

    return dialog;
}


GtkItemFactory *
create_menu(GtkItemFactoryEntry *entries,
            guint n_entries,
            GtkAccelGroup *accel)
{
    GtkItemFactory *menu;

    menu = gtk_item_factory_new(GTK_TYPE_MENU, "<Main>", accel);
    gtk_item_factory_set_translate_func(menu, bmp_menu_translate, NULL,
                                        NULL);
    gtk_item_factory_create_items(menu, n_entries, entries, NULL);

    return menu;
}


void
make_submenu(GtkItemFactory *menu,
             const gchar *item_path,
             GtkItemFactory *submenu)
{
    GtkWidget *item, *menu_;

    item = gtk_item_factory_get_widget(menu, item_path);
    menu_ = gtk_item_factory_get_widget(submenu, "");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu_);
}


gchar *chardet_to_utf8(const gchar *str, gssize len,
                       gsize *arg_bytes_read, gsize *arg_bytes_write, GError **arg_error)
{
#ifdef USE_CHARDET
	char  *det = NULL, *encoding = NULL;
#endif
	gchar *ret = NULL;
	gsize *bytes_read, *bytes_write;
	GError **error;
	gsize my_bytes_read, my_bytes_write;

	bytes_read  = arg_bytes_read ? arg_bytes_read : &my_bytes_read;
	bytes_write = arg_bytes_write ? arg_bytes_write : &my_bytes_write;
	error       = arg_error ? arg_error : NULL;

#ifdef USE_CHARDET
	if(cfg.chardet_detector)
		det = cfg.chardet_detector;

	if(det){
		if(!strncasecmp("japanese", det, sizeof("japanese"))) {
			encoding = (char *)guess_jp(str, strlen(str));
			if (!encoding)
				goto fallback;
		} else if(!strncasecmp("taiwanese", det, sizeof("taiwanese"))) {
			encoding = (char *)guess_tw(str, strlen(str));
			if (!encoding)
				goto fallback;
		} else if(!strncasecmp("chinese", det, sizeof("chinese"))) {
			encoding = (char *)guess_cn(str, strlen(str));
			if (!encoding)
				goto fallback;
		} else if(!strncasecmp("korean", det, sizeof("korean"))) {
			encoding = (char *)guess_kr(str, strlen(str));
			if (!encoding)
				goto fallback;
#ifdef HAVE_UDET
		} else if (!strncasecmp("universal", det, sizeof("universal"))) {
			encoding = (char *)detectCharset((char *)str, strlen(str));
			if (!encoding)
				goto fallback;
#endif
		} else /* none, invalid */
			goto fallback;

		ret = g_convert(str, len, "UTF-8", encoding, bytes_read, bytes_write, error);
	}

fallback:
#endif
	if(!ret && cfg.chardet_fallback){
		gchar **encs=NULL, **enc=NULL;
		encs = g_strsplit_set(cfg.chardet_fallback, " ,:;|/", 0);

		if(encs){
			enc = encs;
			for(enc=encs; *enc ; enc++){
				ret = g_convert(str, len, "UTF-8", *enc, bytes_read, bytes_write, error);
				if(len == *bytes_read){
					break;
				}
			}
			g_strfreev(encs);
		}
	}

#ifdef USE_CHARDET
	/* many tag libraries return 2byte latin1 utf8 character as
	   converted 8bit iso-8859-1 character, if they are asked to return
	   latin1 string.
	 */
	if(!ret){
		ret = g_convert(str, len, "UTF-8", "ISO-8859-1", bytes_read, bytes_write, error);
	}
#endif

	if(ret){
		if(g_utf8_validate(ret, -1, NULL))
			return ret;
		else {
			g_free(ret);
			ret = NULL;
		}
	}
	
	return NULL;	/* if I have no idea, return NULL. */
}
