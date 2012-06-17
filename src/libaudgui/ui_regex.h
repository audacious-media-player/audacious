/*
 * ui_regex.h
 * Copyright 2010 Carlo Bramini
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

#ifndef __UI_REGEX_H__
#define __UI_REGEX_H__

#if defined USE_REGEX_ONIGURUMA
  #include <onigposix.h>

#elif defined USE_REGEX_PCRE
  #include <pcreposix.h>

#elif defined HAVE_REGEX_H
  #include <regex.h>

#elif defined HAVE_RXPOSIX_H
  #include <rxposix.h>

#elif defined HAVE_RX_RXPOSIX_H
  #include <rx/rxposix.h>
#endif

#endif
