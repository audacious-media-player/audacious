/*
 * wrappers.h
 * Copyright 2013 John Lindgren
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

#ifndef AUDTOOL_WRAPPERS_H
#define AUDTOOL_WRAPPERS_H

typedef gboolean (* OnOffFunc) (ObjAudacious * proxy, gboolean show,
 GCancellable * cancellable, GError * * error);

void generic_on_off (int argc, char * * argv, OnOffFunc func);

int get_playlist_length (void);

int get_queue_length (void);
int get_queue_entry (int qpos);
int find_in_queue (int entry);

int get_current_entry (void);
char * get_entry_filename (int entry);
char * get_entry_title (int entry);
int get_entry_length (int entry);
char * get_entry_field (int entry, const char * field);

int get_current_time (void);
void get_current_info (int * bitrate, int * samplerate, int * channels);

#endif /* AUDTOOL_WRAPPERS_H */
