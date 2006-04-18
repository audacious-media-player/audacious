/*
 *  cddb.c  Copyright 1999-2001 Håvard Kvålen <havardk@xmms.org>
 *
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


#include "cddb.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <stdarg.h>

#include <libaudacious/util.h>

#include "http.h"
#include "cdaudio.h"
#include "cdinfo.h"


static guint32 cached_id = 0;
static GtkWidget *server_dialog, *server_clist;
static GtkWidget *debug_window, *debug_clist;
static GList *debug_messages = NULL;
static GList *temp_messages = NULL;
static guint cddb_timeout_id;

G_LOCK_DEFINE_STATIC(list);

void configure_set_cddb_server(gchar * server);

static void
cddb_log(gchar * str, ...)
{
    static GList *end_ptr = NULL;
    static gint message_num = 0;
    va_list args;
    gchar *text;

    va_start(args, str);
    text = g_strdup_vprintf(str, args);
    va_end(args);

    message_num++;
    debug_messages = g_list_prepend(debug_messages, text);
    if (!end_ptr)
        end_ptr = debug_messages;
    if (message_num > CDDB_LOG_MAX) {
        GList *temp;

        temp = g_list_previous(end_ptr);
        temp->next = NULL;
        g_free(end_ptr->data);
        g_list_free_1(end_ptr);
        end_ptr = temp;
        message_num--;
    }
    if (debug_window) {
        G_LOCK(list);
        temp_messages = g_list_append(temp_messages, g_strdup(text));
        G_UNLOCK(list);
    }
}

static gint
cddb_sum(gint in)
{
    gint retval = 0;

    while (in > 0) {
        retval += in % 10;
        in /= 10;
    }
    return retval;
}

guint32
cdda_cddb_compute_discid(cdda_disc_toc_t * info)
{
    gint i;
    guint high = 0, low;

    for (i = info->first_track; i <= info->last_track; i++)
        high += cddb_sum(info->track[i].minute * 60 + info->track[i].second);

    low = (info->leadout.minute * 60 + info->leadout.second) -
        (info->track[info->first_track].minute * 60 +
         info->track[info->first_track].second);

    return ((high % 0xff) << 24 | low << 8 | (info->last_track -
                                              info->first_track + 1));
}

static gchar *
cddb_generate_offset_string(cdda_disc_toc_t * info)
{
    gchar *buffer;
    int i;

    buffer = g_malloc(info->last_track * 7 + 1);

    sprintf(buffer, "%d", LBA(info->track[info->first_track]));

    for (i = info->first_track + 1; i <= info->last_track; i++)
        sprintf(buffer, "%s+%d", buffer, LBA(info->track[i]));

    return buffer;
}

static gchar *
cddb_generate_hello_string(void)
{
    static gchar *buffer;

    if (buffer == NULL) {
        gchar *env, *client = NULL, *version = NULL, **strs = NULL;

        env = getenv("XMMS_CDDB_CLIENT_NAME");
        if (env) {
            strs = g_strsplit(env, " ", 2);
            if (strs && strs[0] && strs[1]) {
                client = strs[0];
                version = strs[1];
            }
        }

        if (!client || !version) {
            client = PACKAGE_NAME;
            version = PACKAGE_VERSION;
        }

        buffer = g_strdup_printf("&hello=nobody+localhost+%s+%s",
                                 client, version);
        if (strs)
            g_strfreev(strs);
    }
    return buffer;
}

static gint
cddb_http_open_connection(const gchar * server, gint port)
{
    gint sock;
    gchar *status;

    if ((sock = http_open_connection(server, 80)) == 0)
        status = "Failed";
    else
        status = "Ok";

    cddb_log("Connecting to CDDB-server %s: %s", server, status);
    return sock;
}


static gboolean
cddb_query(gchar * server, cdda_disc_toc_t * info,
           cddb_disc_header_t * cddb_info)
{
    /*
     * Query the cddb-server for the cd.
     * Returns the *real* diskid and category.
     */

    gint sock;
    gchar *offsets, *getstr;
    gchar buffer[256];
    gchar **response;
    gint i;

    if ((sock = cddb_http_open_connection(server, 80)) == 0)
        return FALSE;

    offsets = cddb_generate_offset_string(info);

    cddb_log("Sending query-command. Disc ID: %08x",
             cdda_cddb_compute_discid(info));

    getstr =
        g_strdup_printf
        ("GET /~cddb/cddb.cgi?cmd=cddb+query+%08x+%d+%s+%d%s&proto=%d HTTP/1.0\r\n\r\n",
         cdda_cddb_compute_discid(info),
         info->last_track - info->first_track + 1, offsets,
         (info->leadout.minute * 60 + info->leadout.second),
         cddb_generate_hello_string(), cdda_cfg.cddb_protocol_level);
	cddb_log(getstr);

    g_free(offsets);
    write(sock, getstr, strlen(getstr));
    g_free(getstr);

    if (http_read_first_line(sock, buffer, 256) < 0) {
        http_close_connection(sock);
        return FALSE;
    }

    response = g_strsplit(buffer, " ", 4);

    cddb_log("Query response: %s", buffer);

    switch (strtol(response[0], NULL, 10)) {
    case 200:
        /* One exact match */
        for (i = 0; i < 4; i++) {
            if (response[i] == NULL) {
                g_strfreev(response);
                return FALSE;
            }
        }
        cddb_info->category = g_strdup(response[1]);
        cddb_info->discid = strtoul(response[2], NULL, 16);
        break;
	case 211:
		/* multiple matches - use first match */
        g_strfreev(response);
        if (http_read_first_line(sock, buffer, 256) < 0) {
            http_close_connection(sock);
            return FALSE;
        }
        response = g_strsplit(buffer, " ", 4);
        for (i = 0; i < 4; i++) {
            if (response[i] == NULL) {
                g_strfreev(response);
                return FALSE;
            }
        }
        cddb_info->category = g_strdup(response[1]);
        cddb_info->discid = strtoul(response[2], NULL, 16);
        break;
    default:                   /* FIXME: Handle other 2xx */
        g_strfreev(response);
        return FALSE;
    }
    http_close_connection(sock);

    g_strfreev(response);
    return TRUE;
}

