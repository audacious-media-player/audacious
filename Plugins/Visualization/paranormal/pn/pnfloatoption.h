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

#ifndef __PN_FLOAT_OPTION_H__
#define __PN_FLOAT_OPTION_H__

#include "pnoption.h"


G_BEGIN_DECLS


#define PN_TYPE_FLOAT_OPTION              (pn_float_option_get_type ())
#define PN_FLOAT_OPTION(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_FLOAT_OPTION, PnFloatOption))
#define PN_FLOAT_OPTION_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_FLOAT_OPTION, PnFloatOptionClass))
#define PN_IS_FLOAT_OPTION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_FLOAT_OPTION))
#define PN_IS_FLOAT_OPTION_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_FLOAT_OPTION))
#define PN_FLOAT_OPTION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_FLOAT_OPTION, PnFloatOptionClass))

typedef struct _PnFloatOption        PnFloatOption;
typedef struct _PnFloatOptionClass   PnFloatOptionClass;

struct _PnFloatOption
{
  PnOption parent;

  gfloat value;
  gfloat min;
  gfloat max;
};

struct _PnFloatOptionClass
{
  PnOptionClass parent_class;
};

/* Creators */
GType                 pn_float_option_get_type                (void);
PnFloatOption      *pn_float_option_new                     (const gchar *name,
								 const gchar *desc);

/* Accessors */
void                  pn_float_option_set_value               (PnFloatOption *float_option,
								 gfloat value);
gfloat                  pn_float_option_get_value               (PnFloatOption *float_option);
void                  pn_float_option_set_min                 (PnFloatOption *float_option,
								 gfloat min);
gfloat                  pn_float_option_get_min                 (PnFloatOption *float_option);
void                  pn_float_option_set_max                 (PnFloatOption *float_option,
								 gfloat max);
gfloat                  pn_float_option_get_max                 (PnFloatOption *float_option);






#endif /* __PN_FLOAT_OPTION_H__ */
