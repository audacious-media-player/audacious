/*
 * debug.h
 * Copyright 2010-2011 John Lindgren
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

#ifndef AUDACIOUS_DEBUG_H
#define AUDACIOUS_DEBUG_H

#include <stdio.h>

#include <audacious/api.h>

#ifdef _AUDACIOUS_CORE
#define AUDDBG(...) do {if (verbose) {printf ("%s:%d [%s]: ", __FILE__, __LINE__, __FUNCTION__); printf (__VA_ARGS__);}} while (0)
#else
#define AUDDBG(...) do {if (* _aud_api_table->verbose) {printf ("%s:%d [%s]: ", __FILE__, __LINE__, __FUNCTION__); printf (__VA_ARGS__);}} while (0)
#endif

#endif