static gint
cddb_check_protocol_level(const gchar * server)
{
    gint level = 0, sock, n;
    gchar *str, buffer[256];

    if ((sock = cddb_http_open_connection(server, 80)) == 0)
        return 0;

    str =
        g_strdup_printf
        ("GET /~cddb/cddb.cgi?cmd=stat%s&proto=1 HTTP/1.0\r\n\r\n",
         cddb_generate_hello_string());

    write(sock, str, strlen(str));
    g_free(str);

    if ((n = http_read_first_line(sock, buffer, 256)) < 0 ||
        atoi(buffer) != 210) {
        if (n > 0)
            cddb_log("Getting cddb protocol level failed: %s", buffer);
        else
            cddb_log("Getting cddb protocol level failed.");

        http_close_connection(sock);
        return 0;
    }

    while (http_read_line(sock, buffer, 256) >= 0) {
        g_strstrip(buffer);
        if (!strncmp(buffer, "max proto:", 10))
            level = atoi(buffer + 10);
        if (!strcmp(buffer, "."))
            break;
    }
    http_close_connection(sock);
    cddb_log("Getting cddb protocol level. Got level %d", level);
    return (MIN(level, CDDB_MAX_PROTOCOL_LEVEL));
}

#define BUF2SIZE (80*3)

