/*
 * handlers_vitals.c
 * Copyright 2005-2013 George Averill, William Pitcock, and John Lindgren
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

#include <stdlib.h>

#include "audtool.h"
#include "wrappers.h"

void get_current_song (int argc, char * * argv)
{
    char * title = get_entry_title (get_current_entry ());
    audtool_report ("%s", title);
    g_free (title);
}

void get_current_song_filename (int argc, char * * argv)
{
    char * filename = get_entry_filename (get_current_entry ());
    audtool_report ("%s", filename);
    g_free (filename);
}

void get_current_song_output_length (int argc, char * * argv)
{
    int time = get_current_time () / 1000;
    audtool_report ("%d:%.2d", time / 60, time % 60);
}

void get_current_song_output_length_seconds (int argc, char * * argv)
{
    int time = get_current_time () / 1000;
    audtool_report ("%d", time);
}

void get_current_song_output_length_frames (int argc, char * * argv)
{
    int time = get_current_time ();
    audtool_report ("%d", time);
}

void get_current_song_length (int argc, char * * argv)
{
    int length = get_entry_length (get_current_entry ()) / 1000;
    audtool_report ("%d:%.2d", length / 60, length % 60);
}

void get_current_song_length_seconds (int argc, char * * argv)
{
    int length = get_entry_length (get_current_entry ()) / 1000;
    audtool_report ("%d", length);
}

void get_current_song_length_frames (int argc, char * * argv)
{
    int length = get_entry_length (get_current_entry ());
    audtool_report ("%d", length);
}

void get_current_song_bitrate (int argc, char * * argv)
{
    int bitrate;
    get_current_info (& bitrate, NULL, NULL);
    audtool_report ("%d", bitrate);
}

void get_current_song_bitrate_kbps (int argc, char * * argv)
{
    int bitrate;
    get_current_info (& bitrate, NULL, NULL);
    audtool_report ("%d", bitrate / 1000);
}

void get_current_song_frequency (int argc, char * * argv)
{
    int samplerate;
    get_current_info (NULL, & samplerate, NULL);
    audtool_report ("%d", samplerate);
}

void get_current_song_frequency_khz (int argc, char * * argv)
{
    int samplerate;
    get_current_info (NULL, & samplerate, NULL);
    audtool_report ("%d", samplerate / 1000);
}

void get_current_song_channels (int argc, char * * argv)
{
    int channels;
    get_current_info (NULL, NULL, & channels);
    audtool_report ("%d", channels);
}

void get_current_song_tuple_field_data (int argc, char * * argv)
{
    if (argc < 2)
    {
        audtool_whine_args (argv[0], "<fieldname>");
        audtool_whine_tuple_fields();
        exit (1);
    }

    char * str = get_entry_field (get_current_entry (), argv[1]);
    audtool_report ("%s", str);
    g_free (str);
}

void get_current_song_info (int argc, char * * argv)
{
    int bitrate, samplerate, channels;
    get_current_info (& bitrate, & samplerate, & channels);
    audtool_report ("rate = %d freq = %d nch = %d", bitrate, samplerate, channels);
}
