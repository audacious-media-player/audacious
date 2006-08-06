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

#ifndef __PN_INTEGER_OPTION_H__
#define __PN_INTEGER_OPTION_H__

#include "pnoption.h"


G_BEGIN_DECLS


#define PN_TYPE_INTEGER_OPTION              (pn_integer_option_get_type ())
#define PN_INTEGER_OPTION(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_INTEGER_OPTION, PnIntegerOption))
#define PN_INTEGER_OPTION_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_INTEGER_OPTION, PnIntegerOptionClass))
#define PN_IS_INTEGER_OPTION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_INTEGER_OPTION))
#define PN_IS_INTEGER_OPTION_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_INTEGER_OPTION))
#define PN_INTEGER_OPTION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_INTEGER_OPTION, PnIntegerOptionClass))

typedef struct _PnIntegerOption        PnIntegerOption;
typedef struct _PnIntegerOptionClass   PnIntegerOptionClass;

struct _PnIntegerOption
{
  PnOption parent;

  gint value;
  gint min;
  gint max;
};

struct _PnIntegerOptionClass
{
  PnOptionClass parent_class;
};

/* Creators */
GType                 pn_integer_option_get_type                (void);
PnIntegerOption      *pn_integer_option_new                     (const gchar *name,
								 const gchar *desc);

/* Accessors */
void                  pn_integer_option_set_value               (PnIntegerOption *integer_option,
								 gint value);
gint                  pn_integer_option_get_value               (PnIntegerOption *integer_option);
void                  pn_integer_option_set_min                 (PnIntegerOption *integer_option,
								 gint min);
gint                  pn_integer_option_get_min                 (PnIntegerOption *integer_option);
void                  pn_integer_option_set_max                 (PnIntegerOption *integer_option,
								 gint max);
gint                  pn_integer_option_get_max                 (PnIntegerOption *integer_option);






#endif /* __PN_INTEGER_OPTION_H__ */
