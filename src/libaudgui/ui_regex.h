/*
 * ui_regex.h
 * Copyright 2010 Carlo Bramini
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
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
