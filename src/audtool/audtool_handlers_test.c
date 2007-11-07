/*
 * Audtool2
 * Copyright (c) 2007 Audacious development team
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <mowgli.h>
#include <locale.h>
#include "libaudclient/audctrl.h"
#include "audtool.h"

void test_activate(gint argc, gchar **argv)
{
    audacious_remote_activate(dbus_proxy);
}

void test_enqueue_to_temp(gint argc, gchar **argv)
{
    if (argc < 2)
    {
        audtool_whine("invalid parameters for %s.", argv[0]);
        audtool_whine("syntax: %s <url>", argv[0]);
        exit(1);
    }

    audacious_remote_playlist_enqueue_to_temp(dbus_proxy, argv[1]);
}

void test_toggle_aot(gint argc, gchar **argv)
{
    if (argc < 2)
    {
        audtool_whine("invalid parameters for %s.", argv[0]);
        audtool_whine("syntax: %s <on/off>", argv[0]);
        exit(1);
    }

    if (!g_ascii_strcasecmp(argv[1], "on")) {
        audacious_remote_toggle_aot(dbus_proxy, TRUE);
        return;
    }
    else if (!g_ascii_strcasecmp(argv[1], "off")) {
        audacious_remote_toggle_aot(dbus_proxy, FALSE);
        return;
    }
}

void test_get_skin(gint argc, gchar **argv)
{
    gchar *skin = NULL;
    skin = audacious_remote_get_skin(dbus_proxy);
    audtool_report("%s", skin);
    g_free(skin);
}

void test_set_skin(gint argc, gchar **argv)
{
    if (argc < 2)
    {
        audtool_whine("invalid parameters for %s.", argv[0]);
        audtool_whine("syntax: %s <skin>", argv[0]);
        exit(1);
    }

    if(!argv[1] || !strcmp(argv[1], ""))
       return;

    audacious_remote_set_skin(dbus_proxy, argv[1]);
}

void test_get_info(gint argc, gchar **argv)
{
    gint rate, freq, nch;

    audacious_remote_get_info(dbus_proxy, &rate, &freq, &nch);
    audtool_report("rate = %d freq = %d nch = %d", rate, freq, nch);
}

void test_ins_url_string(gint argc, gchar **argv)
{
    gint pos = -1;

    if (argc < 3)
    {
        audtool_whine("invalid parameters for %s.", argv[0]);
        audtool_whine("syntax: %s <url> <position>", argv[0]);
        exit(1);
    }

    pos = atoi(argv[2]) - 1;
    if(pos >= 0)
        audacious_remote_playlist_ins_url_string(dbus_proxy, argv[1], pos);
}

void test_get_version(gint argc, gchar **argv)
{
    gchar *version = NULL;
    version = audacious_remote_get_version(dbus_proxy);
    if(version)
        audtool_report("Audacious %s", version);
    g_free(version);
}

void test_get_eq(gint argc, gchar **argv)
{
    double preamp;
    GArray *bands;
    int i;

    audacious_remote_get_eq(dbus_proxy, &preamp, &bands);

    audtool_report("preamp = %.2f", preamp);
    for(i=0; i<10; i++){
        printf("%.2f ", g_array_index(bands, gdouble, i));
    }
    printf("\n");
    g_array_free(bands, TRUE);
}

void test_get_eq_preamp(gint argc, gchar **argv)
{
    audtool_report("preamp = %.2f", audacious_remote_get_eq_preamp(dbus_proxy));
}

void test_get_eq_band(gint argc, gchar **argv)
{
    int band;

    if (argc < 2)
    {
        audtool_whine("invalid parameters for %s.", argv[0]);
        audtool_whine("syntax: %s <band>", argv[0]);
        exit(1);
    }

    band = atoi(argv[1]);

    if (band < 0 || band > 9)
    {
        audtool_whine("band number out of range");
        exit(1);
    }
    
    audtool_report("band %d = %.2f", band, audacious_remote_get_eq_band(dbus_proxy, band));
    
}

void test_set_eq(gint argc, gchar **argv)
{
    gdouble preamp;
    GArray *bands = g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 10);
    int i;

    if (argc < 12)
    {
        audtool_whine("invalid parameters for %s.", argv[0]);
        audtool_whine("syntax: %s <preamp> <band0> <band1> <band2> <band3> <band4> <band5> <band6> <band7> <band8> <band9>", argv[0]);
        exit(1);
    }

    preamp = atof(argv[1]);
    
    for(i=0; i<10; i++){
        gdouble val = atof(argv[i+2]);
        g_array_append_val(bands, val);
    }
    
    audacious_remote_set_eq(dbus_proxy, preamp, bands);
}

void test_set_eq_preamp(gint argc, gchar **argv)
{
    gdouble preamp;

    if (argc < 2)
    {
        audtool_whine("invalid parameters for %s.", argv[0]);
        audtool_whine("syntax: %s <preamp>", argv[0]);
        exit(1);
    }

    preamp = atof(argv[1]);

    audacious_remote_set_eq_preamp(dbus_proxy, preamp);
}

void test_set_eq_band(gint argc, gchar **argv)
{
    int band;
    gdouble preamp;

    if (argc < 3)
    {
        audtool_whine("invalid parameters for %s.", argv[0]);
        audtool_whine("syntax: %s <band> <value>", argv[0]);
        exit(1);
    }

    band = atoi(argv[1]);
    preamp = atof(argv[2]);

    audacious_remote_set_eq_band(dbus_proxy, band, preamp);
}
