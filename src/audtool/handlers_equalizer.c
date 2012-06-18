/*
 * handlers_equalizer.c
 * Copyright 2007-2008 Yoshiki Yazawa and Matti Hämäläinen
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <locale.h>
#include "libaudclient/audctrl.h"
#include "audtool.h"

void equalizer_get_eq(gint argc, gchar **argv)
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

void equalizer_get_eq_preamp(gint argc, gchar **argv)
{
    audtool_report("preamp = %.2f", audacious_remote_get_eq_preamp(dbus_proxy));
}

void equalizer_get_eq_band(gint argc, gchar **argv)
{
    int band;

    if (argc < 2)
    {
        audtool_whine_args(argv[0], "<band>");
        exit(1);
    }

    band = atoi(argv[1]);

    /* FIXME, XXX, TODO: we should have a function for requesting
     * the actual number of bands, if we support dynamic amount some day ...
     * -- ccr
     */
    if (band < 0 || band > 9)
    {
        audtool_whine("band number out of range\n");
        exit(1);
    }

    audtool_report("band %d = %.2f", band, audacious_remote_get_eq_band(dbus_proxy, band));

}

void equalizer_set_eq(gint argc, gchar **argv)
{
    gdouble preamp;
    GArray *bands = g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 10);
    int i;

    if (argc < 12)
    {
        audtool_whine_args(argv[0], "<preamp> <band0> <band1> <band2> <band3> <band4> <band5> <band6> <band7> <band8> <band9>");
        exit(1);
    }

    preamp = atof(argv[1]);

    for(i=0; i<10; i++){
        gdouble val = atof(argv[i+2]);
        g_array_append_val(bands, val);
    }

    audacious_remote_set_eq(dbus_proxy, preamp, bands);
}

void equalizer_set_eq_preamp(gint argc, gchar **argv)
{
    gdouble preamp;

    if (argc < 2)
    {
        audtool_whine_args(argv[0], "<preamp>");
        exit(1);
    }

    preamp = atof(argv[1]);

    audacious_remote_set_eq_preamp(dbus_proxy, preamp);
}

void equalizer_set_eq_band(gint argc, gchar **argv)
{
    int band;
    gdouble preamp;

    if (argc < 3)
    {
        audtool_whine_args(argv[0], "<band> <value>");
        exit(1);
    }

    band = atoi(argv[1]);
    preamp = atof(argv[2]);

    audacious_remote_set_eq_band(dbus_proxy, band, preamp);
}

void equalizer_active(gint argc, gchar **argv)
{
    if (argc < 2)
    {
        audtool_whine_args(argv[0], "<on/off>");
        exit(1);
    }

    if (!g_ascii_strcasecmp(argv[1], "on")) {
        audacious_remote_eq_activate(dbus_proxy, TRUE);
    }
    else if (!g_ascii_strcasecmp(argv[1], "off")) {
        audacious_remote_eq_activate(dbus_proxy, FALSE);
    }
}