static gboolean
cddb_read(gchar * server, cddb_disc_header_t * cddb_info, cdinfo_t * cdinfo)
{
    gint sock;
    gchar *readstr;
    gchar buffer[256], buffer2[BUF2SIZE];
    gchar *realstr, *temp;
    gint len, command, bufs;
    gint num, oldnum;

    if ((sock = cddb_http_open_connection(server, 80)) == 0)
        return FALSE;

    cddb_log("Sending read-command. Disc ID: %08x. Category: %s",
             cddb_info->discid, cddb_info->category);

    readstr =
        g_strdup_printf
        ("GET /~cddb/cddb.cgi?cmd=cddb+read+%s+%08x%s&proto=%d HTTP/1.0\r\n\r\n",
         cddb_info->category, cddb_info->discid,
         cddb_generate_hello_string(), cdda_cfg.cddb_protocol_level);
	cddb_log(getstr);

    write(sock, readstr, strlen(readstr));
    g_free(readstr);

    if (http_read_first_line(sock, buffer, 256) < 0) {
        http_close_connection(sock);
        return FALSE;
    }

    cddb_log("Read response: %s", buffer);

    command = 1;
    bufs = 0;
    oldnum = -1;
    do {
/*              fprintf(stderr,"%s\n",buffer); */
        realstr = strchr(buffer, '=');
        if (buffer[0] == '#' || !realstr)
            continue;

        realstr++;
        len = strlen(realstr);

        switch (command) {
        case 1:
            if (!strncmp(buffer, "DISCID", 6))
                break;
            command++;
        case 2:
            if (!strncmp(buffer, "DTITLE", 6)) {
                strncpy(buffer2 + bufs, realstr, BUF2SIZE - bufs);
                bufs += len;
                break;
            }
            if (bufs > 0) {
                buffer2[BUF2SIZE - 1] = '\0';
                if ((temp = strstr(buffer2, " / ")) != NULL) {
                    cdda_cdinfo_cd_set(cdinfo, g_strdup(temp + 3),
                                       g_strndup(buffer2, temp - buffer2));
                }
                else
                    cdda_cdinfo_cd_set(cdinfo, g_strdup(buffer2),
                                       g_strdup(buffer2));
                bufs = 0;
            }
            command++;
        case 3:
            if (!strncmp(buffer, "TTITLE", 6)) {
                num = atoi(buffer + 6);
                if (oldnum < 0 || num == oldnum) {
                    strncpy(buffer2 + bufs, realstr, BUF2SIZE - bufs);
                    bufs += len;
                }
                else {
                    buffer2[BUF2SIZE - 1] = '\0';
                    cdda_cdinfo_track_set(cdinfo, oldnum + 1, NULL,
                                          g_strdup(buffer2));
                    strncpy(buffer2, realstr, BUF2SIZE);
                    bufs = len;
                }
                oldnum = num;
                break;
            }
            if (oldnum >= 0)
                cdda_cdinfo_track_set(cdinfo, oldnum + 1, NULL,
                                      g_strdup(buffer2));
            bufs = 0;
            oldnum = -1;
            command++;
        case 4:
            if (!strncmp(buffer, "EXTD", 4)) {
                break;
            }
            command++;
        case 5:
            if (!strncmp(buffer, "EXTT", 4)) {
                break;
            }
            command++;
        case 6:
            if (!strncmp(buffer, "PLAYORDER", 9)) {
                break;
            }
            command++;
        default:
            g_log(NULL, G_LOG_LEVEL_WARNING, "%s: illegal cddb-data: %s",
                  PACKAGE_NAME, buffer);
            break;
        }

    } while (http_read_line(sock, buffer, 256) >= 0);

    if (oldnum >= 0)
        cdda_cdinfo_track_set(cdinfo, oldnum + 1, NULL, g_strdup(buffer2));

    http_close_connection(sock);
    return TRUE;
}

static gint
cddb_get_protocol_level(void)
{
    if (cdda_cfg.cddb_protocol_level < 1)
        cdda_cfg.cddb_protocol_level =
            cddb_check_protocol_level(cdda_cfg.cddb_server);

    return cdda_cfg.cddb_protocol_level;
}

