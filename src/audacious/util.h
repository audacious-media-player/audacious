/*
 * util.h
 * Copyright 2009-2011 John Lindgren
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

#ifndef AUDACIOUS_UTIL_H
#define AUDACIOUS_UTIL_H

#include <sys/types.h>
#include <libaudcore/core.h>

typedef bool_t(*DirForeachFunc) (const char * path,
                                   const char * basename,
                                   void * user_data);

bool_t dir_foreach (const char * path, DirForeachFunc func, void * user_data);

void make_directory(const char * path, mode_t mode);
char * write_temp_file (void * data, int64_t len);

char * get_path_to_self (void);

void describe_song (const char * filename, const Tuple * tuple,
 char * * title, char * * artist, char * * album);

#endif /* AUDACIOUS_UTIL_H */
