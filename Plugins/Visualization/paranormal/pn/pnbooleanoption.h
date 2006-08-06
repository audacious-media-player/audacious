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

#ifndef __PN_BOOLEAN_OPTION_H__
#define __PN_BOOLEAN_OPTION_H__

#include "pnoption.h"


G_BEGIN_DECLS


#define PN_TYPE_BOOLEAN_OPTION              (pn_boolean_option_get_type ())
#define PN_BOOLEAN_OPTION(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_BOOLEAN_OPTION, PnBooleanOption))
#define PN_BOOLEAN_OPTION_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_BOOLEAN_OPTION, PnBooleanOptionClass))
#define PN_IS_BOOLEAN_OPTION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_BOOLEAN_OPTION))
#define PN_IS_BOOLEAN_OPTION_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_BOOLEAN_OPTION))
#define PN_BOOLEAN_OPTION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_BOOLEAN_OPTION, PnBooleanOptionClass))

typedef struct _PnBooleanOption        PnBooleanOption;
typedef struct _PnBooleanOptionClass   PnBooleanOptionClass;

struct _PnBooleanOption
{
  PnOption parent;

  gboolean value;
};

struct _PnBooleanOptionClass
{
  PnOptionClass parent_class;
};

/* Creators */
GType                 pn_boolean_option_get_type                (void);
PnBooleanOption      *pn_boolean_option_new                     (const gchar *name,
								 const gchar *desc);

/* Accessors */
void                  pn_boolean_option_set_value               (PnBooleanOption *boolean_option,
								 gboolean value);
gboolean              pn_boolean_option_get_value               (PnBooleanOption *boolean_option);

#endif /* __PN_BOOLEAN_OPTION_H__ */