static GList *
cddb_get_server_list(const gchar * server, gint protocol_level)
{
    gint sock;
    gchar *getstr;
    gchar buffer[256];
    gchar **message;
    GList *list = NULL;

    if ((sock = cddb_http_open_connection(server, 80)) == 0)
        return NULL;

    cddb_log("Sending sites-command");

    getstr =
        g_strdup_printf
        ("GET /~cddb/cddb.cgi?cmd=sites%s&proto=%d HTTP/1.0\r\n\r\n",
         cddb_generate_hello_string(), protocol_level);
	cddb_log(getstr);

    write(sock, getstr, strlen(getstr));
    g_free(getstr);

    if (http_read_first_line(sock, buffer, 256) < 0) {
        http_close_connection(sock);
        return NULL;
    }

    cddb_log("Sites response: %s", buffer);

    switch (atoi(buffer)) {
    case 210:
        while ((http_read_line(sock, buffer, 256)) > 1) {
            message = g_strsplit(buffer, " ", 6);
            if (message && message[0] && message[1] &&
                !strcasecmp(message[1], "http")) {
                list = g_list_prepend(list, message);
            }
            else {
                /* Ignore non-http servers */
                g_strfreev(message);
            }
        }
        list = g_list_reverse(list);
        break;
    case 401:
        /* No site information available */
        break;
    default:
        break;
    }
    http_close_connection(sock);
    return list;
}

gint
search_for_discid(gchar * abs_filename, gchar ** cddb_file, guint32 disc_id)
{
    GDir *dir;
    const gchar *dir_entry;
    gchar tmp_id[10];

    if (!(dir = g_dir_open(abs_filename, 0, NULL)))
        return (0);

    memset(tmp_id, 0, 10);

    snprintf(tmp_id, sizeof(tmp_id), "%08x", disc_id);
    while ((dir_entry = g_dir_read_name(dir))) {
        if (!strncmp(tmp_id, dir_entry, 8)) {
            cddb_file[0] = g_build_filename(abs_filename, dir_entry, NULL);
            g_dir_close(dir);
            return (1);
        }
    }
    g_dir_close(dir);

    return (0);
}

gint
scan_cddb_dir(gchar * server, gchar ** cddb_file, guint32 disc_id)
{

    GDir *dir;
    const gchar *dir_entry;
    gchar abs_filename[FILENAME_MAX];

    if (!(dir = g_dir_open(&server[7], 0, NULL))) {
        return 0;
    }

    while ((dir_entry = g_dir_read_name(dir))) {
        strcpy(abs_filename, &server[7]);
        if (abs_filename[strlen(abs_filename) - 1] != '/') {
            strcat(abs_filename, "/");
        }
        strcat(abs_filename, dir_entry);

        if (dir_entry[0] != '.' &&
            g_file_test(abs_filename, G_FILE_TEST_IS_DIR) &&
            search_for_discid(abs_filename, cddb_file, disc_id)) {
            break;
        }
    }

    g_dir_close(dir);
    return (cddb_file[0] != NULL);
}

