/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2008  Audacious development team
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "util.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "platform/smartinclude.h"
#include <errno.h>

#ifdef HAVE_FTS_H
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <fts.h>
#endif

#include "input.h"
#include "playback.h"
#include "audstrings.h"

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
        AUDDBG("Ignoring path: name too long (%s)\n", path);
        return TRUE;
    }

    if (vfs_file_test(path, G_FILE_TEST_IS_REGULAR)) {
        if (!strcasecmp(basename, context->to_match)) {
            context->match = g_strdup(path);
            context->found = TRUE;
            return TRUE;
        }
    }
    else if (vfs_file_test(path, G_FILE_TEST_IS_DIR)) {
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
    gchar *out = NULL;

    context.to_match = filename;
    context.match = NULL;
    context.found = FALSE;

    dir_foreach(path, find_file_func, &context, NULL);

    if (context.match)
    {
        out = g_filename_to_uri(context.match, NULL, NULL);
        g_free(context.match);
    }

    return out;
}

gchar *
find_path_recursively(const gchar * path, const gchar * filename)
{
    FindFileContext context;

    context.to_match = filename;
    context.match = NULL;
    context.found = FALSE;

    dir_foreach(path, find_file_func, &context, NULL);

    return context.match;
}

static void
strip_string(GString *string)
{
    while (string->len > 0 && string->str[0] == ' ')
        g_string_erase(string, 0, 1);

    while (string->len > 0 && string->str[string->len - 1] == ' ')
        g_string_erase(string, string->len - 1, 1);
}

static void
strip_lower_string(GString *string)
{
    gchar *lower;
    strip_string(string);

    lower = g_ascii_strdown(string->str, -1);
    g_free(string->str);
    string->str = lower;
}

static void
close_ini_file_free_value(gpointer value)
{
    g_free((gchar*)value);
}

static void
close_ini_file_free_section(gpointer section)
{
    g_hash_table_destroy((GHashTable*)section);
}

INIFile *
open_ini_file(const gchar *filename)
{
    GHashTable *ini_file = NULL;
    GHashTable *section = NULL;
    GString *section_name, *key_name, *value;
    gpointer section_hash, key_hash;
    gchar *buffer = NULL;
    gsize off = 0;
    gsize filesize = 0;

    unsigned char x[] = { 0xff, 0xfe, 0x00 };

    g_return_val_if_fail(filename, NULL);
    vfs_file_get_contents(filename, &buffer, &filesize);
    if (buffer == NULL)
        return NULL;

    /*
     * Convert UTF-16 into something useful. Original implementation
     * by incomp@#audacious. Cleanups \nenolod
     * FIXME: can't we use a GLib function for that? -- 01mf02
     */
    if (filesize > 2 && !memcmp(&buffer[0],&x,2))
    {
        gchar *outbuf = g_malloc (filesize);   /* it's safe to waste memory. */
        guint counter;

        for (counter = 2; counter < filesize; counter += 2)
        {
            if (!memcmp(&buffer[counter+1], &x[2], 1)) {
                outbuf[(counter-2)/2] = buffer[counter];
            } else {
                g_free(buffer);
                g_free(outbuf);
                return NULL;
            }
        }

        outbuf[(counter-2)/2] = '\0';

        if ((filesize - 2) / 2 == (counter - 2) / 2)
        {
            g_free(buffer);
            buffer = outbuf;
        }
        else
        {
            g_free(buffer);
            g_free(outbuf);
            return NULL;    /* XXX wrong encoding */
        }
    }

    section_name = g_string_new("");
    key_name = g_string_new(NULL);
    value = g_string_new(NULL);

    ini_file = g_hash_table_new_full(NULL, NULL, NULL,
                                     close_ini_file_free_section);
    section = g_hash_table_new_full(NULL, NULL, NULL,
                                    close_ini_file_free_value);
    /* make a nameless section which should store all entries that are not
     * embedded in a section */
    section_hash = GINT_TO_POINTER(g_string_hash(section_name));
    g_hash_table_insert(ini_file, section_hash, section);

    while (off < filesize)
    {
        /* ignore the following characters */
        if (buffer[off] == '\r' || buffer[off] == '\n' ||
            buffer[off] == ' '  || buffer[off] == '\t')
        {
            if (buffer[off] == '\n')
            {
                g_string_free(key_name, TRUE);
                g_string_free(value, TRUE);
                key_name = g_string_new(NULL);
                value = g_string_new(NULL);
            }

            off++;
            continue;
        }

        /* if we encounter a possible section statement */
        if (buffer[off] == '[')
        {
            g_string_free(section_name, TRUE);
            section_name = g_string_new(NULL);
            off++;

            if (off >= filesize)
                goto return_sequence;

            while (buffer[off] != ']')
            {
                /* if the section statement has not been closed before a
                 * linebreak */
                if (buffer[off] == '\n')
                    break;

                g_string_append_c(section_name, buffer[off]);
                off++;
                if (off >= filesize)
                    goto return_sequence;
            }
            if (buffer[off] == '\n')
                continue;
            if (buffer[off] == ']')
            {
                off++;
                if (off >= filesize)
                    goto return_sequence;

                strip_lower_string(section_name);
                section_hash = GINT_TO_POINTER(g_string_hash(section_name));

                /* if this section already exists, we don't make a new one,
                 * but reuse the old one */
                if (g_hash_table_lookup(ini_file, section_hash) != NULL)
                    section = g_hash_table_lookup(ini_file, section_hash);
                else
                {
                    section = g_hash_table_new_full(NULL, NULL, NULL,
                                                    close_ini_file_free_value);
                    g_hash_table_insert(ini_file, section_hash, section);
                }

                continue;
            }
        }

        if (buffer[off] == '=')
        {
            off++;
            if (off >= filesize)
                goto return_sequence;

            while (buffer[off] != '\n' && buffer[off] != '\r')
            {
                g_string_append_c(value, buffer[off]);
                off++;
                if (off >= filesize)
                    break;
            }

            strip_lower_string(key_name);
            key_hash = GINT_TO_POINTER(g_string_hash(key_name));
            strip_string(value);

            if (key_name->len > 0 && value->len > 0)
                g_hash_table_insert(section, key_hash, g_strdup(value->str));
        }
        else
        {
            g_string_append_c(key_name, buffer[off]);
            off++;
            if (off >= filesize)
                goto return_sequence;
        }
    }

return_sequence:
    g_string_free(section_name, TRUE);
    g_string_free(key_name, TRUE);
    g_string_free(value, TRUE);
    g_free(buffer);
    return ini_file;
}

