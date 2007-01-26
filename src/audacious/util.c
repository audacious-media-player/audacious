/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 2 of the License.
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

#define NEED_GLADE
#include "util.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "platform/smartinclude.h"
#include <errno.h>

#ifdef HAVE_FTS_H
#  include <fts.h>
#endif

#include "glade.h"
#include "input.h"
#include "main.h"
#include "playback.h"
#include "strings.h"
#include "ui_playlist.h"

#ifdef USE_CHARDET
    #include "../librcd/librcd.h"
#ifdef HAVE_UDET
    #include <libudet_c.h>
#endif
#endif

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
#ifndef HAVE_MKDTEMP
    mode_t mode755 = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
#endif

    if ((type = archive_get_type(filename)) <= ARCHIVE_DIR)
        return NULL;

#ifdef HAVE_MKDTEMP
    tmpdir = g_build_filename(g_get_tmp_dir(), "audacious.XXXXXXXX", NULL);
    if (!mkdtemp(tmpdir)) {
        g_free(tmpdir);
        g_message("Unable to load skin: Failed to create temporary "
                  "directory: %s", g_strerror(errno));
        return NULL;
    }
#else
    tmpdir = g_strdup_printf("%s/audacious.%ld", g_get_tmp_dir(), rand());
    make_directory(tmpdir, mode755);
#endif

    escaped_filename = escape_shell_chars(filename);
    cmd = archive_extract_funcs[type] (escaped_filename, tmpdir);
    g_free(escaped_filename);

    if (!cmd) {
        g_message("extraction function is NULL!");
        g_free(tmpdir);
        return NULL;
    }

    if(system(cmd) == -1)
    {
        g_message("could not execute cmd %s",cmd);
        g_free(cmd);
        return NULL;
    }
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
        outbuf = g_malloc (filesize);   /* it's safe to waste memory. */

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
        return NULL;    /* XXX wrong encoding */
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
        while (!isdigit((int) *ptr) && (*ptr) != '\0')
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
    GtkWidget *win, *vbox, *bbox, *enqueue, *ok, *cancel, *combo, *entry, 
          *label;
    GList *url;

    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), _("Add/Open URL Dialog"));
    gtk_window_set_type_hint(GTK_WINDOW(win), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(win), 400, -1);
    gtk_container_set_border_width(GTK_CONTAINER(win), 12);

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    label = gtk_label_new(caption);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

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

/*
 * Resizes a GDK pixmap.
 */
GdkPixmap *audacious_pixmap_resize(GdkWindow *src, GdkGC *src_gc, GdkPixmap *in, gint width, gint height)
{
    GdkPixmap *out;
    gint owidth, oheight;

    g_return_val_if_fail(src != NULL, NULL);
    g_return_val_if_fail(src_gc != NULL, NULL);
    g_return_val_if_fail(in != NULL, NULL);
    g_return_val_if_fail(width > 0 && height > 0, NULL);

    gdk_drawable_get_size(in, &owidth, &oheight);

    if (oheight == height && owidth == width)
        return NULL;

    out = gdk_pixmap_new(src, width, height, -1);

    gdk_draw_rectangle(out, src_gc, TRUE, 0, 0, width, height);

    gdk_window_copy_area(out, src_gc, 0, 0, in, 0, 0, owidth, oheight);
    g_object_unref(src);

    return out;
}