gint
cddb_read_file(gchar * file, cddb_disc_header_t * cddb_info,
               cdinfo_t * cdinfo)
{
    FILE *fd;
    gchar buffer[256], buffer2[BUF2SIZE];
    gchar *realstr, *temp;
    gint len, command, bufs;
    gint num, oldnum;

    if ((fd = fopen(file, "r")) == NULL)
        return 0;

    command = 1;
    bufs = 0;
    oldnum = -1;
    while (fgets(buffer, 256, fd) != NULL) {
        realstr = strchr(buffer, '=');
        if (buffer[0] == '#' || !realstr)
            continue;

        realstr++;
        len = strlen(realstr);
        if (realstr[len - 1] == '\n')
            realstr[--len] = '\0';  /* remove newline */

        switch (command) {
        case 1:
            if (!strncmp(buffer, "DISCID", 6))
                break;
            command++;
        case 2:
            if (!strncmp(buffer, "DTITLE", 6)) {
                strncpy(buffer2 + bufs, realstr, BUF2SIZE - bufs);
                bufs += len;
                break;
            }
            if (bufs > 0) {
                buffer2[BUF2SIZE - 1] = '\0';
                if ((temp = strstr(buffer2, " / ")) != NULL) {
                    cdda_cdinfo_cd_set(cdinfo, g_strdup(temp + 3),
                                       g_strndup(buffer2, temp - buffer2));
                }
                else
                    cdda_cdinfo_cd_set(cdinfo, g_strdup(buffer2),
                                       g_strdup(buffer2));
                bufs = 0;
            }
            command++;
        case 3:
            if (!strncmp(buffer, "TTITLE", 6)) {
                num = atoi(buffer + 6);
                if (oldnum < 0 || num == oldnum) {
                    strncpy(buffer2 + bufs, realstr, BUF2SIZE - bufs);
                    bufs += len;
                }
                else {
                    buffer2[BUF2SIZE - 1] = '\0';
                    cdda_cdinfo_track_set(cdinfo, oldnum + 1, NULL,
                                          g_strdup(buffer2));
                    strncpy(buffer2, realstr, BUF2SIZE);
                    bufs = len;
                }
                oldnum = num;
                break;
            }
            if (oldnum >= 0)
                cdda_cdinfo_track_set(cdinfo, oldnum + 1, NULL,
                                      g_strdup(buffer2));
            bufs = 0;
            oldnum = -1;
            command++;
        case 4:
            if (!strncmp(buffer, "EXTD", 4)) {
                break;
            }
            command++;
        case 5:
            if (!strncmp(buffer, "EXTT", 4)) {
                break;
            }
            command++;
        case 6:
            if (!strncmp(buffer, "PLAYORDER", 9)) {
                break;
            }
            command++;
        default:
            g_log(NULL, G_LOG_LEVEL_WARNING, "%s: illegal cddb-data: %s",
                  PACKAGE_NAME, buffer);
            break;
        }

    }

    if (oldnum >= 0)
        cdda_cdinfo_track_set(cdinfo, oldnum + 1, NULL, g_strdup(buffer2));

    fclose(fd);
    return (1);
}


void
cdda_cddb_get_info(cdda_disc_toc_t * toc, cdinfo_t * cdinfo)
{
    guint32 disc_id;
    cddb_disc_header_t cddb_disc_info;
    gchar *cddb_file[1];
    disc_id = cdda_cddb_compute_discid(toc);
    cddb_file[0] = NULL;

    if ((cached_id != disc_id)
        && (strncmp(cdda_cfg.cddb_server, "file://", 7) != 0)) {
        if (cddb_get_protocol_level() == 0)
            return;

        cached_id = disc_id;
        if (!cddb_query(cdda_cfg.cddb_server, toc, &cddb_disc_info))
            return;
        if (!cddb_read(cdda_cfg.cddb_server, &cddb_disc_info, cdinfo))
            return;
        cdinfo->is_valid = TRUE;

    }
    else if ((cached_id != disc_id)
             && (strncmp(cdda_cfg.cddb_server, "file://", 7) == 0)) {
        cached_id = disc_id;
        if (!scan_cddb_dir(cdda_cfg.cddb_server, cddb_file, disc_id))
            return;
        if (!cddb_read_file(cddb_file[0], &cddb_disc_info, cdinfo)) {
            g_free(cddb_file[0]);
            return;
        }
        cdinfo->is_valid = TRUE;
        g_free(cddb_file[0]);
    }
}

