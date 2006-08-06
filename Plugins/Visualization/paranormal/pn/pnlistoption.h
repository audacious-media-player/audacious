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

#ifndef __PN_LIST_OPTION_H__
#define __PN_LIST_OPTION_H__

#include "pnoption.h"

G_BEGIN_DECLS

#define PN_TYPE_LIST_OPTION              (pn_list_option_get_type ())
#define PN_LIST_OPTION(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_LIST_OPTION, PnListOption))
#define PN_LIST_OPTION_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_LIST_OPTION, PnListOptionClass))
#define PN_IS_LIST_OPTION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_LIST_OPTION))
#define PN_IS_LIST_OPTION_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_LIST_OPTION))
#define PN_LIST_OPTION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_LIST_OPTION, PnListOptionClass))

typedef struct _PnListOption        PnListOption;
typedef struct _PnListOptionClass   PnListOptionClass;

struct _PnListOption
{
  PnOption parent;

  GArray *items;
  gint index;
};

struct _PnListOptionClass
{
  PnOptionClass parent_class;
};

/* Creators */
GType                 pn_list_option_get_type                (void);
PnListOption         *pn_list_option_new                     (const gchar *name,
							      const gchar *desc);

/* Accessors */
void                  pn_list_option_add_item                (PnListOption *list_option,
							      const gchar *item);
void                  pn_list_option_set_index               (PnListOption *list_option,
							      guint index);
gint                  pn_list_option_get_index               (PnListOption *list_option);

#endif /* __PN_LIST_OPTION_H__ */