GdkImage *create_dblsize_image(GdkImage * img)
{
    GdkImage *dblimg;
    register guint x, y;

    /*
     * This needs to be optimized
     */

    dblimg =
    gdk_image_new(GDK_IMAGE_NORMAL, gdk_visual_get_system(),
              img->width << 1, img->height << 1);
    if (dblimg->bpp == 1) {
    register guint8 *srcptr, *ptr, *ptr2, pix;

    srcptr = GDK_IMAGE(img)->mem;
    ptr = GDK_IMAGE(dblimg)->mem;
    ptr2 = ptr + dblimg->bpl;

    for (y = 0; y < img->height; y++) {
        for (x = 0; x < img->width; x++) {
        pix = *srcptr++;
        *ptr++ = pix;
        *ptr++ = pix;
        *ptr2++ = pix;
        *ptr2++ = pix;
        }
        srcptr += img->bpl - img->width;
        ptr += (dblimg->bpl << 1) - dblimg->width;
        ptr2 += (dblimg->bpl << 1) - dblimg->width;
    }
    }
    if (dblimg->bpp == 2) {
    guint16 *srcptr, *ptr, *ptr2, pix;

    srcptr = (guint16 *) GDK_IMAGE_XIMAGE(img)->data;
    ptr = (guint16 *) GDK_IMAGE_XIMAGE(dblimg)->data;
    ptr2 = ptr + (dblimg->bpl >> 1);

    for (y = 0; y < img->height; y++) {
        for (x = 0; x < img->width; x++) {
        pix = *srcptr++;
        *ptr++ = pix;
        *ptr++ = pix;
        *ptr2++ = pix;
        *ptr2++ = pix;
        }
        srcptr += (img->bpl >> 1) - img->width;
        ptr += (dblimg->bpl) - dblimg->width;
        ptr2 += (dblimg->bpl) - dblimg->width;
    }
    }
    if (dblimg->bpp == 3) {
    register guint8 *srcptr, *ptr, *ptr2, pix1, pix2, pix3;

    srcptr = GDK_IMAGE(img)->mem;
    ptr = GDK_IMAGE(dblimg)->mem;
    ptr2 = ptr + dblimg->bpl;

    for (y = 0; y < img->height; y++) {
        for (x = 0; x < img->width; x++) {
        pix1 = *srcptr++;
        pix2 = *srcptr++;
        pix3 = *srcptr++;
        *ptr++ = pix1;
        *ptr++ = pix2;
        *ptr++ = pix3;
        *ptr++ = pix1;
        *ptr++ = pix2;
        *ptr++ = pix3;
        *ptr2++ = pix1;
        *ptr2++ = pix2;
        *ptr2++ = pix3;
        *ptr2++ = pix1;
        *ptr2++ = pix2;
        *ptr2++ = pix3;

        }
        srcptr += img->bpl - (img->width * 3);
        ptr += (dblimg->bpl << 1) - (dblimg->width * 3);
        ptr2 += (dblimg->bpl << 1) - (dblimg->width * 3);
    }
    }
    if (dblimg->bpp == 4) {
    register guint32 *srcptr, *ptr, *ptr2, pix;

    srcptr = (guint32 *) GDK_IMAGE(img)->mem;
    ptr = (guint32 *) GDK_IMAGE(dblimg)->mem;
    ptr2 = ptr + (dblimg->bpl >> 2);

    for (y = 0; y < img->height; y++) {
        for (x = 0; x < img->width; x++) {
        pix = *srcptr++;
        *ptr++ = pix;
        *ptr++ = pix;
        *ptr2++ = pix;
        *ptr2++ = pix;
        }
        srcptr += (img->bpl >> 2) - img->width;
        ptr += (dblimg->bpl >> 1) - dblimg->width;
        ptr2 += (dblimg->bpl >> 1) - dblimg->width;
    }
    }
    return dblimg;
}

/* URL-decode a file: URL path, return NULL if it's not what we expect */
gchar *
xmms_urldecode_path(const gchar * encoded_path)
{
    const gchar *cur, *ext;
    gchar *path, *tmp;
    gint realchar;

    if (!encoded_path)
        return NULL;

    if (!str_has_prefix_nocase(encoded_path, "file:"))
        return NULL;

    cur = encoded_path + 5;

    if (str_has_prefix_nocase(cur, "//localhost"))
        cur += 11;

    if (*cur == '/')
        while (cur[1] == '/')
            cur++;

    tmp = g_malloc0(strlen(cur) + 1);

    while ((ext = strchr(cur, '%')) != NULL) {
        strncat(tmp, cur, ext - cur);
        ext++;
        cur = ext + 2;
        if (!sscanf(ext, "%2x", &realchar)) {
            /* Assume it is a literal '%'.  Several file
             * managers send unencoded file: urls on drag
             * and drop. */
            realchar = '%';
            cur -= 2;
        }
        tmp[strlen(tmp)] = realchar;
    }

    path = g_strconcat(tmp, cur, NULL);
    g_free(tmp);
    return path;
}