void
cdda_cddb_set_server(const gchar * new_server)
{
    if (strcmp(cdda_cfg.cddb_server, new_server)) {
        g_free(cdda_cfg.cddb_server);
        cdda_cfg.cddb_server = g_strdup(new_server);
        cdda_cfg.cddb_protocol_level = 0;
        cached_id = 0;
    }
}


static gchar *
cddb_position_string(gchar * input)
{
    gchar deg[4], min[3];
    if (input == NULL || strlen(input) < 7)
        return g_strdup("");
    strncpy(deg, input + 1, 3);
    deg[3] = '\0';
    strncpy(min, input + 5, 2);
    min[2] = '\0';
    return g_strdup_printf("%2d°%s'%c", atoi(deg), min, input[0]);
}

static void
cddb_server_dialog_ok_cb(GtkWidget * w, gpointer data)
{
    gchar *text;
    gint pos;
    GtkEntry *entry = GTK_ENTRY(data);

    if (!GTK_CLIST(server_clist)->selection)
        return;
    pos = GPOINTER_TO_INT(GTK_CLIST(server_clist)->selection->data);
    gtk_clist_get_text(GTK_CLIST(server_clist), pos, 0, &text);
    cdda_cddb_set_server(text);
    gtk_entry_set_text(entry, text);
    gtk_widget_destroy(server_dialog);
}

static void
cddb_server_dialog_select(GtkWidget * w, gint row, gint column,
                          GdkEvent * event, gpointer data)
{
    if (event->type == GDK_2BUTTON_PRESS)
        cddb_server_dialog_ok_cb(NULL, NULL);
}

