/*
 * i18n.h
 * Copyright 2007 Ariadne Conill
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

#ifndef AUDACIOUS_I18N_H
#define AUDACIOUS_I18N_H

#ifdef HAVE_GETTEXT

#include <libintl.h>

#define _(String) dgettext(PACKAGE, String)

#ifdef gettext_noop
#define N_(String) gettext_noop(String)
#else
#define N_(String) (String)
#endif

#else

#define _(String) (String)
#define N_(String) (String)
#define dgettext(package, str) (str)
#define dngettext(package, str1, str2, count) (count > 1 ? str2 : str1)
#define ngettext(str1, str2, count) (count > 1 ? str2 : str1)

#endif

#endif /* AUDACIOUS_I18N_H */
