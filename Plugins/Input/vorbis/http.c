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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "vorbis.h"
#include "http.h"
#include "libaudacious/util.h"
#include "audacious/plugin.h"


#define min(x,y) ((x)<(y)?(x):(y))
#define min3(x,y,z) (min(x,y)<(z)?min(x,y):(z))
#define min4(x,y,z,w) (min3(x,y,z)<(w)?min3(x,y,z):(w))

static gchar *ice_name = NULL;

static gboolean prebuffering, going, eof = FALSE;
static gint sock, rd_index, wr_index, buffer_length, prebuffer_length;
static guint64 buffer_read = 0;
static gchar *buffer;
static GThread *thread;
static GtkWidget *error_dialog = NULL;

extern vorbis_config_t vorbis_cfg;
extern InputPlugin vorbis_ip;
extern int vorbis_playing;

static VFSFile *output_file = NULL;

#define BASE64_LENGTH(len) (4 * (((len) + 2) / 3))

/* Encode the string S of length LENGTH to base64 format and place it
   to STORE.  STORE will be 0-terminated, and must point to a writable
   buffer of at least 1+BASE64_LENGTH(length) bytes.  */
static void
base64_encode(const gchar * s, gchar * store, gint length)
{
    /* Conversion table.  */
    static gchar tbl[64] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
        'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
        'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3',
        '4', '5', '6', '7', '8', '9', '+', '/'
    };
    gint i;
    guchar *p = (guchar *) store;

    /* Transform the 3x8 bits to 4x6 bits, as required by base64.  */
    for (i = 0; i < length; i += 3) {
        *p++ = tbl[s[0] >> 2];
        *p++ = tbl[((s[0] & 3) << 4) + (s[1] >> 4)];
        *p++ = tbl[((s[1] & 0xf) << 2) + (s[2] >> 6)];
        *p++ = tbl[s[2] & 0x3f];
        s += 3;
    }
    /* Pad the result if necessary...  */
    if (i == length + 1)
        *(p - 1) = '=';
    else if (i == length + 2)
        *(p - 1) = *(p - 2) = '=';
    /* ...and zero-terminate it.  */
    *p = '\0';
}

/* Create the authentication header contents for the `Basic' scheme.
   This is done by encoding the string `USER:PASS' in base64 and
   prepending `HEADER: Basic ' to it.  */
static gchar *
basic_authentication_encode(const gchar * user,
                            const gchar * passwd, const gchar * header)
{
    gchar *t1, *t2, *res;
    gint len1 = strlen(user) + 1 + strlen(passwd);
    gint len2 = BASE64_LENGTH(len1);

    t1 = g_strdup_printf("%s:%s", user, passwd);
    t2 = g_malloc0(len2 + 1);
    base64_encode(t1, t2, len1);
    res = g_strdup_printf("%s: Basic %s\r\n", header, t2);
    g_free(t2);
    g_free(t1);

    return res;
}

static void
parse_url(const gchar * url, gchar ** user, gchar ** pass,
          gchar ** host, int *port, gchar ** filename)
{
    gchar *h, *p, *pt, *f, *temp, *ptr;

    temp = g_strdup(url);
    ptr = temp;

    if (!strncasecmp("http://", ptr, 7))
        ptr += 7;
    h = strchr(ptr, '@');
    f = strchr(ptr, '/');
    if (h != NULL && (!f || h < f)) {
        *h = '\0';
        p = strchr(ptr, ':');
        if (p != NULL && p < h) {
            *p = '\0';
            p++;
            *pass = g_strdup(p);
        }
        else
            *pass = NULL;
        *user = g_strdup(ptr);
        h++;
        ptr = h;
    }
    else {
        *user = NULL;
        *pass = NULL;
        h = ptr;
    }
    pt = strchr(ptr, ':');
    if (pt != NULL && (f == NULL || pt < f)) {
        *pt = '\0';
        *port = atoi(pt + 1);
    }
    else {
        if (f)
            *f = '\0';
        *port = 80;
    }
    *host = g_strdup(h);

    if (f)
        *filename = g_strdup(f + 1);
    else
        *filename = NULL;
    g_free(temp);
}