void
cdda_cddb_show_server_dialog(GtkWidget * w, gpointer data)
{
    GtkWidget *vbox, *bbox, *okbutton, *cancelbutton;
    GtkEntry *server_entry = GTK_ENTRY(data);
    gchar *titles[] = { "Server", "Latitude", "Longitude", "Description" };
    GList *servers;
    const gchar *server;
    gint level;

    if (server_dialog)
        return;

    server = gtk_entry_get_text(server_entry);

    if ((level = cddb_check_protocol_level(server)) < 3) {
        if (!level)
            xmms_show_message("CDDB",
                              "Unable to connect to CDDB-server",
                              "Ok", FALSE, NULL, NULL);
        else
            /* CDDB level < 3 has the "sites" command,
               but the format is different. Not supported yet */
            xmms_show_message("CDDB",
                              "Can't get server list from the current CDDB-server\n"
                              "Unsupported CDDB protocol level",
                              "Ok", FALSE, NULL, NULL);
        return;
    }

    if ((servers = cddb_get_server_list(server, level)) == NULL) {
        xmms_show_message("CDDB",
                          "No site information available",
                          "Ok", FALSE, NULL, NULL);
        return;
    }

    server_dialog = gtk_dialog_new();
    g_signal_connect(G_OBJECT(server_dialog), "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &server_dialog);
    gtk_window_set_title(GTK_WINDOW(server_dialog), "CDDB servers");
    gtk_window_set_modal(GTK_WINDOW(server_dialog), TRUE);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(server_dialog)->vbox), vbox,
                       TRUE, TRUE, 0);

    server_clist = gtk_clist_new_with_titles(4, titles);
    g_signal_connect(G_OBJECT(server_clist), "select-row",
                     G_CALLBACK(cddb_server_dialog_select), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), server_clist, TRUE, TRUE, 0);

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(server_dialog)->action_area),
                       bbox, TRUE, TRUE, 0);

    okbutton = gtk_button_new_with_label("Ok");
    g_signal_connect(G_OBJECT(okbutton), "clicked",
                     G_CALLBACK(cddb_server_dialog_ok_cb), data);
    gtk_box_pack_start(GTK_BOX(bbox), okbutton, TRUE, TRUE, 0);
    cancelbutton = gtk_button_new_with_label("Cancel");
    g_signal_connect_swapped(G_OBJECT(cancelbutton), "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             GTK_OBJECT(server_dialog));
    gtk_box_pack_start(GTK_BOX(bbox), cancelbutton, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS(okbutton, GTK_CAN_DEFAULT);
    GTK_WIDGET_SET_FLAGS(cancelbutton, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(okbutton);

    while (servers) {
        gchar *row[4];
        gint i;

        row[0] = g_strdup(((gchar **) servers->data)[0]);
        row[1] = cddb_position_string(((gchar **) servers->data)[4]);
        row[2] = cddb_position_string(((gchar **) servers->data)[5]);
        row[3] = g_strdup(((gchar **) servers->data)[6]);
        gtk_clist_append(GTK_CLIST(server_clist), row);
        for (i = 0; i < 4; i++)
            g_free(row[i]);
        g_strfreev(servers->data);
        servers = g_list_next(servers);
    }
    g_list_free(servers);
    gtk_clist_columns_autosize(GTK_CLIST(server_clist));
    gtk_widget_show_all(server_dialog);
}

static gboolean
cddb_update_log_window(gpointer data)
{
    if (!debug_window) {
        cddb_timeout_id = 0;
        return FALSE;
    }

    G_LOCK(list);
    if (temp_messages != NULL) {
        GList *temp;
        GDK_THREADS_ENTER();
        gtk_clist_freeze(GTK_CLIST(debug_clist));
        for (temp = temp_messages; temp; temp = temp->next) {
            gchar *text = temp->data;
            gtk_clist_append(GTK_CLIST(debug_clist), &text);
            g_free(text);
        }
        gtk_clist_columns_autosize(GTK_CLIST(debug_clist));
        gtk_clist_thaw(GTK_CLIST(debug_clist));
        gtk_clist_moveto(GTK_CLIST(debug_clist),
                         GTK_CLIST(debug_clist)->rows - 1, -1, 0.5, 0);
        GDK_THREADS_LEAVE();
        g_list_free(temp_messages);
        temp_messages = NULL;
    }
    G_UNLOCK(list);
    return TRUE;
}


void
cdda_cddb_show_network_window(GtkWidget * w, gpointer data)
{
    GtkWidget *vbox, *bbox, *close, *scroll_win;
    GList *temp;

    if (debug_window)
        return;

    debug_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(debug_window), "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &debug_window);
    gtk_window_set_title(GTK_WINDOW(debug_window), "CDDB networkdebug");
    gtk_window_set_resizable(GTK_WINDOW(debug_window), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(debug_window), 400, 150);
    gtk_container_border_width(GTK_CONTAINER(debug_window), 10);

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(debug_window), vbox);

    scroll_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    debug_clist = gtk_clist_new(1);
    gtk_container_add(GTK_CONTAINER(scroll_win), debug_clist);
    gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);

    temp = debug_messages;
    while (temp) {
        gtk_clist_prepend(GTK_CLIST(debug_clist), (gchar **) & temp->data);
        temp = g_list_next(temp);
    }

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

    close = gtk_button_new_with_label("Close");
    g_signal_connect_swapped(G_OBJECT(close), "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             GTK_OBJECT(debug_window));
    GTK_WIDGET_SET_FLAGS(close, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bbox), close, TRUE, TRUE, 0);
    gtk_widget_grab_default(close);

    gtk_clist_columns_autosize(GTK_CLIST(debug_clist));
    gtk_clist_set_button_actions(GTK_CLIST(debug_clist), 0,
                                 GTK_BUTTON_IGNORED);
    gtk_clist_moveto(GTK_CLIST(debug_clist),
                     GTK_CLIST(debug_clist)->rows - 1, -1, 0, 0);

    cddb_timeout_id = gtk_timeout_add(500, cddb_update_log_window, NULL);
    gtk_widget_show_all(debug_window);
}

void
cddb_quit(void)
{
    if (cddb_timeout_id)
        gtk_timeout_remove(cddb_timeout_id);
    cddb_timeout_id = 0;
}
