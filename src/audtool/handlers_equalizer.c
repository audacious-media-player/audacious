/*
 * handlers_equalizer.c
 * Copyright 2007-2013 Yoshiki Yazawa, Matti Hämäläinen, and John Lindgren
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

#include "audtool.h"
#include "wrappers.h"

#define NUM_BANDS 10

void equalizer_get_eq (int argc, char * * argv)
{
    double preamp = 0.0;
    GVariant * var = NULL;

    obj_audacious_call_get_eq_sync (dbus_proxy, & preamp, & var, NULL, NULL);

    if (! var || ! g_variant_is_of_type (var, G_VARIANT_TYPE ("ad")))
        exit (1);

    audtool_report ("preamp = %.2f", preamp);

    size_t nbands = 0;
    const double * bands = g_variant_get_fixed_array (var, & nbands, sizeof (double));

    if (nbands != NUM_BANDS)
        exit (1);

    for (int i = 0; i < NUM_BANDS; i ++)
        printf ("%.2f ", bands[i]);

    printf ("\n");
    g_variant_unref (var);
}

void equalizer_get_eq_preamp (int argc, char * * argv)
{
    double preamp = 0;
    obj_audacious_call_get_eq_preamp_sync (dbus_proxy, & preamp, NULL, NULL);
    audtool_report ("preamp = %.2f", preamp);
}

void equalizer_get_eq_band (int argc, char * * argv)
{
    if (argc < 2)
    {
        audtool_whine_args (argv[0], "<band>");
        exit (1);
    }

    int band = atoi (argv[1]);

    double level = 0;
    obj_audacious_call_get_eq_band_sync (dbus_proxy, band, & level, NULL, NULL);
    audtool_report ("band %d = %.2f", band, level);
}

void equalizer_set_eq (int argc, char * * argv)
{
    if (argc < 2 + NUM_BANDS)
    {
        audtool_whine_args (argv[0], "<preamp> <band0> <band1> <band2> <band3> "
         "<band4> <band5> <band6> <band7> <band8> <band9>");
        exit (1);
    }

    double preamp = atof (argv[1]);
    double bands[NUM_BANDS];

    for (int i = 0; i < NUM_BANDS; i ++)
        bands[i] = atof (argv[i + 2]);

    GVariant * var = g_variant_new_fixed_array (G_VARIANT_TYPE_DOUBLE, bands,
     NUM_BANDS, sizeof (double));
    obj_audacious_call_set_eq_sync (dbus_proxy, preamp, var, NULL, NULL);
}

void equalizer_set_eq_preamp (int argc, char * * argv)
{
    if (argc < 2)
    {
        audtool_whine_args (argv[0], "<preamp>");
        exit (1);
    }

    double preamp = atof (argv[1]);
    obj_audacious_call_set_eq_preamp_sync (dbus_proxy, preamp, NULL, NULL);
}

void equalizer_set_eq_band (int argc, char * * argv)
{
    if (argc < 3)
    {
        audtool_whine_args (argv[0], "<band> <value>");
        exit (1);
    }

    int band = atoi (argv[1]);
    double level = atof (argv[2]);
    obj_audacious_call_set_eq_band_sync (dbus_proxy, band, level, NULL, NULL);
}

void equalizer_active (int argc, char * * argv)
{
    generic_on_off (argc, argv, obj_audacious_call_equalizer_activate_sync);
}