void
vorbis_http_close(void)
{
    going = FALSE;

    g_thread_join(thread);
    g_free(ice_name);
    ice_name = NULL;
}


static gint
http_used(void)
{
    if (wr_index >= rd_index)
        return wr_index - rd_index;
    return buffer_length - (rd_index - wr_index);
}

static gint
http_free(void)
{
    if (rd_index > wr_index)
        return (rd_index - wr_index) - 1;
    return (buffer_length - (wr_index - rd_index)) - 1;
}

static void
http_wait_for_data(gint bytes)
{
    while ((prebuffering || http_used() < bytes) && !eof && going
           && vorbis_playing)
        xmms_usleep(10000);
}

static void
show_error_message(gchar * error)
{
    if (!error_dialog) {
        GDK_THREADS_ENTER();
        error_dialog = xmms_show_message(_("Error"), error, _("Ok"), FALSE,
                                         NULL, NULL);
        g_signal_connect(G_OBJECT(error_dialog),
                         "destroy",
                         G_CALLBACK(gtk_widget_destroyed), &error_dialog);
        GDK_THREADS_LEAVE();
    }
}

int
vorbis_http_read(gpointer data, gint length)
{
    gint len, cnt, off = 0;

    http_wait_for_data(length);

    if (!going && !vorbis_playing)
        return 0;
    len = min(http_used(), length);

    while (len && http_used()) {
        cnt = min3(len, buffer_length - rd_index, http_used());
        if (output_file)
            vfs_fwrite(buffer + rd_index, 1, cnt, output_file);

        memcpy((gchar *) data + off, buffer + rd_index, cnt);
        rd_index = (rd_index + cnt) % buffer_length;
        buffer_read += cnt;
        len -= cnt;
        off += cnt;
    }
    return off;
}

static gboolean
http_check_for_data(void)
{

    fd_set set;
    struct timeval tv;
    gint ret;

    tv.tv_sec = 0;
    tv.tv_usec = 20000;
    FD_ZERO(&set);
    FD_SET(sock, &set);
    ret = select(sock + 1, &set, NULL, NULL, &tv);
    if (ret > 0)
        return TRUE;
    return FALSE;
}

gint
vorbis_http_read_line(gchar * buf, gint size)
{
    gint i = 0;

    while (going && i < size - 1) {
        if (http_check_for_data()) {
            if (read(sock, buf + i, 1) <= 0)
                return -1;
            if (buf[i] == '\n')
                break;
            if (buf[i] != '\r')
                i++;
        }
    }
    if (!going)
        return -1;
    buf[i] = '\0';
    return i;
}

