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

#ifndef __PN_OPTION_H__
#define __PN_OPTION_H__

#include "pnuserobject.h"
#include "pnoptionwidget.h"


G_BEGIN_DECLS


#define PN_TYPE_OPTION              (pn_option_get_type ())
#define PN_OPTION(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_OPTION, PnOption))
#define PN_OPTION_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_OPTION, PnOptionClass))
#define PN_IS_OPTION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_OPTION))
#define PN_IS_OPTION_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_OPTION))
#define PN_OPTION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_OPTION, PnOptionClass))

typedef struct _PnOption        PnOption;
typedef struct _PnOptionClass   PnOptionClass;

struct _PnOption
{
  PnUserObject parent;
};

struct _PnOptionClass
{
  PnUserObjectClass parent_class;

  GType widget_type;
};

/* Creators */
GType                 pn_option_get_type                (void);
PnOptionWidget       *pn_option_new_widget              (PnOption *option);





#endif /* __PN_OPTION_H__ */