/**
 * Frees the memory allocated for inifile.
 */
void
close_ini_file(INIFile *inifile)
{
    g_return_if_fail(inifile);
    g_hash_table_destroy(inifile);
}

/**
 * Returns a string that corresponds to correct section and key in inifile.
 *
 * Returns NULL if value was not found in inifile. Otherwise returns a copy
 * of string pointed by "section" and "key". Returned string should be freed
 * after use.
 */
gchar *
read_ini_string(INIFile *inifile, const gchar *section, const gchar *key)
{
    GString *section_string;
    GString *key_string;
    gchar *value = NULL;
    gpointer section_hash, key_hash;
    GHashTable *section_table;

    g_return_val_if_fail(inifile, NULL);

    section_string = g_string_new(section);
    key_string = g_string_new(key);
    value = NULL;

    strip_lower_string(section_string);
    strip_lower_string(key_string);
    section_hash = GINT_TO_POINTER(g_string_hash(section_string));
    key_hash = GINT_TO_POINTER(g_string_hash(key_string));
    section_table = g_hash_table_lookup(inifile, section_hash);

    if (section_table) {
        value = g_strdup(g_hash_table_lookup(section_table,
                                             GINT_TO_POINTER(key_hash)));
    }

    g_string_free(section_string, TRUE);
    g_string_free(key_string, TRUE);

    return value;
}