static gpointer
http_buffer_loop(gpointer arg)
{
    gchar line[1024], *user, *pass, *host, *filename,
        *status, *url, *temp, *file;
    gchar *chost;
    gint cnt, written, error, port, cport;
    guint err_len;
    gboolean redirect;
    fd_set set;
    struct hostent *hp;
    struct sockaddr_in address;
    struct timeval tv;

    url = (gchar *) arg;
    do {
        redirect = FALSE;

        g_strstrip(url);

        parse_url(url, &user, &pass, &host, &port, &filename);

        if ((!filename || !*filename) && url[strlen(url) - 1] != '/')
            temp = g_strconcat(url, "/", NULL);
        else
            temp = g_strdup(url);
        g_free(url);
        url = temp;

        chost = vorbis_cfg.use_proxy ? vorbis_cfg.proxy_host : host;
        cport = vorbis_cfg.use_proxy ? vorbis_cfg.proxy_port : port;

        sock = socket(AF_INET, SOCK_STREAM, 0);
        fcntl(sock, F_SETFL, O_NONBLOCK);
        address.sin_family = AF_INET;

        status = g_strdup_printf(_("LOOKING UP %s"), chost);
        vorbis_ip.set_info_text(status);
        g_free(status);

        if (!(hp = gethostbyname(chost))) {
            status = g_strdup_printf(_("Couldn't look up host %s"), chost);
            show_error_message(status);
            g_free(status);

            vorbis_ip.set_info_text(NULL);
            eof = TRUE;
        }

        if (!eof) {
            memcpy(&address.sin_addr.s_addr, *(hp->h_addr_list),
                   sizeof(address.sin_addr.s_addr));
            address.sin_port = g_htons(cport);

            status = g_strdup_printf(_("CONNECTING TO %s:%d"), chost, cport);
            vorbis_ip.set_info_text(status);
            g_free(status);
            if (connect
                (sock, (struct sockaddr *) &address,
                 sizeof(struct sockaddr_in)) == -1) {
                if (errno != EINPROGRESS) {
                    status =
                        g_strdup_printf(_("Couldn't connect to host %s"),
                                        chost);
                    show_error_message(status);
                    g_free(status);

                    vorbis_ip.set_info_text(NULL);
                    eof = TRUE;
                }
            }
            while (going) {
                tv.tv_sec = 0;
                tv.tv_usec = 10000;
                FD_ZERO(&set);
                FD_SET(sock, &set);
                if (select(sock + 1, NULL, &set, NULL, &tv) > 0) {
                    err_len = sizeof(error);
                    getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &err_len);
                    if (error) {
                        status =
                            g_strdup_printf(_
                                            ("Couldn't connect to host %s"),
                                            chost);
                        show_error_message(status);
                        g_free(status);

                        vorbis_ip.set_info_text(NULL);
                        eof = TRUE;

                    }
                    break;
                }
            }
            if (!eof) {
                gchar *auth = NULL, *proxy_auth = NULL;

                if (user && pass)
                    auth =
                        basic_authentication_encode(user, pass,
                                                    "Authorization");

                if (vorbis_cfg.use_proxy) {
                    file = g_strdup(url);
                    if (vorbis_cfg.proxy_use_auth && vorbis_cfg.proxy_user
                        && vorbis_cfg.proxy_pass) {
                        proxy_auth =
                            basic_authentication_encode(vorbis_cfg.
                                                        proxy_user,
                                                        vorbis_cfg.
                                                        proxy_pass,
                                                        "Proxy-Authorization");
                    }
                }
                else
                    file = g_strconcat("/", filename, NULL);
                temp = g_strdup_printf("GET %s HTTP/1.0\r\n"
                                       "Host: %s\r\n"
                                       "User-Agent: %s/%s\r\n"
                                       "%s%s\r\n",
                                       file, host, PACKAGE_NAME, PACKAGE_VERSION,
                                       proxy_auth ? proxy_auth : "",
                                       auth ? auth : "");
                g_free(file);
                if (proxy_auth)
                    g_free(proxy_auth);
                if (auth)
                    g_free(auth);
                write(sock, temp, strlen(temp));
                g_free(temp);
                vorbis_ip.set_info_text(_("CONNECTED: WAITING FOR REPLY"));
                while (going && !eof) {
                    if (http_check_for_data()) {
                        if (vorbis_http_read_line(line, 1024)) {
                            status = strchr(line, ' ');
                            if (status) {
                                if (status[1] == '2')
                                    break;
                                else if (status[1] == '3'
                                         && status[2] == '0'
                                         && status[3] == '2') {
                                    while (going) {
                                        if (http_check_for_data()) {
                                            if ((cnt =
                                                 vorbis_http_read_line
                                                 (line, 1024)) != -1) {
                                                if (!cnt)
                                                    break;
                                                if (!strncmp
                                                    (line, "Location:", 9)) {
                                                    g_free(url);
                                                    url = g_strdup(line + 10);
                                                }
                                            }
                                            else {
                                                eof = TRUE;
                                                vorbis_ip.set_info_text(NULL);
                                                break;
                                            }
                                        }
                                    }
                                    redirect = TRUE;
                                    break;
                                }
                                else {
                                    status =
                                        g_strdup_printf(_
                                                        ("Couldn't connect to host %s\nServer reported: %s"),
                                                        chost, status);
                                    show_error_message(status);
                                    g_free(status);
                                    break;
                                }
                            }
                        }
                        else {
                            eof = TRUE;
                            vorbis_ip.set_info_text(NULL);
                        }
                    }
                }

                while (going && !redirect) {
                    if (http_check_for_data()) {
                        if ((cnt = vorbis_http_read_line(line, 1024)) != -1) {
                            if (!cnt)
                                break;
                            if (!strncmp(line, "ice-name:", 9))
                                ice_name = g_strdup(line + 9);

                        }
                        else {
                            eof = TRUE;
                            vorbis_ip.set_info_text(NULL);
                            break;
                        }
                    }
                }
            }
        }

        if (redirect) {
            if (output_file) {
                vfs_fclose(output_file);
                output_file = NULL;
            }
            close(sock);
            g_free(user);
            g_free(pass);
            g_free(host);
            g_free(filename);
        }
    } while (redirect);

    if (vorbis_cfg.save_http_stream) {
        gchar *output_name;
        file = vorbis_http_get_title(url);
        output_name = file;
        if (!strncasecmp(output_name, "http://", 7))
            output_name += 7;
        temp = strrchr(output_name, '.');
        if (temp && !strcasecmp(temp, ".ogg"))
            *temp = '\0';

        while ((temp = strchr(output_name, '/')))
            *temp = '_';
        output_name =
            g_strdup_printf("%s/%s.ogg", vorbis_cfg.save_http_path,
                            output_name);

        g_free(file);

        output_file = vfs_fopen(output_name, "wb");
        g_free(output_name);
    }

    while (going) {

        if (!http_used() && !vorbis_ip.output->buffer_playing())
            prebuffering = TRUE;
        if (http_free() > 0 && !eof) {
            if (http_check_for_data()) {
                cnt = min(http_free(), buffer_length - wr_index);
                if (cnt > 1024)
                    cnt = 1024;
                written = read(sock, buffer + wr_index, cnt);
                if (written <= 0) {
                    eof = TRUE;
                    if (prebuffering) {
                        prebuffering = FALSE;

                        vorbis_ip.set_info_text(NULL);
                    }

                }
                else
                    wr_index = (wr_index + written) % buffer_length;
            }

            if (prebuffering) {
                if (http_used() > prebuffer_length) {
                    prebuffering = FALSE;
                    vorbis_ip.set_info_text(NULL);
                }
                else {
                    status =
                        g_strdup_printf(_("PRE-BUFFERING: %dKB/%dKB"),
                                        http_used() / 1024,
                                        prebuffer_length / 1024);
                    vorbis_ip.set_info_text(status);
                    g_free(status);
                }

            }
        }
        else
            xmms_usleep(10000);

    }
    if (output_file) {
        vfs_fclose(output_file);
        output_file = NULL;
    }
    close(sock);


    g_free(user);
    g_free(pass);
    g_free(host);
    g_free(filename);
    g_free(buffer);
    g_free(url);

    return NULL;
}

gint
vorbis_http_open(const gchar * _url)
{
    gchar *url;

    url = g_strdup(_url);

    rd_index = 0;
    wr_index = 0;
    buffer_length = vorbis_cfg.http_buffer_size * 1024;
    prebuffer_length = (buffer_length * vorbis_cfg.http_prebuffer) / 100;
    buffer_read = 0;
    prebuffering = TRUE;
    going = TRUE;
    eof = FALSE;
    buffer = g_malloc(buffer_length);

    thread = g_thread_create(http_buffer_loop, url, TRUE, NULL);

    return 0;
}

gchar *
vorbis_http_get_title(const gchar * url)
{
    gchar *url_basename;

    if (ice_name)
        return g_strdup(ice_name);

    url_basename = g_path_get_basename(url);

    if (strlen(url_basename) > 0)
        return url_basename;

    g_free(url_basename);

    return g_strdup(url);
}
