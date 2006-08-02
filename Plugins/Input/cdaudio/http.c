/*
 *  http.c
 *  Some simple routines for connecting to a remote tcp socket
 *  Copyright 1999 Håvard Kvålen <havardk@sol.no>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
 */

/* FIXME: We need to have *one* place in xmms where you configure proxies */

#include "http.h"

gint
http_open_connection(const gchar * server, gint port)
{
    gint sock;
#ifdef USE_IPV6
    struct addrinfo hints, *res, *res0;
    char service[6];
#else
    struct hostent *host;
    struct sockaddr_in address;
#endif

#ifdef USE_IPV6
    snprintf(service, 6, "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(server, service, &hints, &res0))
        return 0;

    for (res = res0; res; res = res->ai_next) {
        sock = socket (res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sock < 0) {
            if (res->ai_next)
                continue;
            else {
                freeaddrinfo(res0);
                return 0;
            }
        }
        if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
            if (res->ai_next) {
                close(sock);
                continue;
            } else {
                freeaddrinfo(res0);
                return 0;
            }
        }
        freeaddrinfo(res0);
        return sock;
    }
#else
    sock = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;

    if (!(host = gethostbyname(server)))
        return 0;

    memcpy(&address.sin_addr.s_addr, *(host->h_addr_list),
           sizeof(address.sin_addr.s_addr));
    address.sin_port = g_htons(port);

    if (connect
        (sock, (struct sockaddr *) &address,
         sizeof(struct sockaddr_in)) == -1)
        return 0;
#endif

    return sock;
}

void
http_close_connection(gint sock)
{
    shutdown(sock, 2);
    close(sock);
}

gint
http_read_line(gint sock, gchar * buf, gint size)
{
    gint i = 0;

    while (i < size - 1) {
        if (read(sock, buf + i, 1) <= 0) {
            if (i == 0)
                return -1;
            else
                break;
        }
        if (buf[i] == '\n')
            break;
        if (buf[i] != '\r')
            i++;
    }
    buf[i] = '\0';
    return i;
}

gint
http_read_first_line(gint sock, gchar * buf, gint size)
{
    /* Skips the HTTP-header, if there is one, and reads the first line into buf.
       Returns number of bytes read. */

    gint i;
    /* Skip the HTTP-header */
    if ((i = http_read_line(sock, buf, size)) < 0)
        return -1;
    if (!strncmp(buf, "HTTP", 4)) { /* Check to make sure its not HTTP/0.9 */
        while (http_read_line(sock, buf, size) > 0)
            /* nothing */ ;
        if ((i = http_read_line(sock, buf, size)) < 0)
            return -1;
    }

    return i;
}

gchar *
http_get(gchar * url)
{
    gchar *server, *getstr, *buf = NULL, *bptr;
    gchar *gs, *gc, *turl = url;
    gint sock, n, bsize, port = 0;

    /* Skip past ``http://'' part of URL */
    if (!strncmp(turl, "http:", 5)) {
        turl += 5;
        if (!strncmp(turl, "//", 2))
            turl += 2;
    }

    /* If path starts with a '/', we are referring to localhost */
    if (turl[0] == '/')
        server = "localhost";
    else
        server = turl;

    /* Check if URL contains port specification */
    gc = strchr(turl, ':');
    gs = strchr(turl, '/');

    if (gc != NULL && gc < gs) {
        port = atoi(gc + 1);
        *gc = '\0';
    }
    if (port == 0)
        port = 80;

    /* Make sure that server string is null terminated. */
    if (gs)
        *gs = '\0';


    /*
     * Now, open connection to server.
     */
    sock = http_open_connection(server, port);

    /* Repair the URL string that we broke earlier on */
    if (gs)
        *gs = '/';
    if (gc && gc == '\0')
        *gc = ':';

    if (sock == 0)
        return NULL;

    /*
     * Send query to socket.
     */
    getstr = g_strdup_printf("GET %s HTTP/1.0\r\n\r\n", gs ? gs : "/");
/*  	getstr = g_strdup_printf("GET %s HTTP/1.0\r\n\r\n", url ? url : "/"); */

    if (write(sock, getstr, strlen(getstr)) == -1) {
        http_close_connection(sock);
        return NULL;
    }

    /*
     * Start receiving result.
     */
    bsize = 4096;
    bptr = buf = g_malloc(bsize);

    if ((n = http_read_first_line(sock, bptr, bsize)) == -1) {
        g_free(buf);
        buf = NULL;
        goto Done;
    }

    bsize -= n;
    bptr += n;

    while (bsize > 0 && (n = http_read_line(sock, bptr, bsize)) != -1) {
        bptr += n;
        bsize -= n;
    }

  Done:
    http_close_connection(sock);

    /*
     * Return result buffer to caller.
     */
    return buf;
}