GArray *
read_ini_array(INIFile *inifile, const gchar *section, const gchar *key)
{
    gchar *temp;
    GArray *a;

    if ((temp = read_ini_string(inifile, section, key)) == NULL)
        return NULL;

    a = string_to_garray(temp);
    g_free(temp);
    return a;
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

/**
 * util_info_dialog:
 * @title: The title of the message to show.
 * @text: The text of the message to show.
 * @button_text: The text of the button which will close the messagebox.
 * @modal: Whether or not the messagebox should be modal.
 * @button_action: Code to execute on when the messagebox is closed, or %NULL.
 * @action_data: Optional opaque data to pass to @button_action.
 *
 * Displays a message box.
 *
 * Return value: A GTK widget handle for the message box.
 **/
GtkWidget *
util_info_dialog(const gchar * title, const gchar * text,
                 const gchar * button_text, gboolean modal,
                 GCallback button_action, gpointer action_data)
{
  GtkWidget *dialog;
  GtkWidget *dialog_vbox, *dialog_hbox, *dialog_bbox;
  GtkWidget *dialog_bbox_b1;
  GtkWidget *dialog_textlabel;
  GtkWidget *dialog_icon;

  dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_type_hint( GTK_WINDOW(dialog) , GDK_WINDOW_TYPE_HINT_DIALOG );
  gtk_window_set_modal( GTK_WINDOW(dialog) , modal );
  gtk_window_set_title( GTK_WINDOW(dialog) , title );
  gtk_container_set_border_width( GTK_CONTAINER(dialog) , 10 );

  dialog_vbox = gtk_vbox_new( FALSE , 0 );
  dialog_hbox = gtk_hbox_new( FALSE , 0 );

  /* icon */
  dialog_icon = gtk_image_new_from_stock( GTK_STOCK_DIALOG_INFO , GTK_ICON_SIZE_DIALOG );
  gtk_box_pack_start( GTK_BOX(dialog_hbox) , dialog_icon , FALSE , FALSE , 2 );

  /* label */
  dialog_textlabel = gtk_label_new( text );
  /* gtk_label_set_selectable( GTK_LABEL(dialog_textlabel) , TRUE ); */
  gtk_box_pack_start( GTK_BOX(dialog_hbox) , dialog_textlabel , TRUE , TRUE , 2 );

  gtk_box_pack_start( GTK_BOX(dialog_vbox) , dialog_hbox , FALSE , FALSE , 2 );
  gtk_box_pack_start( GTK_BOX(dialog_vbox) , gtk_hseparator_new() , FALSE , FALSE , 4 );

  dialog_bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout( GTK_BUTTON_BOX(dialog_bbox) , GTK_BUTTONBOX_END );
  dialog_bbox_b1 = gtk_button_new_with_label( button_text );
  g_signal_connect_swapped( G_OBJECT(dialog_bbox_b1) , "clicked" ,
                            G_CALLBACK(gtk_widget_destroy) , dialog );
  if ( button_action )
    g_signal_connect( G_OBJECT(dialog_bbox_b1) , "clicked" ,
                      button_action , action_data );

  gtk_container_add( GTK_CONTAINER(dialog_bbox) , dialog_bbox_b1 );
  gtk_box_pack_start( GTK_BOX(dialog_vbox) , dialog_bbox , FALSE , FALSE , 0 );

  gtk_container_add( GTK_CONTAINER(dialog) , dialog_vbox );

  GTK_WIDGET_SET_FLAGS( dialog_bbox_b1 , GTK_CAN_DEFAULT);
  gtk_widget_grab_default( dialog_bbox_b1 );

  gtk_widget_show_all(dialog);

  return dialog;
}


/**
 * util_get_localdir:
 *
 * Returns a string with the full path of Audacious local datadir (where config files are placed).
 * It's useful in order to put in the right place custom config files for audacious plugins.
 *
 * Return value: a string with full path of Audacious local datadir (should be freed after use)
 **/
gchar*
util_get_localdir(void)
{
  gchar *datadir;
  gchar *tmp;

  if ( (tmp = getenv("XDG_CONFIG_HOME")) == NULL )
    datadir = g_build_filename( g_get_home_dir() , ".config" , "audacious" ,  NULL );
  else
    datadir = g_build_filename( tmp , "audacious" , NULL );

  return datadir;
}


gchar *
construct_uri(gchar *string, const gchar *playlist_name) // uri, path and anything else
{
    gchar *filename = g_strdup(string);
    gchar *tmp, *path;
    gchar *uri = NULL;

    /* try to translate dos path */
    convert_dos_path(filename); /* in place replacement */

    // make full path uri here
    // case 1: filename is raw full path or uri
    if (filename[0] == '/' || strstr(filename, "://")) {
        uri = g_filename_to_uri(filename, NULL, NULL);
        if(!uri)
            uri = g_strdup(filename);
        g_free(filename);
    }
    // case 2: filename is not raw full path nor uri, playlist path is full path
    // make full path by replacing last part of playlist path with filename. (using g_build_filename)
    else if (playlist_name[0] == '/' || strstr(playlist_name, "://")) {
        path = g_filename_from_uri(playlist_name, NULL, NULL);
        if (!path)
            path = g_strdup(playlist_name);
        tmp = strrchr(path, '/'); *tmp = '\0';
        tmp = g_build_filename(path, filename, NULL);
        g_free(path); g_free(filename);
        uri = g_filename_to_uri(tmp, NULL, NULL);
        g_free(tmp);
    }
    // case 3: filename is not raw full path nor uri, playlist path is not full path
    // just abort.
    else {
        g_free(filename);
        uri = NULL;
    }

    AUDDBG("uri=%s\n", uri);
    return uri;
}

/*
 * minimize number of realloc's:
 *  - set N to nearest power of 2 not less then N
 *  - double it
 *
 *  -- asphyx
 *
 * XXX: what's so smart about this?? seems wasteful and silly. --nenolod
 */
gpointer
smart_realloc(gpointer ptr, gsize *size)
{
    *size = (gsize)pow(2, ceil(log(*size) / log(2)) + 1);
    return g_realloc(ptr, *size);
}

/* local files -- not URI's */
gint file_get_mtime (const gchar * filename)
{
    struct stat info;

    if (stat (filename, & info))
        return -1;

    return info.st_mtime;
}

void
make_directory(const gchar * path, mode_t mode)
{
    if (g_mkdir_with_parents(path, mode) == 0)
        return;

    g_printerr(_("Could not create directory (%s): %s\n"), path,
               g_strerror(errno));
}

#define URL_HISTORY_MAX_SIZE 30

void
util_add_url_history_entry(const gchar * url)
{
    if (g_list_find_custom(cfg.url_history, url, (GCompareFunc) strcasecmp))
        return;

    cfg.url_history = g_list_prepend(cfg.url_history, g_strdup(url));

    while (g_list_length(cfg.url_history) > URL_HISTORY_MAX_SIZE) {
        GList *node = g_list_last(cfg.url_history);
        g_free(node->data);
        cfg.url_history = g_list_delete_link(cfg.url_history, node);
    }
}
