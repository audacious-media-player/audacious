/*
 * export.h
 * Copyright 2016 John Lindgren
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

#ifndef LIBAUDQT_EXPORT_H
#define LIBAUDQT_EXPORT_H

#ifdef _WIN32
  #ifdef LIBAUDQT_BUILD
    #define LIBAUDQT_PUBLIC __declspec(dllexport)
  #else
    #define LIBAUDQT_PUBLIC __declspec(dllimport)
  #endif
#else
  #define LIBAUDQT_PUBLIC __attribute__ ((visibility ("default")))
#endif

#endif // LIBAUDQT_EXPORT_H
