/* Paranormal - A highly customizable audio visualization library
 * Copyright (C) 2001  Jamie Gennis <jgennis@mindspring.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>

#include <stdarg.h>
#include <glib.h>
#include "pnerror.h"

static void     pn_error_default_handler        (const gchar *str);

static PnErrorFunc error_func = pn_error_default_handler;

static void
pn_error_default_handler (const gchar *str)
{
  g_log ("paranormal", G_LOG_LEVEL_WARNING, str);
}

PnErrorFunc
pn_error_set_handler (PnErrorFunc func)
{
  PnErrorFunc old_func = error_func;

  if (! func)
    error_func = pn_error_default_handler;
  else
    error_func = func;

  return old_func;
}

void
pn_error (const gchar *fmt, ...)
{
  va_list ap;
  gchar *str;

  g_return_if_fail (fmt != NULL);

  va_start (ap, fmt);
  str = g_strdup_vprintf (fmt, ap);
  va_end (ap);

  error_func (str);

  g_free (str);
}
			  
